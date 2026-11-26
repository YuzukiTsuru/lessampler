#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Audio/SampleSource.h"
#include "Config/Config.h"
#include "Config/VocoderKind.h"
#include "Core/AudioInfo.h"
#include "Core/Exception.h"
#include "Engine/RenderEngine.h"
#include "Host/UTAU/Shine.h"
#include "Vocoder/Common/F0Estimator.h"
#include "Vocoder/VocoderBackend.h"
#if defined(LESSAMPLER_ENABLE_LLSM)
#include "Vocoder/Llsm/LlsmCache.h"
#endif

using namespace lessampler;

namespace {

    bool all_finite(std::span<const double> samples) {
        return std::ranges::all_of(samples, [](double value) { return std::isfinite(value); });
    }

    double rms_of(std::span<const double> samples) {
        if (samples.empty()) {
            return 0.0;
        }
        double sum = 0.0;
        for (double value: samples) {
            sum += value * value;
        }
        return std::sqrt(sum / static_cast<double>(samples.size()));
    }

    /*
     * Coarse autocorrelation pitch estimate. Good enough to catch a backend
     * that renders silence, noise, or a completely wrong note.
     */
    double estimate_pitch(std::span<const double> samples, int fs) {
        if (samples.size() < static_cast<std::size_t>(fs / 4)) {
            return 0.0;
        }
        const std::size_t segment_length = std::min<std::size_t>(samples.size(), static_cast<std::size_t>(fs / 3));
        const std::size_t start = (samples.size() - segment_length) / 2;
        const auto segment = samples.subspan(start, segment_length);

        const int min_lag = std::max(2, fs / 500);
        const int max_lag = std::min<int>(fs / 80, static_cast<int>(segment.size()) - 1);
        double best_score = 0.0;
        int best_lag = 0;
        for (int lag = min_lag; lag <= max_lag; ++lag) {
            double score = 0.0;
            for (std::size_t i = 0; i + static_cast<std::size_t>(lag) < segment.size(); ++i) {
                score += segment[i] * segment[i + static_cast<std::size_t>(lag)];
            }
            score /= static_cast<double>(segment.size() - static_cast<std::size_t>(lag));
            if (score > best_score) {
                best_score = score;
                best_lag = lag;
            }
        }
        return best_lag > 0 ? static_cast<double>(fs) / static_cast<double>(best_lag) : 0.0;
    }

    RenderParams make_params(const std::filesystem::path &input, const SampleSource &source) {
        const auto output = input.parent_path() / "backend-test-out.wav";
        std::array<std::string, 14> storage{
                "lessampler", input.string(), output.string(), "A4", "100", "", "0", "1000",
                "100", "0", "100", "0", "!120", ""};
        std::array<std::string_view, 14> args{};
        std::ranges::transform(storage, args.begin(), [](const std::string &value) { return std::string_view(value); });

        const AudioInfo info = source.audio_info(Config{}.audio_model_frame_period);
        Shine shine(args, info, Shine::SHINE_MODE::UTAU);
        return shine.GetShine();
    }

    void check_render(const backend::RenderResult &result, const RenderParams &params, int fs,
                      double expected_pitch) {
        CHECK_EQ(result.fs, fs);
        CHECK_EQ(result.samples.size(), static_cast<std::size_t>(params.output_samples));
        CHECK(all_finite(result.samples));
        CHECK(rms_of(result.samples) > 1e-4);
        CHECK_NEAR(estimate_pitch(result.samples, result.fs), expected_pitch, 25.0);
    }

}// namespace

