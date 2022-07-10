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
#include <mutex>

#if defined(_WIN32) || defined(UNDER_CE)

#include <windows.h>

#endif

class Timer {
public:
    Timer(Timer &other) = delete;

    void operator=(const Timer &) = delete;

    static Timer *GetInstance() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (TimerInstance_ == nullptr) {
            TimerInstance_ = new Timer();
        }
        return TimerInstance_;
    };

    void SetTimer() {
        start_time = get_perf_count();
    }

    [[nodiscard]] uint64_t StopTimer() {
        end_time = get_perf_count();
        return end_time - start_time / 10000;
    }

protected:
    Timer() = default;

    ~Timer() = default;

    uint64_t start_time = 0;
    uint64_t end_time = 0;

private:
    static Timer *TimerInstance_;
    static std::mutex mutex_;

    static uint64_t get_perf_count() {
#if defined(__linux__) || defined(__ANDROID__) || defined(__QNX__) || defined(__CYGWIN__)
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)((uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * 1000000000);
#elif defined(_WIN32) || defined(UNDER_CE)
        LARGE_INTEGER ln;
        QueryPerformanceCounter(&ln);
        return (uint64_t) ln.QuadPart;
#endif
    };
};

Timer *Timer::TimerInstance_ = nullptr;
std::mutex Timer::mutex_;

int main() {
    Timer::GetInstance();
}