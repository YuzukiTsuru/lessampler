/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ConfigParser.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <fstream>
#include <optional>
#include <string_view>

#include "Utils/exception.h"
#include "lessconfig.h"

namespace {

    std::string_view trim(std::string_view value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.remove_prefix(1);
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
            value.remove_suffix(1);
        }
        return value;
    }

    std::string toml_quote(std::string_view value) {
        std::string result;
        result.reserve(value.size() + 2);
        result.push_back('"');
        for (char c: value) {
            if (c == '\\' || c == '"') {
                result.push_back('\\');
            }
            result.push_back(c);
        }
        result.push_back('"');
        return result;
    }

    [[noreturn]] void invalid_value(std::string_view section, std::string_view key, std::string_view reason) {
        throw parameter_error(std::format("{}.{}: {}", section, key, reason));
    }

    double read_double(const toml::table &table, std::string_view section, std::string_view key, double current) {
        const auto node = table[key];
        if (!node) {
            return current;
        }
        if (auto value = node.value<double>()) {
            return *value;
        }
        if (auto value = node.value<std::int64_t>()) {
            return static_cast<double>(*value);
        }
        invalid_value(section, key, "expected a number");
    }

    std::int64_t read_int(const toml::table &table, std::string_view section, std::string_view key, std::int64_t current) {
        const auto node = table[key];
        if (!node) {
            return current;
        }
        if (auto value = node.value<std::int64_t>()) {
            return *value;
        }
        if (auto value = node.value<double>()) {
            const auto rounded = static_cast<std::int64_t>(*value);
            if (static_cast<double>(rounded) == *value) {
                return rounded;
            }
            invalid_value(section, key, "expected an integer");
        }
        invalid_value(section, key, "expected a number");
    }

    void require_positive(double value, std::string_view section, std::string_view key) {
        if (!(value > 0.0)) {
            invalid_value(section, key, "must be greater than zero");
        }
    }

    template<typename T>
    bool parse_number(std::string_view text, T &value) {
        const auto *begin = text.data();
        const auto *end = begin + text.size();
        const auto result = std::from_chars(begin, end, value);
        return result.ec == std::errc{} && result.ptr == end;
    }

    bool parse_bool(std::string_view text, bool &value) {
        if (text == "1" || text == "true" || text == "TRUE" || text == "True") {
            value = true;
            return true;
        }
        if (text == "0" || text == "false" || text == "FALSE" || text == "False") {
            value = false;
            return true;
        }
        return false;
    }

}// namespace