int main() {
    CHECK_EQ(VocoderKindName(VocoderKind::world), "world");
    CHECK_EQ(VocoderKindName(VocoderKind::llsm), "llsm");
    CHECK_EQ(VocoderKindName(VocoderKind::neural), "neural");

    const auto world_name = ParseVocoderKind("WORLD");
    const auto llsm_name = ParseVocoderKind("Llsm");
    const auto neural_name = ParseVocoderKind("Neural");
    CHECK(world_name.has_value());
    CHECK(llsm_name.has_value());
    CHECK(neural_name.has_value());
    if (world_name) {
        CHECK_EQ(*world_name, VocoderKind::world);
    }
    if (llsm_name) {
        CHECK_EQ(*llsm_name, VocoderKind::llsm);
    }
    if (neural_name) {
        CHECK_EQ(*neural_name, VocoderKind::neural);
    }
    CHECK(!ParseVocoderKind("nope").has_value());

    {
        const std::string names = backend::AvailableVocoderBackends();
        std::size_t start = 0;
        while (start < names.size()) {
            const auto end = names.find(' ', start);
            const auto name = names.substr(start, end == std::string::npos ? std::string::npos : end - start);
            CHECK(ParseVocoderKind(name).has_value());
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
    }

    const auto root = std::filesystem::temp_directory_path() / "lessampler-backend-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const auto test_data = std::filesystem::path(LESSAMPLER_TEST_DATA_DIR) / "vaiueo2d.wav";
    const auto input = root / "input.wav";
    std::filesystem::copy_file(test_data, input);

    const SampleSource source(input);
    const auto params = make_params(input, source);
    REQUIRE(params.output_samples > 0);
    REQUIRE(params.required_frame > 0);
    REQUIRE(params.pitch_bend.size() >= 2);

    Config configure;
    const backend::AnalysisRequest analysis_request{source, configure};

    auto world_backend = backend::CreateVocoderBackend(VocoderKind::world);
    REQUIRE(world_backend != nullptr);
    CHECK_EQ(world_backend->kind(), VocoderKind::world);
    CHECK_EQ(world_backend->name(), "world");
    CHECK(world_backend->analysis_version() > 0);

    const auto world_analysis = world_backend->Prepare(analysis_request);
    REQUIRE(world_analysis != nullptr);
    CHECK_EQ(world_analysis->kind(), VocoderKind::world);
    check_render(world_backend->Synthesize({*world_analysis, params}), params, source.info().sample_rate, 440.0);

    // A uniform +100 cent bend must move A4 to A#4 in both backends.
    auto shifted = params;
    std::ranges::fill(shifted.pitch_bend, 100);
    check_render(world_backend->Synthesize({*world_analysis, shifted}), shifted, source.info().sample_rate, 466.16);

    // One engine reuses the note-independent analysis in memory.
    RenderEngine engine(configure);
    const auto engine_first = engine.Render(source, params);
    const auto engine_second = engine.Render(source, params);
    CHECK(engine_first.samples == engine_second.samples);
    const auto engine_stats = engine.stats();
    CHECK_EQ(engine_stats.analysis_misses, 1);
    CHECK_EQ(engine_stats.analysis_hits, 1);

    // Explicit natural style is still the natural path.
    auto natural_params = params;
    natural_params.style.from_flags = true;
    const auto natural_again = engine.Render(source, natural_params);
    CHECK(natural_again.samples == engine_first.samples);

    // A style change is synthesis-only: the analysis is reused and the PCM
    // changes without a second Prepare call.
    auto clear_params = params;
    clear_params.style.from_flags = true;
    clear_params.style.start = StyleWeights{1.0, 0.0, 0.0};
    clear_params.style.end = clear_params.style.start;
    const auto clear_result = engine.Render(source, clear_params);
    CHECK(all_finite(clear_result.samples));
    CHECK(clear_result.samples != engine_first.samples);
    CHECK_EQ(engine.stats().analysis_misses, 1);
    CHECK_EQ(engine.stats().analysis_hits, 3);

    // A voicebank default applies when the note carries no style flags.
    Config clear_config = configure;
    clear_config.style_default = VoiceStyleKind::clear;
    RenderEngine clear_engine(clear_config);
    CHECK(clear_engine.Render(source, params).samples != engine_first.samples);

    // Per-note flags override the voicebank default.
    Config growl_config = configure;
    growl_config.style_default = VoiceStyleKind::growl;
    RenderEngine growl_engine(growl_config);
    CHECK(growl_engine.Render(source, clear_params).samples == clear_result.samples);

    auto growl_params = params;
    growl_params.style.from_flags = true;
    growl_params.style.start = StyleWeights{0.0, 0.0, 1.0};
    growl_params.style.end = growl_params.style.start;
    const auto growl_result = engine.Render(source, growl_params);
    CHECK(all_finite(growl_result.samples));
    CHECK(growl_result.samples != engine_first.samples);
    CHECK(std::ranges::all_of(growl_result.samples, [](double value) { return std::abs(value) <= 1.0; }));

    // Source preprocessing is part of the analysis key: enabling it forces a
    // fresh analysis and changes the rendered PCM.
    Config preprocess_config = configure;
    preprocess_config.preprocess.enabled = true;
    preprocess_config.preprocess.highpass_hz = 80.0;
    preprocess_config.preprocess.declick = false;
    preprocess_config.preprocess.denoise = false;
    RenderEngine preprocess_engine(preprocess_config);
    const auto preprocessed = preprocess_engine.Render(source, params);
    CHECK(all_finite(preprocessed.samples));
    CHECK(preprocessed.samples != engine_first.samples);
    CHECK_EQ(preprocess_engine.stats().analysis_misses, 1);

#if defined(LESSAMPLER_ENABLE_LLSM)
    const auto llsm_config = ResolveF0EstimatorConfig(configure, VocoderKind::llsm);
    const backend::AnalysisRequest llsm_request{source, llsm_config};
    auto llsm_backend = backend::CreateVocoderBackend(VocoderKind::llsm);
    REQUIRE(llsm_backend != nullptr);
    CHECK_EQ(llsm_backend->kind(), VocoderKind::llsm);
    CHECK_EQ(llsm_backend->name(), "llsm");
    CHECK(llsm_backend->analysis_version() > 0);

    const auto llsm_analysis = llsm_backend->Prepare(llsm_request);
    REQUIRE(llsm_analysis != nullptr);
    const auto llsm_result = llsm_backend->Synthesize({*llsm_analysis, params});
    check_render(llsm_result, params, source.info().sample_rate, 440.0);

    auto llsm_clear = params;
    llsm_clear.style.from_flags = true;
    llsm_clear.style.start = StyleWeights{1.0, 0.0, 0.0};
    llsm_clear.style.end = llsm_clear.style.start;
    const auto llsm_clear_result = llsm_backend->Synthesize({*llsm_analysis, llsm_clear});
    CHECK(all_finite(llsm_clear_result.samples));
    CHECK(llsm_clear_result.samples != llsm_result.samples);

    const auto cache_path = backend::LlsmCachePath(input);
    CHECK(std::filesystem::exists(cache_path));
    CHECK(backend::LoadLlsmCache(input, llsm_config) != nullptr);

    // Prepare with the cache present and a fresh source (no PCM read needed).
    const SampleSource warm_source(input);
    const auto warm_analysis = llsm_backend->Prepare({warm_source, llsm_config});
    REQUIRE(warm_analysis != nullptr);
    const auto warm_llsm = llsm_backend->Synthesize({*warm_analysis, params});
    CHECK(warm_llsm.samples == llsm_result.samples);
    CHECK(all_finite(warm_llsm.samples));

    // A source change invalidates the layer-1 cache and regenerates it.
    const auto original_mtime = std::filesystem::last_write_time(input);
    std::filesystem::last_write_time(input, original_mtime + std::chrono::seconds(2));
    CHECK(backend::LoadLlsmCache(input, llsm_config) == nullptr);
    const SampleSource regenerated_source(input);
    const auto regenerated_analysis = llsm_backend->Prepare({regenerated_source, llsm_config});
    const auto regenerated = llsm_backend->Synthesize({*regenerated_analysis, params});
    CHECK(regenerated.samples == llsm_result.samples);
    CHECK(std::filesystem::exists(cache_path));

    // The F0/analysis settings are part of the cache key.
    Config changed = llsm_config;
    changed.f0_speed = 2;
    CHECK(backend::LoadLlsmCache(input, changed) == nullptr);

    // A corrupt cache is ignored and regenerated.
    {
        std::ofstream corrupt(cache_path, std::ios::binary | std::ios::trunc);
        corrupt << "not an LLSM cache";
    }
    CHECK(backend::LoadLlsmCache(input, llsm_config) == nullptr);
    const SampleSource recovered_source(input);
    const auto recovered_analysis = llsm_backend->Prepare({recovered_source, llsm_config});
    const auto recovered = llsm_backend->Synthesize({*recovered_analysis, params});
    CHECK(recovered.samples == llsm_result.samples);
    CHECK(std::filesystem::exists(cache_path));

    check_render(llsm_backend->Synthesize({*recovered_analysis, shifted}), shifted,
                 source.info().sample_rate, 466.16);
#else
    bool rejected = false;
    try {
        static_cast<void>(backend::CreateVocoderBackend(VocoderKind::llsm));
    } catch (const parameter_error &) {
        rejected = true;
    }
    CHECK(rejected);
#endif

#if defined(LESSAMPLER_ENABLE_NEURAL)
    auto neural_backend = backend::CreateVocoderBackend(VocoderKind::neural);
    REQUIRE(neural_backend != nullptr);
    CHECK_EQ(neural_backend->kind(), VocoderKind::neural);
    CHECK_EQ(neural_backend->name(), "neural");
    CHECK(neural_backend->analysis_version() > 0);
    {
        // The backend is built, but the model is deliberately user supplied.
        bool no_model_rejected = false;
        try {
            static_cast<void>(neural_backend->Prepare({source, Config{}}));
        } catch (const parameter_error &) {
            no_model_rejected = true;
        }
        CHECK(no_model_rejected);
    }

    // A real model is optional because its weights are not distributed with
    // lessampler. Set LESSAMPLER_NEURAL_MODEL to an NHVSing ONNX file to run
    // the full render path.
    if (const char *model_path = std::getenv("LESSAMPLER_NEURAL_MODEL"); model_path != nullptr && *model_path != '\0') {
        Config neural_config;
        neural_config.vocoder = VocoderKind::neural;
        neural_config.neural_model_path = model_path;
        neural_config.neural_threads = 2;
        const auto neural_analysis = neural_backend->Prepare({source, neural_config});
        REQUIRE(neural_analysis != nullptr);
        CHECK_EQ(neural_analysis->kind(), VocoderKind::neural);
        check_render(neural_backend->Synthesize({*neural_analysis, params}), params,
                     source.info().sample_rate, 440.0);
        check_render(neural_backend->Synthesize({*neural_analysis, shifted}), shifted,
                     source.info().sample_rate, 466.16);
    }
#else
    bool neural_rejected = false;
    try {
        static_cast<void>(backend::CreateVocoderBackend(VocoderKind::neural));
    } catch (const parameter_error &) {
        neural_rejected = true;
    }
    CHECK(neural_rejected);
#endif

    std::filesystem::remove_all(root);
    return test::finish();
}
