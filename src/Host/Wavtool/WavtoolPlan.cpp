/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Host/Wavtool/WavtoolPlan.h"

#include <cmath>
#include <cstdint>
#include <format>
#include <string>
#include <utility>

#include <toml++/toml.hpp>

#include "Audio/Dsp/MasterChain.h"
#include "Audio/WavIO.h"
#include "Core/Exception.h"
#include "Host/Wavtool/WavAssembly.h"

namespace lessampler::wavtool {

    namespace {

        std::filesystem::path from_utf8(std::string_view text) {
            std::u8string utf8;
            utf8.reserve(text.size());
            for (char character: text) {
                utf8.push_back(static_cast<char8_t>(character));
            }
            return std::filesystem::path(utf8);
        }

        std::string required_string(const toml::table &table, std::string_view key, std::string_view context) {
            const auto value = table[key].value<std::string>();
            if (!value) {
                throw parameter_error(std::format("{} is missing the string field '{}'", context, key));
            }
            return *value;
        }

        double required_double(const toml::table &table, std::string_view key, std::string_view context) {
            const auto value = table[key].value<double>();
            if (!value) {
                throw parameter_error(std::format("{} is missing the numeric field '{}'", context, key));
            }
            if (!std::isfinite(*value)) {
                throw parameter_error(std::format("{} field '{}' is not finite", context, key));
            }
            return *value;
        }

        double optional_double(const toml::table &table, std::string_view key, double fallback, std::string_view context) {
            const auto value = table[key].value<double>();
            if (!value) {
                return fallback;
            }
            if (!std::isfinite(*value)) {
                throw parameter_error(std::format("{} field '{}' is not finite", context, key));
            }
            return *value;
        }

        int optional_int(const toml::table &table, std::string_view key, std::string_view context) {
            const auto value = table[key].value<std::int64_t>();
            if (!value) {
                return 0;
            }
            if (*value <= 0 || *value > 2147483647) {
                throw parameter_error(std::format("{} field '{}' must be a positive integer", context, key));
            }
            return static_cast<int>(*value);
        }

    }// namespace

    WavtoolPlan WavtoolPlan::Parse(const std::filesystem::path &path) {
        toml::table table;
        try {
            table = toml::parse_file(path.native());
        } catch (const toml::parse_error &error) {
            throw parameter_error(std::format("invalid wavtool plan {}: {}", path.string(), std::string(error.description())));
        }

        WavtoolPlan plan;
        plan.output = from_utf8(required_string(table, "output", "wavtool plan"));
        plan.sample_rate = optional_int(table, "sample_rate", "wavtool plan");
        plan.channels = optional_int(table, "channels", "wavtool plan");
        plan.peak_normalize = optional_double(table, "peak_normalize", 0.0, "wavtool plan");
        if (plan.peak_normalize < 0.0) {
            throw parameter_error("wavtool plan peak_normalize must not be negative");
        }
        if (const auto *master = table.get("master")) {
            const auto value = master->value<std::string>();
            if (!value) {
                throw parameter_error("wavtool plan field 'master' must be a string");
            }
            plan.master = *value;
        }

        const auto *notes = table["notes"].as_array();
        if (notes == nullptr) {
            throw parameter_error("wavtool plan is missing the [[notes]] array");
        }
        plan.notes.reserve(notes->size());
        for (const auto &node: *notes) {
            const auto *note = node.as_table();
            if (note == nullptr) {
                throw parameter_error("wavtool plan notes must be tables");
            }
            WavtoolPlanNote entry;
            entry.input = from_utf8(required_string(*note, "input", "wavtool plan note"));
            entry.position_ms = required_double(*note, "position_ms", "wavtool plan note");
            entry.skip_ms = optional_double(*note, "skip_ms", 0.0, "wavtool plan note");
            entry.length_ms = required_double(*note, "length_ms", "wavtool plan note");
            if (entry.length_ms < 0.0) {
                throw parameter_error("wavtool plan note length_ms must not be negative");
            }
            const auto envelope = note->get("envelope");
            if (envelope != nullptr) {
                const auto text = envelope->value<std::string>();
                if (!text) {
                    throw parameter_error("wavtool plan note field 'envelope' must be a string");
                }
                entry.envelope = Envelope::Parse(*text);
            }
            plan.notes.push_back(std::move(entry));
        }

        return plan;
    }

    void WavtoolPlan::Run(const WavtoolPlan &plan) {
        if (plan.output.empty()) {
            throw parameter_error("wavtool plan output is empty");
        }
        if (plan.notes.empty()) {
            throw parameter_error("wavtool plan has no notes");
        }

        int sample_rate = plan.sample_rate;
        int channels = plan.channels;
        if (sample_rate <= 0 || channels <= 0) {
            for (const auto &note: plan.notes) {
                if (!std::filesystem::exists(note.input)) {
                    continue;
                }
                const auto info = WavIO::ProbeWav(note.input);
                if (sample_rate <= 0) {
                    sample_rate = info.sample_rate;
                }
                if (channels <= 0) {
                    channels = info.channels;
                }
                break;
            }
        }
        if (sample_rate <= 0) {
            sample_rate = 44100;
        }
        if (channels <= 0) {
            channels = 1;
        }

        WavBuffer output;
        output.sample_rate = sample_rate;
        output.channels = channels;

        for (const auto &note: plan.notes) {
            const auto position = static_cast<std::int64_t>(
                    std::llround(note.position_ms * static_cast<double>(sample_rate) / 1000.0));
            WavAssembly::AppendAt(output, WavSegment{note.input, note.skip_ms, note.length_ms, note.envelope}, position);
        }

        MasterChain::Process(output.samples, output.sample_rate, output.channels,
                             MasterOptions::Preset(plan.master));
        WavAssembly::Write(output, plan.output, plan.peak_normalize);
    }

}// namespace lessampler::wavtool
