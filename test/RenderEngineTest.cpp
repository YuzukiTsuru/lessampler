#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
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

    const auto world_name = ParseVocoderKind("WORLD");
    const auto llsm_name = ParseVocoderKind("Llsm");
    CHECK(world_name.has_value());
    CHECK(llsm_name.has_value());
    if (world_name) {
        CHECK_EQ(*world_name, VocoderKind::world);
    }
    if (llsm_name) {
        CHECK_EQ(*llsm_name, VocoderKind::llsm);
    }
    CHECK(!ParseVocoderKind("nope").has_value());

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

#if defined(LESSAMPLER_ENABLE_LLSM)
    auto llsm_backend = backend::CreateVocoderBackend(VocoderKind::llsm);
    REQUIRE(llsm_backend != nullptr);
    CHECK_EQ(llsm_backend->kind(), VocoderKind::llsm);
    CHECK_EQ(llsm_backend->name(), "llsm");
    CHECK(llsm_backend->analysis_version() > 0);

    const auto llsm_analysis = llsm_backend->Prepare(analysis_request);
    REQUIRE(llsm_analysis != nullptr);
    const auto llsm_result = llsm_backend->Synthesize({*llsm_analysis, params});
    check_render(llsm_result, params, source.info().sample_rate, 440.0);

    const auto cache_path = backend::LlsmCachePath(input);
    CHECK(std::filesystem::exists(cache_path));
    CHECK(backend::LoadLlsmCache(input, configure) != nullptr);

    // Prepare with the cache present and a fresh source (no PCM read needed).
    const SampleSource warm_source(input);
    const auto warm_analysis = llsm_backend->Prepare({warm_source, configure});
    REQUIRE(warm_analysis != nullptr);
    const auto warm_llsm = llsm_backend->Synthesize({*warm_analysis, params});
    CHECK(warm_llsm.samples == llsm_result.samples);
    CHECK(all_finite(warm_llsm.samples));

    // A source change invalidates the layer-1 cache and regenerates it.
    const auto original_mtime = std::filesystem::last_write_time(input);
    std::filesystem::last_write_time(input, original_mtime + std::chrono::seconds(2));
    CHECK(backend::LoadLlsmCache(input, configure) == nullptr);
    const SampleSource regenerated_source(input);
    const auto regenerated_analysis = llsm_backend->Prepare({regenerated_source, configure});
    const auto regenerated = llsm_backend->Synthesize({*regenerated_analysis, params});
    CHECK(regenerated.samples == llsm_result.samples);
    CHECK(std::filesystem::exists(cache_path));

    // The F0/analysis settings are part of the cache key.
    Config changed = configure;
    changed.f0_speed = 2;
    CHECK(backend::LoadLlsmCache(input, changed) == nullptr);

    // A corrupt cache is ignored and regenerated.
    {
        std::ofstream corrupt(cache_path, std::ios::binary | std::ios::trunc);
        corrupt << "not an LLSM cache";
    }
    CHECK(backend::LoadLlsmCache(input, configure) == nullptr);
    const SampleSource recovered_source(input);
    const auto recovered_analysis = llsm_backend->Prepare({recovered_source, configure});
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

    std::filesystem::remove_all(root);
    return test::finish();
}
