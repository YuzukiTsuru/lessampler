/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_TIMER_H
#define LESSAMPLER_TIMER_H

#include <chrono>
#include <format>
#include <string>
#include <string_view>

namespace lessampler {

    class Timer {
    public:
        Timer() : start_time(get_now()), global_start_time(start_time) {}

        void SetTimer() {
            start_time = get_now();
        }

        [[nodiscard]] std::string GetTimer(std::string_view info) const {
            return std::format("{}{}", info, format_duration(get_now() - start_time));
        }

        [[nodiscard]] std::string EndTimer() const {
            return format_duration(get_now() - global_start_time);
        }

    private:
        using Clock = std::chrono::steady_clock;
        using Duration = std::chrono::duration<double, std::milli>;

        static Clock::time_point get_now() {
            return Clock::now();
        }

        static std::string format_duration(Duration duration) {
            if (duration.count() < 1.0) {
                return std::format("{:.0f}us", duration.count() * 1000.0);
            }
            return std::format("{:.0f}ms", duration.count());
        }

        Clock::time_point start_time;
        Clock::time_point global_start_time;
    };

}// namespace lessampler

#endif//LESSAMPLER_TIMER_H
