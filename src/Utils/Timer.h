/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/6/23.
//

#include <iostream>
#include <chrono>

class Timer {
public:
    Timer() {
        g_start_time = get_perf_count();
        start_time = g_start_time;
    };

    void SetTimer() {
        end_time = 0;
        start_time = get_perf_count();
    }

    uint64_t GetTimer() {
        end_time = get_perf_count();
        return end_time - start_time;
    }

    std::string GetTimer(const std::string &info) {
        end_time = get_perf_count();
        return [&]() -> std::string {
            auto time = end_time - start_time;
            if (time / 1000 == 0)
                return info + std::to_string(end_time - start_time) + "us";
            else
                return info + std::to_string((end_time - start_time) / 1000) + "ms";
        }();
    }

    std::string EndTimer() {
        g_end_time = get_perf_count();
        return [&]() -> std::string {
            auto time = g_end_time - g_start_time;
            if (time / 1000 == 0)
                return std::to_string(g_end_time - g_start_time) + "us";
            else
                return std::to_string((g_end_time - g_start_time) / 1000) + "ms";
        }();
    }

protected:
    uint64_t start_time = 0;
    uint64_t end_time = 0;

    uint64_t g_start_time = 0;
    uint64_t g_end_time = 0;

private:
    static uint64_t get_perf_count() {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    };
};

