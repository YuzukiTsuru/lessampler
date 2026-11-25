#include "TestSupport.h"

#include <cmath>
#include <filesystem>
#include <vector>

#include "Audio/WavIO.h"
#include "Core/Exception.h"

extern "C" {
#include <sndfile.h>

using namespace lessampler;
}

int main() {
    const auto root = std::filesystem::temp_directory_path() / "lessampler-wavio-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto mono_path = root / "mono.wav";
    std::vector<double> mono(1000);
    for (std::size_t i = 0; i < mono.size(); ++i) {
        mono[i] = 0.25 * std::sin(static_cast<double>(i) * 0.05);
    }

    WavIO::WriteWav(mono_path, mono, 48000);
    const auto mono_info = WavIO::ProbeWav(mono_path);
    CHECK_EQ(mono_info.sample_rate, 48000);
    CHECK_EQ(mono_info.channels, 1);
    CHECK_EQ(mono_info.frames, mono.size());

    const auto mono_read = WavIO::ReadWav(mono_path);
    CHECK_EQ(mono_read.sample_rate, 48000);
    CHECK_EQ(mono_read.channels, 1);
    CHECK_EQ(mono_read.samples.size(), mono.size());
    for (std::size_t i = 0; i < mono.size(); ++i) {
        CHECK_NEAR(mono_read.samples[i], mono[i], 1e-4);
    }

    const auto stereo_path = root / "stereo.wav";
    SF_INFO info{};
    info.channels = 2;
    info.samplerate = 44100;
    info.frames = 4;
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE *file = sf_open(stereo_path.string().c_str(), SFM_WRITE, &info);
    REQUIRE(file != nullptr);
    const std::vector<double> interleaved{1.0, 0.0, 0.0, 1.0, 0.5, 0.5, -1.0, -1.0};
    CHECK_EQ(sf_write_double(file, interleaved.data(), static_cast<sf_count_t>(interleaved.size())),
             static_cast<sf_count_t>(interleaved.size()));
    sf_close(file);

    const auto stereo_info = WavIO::ProbeWav(stereo_path);
    CHECK_EQ(stereo_info.sample_rate, 44100);
    CHECK_EQ(stereo_info.channels, 2);
    CHECK_EQ(stereo_info.frames, 4);

    const auto stereo_read = WavIO::ReadWav(stereo_path);
    CHECK_EQ(stereo_read.sample_rate, 44100);
    CHECK_EQ(stereo_read.channels, 2);
    REQUIRE(stereo_read.samples.size() == 4);
    CHECK_NEAR(stereo_read.samples[0], 0.5, 1e-4);
    CHECK_NEAR(stereo_read.samples[1], 0.5, 1e-4);
    CHECK_NEAR(stereo_read.samples[2], 0.5, 1e-4);
    CHECK_NEAR(stereo_read.samples[3], -1.0, 1e-4);

    const auto stereo_frames = WavIO::ReadWavFrames(stereo_path);
    CHECK_EQ(stereo_frames.sample_rate, 44100);
    CHECK_EQ(stereo_frames.channels, 2);
    CHECK_EQ(stereo_frames.frames(), 4);
    REQUIRE(stereo_frames.samples.size() == interleaved.size());
    for (std::size_t i = 0; i < interleaved.size(); ++i) {
        CHECK_NEAR(stereo_frames.samples[i], interleaved[i], 1e-4);
    }

    const auto frames_path = root / "frames.wav";
    WavIO::WriteWavFrames(frames_path, interleaved, 44100, 2);
    const auto frames_read = WavIO::ReadWavFrames(frames_path);
    CHECK_EQ(frames_read.sample_rate, 44100);
    CHECK_EQ(frames_read.channels, 2);
    CHECK_EQ(frames_read.frames(), 4);
    REQUIRE(frames_read.samples.size() == interleaved.size());
    for (std::size_t i = 0; i < interleaved.size(); ++i) {
        CHECK_NEAR(frames_read.samples[i], interleaved[i], 1e-4);
    }

    bool rejected = false;
    try {
        WavIO::ReadWav(root / "missing.wav");
    } catch (const audio_file_error &) {
        rejected = true;
    }
    CHECK(rejected);

    rejected = false;
    try {
        WavIO::ProbeWav(root / "missing.wav");
    } catch (const audio_file_error &) {
        rejected = true;
    }
    CHECK(rejected);

    std::filesystem::remove_all(root);
    return test::finish();
}
