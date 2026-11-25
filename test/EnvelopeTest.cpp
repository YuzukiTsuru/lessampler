#include "TestSupport.h"

#include <cmath>
#include <vector>

#include "Host/Wavtool/Envelope.h"

using namespace lessampler::wavtool;

int main() {
    const auto fallback = Envelope::Default();
    CHECK_NEAR(fallback.p1, 0.0, 1e-9);
    CHECK_NEAR(fallback.p2, 5.0, 1e-9);
    CHECK_NEAR(fallback.p3, 35.0, 1e-9);
    CHECK_NEAR(fallback.p4, 0.0, 1e-9);
    CHECK_NEAR(fallback.p5, 10.0, 1e-9);
    CHECK_NEAR(fallback.v1, 0.0, 1e-9);
    CHECK_NEAR(fallback.v2, 100.0, 1e-9);
    CHECK_NEAR(fallback.v3, 100.0, 1e-9);
    CHECK_NEAR(fallback.v4, 0.0, 1e-9);
    CHECK_NEAR(fallback.v5, 100.0, 1e-9);

    const auto seven = Envelope::Parse("0,35.7,35.7,0,100,100,0");
    CHECK_NEAR(seven.p1, 0.0, 1e-9);
    CHECK_NEAR(seven.p2, 35.7, 1e-9);
    CHECK_NEAR(seven.p3, 35.7, 1e-9);
    CHECK_NEAR(seven.v1, 0.0, 1e-9);
    CHECK_NEAR(seven.v2, 100.0, 1e-9);
    CHECK_NEAR(seven.v3, 100.0, 1e-9);
    CHECK_NEAR(seven.v4, 0.0, 1e-9);
    CHECK_NEAR(seven.p4, 0.0, 1e-9);
    CHECK_NEAR(seven.p5, 10.0, 1e-9);
    CHECK_NEAR(seven.v5, 100.0, 1e-9);

    const auto full = Envelope::Parse("10,20,30,0,80,90,100,%,5,6,110");
    CHECK_NEAR(full.p1, 10.0, 1e-9);
    CHECK_NEAR(full.p2, 20.0, 1e-9);
    CHECK_NEAR(full.p3, 30.0, 1e-9);
    CHECK_NEAR(full.v1, 0.0, 1e-9);
    CHECK_NEAR(full.v2, 80.0, 1e-9);
    CHECK_NEAR(full.v3, 90.0, 1e-9);
    CHECK_NEAR(full.v4, 100.0, 1e-9);
    CHECK_NEAR(full.p4, 5.0, 1e-9);
    CHECK_NEAR(full.p5, 6.0, 1e-9);
    CHECK_NEAR(full.v5, 110.0, 1e-9);

    const auto odd = Envelope::Parse("0,53.6,7,0,100,153,0,%,35");
    CHECK_NEAR(odd.p1, 0.0, 1e-9);
    CHECK_NEAR(odd.p2, 53.6, 1e-9);
    CHECK_NEAR(odd.p3, 7.0, 1e-9);
    CHECK_NEAR(odd.v3, 153.0, 1e-9);
    CHECK_NEAR(odd.p4, 35.0, 1e-9);

    const auto empty = Envelope::Parse("");
    CHECK_NEAR(empty.p2, fallback.p2, 1e-9);
    CHECK_NEAR(empty.v2, fallback.v2, 1e-9);

    const auto loud = Envelope::Parse("0,0,0,300,300,300,300,%,0,0,300");
    CHECK_NEAR(loud.v1, 200.0, 1e-9);
    CHECK_NEAR(loud.v2, 200.0, 1e-9);
    CHECK_NEAR(loud.v3, 200.0, 1e-9);
    CHECK_NEAR(loud.v4, 200.0, 1e-9);
    CHECK_NEAR(loud.v5, 200.0, 1e-9);

    const auto gains = fallback.GainCurve(1000, 1000);
    REQUIRE(gains.size() == 1000);
    CHECK_NEAR(gains[0], 0.0, 0.01);
    CHECK_NEAR(gains[5], 1.0, 0.01);
    CHECK_NEAR(gains[500], 1.0, 0.01);
    CHECK_NEAR(gains[965], 1.0, 0.01);
    CHECK(gains[999] < 0.05);
    for (double gain: gains) {
        CHECK(gain >= 0.0 && gain <= 1.0);
    }

    const auto crossed = Envelope::Parse("100,50,200,0,100,100,0");
    const auto crossed_gains = crossed.GainCurve(1000, 1000);
    REQUIRE(crossed_gains.size() == 1000);
    for (double gain: crossed_gains) {
        CHECK(std::isfinite(gain));
        CHECK(gain >= 0.0 && gain <= 1.0);
    }

    const auto none = fallback.GainCurve(0, 44100);
    CHECK(none.empty());

    return test::finish();
}
