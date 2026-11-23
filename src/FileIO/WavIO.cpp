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

#include "Utils/exception.h"

extern "C" {
#include <sndfile.h>
}

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

WavData WavIO::ReadWav(const std::filesystem::path &path) {
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

    const sf_count_t read = sf_read_double(file.get(), buffer.data(), static_cast<sf_count_t>(buffer.size()));
    if (read < 0) {
        throw audio_file_error("failed to read " + path.string());
    }

    const auto read_frames = static_cast<std::size_t>(read) / channels;
    WavData data;
    data.sample_rate = info.samplerate;
    data.channels = info.channels;
    data.samples.resize(read_frames);

    for (std::size_t frame = 0; frame < read_frames; ++frame) {
        double sum = 0.0;
        for (std::size_t channel = 0; channel < channels; ++channel) {
            sum += buffer[frame * channels + channel];
        }
        data.samples[frame] = sum / static_cast<double>(channels);
    }

    return data;
}

void WavIO::WriteWav(const std::filesystem::path &path, std::span<const double> x, int fs) {
    SF_INFO info{};
    info.channels = 1;
    info.samplerate = fs;
    info.frames = static_cast<sf_count_t>(x.size());
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SndFilePtr file(open_file(path, SFM_WRITE, info));
    if (!file) {
        throw audio_file_error(path.string());
    }

    const auto written = sf_write_double(file.get(), x.data(), static_cast<sf_count_t>(x.size()));
    if (written != static_cast<sf_count_t>(x.size())) {
        throw audio_file_error("failed to write " + path.string());
    }
}
