/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_LOG_H
#define LESSAMPLER_LOG_H

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace lessampler {
    namespace log {

        enum class Level : std::uint8_t {
            dump,
            eval,
            debug,
            ok,
            info,
            warn,
            error,
            critical,
        };

        namespace color {
            inline constexpr std::string_view reset = "\033[0m";
            inline constexpr std::string_view red = "\033[31m";
            inline constexpr std::string_view green = "\033[32m";
            inline constexpr std::string_view yellow = "\033[33m";
            inline constexpr std::string_view blue = "\033[34m";
            inline constexpr std::string_view magenta = "\033[35m";
            inline constexpr std::string_view cyan = "\033[36m";
            inline constexpr std::string_view white = "\033[37m";
            inline constexpr std::string_view on_red = "\033[41m";
            inline constexpr std::string_view bold = "\033[1m";
            inline constexpr std::string_view dim = "\033[2m";
            inline constexpr std::string_view bright_cyan = "\033[96m";
            inline constexpr std::string_view bright_magenta = "\033[95m";
        }// namespace color

        inline std::atomic<bool> &debug_flag() {
            static std::atomic<bool> enabled{false};
            return enabled;
        }

        inline void SetDebug(bool enabled) {
            debug_flag().store(enabled, std::memory_order_relaxed);
        }

        [[nodiscard]] inline bool DebugEnabled() {
            return debug_flag().load(std::memory_order_relaxed);
        }

        inline std::mutex &log_mutex() {
            static std::mutex mutex;
            return mutex;
        }

        inline void enable_ansi() {
#if defined(_WIN32)
            static bool enabled = []() {
                HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
                if (handle == INVALID_HANDLE_VALUE) {
                    return false;
                }
                DWORD mode = 0;
                if (!GetConsoleMode(handle, &mode)) {
                    return false;
                }
                return SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
            }();
            (void) enabled;
#endif
        }

        inline std::string_view level_name(Level level) {
            switch (level) {
                case Level::dump:
                    return "DUMP";
                case Level::eval:
                    return "TIME";
                case Level::debug:
                    return "DEBUG";
                case Level::ok:
                    return "OKAY";
                case Level::info:
                    return "INFO";
                case Level::warn:
                    return "WARN";
                case Level::error:
                    return "ERRO";
                case Level::critical:
                    return "CRIT";
            }
            return "INFO";
        }

        inline std::string_view level_color(Level level) {
            switch (level) {
                case Level::ok:
                    return color::green;
                case Level::info:
                    return color::cyan;
                case Level::warn:
                    return color::yellow;
                case Level::error:
                    return color::red;
                case Level::critical:
                    return color::on_red;
                case Level::eval:
                    return color::magenta;
                case Level::debug:
                case Level::dump:
                    return color::white;
            }
            return color::reset;
        }

        class Line {
        public:
            Line(Level level, bool active, std::string_view file, std::string_view function, int line)
                : level_(level), active_(active), file_(file), function_(function), line_number_(line) {}

            Line(const Line &) = delete;
            Line &operator=(const Line &) = delete;

            Line(Line &&other) noexcept
                : level_(other.level_), active_(other.active_), file_(other.file_),
                  function_(other.function_), line_number_(other.line_number_), message_(std::move(other.message_)) {
                other.active_ = false;
            }

            Line &operator=(Line &&) = delete;

            ~Line() {
                if (!active_ || message_.empty()) {
                    return;
                }

                std::lock_guard<std::mutex> lock(log_mutex());
                enable_ansi();
                std::cout << level_color(level_) << "[" << level_name(level_) << "]" << color::reset << " ";
                if (level_ == Level::debug || level_ == Level::dump) {
                    std::cout << color::cyan << "[" << function_ << "]" << color::reset << " "
                              << color::yellow << "[" << file_ << ":" << line_number_ << "]" << color::reset << " ";
                }
                std::cout << message_ << std::endl;
            }

            template<typename T>
            Line &operator<<(const T &value) {
                if (active_) {
                    message_ += std::format("{}", value);
                }
                return *this;
            }

            Line &operator<<(const std::filesystem::path &value) {
                if (active_) {
                    message_ += value.string();
                }
                return *this;
            }

        private:
            Level level_;
            bool active_;
            std::string_view file_;
            std::string_view function_;
            int line_number_;
            std::string message_;
        };

        class DebugLine : public Line {
        public:
            DebugLine(std::string_view file, std::string_view function, int line)
                : Line(Level::debug, DebugEnabled(), file, function, line) {}
        };

        inline Line dump_line(std::string_view file, std::string_view function, int line) {
#if defined(LESSAMPLER_DUMP_DATA)
            return {Level::dump, true, file, function, line};
#else
            return {Level::dump, false, file, function, line};
#endif
        }

        inline Line eval_line(std::string_view file, std::string_view function, int line) {
#if defined(LESSAMPLER_TIME_EVAL)
            return {Level::eval, true, file, function, line};
#else
            return {Level::eval, false, file, function, line};
#endif
        }

        inline Line make_line(Level level, std::string_view file, std::string_view function, int line) {
            return {level, true, file, function, line};
        }

    }// namespace log
}// namespace lessampler

#if defined(__GNUC__) || defined(__clang__)
#define LOG_FUNCTION_ __PRETTY_FUNCTION__
#else
#define LOG_FUNCTION_ __func__
#endif

#define LOG_DUMP ::lessampler::log::dump_line(__FILE__, LOG_FUNCTION_, __LINE__)
#define LOG_EVAL ::lessampler::log::eval_line(__FILE__, LOG_FUNCTION_, __LINE__)
#define LOG_DEBUG ::lessampler::log::DebugLine(__FILE__, LOG_FUNCTION_, __LINE__)
#define LOG_OK ::lessampler::log::make_line(::lessampler::log::Level::ok, __FILE__, LOG_FUNCTION_, __LINE__)
#define LOG_INFO ::lessampler::log::make_line(::lessampler::log::Level::info, __FILE__, LOG_FUNCTION_, __LINE__)
#define LOG_WARN ::lessampler::log::make_line(::lessampler::log::Level::warn, __FILE__, LOG_FUNCTION_, __LINE__)
#define LOG_ERROR ::lessampler::log::make_line(::lessampler::log::Level::error, __FILE__, LOG_FUNCTION_, __LINE__)
#define LOG_CRITICAL ::lessampler::log::make_line(::lessampler::log::Level::critical, __FILE__, LOG_FUNCTION_, __LINE__)

#endif//LESSAMPLER_LOG_H
