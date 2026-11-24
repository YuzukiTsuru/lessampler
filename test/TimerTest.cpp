#include "TestSupport.h"

#include <chrono>
#include <thread>

#include "Core/Timer.h"

using namespace lessampler;

int main() {
    Timer timer;
    timer.SetTimer();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    const auto elapsed = timer.GetTimer("elapsed: ");
    CHECK(elapsed.starts_with("elapsed: "));
    CHECK(elapsed.size() > std::string("elapsed: ").size());

    const auto total = timer.EndTimer();
    CHECK(!total.empty());

    return test::finish();
}