void apply_config(const toml::table &table, lessConfigure &configure) {
    if (const auto *section = table["config"].as_table()) {
        if (const auto value = (*section)["version"].value<std::string>()) {
            configure.version = *value;
        }
        if (const auto value = (*section)["debug"].value<bool>()) {
            configure.debug_mode = *value;
        }
    }

    if (const auto *section = table["audio_model"].as_table()) {
        if (const auto value = (*section)["backend"].value<std::string>()) {
            const auto backend = lessbackend::ParseVocoderKind(*value);
            if (!backend) {
                invalid_value("audio_model", "backend", "expected \"world\" or \"llsm\"");
            }
            configure.vocoder = *backend;
        }
        configure.audio_model_frame_period = read_double(*section, "audio_model", "frame_period",
                                                         configure.audio_model_frame_period);
        configure.model_amp = read_double(*section, "audio_model", "model_amp", configure.model_amp);

        const auto fft_node = (*section)["fft_size"];
        if (const auto value = fft_node.value<std::string>()) {
            if (*value == "auto") {
                configure.custom_fft_size = false;
                configure.fft_size = 0;
            } else {
                invalid_value("audio_model", "fft_size", "expected \"auto\" or an integer");
            }
        } else {
            const auto int_value = read_int(*section, "audio_model", "fft_size", configure.fft_size);
            if (int_value <= 0) {
                invalid_value("audio_model", "fft_size", "must be greater than zero");
            }
            configure.custom_fft_size = true;
            configure.fft_size = static_cast<int>(int_value);
        }
    }

    if (const auto *section = table["f0"].as_table()) {
        if (const auto value = (*section)["f0_mode"].value<std::string>()) {
            configure.f0_mode = lessConfigure::parse_f0_mode(*value);
            if (configure.f0_mode == lessConfigure::F0_MODE::F0_MODE_UNKNOWN) {
                invalid_value("f0", "f0_mode", "expected \"DIO\" or \"HARVEST\"");
            }
        }
        configure.f0_speed = static_cast<int>(read_int(*section, "f0", "f0_speed", configure.f0_speed));
        configure.f0_dio_floor = read_double(*section, "f0", "f0_dio_floor", configure.f0_dio_floor);
        configure.f0_harvest_floor = read_double(*section, "f0", "f0_harvest_floor", configure.f0_harvest_floor);
        configure.f0_cheap_trick_floor = read_double(*section, "f0", "f0_cheap_trick_floor",
                                                     configure.f0_cheap_trick_floor);
        configure.f0_allow_range = read_double(*section, "f0", "f0_allow_range", configure.f0_allow_range);
    }

    if (const auto *section = table["ap"].as_table()) {
        configure.ap_threshold = read_double(*section, "ap", "ap_threshold", configure.ap_threshold);
    }

    require_positive(configure.audio_model_frame_period, "audio_model", "frame_period");
    if (configure.model_amp < 0.0) {
        invalid_value("audio_model", "model_amp", "must be zero or greater");
    }
    if (configure.custom_fft_size && configure.fft_size <= 0) {
        invalid_value("audio_model", "fft_size", "must be greater than zero");
    }
    if (configure.f0_speed <= 0) {
        invalid_value("f0", "f0_speed", "must be greater than zero");
    }
    require_positive(configure.f0_dio_floor, "f0", "f0_dio_floor");
    require_positive(configure.f0_harvest_floor, "f0", "f0_harvest_floor");
    require_positive(configure.f0_cheap_trick_floor, "f0", "f0_cheap_trick_floor");
    if (configure.f0_allow_range < 0.0) {
        invalid_value("f0", "f0_allow_range", "must be zero or greater");
    }
    if (configure.ap_threshold < 0.0 || configure.ap_threshold > 1.0) {
        invalid_value("ap", "ap_threshold", "must be between 0 and 1");
    }
}

std::string default_config_toml() {
    lessConfigure configure;
    configure.version = PROJECT_GIT_HASH;
    return std::format(R"(# lessampler configuration
# Changing audio_model, f0, or ap settings invalidates existing .lessaudio caches.

[config]
version = {}
debug = {}

[audio_model]
backend = "{}"           # "world" or "llsm"
frame_period = {}
fft_size = "auto"        # "auto" or an integer such as 1024
model_amp = {}           # 0 disables AutoAMP before modeling

[f0]
f0_mode = "{}"           # "DIO" or "HARVEST"
f0_speed = {}
f0_dio_floor = {}
f0_harvest_floor = {}
f0_cheap_trick_floor = {}
f0_allow_range = {}

[ap]
ap_threshold = {}
)",
                       toml_quote(configure.version),
                       configure.debug_mode,
                       lessbackend::VocoderKindName(configure.vocoder),
                       configure.audio_model_frame_period,
                       configure.model_amp,
                       lessConfigure::f0_mode_name(configure.f0_mode),
                       configure.f0_speed,
                       configure.f0_dio_floor,
                       configure.f0_harvest_floor,
                       configure.f0_cheap_trick_floor,
                       configure.f0_allow_range,
                       configure.ap_threshold);
}

