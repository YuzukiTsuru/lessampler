/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WavIO.h"

#include <memory>
#include <utility>

#include "Core/Exception.h"

extern "C" {
#include <sndfile.h>
}

namespace lessampler {

    namespace {

        struct SndFileDeleter {
            void operator()(SNDFILE *file) const {
                if (file != nullptr) {
                    sf_close(file);
                }
            }
        };

        using SndFilePtr = std::unique_ptr<SNDFILE, SndFileDeleter>;

        SNDFILE *open_file(const std::filesystem::path &path, int mode, SF_INFO &info) {
#if defined(_WIN32)
            return sf_wchar_open(path.c_str(), mode, &info);
#else
            return sf_open(path.string().c_str(), mode, &info);
#endif
        }

    }// namespace

    WavInfo WavIO::ProbeWav(const std::filesystem::path &path) {
        SF_INFO info{};
        info.format = 0;

        SndFilePtr file(open_file(path, SFM_READ, info));
        if (!file) {
            throw audio_file_error(path.string());
        }
        if (info.frames <= 0 || info.channels <= 0 || info.samplerate <= 0) {
            throw audio_file_error("empty or invalid audio file: " + path.string());
        }

        return WavInfo{info.samplerate, info.channels, static_cast<std::size_t>(info.frames)};
    }

    WavData WavIO::ReadWav(const std::filesystem::path &path) {
        auto frames = ReadWavFrames(path);

        const auto channels = static_cast<std::size_t>(frames.channels);
        WavData data;
        data.sample_rate = frames.sample_rate;
        data.channels = frames.channels;
        data.samples.resize(frames.frames());

        for (std::size_t frame = 0; frame < data.samples.size(); ++frame) {
            double sum = 0.0;
            for (std::size_t channel = 0; channel < channels; ++channel) {
                sum += frames.samples[frame * channels + channel];
            }
            data.samples[frame] = sum / static_cast<double>(channels);
        }

        return data;
    }

    WavFrames WavIO::ReadWavFrames(const std::filesystem::path &path) {
        SF_INFO info{};
        info.format = 0;

        SndFilePtr file(open_file(path, SFM_READ, info));
        if (!file) {
            throw audio_file_error(path.string());
        }

        if (info.frames <= 0 || info.channels <= 0) {
            throw audio_file_error("empty or invalid audio file: " + path.string());
        }

        const auto channels = static_cast<std::size_t>(info.channels);
        const auto frames = static_cast<std::size_t>(info.frames);
        std::vector<double> buffer(frames * channels);

        const sf_count_t read = sf_readf_double(file.get(), buffer.data(), static_cast<sf_count_t>(frames));
        if (read < 0) {
            throw audio_file_error("failed to read " + path.string());
        }

        buffer.resize(static_cast<std::size_t>(read) * channels);
        return WavFrames{std::move(buffer), info.samplerate, info.channels};
    }

    void WavIO::WriteWav(const std::filesystem::path &path, std::span<const double> x, int fs) {
        WriteWavFrames(path, x, fs, 1);
    }

    void WavIO::WriteWavFrames(const std::filesystem::path &path, std::span<const double> interleaved, int fs, int channels) {
        if (fs <= 0 || channels <= 0) {
            throw audio_file_error("invalid WAV layout for " + path.string());
        }
        if (interleaved.empty() || interleaved.size() % static_cast<std::size_t>(channels) != 0) {
            throw audio_file_error("invalid interleaved sample count for " + path.string());
        }

        SF_INFO info{};
        info.channels = channels;
        info.samplerate = fs;
        info.frames = static_cast<sf_count_t>(interleaved.size() / static_cast<std::size_t>(channels));
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

        SndFilePtr file(open_file(path, SFM_WRITE, info));
        if (!file) {
            throw audio_file_error(path.string());
        }

        const auto written = sf_write_double(file.get(), interleaved.data(), static_cast<sf_count_t>(interleaved.size()));
        if (written != static_cast<sf_count_t>(interleaved.size())) {
            throw audio_file_error("failed to write " + path.string());
        }
    }

}// namespace lessampler
