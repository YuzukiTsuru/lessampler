#include "TestSupport.h"

#include <format>
#include <thread>
#include <vector>

#include "Core/Log.h"


int main() {
    lessampler::log::SetDebug(false);
    LOG_DEBUG << "This debug message must stay hidden";
    CHECK(!lessampler::log::DebugEnabled());

    lessampler::log::SetDebug(true);
    CHECK(lessampler::log::DebugEnabled());
    LOG_DEBUG << "Debug is enabled";

    lessampler::log::SetDebug(false);
    CHECK(!lessampler::log::DebugEnabled());

    LOG_INFO << std::format("Formatted value: {}", 42);
    LOG_WARN << "Warning message";
    LOG_ERROR << "Error message";
    LOG_OK << "OK message";

    std::vector<std::jthread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([i] {
            for (int j = 0; j < 20; ++j) {
                LOG_INFO << std::format("thread {} message {}", i, j);
            }
        });
    }
    threads.clear();

    return test::finish();
}
