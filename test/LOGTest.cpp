#include "TestSupport.h"

#include <format>
#include <thread>
#include <vector>

#include "Utils/LOG.h"

int main() {
    lessampler_log::debug_flag().store(false);
    YALL_DEBUG_ << "This debug message must stay hidden";
    CHECK(!lessampler_log::debug_flag().load());

    YALL_DEBUG_.EnableDebug();
    CHECK(lessampler_log::debug_flag().load());
    YALL_DEBUG_ << "Debug is enabled";

    YALL_DEBUG_.DisableDebug();
    CHECK(!lessampler_log::debug_flag().load());

    YALL_INFO_ << std::format("Formatted value: {}", 42);
    YALL_WARN_ << "Warning message";
    YALL_ERROR_ << "Error message";
    YALL_OK_ << "OK message";

    std::vector<std::jthread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([i] {
            for (int j = 0; j < 20; ++j) {
                YALL_INFO_ << std::format("thread {} message {}", i, j);
            }
        });
    }
    threads.clear();

    return test::finish();
}
