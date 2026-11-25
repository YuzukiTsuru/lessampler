/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Envelope.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <string_view>
#include <system_error>
#include <vector>

#include "Core/Exception.h"

namespace lessampler {

    namespace {

        struct Point {
            double x = 0.0;
            double gain = 0.0;
        };

        std::string_view trim(std::string_view text) {
            while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r')) {
                text.remove_prefix(1);
            }
            while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) {
                text.remove_suffix(1);
            }
            return text;
        }

        double parse_value(std::string_view text, double fallback) {
            text = trim(text);
            if (text.empty()) {
                return fallback;
            }
            double value{};
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(value)) {
                return fallback;
            }
            return value;
        }

        std::vector<std::string_view> split(std::string_view text) {
            std::vector<std::string_view> fields;
            while (true) {
                const auto comma = text.find(',');
                if (comma == std::string_view::npos) {
                    fields.push_back(text);
                    break;
                }
                fields.push_back(text.substr(0, comma));
                text.remove_prefix(comma + 1);
            }
            return fields;
        }

        double clamp_volume(double value) {
            return std::clamp(value, 0.0, 200.0);
        }

    }// namespace

    Envelope Envelope::Default() {
        return Envelope{};
    }

    Envelope Envelope::Parse(std::string_view text) {
        Envelope envelope = Default();
        text = trim(text);
        if (text.empty()) {
            return envelope;
        }

        const auto fields = split(text);

        const auto field_value = [&fields](std::size_t index, double fallback) {
            if (index >= fields.size()) {
                return fallback;
            }
            const auto field = trim(fields[index]);
            if (field.empty()) {
                return 0.0;
            }
            return parse_value(field, fallback);
        };

        envelope.p1 = field_value(0, envelope.p1);
        envelope.p2 = field_value(1, envelope.p2);
        envelope.p3 = field_value(2, envelope.p3);
        envelope.v1 = field_value(3, envelope.v1);
        envelope.v2 = field_value(4, envelope.v2);
        envelope.v3 = field_value(5, envelope.v3);
        envelope.v4 = field_value(6, envelope.v4);

        // Optional release section: [%], p4, p5, v5. A bare numeric first
        // element is accepted too, for hosts that omit the % placeholder.
        std::size_t index = 7;
        if (index < fields.size() && trim(fields[index]) == "%") {
            ++index;
        }
        envelope.p4 = field_value(index, envelope.p4);
        envelope.p5 = field_value(index + 1, envelope.p5);
        envelope.v5 = field_value(index + 2, envelope.v5);

        envelope.p1 = std::max(0.0, envelope.p1);
        envelope.p2 = std::max(0.0, envelope.p2);
        envelope.p3 = std::max(0.0, envelope.p3);
        envelope.p4 = std::max(0.0, envelope.p4);
        envelope.p5 = std::max(0.0, envelope.p5);
        envelope.v1 = clamp_volume(envelope.v1);
        envelope.v2 = clamp_volume(envelope.v2);
        envelope.v3 = clamp_volume(envelope.v3);
        envelope.v4 = clamp_volume(envelope.v4);
        envelope.v5 = clamp_volume(envelope.v5);

        return envelope;
    }

    std::vector<double> Envelope::GainCurve(std::size_t sample_count, int sample_rate) const {
        if (sample_count == 0) {
            return {};
        }
        if (sample_rate <= 0) {
            throw parameter_error("Envelope sample rate must be positive");
        }

        const double length_ms = static_cast<double>(sample_count) * 1000.0 / static_cast<double>(sample_rate);

        std::vector<Point> points;
        points.reserve(7);
        points.push_back(Point{0.0, 0.0});
        points.push_back(Point{p1, v1 / 100.0});
        points.push_back(Point{p1 + p2, v2 / 100.0});
        points.push_back(Point{p1 + p2 + p5, v5 / 100.0});
        points.push_back(Point{length_ms - p4 - p3, v3 / 100.0});
        points.push_back(Point{length_ms - p4, v4 / 100.0});
        points.push_back(Point{length_ms, 0.0});

        for (auto &point: points) {
            point.x = std::clamp(point.x, 0.0, length_ms);
        }
        for (std::size_t i = 1; i < points.size(); ++i) {
            points[i].x = std::max(points[i].x, points[i - 1].x);
        }

        std::vector<double> gains(sample_count);
        std::size_t segment = 0;
        for (std::size_t i = 0; i < sample_count; ++i) {
            const double time_ms = static_cast<double>(i) * 1000.0 / static_cast<double>(sample_rate);
            while (segment + 1 < points.size() && time_ms > points[segment + 1].x) {
                ++segment;
            }

            const Point &left = points[segment];
            const Point &right = points[std::min(segment + 1, points.size() - 1)];
            if (right.x <= left.x) {
                gains[i] = right.gain;
            } else {
                const double fraction = std::clamp((time_ms - left.x) / (right.x - left.x), 0.0, 1.0);
                gains[i] = left.gain + (right.gain - left.gain) * fraction;
            }
        }

        return gains;
    }

}// namespace lessampler
