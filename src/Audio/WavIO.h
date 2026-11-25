/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WAVIO_H
#define LESSAMPLER_WAVIO_H

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace lessampler {

    struct WavData {
        std::vector<double> samples;
        int sample_rate = 0;
        int channels = 0;
    };

    /*
     * Interleaved sample data. Unlike WavData, the channel layout is preserved
     * so it can be read, mixed, and written back without a downmix.
     */
    struct WavFrames {
        std::vector<double> samples;
        int sample_rate = 0;
        int channels = 0;

        [[nodiscard]] std::size_t frames() const {
            return channels > 0 ? samples.size() / static_cast<std::size_t>(channels) : 0;
        }
    };

    struct WavInfo {
        int sample_rate = 0;
        int channels = 0;
        std::size_t frames = 0;
    };

    class WavIO {
    public:
        /*
     * Read the WAV header without loading the sample data.
     */
        static WavInfo ProbeWav(const std::filesystem::path &path);

        /*
     * Read a WAV file as mono. Multi-channel input is averaged per frame.
     */
        static WavData ReadWav(const std::filesystem::path &path);

        /*
     * Read a WAV file without downmixing. samples is interleaved.
     */
        static WavFrames ReadWavFrames(const std::filesystem::path &path);

        /*
     * Write mono 16-bit PCM.
     */
        static void WriteWav(const std::filesystem::path &path, std::span<const double> x, int fs);

        /*
     * Write interleaved mono or multi-channel 16-bit PCM.
     */
        static void WriteWavFrames(const std::filesystem::path &path, std::span<const double> interleaved, int fs, int channels);
    };

}// namespace lessampler

#endif//LESSAMPLER_WAVIO_H