void write_config_toml(const std::filesystem::path &path, const lessConfigure &configure) {
    std::ofstream out(path);
    if (!out) {
        throw file_open_error(path.string());
    }

    const std::string fft_size = configure.custom_fft_size ? std::to_string(configure.fft_size) : "\"auto\"";
    out << std::format(R"(# lessampler configuration
# Changing audio_model, f0, or ap settings invalidates existing .lessaudio caches.

[config]
version = {}
debug = {}

[audio_model]
backend = "{}"
frame_period = {}
fft_size = {}
model_amp = {}

[f0]
f0_mode = "{}"
f0_speed = {}
f0_dio_floor = {}
f0_harvest_floor = {}
f0_cheap_trick_floor = {}
f0_allow_range = {}

[ap]
ap_threshold = {}
)",
                       toml_quote(configure.version),
                       configure.debug_mode,
                       lessbackend::VocoderKindName(configure.vocoder),
                       configure.audio_model_frame_period,
                       fft_size,
                       configure.model_amp,
                       lessConfigure::f0_mode_name(configure.f0_mode),
                       configure.f0_speed,
                       configure.f0_dio_floor,
                       configure.f0_harvest_floor,
                       configure.f0_cheap_trick_floor,
                       configure.f0_allow_range,
                       configure.ap_threshold);

    if (!out) {
        throw file_open_error(path.string());
    }
}

bool parse_legacy_ini_file(const std::filesystem::path &path, lessConfigure &configure) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }

    std::string line;
    std::string section;
    while (std::getline(in, line)) {
        std::string_view value(line);
        if (!value.empty() && value.back() == '\r') {
            value.remove_suffix(1);
        }
        value = trim(value);
        if (value.empty() || value.front() == '#' || value.front() == ';') {
            continue;
        }

        if (value.front() == '[' && value.back() == ']') {
            section = std::string(trim(value.substr(1, value.size() - 2)));
            continue;
        }

        const auto separator = value.find('=');
        if (separator == std::string_view::npos) {
            continue;
        }

        const auto key = trim(value.substr(0, separator));
        auto raw_value = trim(value.substr(separator + 1));
        const auto comment = raw_value.find_first_of("#;");
        if (comment != std::string_view::npos) {
            raw_value = trim(raw_value.substr(0, comment));
        }

        if (section == "config") {
            if (key == "version") {
                configure.version = std::string(raw_value);
            } else if (key == "debug") {
                if (!parse_bool(raw_value, configure.debug_mode)) {
                    return false;
                }
            }
        } else if (section == "audio_model") {
            if (key == "backend") {
                const auto backend = lessbackend::ParseVocoderKind(raw_value);
                if (!backend) {
                    return false;
                }
                configure.vocoder = *backend;
            } else if (key == "frame_period") {
                if (!parse_number(raw_value, configure.audio_model_frame_period)) {
                    return false;
                }
            } else if (key == "fft_size") {
                if (raw_value == "auto") {
                    configure.custom_fft_size = false;
                    configure.fft_size = 0;
                } else {
                    if (!parse_number(raw_value, configure.fft_size)) {
                        return false;
                    }
                    configure.custom_fft_size = true;
                }
            } else if (key == "model_amp") {
                if (!parse_number(raw_value, configure.model_amp)) {
                    return false;
                }
            }
        } else if (section == "f0") {
            if (key == "f0_mode") {
                configure.f0_mode = lessConfigure::parse_f0_mode(raw_value);
                if (configure.f0_mode == lessConfigure::F0_MODE::F0_MODE_UNKNOWN) {
                    return false;
                }
            } else if (key == "f0_speed") {
                if (!parse_number(raw_value, configure.f0_speed)) {
                    return false;
                }
            } else if (key == "f0_dio_floor") {
                if (!parse_number(raw_value, configure.f0_dio_floor)) {
                    return false;
                }
            } else if (key == "f0_harvest_floor") {
                if (!parse_number(raw_value, configure.f0_harvest_floor)) {
                    return false;
                }
            } else if (key == "f0_cheap_trick_floor") {
                if (!parse_number(raw_value, configure.f0_cheap_trick_floor)) {
                    return false;
                }
            } else if (key == "f0_allow_range") {
                if (!parse_number(raw_value, configure.f0_allow_range)) {
                    return false;
                }
            }
        } else if (section == "ap") {
            if (key == "ap_threshold") {
                if (!parse_number(raw_value, configure.ap_threshold)) {
                    return false;
                }
            }
        }
    }

    return true;
}
