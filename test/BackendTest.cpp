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

#include "AudioModel/AudioInfo.h"
#include "Backend/VocoderBackend.h"
#include "Backend/VocoderKind.h"
#if defined(LESSAMPLER_ENABLE_LLSM)
#include "Backend/LlsmCache.h"
#endif
#include "ConfigUnit/lessConfigure.h"
#include "FileIO/WavIO.h"
#include "Shine/Shine.h"
#include "Utils/exception.h"

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

    ShinePara make_shine(const std::filesystem::path &input, const WavData &wav) {
        const auto output = input.parent_path() / "backend-test-out.wav";
        std::array<std::string, 14> storage{
                "lessampler", input.string(), output.string(), "A4", "100", "", "0", "1000",
                "100", "0", "100", "0", "!120", ""};
        std::array<std::string_view, 14> args{};
        std::ranges::transform(storage, args.begin(), [](const std::string &value) { return std::string_view(value); });

        const AudioInfo info{wav.sample_rate, wav.samples.size(), lessConfigure{}.audio_model_frame_period};
        Shine shine(args, info, Shine::SHINE_MODE::UTAU);
        return shine.GetShine();
    }

}// namespace

int main() {
    CHECK_EQ(lessbackend::VocoderKindName(lessbackend::VocoderKind::world), "world");
    CHECK_EQ(lessbackend::VocoderKindName(lessbackend::VocoderKind::llsm), "llsm");

    const auto world_name = lessbackend::ParseVocoderKind("WORLD");
    const auto llsm_name = lessbackend::ParseVocoderKind("Llsm");
    CHECK(world_name.has_value());
    CHECK(llsm_name.has_value());
    if (world_name) {
        CHECK_EQ(*world_name, lessbackend::VocoderKind::world);
    }
    if (llsm_name) {
        CHECK_EQ(*llsm_name, lessbackend::VocoderKind::llsm);
    }
    CHECK(!lessbackend::ParseVocoderKind("nope").has_value());

    const auto root = std::filesystem::temp_directory_path() / "lessampler-backend-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const auto source = std::filesystem::path(LESSAMPLER_TEST_DATA_DIR) / "vaiueo2d.wav";
    const auto input = root / "input.wav";
    std::filesystem::copy_file(source, input);

    const auto wav = WavIO::ReadWav(input);
    REQUIRE(!wav.samples.empty());
    const auto shine = make_shine(input, wav);
    REQUIRE(shine.output_samples > 0);
    REQUIRE(shine.required_frame > 0);
    REQUIRE(shine.pitch_bend.size() >= 2);

    lessConfigure configure;
    lessbackend::RenderRequest request{input, wav.samples, wav.sample_rate, shine, configure};

    auto world_backend = lessbackend::CreateVocoderBackend(lessbackend::VocoderKind::world);
    REQUIRE(world_backend != nullptr);
    CHECK_EQ(world_backend->kind(), lessbackend::VocoderKind::world);
    CHECK_EQ(world_backend->name(), "world");

    const auto world_result = world_backend->Render(request);
    CHECK_EQ(world_result.fs, wav.sample_rate);
    CHECK_EQ(world_result.samples.size(), static_cast<std::size_t>(shine.output_samples));
    CHECK(all_finite(world_result.samples));
    CHECK(rms_of(world_result.samples) > 1e-4);
    CHECK_NEAR(estimate_pitch(world_result.samples, world_result.fs), 440.0, 25.0);

    // A uniform +100 cent bend must move A4 to A#4 in both backends.
    auto shifted = shine;
    std::ranges::fill(shifted.pitch_bend, 100);
    const lessbackend::RenderRequest shifted_request{input, wav.samples, wav.sample_rate, shifted, configure};
    const auto shifted_world = world_backend->Render(shifted_request);
    CHECK_EQ(shifted_world.samples.size(), static_cast<std::size_t>(shifted.output_samples));
    CHECK_NEAR(estimate_pitch(shifted_world.samples, shifted_world.fs), 466.16, 25.0);

#if defined(LESSAMPLER_ENABLE_LLSM)
    auto llsm_backend = lessbackend::CreateVocoderBackend(lessbackend::VocoderKind::llsm);
    REQUIRE(llsm_backend != nullptr);
    CHECK_EQ(llsm_backend->kind(), lessbackend::VocoderKind::llsm);
    CHECK_EQ(llsm_backend->name(), "llsm");

    const auto llsm_result = llsm_backend->Render(request);
    CHECK_EQ(llsm_result.fs, wav.sample_rate);
    CHECK_EQ(llsm_result.samples.size(), static_cast<std::size_t>(shine.output_samples));
    CHECK(all_finite(llsm_result.samples));
    CHECK(rms_of(llsm_result.samples) > 1e-4);
    CHECK_NEAR(estimate_pitch(llsm_result.samples, llsm_result.fs), 440.0, 25.0);

    const auto cache_path = lessbackend::LlsmCachePath(input);
    CHECK(std::filesystem::exists(cache_path));
    CHECK(lessbackend::LoadLlsmCache(input, configure) != nullptr);

    const auto warm_llsm = llsm_backend->Render(request);
    CHECK(warm_llsm.samples == llsm_result.samples);
    CHECK(all_finite(warm_llsm.samples));

    // A source change invalidates the layer-1 cache and regenerates it.
    const auto original_mtime = std::filesystem::last_write_time(input);
    std::filesystem::last_write_time(input, original_mtime + std::chrono::seconds(2));
    CHECK(lessbackend::LoadLlsmCache(input, configure) == nullptr);
    const auto regenerated = llsm_backend->Render(request);
    CHECK(regenerated.samples == llsm_result.samples);
    CHECK(std::filesystem::exists(cache_path));

    // The F0/analysis settings are part of the cache key.
    lessConfigure changed = configure;
    changed.f0_speed = 2;
    CHECK(lessbackend::LoadLlsmCache(input, changed) == nullptr);

    // A corrupt cache is ignored and regenerated.
    {
        std::ofstream corrupt(cache_path, std::ios::binary | std::ios::trunc);
        corrupt << "not an LLSM cache";
    }
    CHECK(lessbackend::LoadLlsmCache(input, configure) == nullptr);
    const auto recovered = llsm_backend->Render(request);
    CHECK(recovered.samples == llsm_result.samples);
    CHECK(std::filesystem::exists(cache_path));

    const auto shifted_llsm = llsm_backend->Render(shifted_request);
    CHECK_EQ(shifted_llsm.samples.size(), static_cast<std::size_t>(shifted.output_samples));
    CHECK(all_finite(shifted_llsm.samples));
    CHECK_NEAR(estimate_pitch(shifted_llsm.samples, shifted_llsm.fs), 466.16, 25.0);
#else
    bool rejected = false;
    try {
        static_cast<void>(lessbackend::CreateVocoderBackend(lessbackend::VocoderKind::llsm));
    } catch (const parameter_error &) {
        rejected = true;
    }
    CHECK(rejected);
#endif

    std::filesystem::remove_all(root);
    return test::finish();
}
