#include "TestSupport.h"

#include <cmath>
#include <limits>
#include <vector>

#include "Audio/AutoAMP.h"

using namespace lessampler;

int main() {
    std::vector<double> input{0.5, -0.25, std::numeric_limits<double>::quiet_NaN()};
    AutoAMP::ApplyInPlace(input, 1.0);

    REQUIRE(input.size() == 3);
    CHECK_NEAR(input[0], 0.5, 1e-12);
    CHECK_NEAR(input[1], -0.25, 1e-12);
    CHECK_NEAR(input[2], 0.0, 1e-12);
    for (double value: input) {
        CHECK(std::isfinite(value));
        CHECK(std::abs(value) <= 1.0);
    }

    std::vector<double> zeros(4, 0.0);
    AutoAMP::ApplyInPlace(zeros, 1.0);
    for (double value: zeros) {
        CHECK_NEAR(value, 0.0, 1e-12);
    }

    std::vector<double> large{100.0, -100.0};
    AutoAMP::ApplyInPlace(large, 1.0);
    REQUIRE(large.size() == 2);
    CHECK_NEAR(large[0], 0.5, 1e-12);
    CHECK_NEAR(large[1], -0.5, 1e-12);

    return test::finish();
}
