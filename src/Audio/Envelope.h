/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WAVTOOL_ENVELOPE_H
#define LESSAMPLER_WAVTOOL_ENVELOPE_H

#include <cstddef>
#include <string_view>
#include <vector>

namespace lessampler {

    /*
 * UTAU note envelope.
 *
 * The UST representation is p1,p2,p3,v1,v2,v3,v4[,%,p4,p5,v5]. The % token is
 * the overlap placeholder used by the wavtool argument list; it does not
 * participate in the gain curve. Missing trailing fields use the UTAU
 * defaults. Positions are milliseconds; volumes are percentages (0..200).
 */
    class Envelope {
    public:
        inline static constexpr std::string_view kDefault = "0,5,35,0,100,100,0,%,0,10,100";

        [[nodiscard]] static Envelope Default();

        /*
     * Parse a UST-style envelope string. Malformed numeric fields are treated
     * as zero and the result is always usable.
     */
        [[nodiscard]] static Envelope Parse(std::string_view text);

        /*
     * Piecewise-linear gain over a segment of sample_count samples.
     * The first point is relative to the start of the processed segment.
     * Gains are normalized (100% -> 1.0).
     */
        [[nodiscard]] std::vector<double> GainCurve(std::size_t sample_count, int sample_rate) const;

        double p1 = 0.0;
        double p2 = 5.0;
        double p3 = 35.0;
        double p4 = 0.0;
        double p5 = 10.0;
        double v1 = 0.0;
        double v2 = 100.0;
        double v3 = 100.0;
        double v4 = 0.0;
        double v5 = 100.0;
    };

}// namespace lessampler

#endif//LESSAMPLER_WAVTOOL_ENVELOPE_H
