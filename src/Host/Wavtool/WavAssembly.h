/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WAVTOOL_WAVASSEMBLY_H
#define LESSAMPLER_WAVTOOL_WAVASSEMBLY_H

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

#include "Host/Wavtool/Envelope.h"

namespace lessampler::wavtool {

    /*
 * Interleaved double-precision mix buffer. Samples are only clamped when the
 * buffer is written, so overlap sums keep their full precision.
 */
    struct WavBuffer {
        std::vector<double> samples;
        int sample_rate = 0;
        int channels = 0;

        [[nodiscard]] std::size_t frames() const {
            return channels > 0 ? samples.size() / static_cast<std::size_t>(channels) : 0;
        }

        void resize_frames(std::size_t count) {
            samples.resize(count * static_cast<std::size_t>(channels), 0.0);
        }
    };

    struct WavSegment {
        std::filesystem::path input;
        double skip_ms = 0.0;
        double length_ms = 0.0;
        Envelope envelope;
    };

    class WavAssembly {
    public:
        /*
     * Read an existing output file, or create an empty buffer with the given
     * fallback layout. A zero-byte file is treated as an empty output.
     */
        [[nodiscard]] static WavBuffer LoadOrCreate(const std::filesystem::path &output, int fallback_rate, int fallback_channels);

        /*
     * Append a segment at an absolute position. position_samples may be
     * negative; the leading part of the segment is then dropped.
     */
        static void AppendAt(WavBuffer &output, const WavSegment &segment, std::int64_t position_samples);

        /*
     * Classic wavtool placement: the segment starts overlap_ms before the
     * current end of the output. A negative overlap inserts silence.
     */
        static void AppendRelative(WavBuffer &output, const WavSegment &segment, double overlap_ms);

        /*
     * Clamp to [-1, 1] and write 16-bit PCM. When peak_limit is > 0 and the
     * mixed peak exceeds it, the whole buffer is scaled down first.
     */
        static void Write(const WavBuffer &output, const std::filesystem::path &path, double peak_limit = 0.0);

        [[nodiscard]] static double Peak(std::span<const double> samples);
    };

}// namespace lessampler::wavtool

#endif//LESSAMPLER_WAVTOOL_WAVASSEMBLY_H
