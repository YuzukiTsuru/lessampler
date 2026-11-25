#include "TestSupport.h"

#include <array>
#include <string_view>

#include "Core/Exception.h"
#include "Host/Wavtool/WavtoolProtocol.h"

using namespace lessampler;
using namespace lessampler::wavtool;

int main() {
    const std::array<std::string_view, 16> args{
            "lessampler", "out.wav", "in.wav", "10", "100",
            "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};

    const auto request = WavtoolProtocol::Parse(args);
    CHECK_EQ(request.output.string(), "out.wav");
    CHECK_EQ(request.input.string(), "in.wav");
    CHECK_NEAR(request.skip_ms, 10.0, 1e-9);
    CHECK_NEAR(request.length_ms, 100.0, 1e-9);
    CHECK_NEAR(request.overlap_ms, 8.0, 1e-9);
    CHECK_NEAR(request.envelope.p1, 1.0, 1e-9);
    CHECK_NEAR(request.envelope.p2, 2.0, 1e-9);
    CHECK_NEAR(request.envelope.p3, 3.0, 1e-9);
    CHECK_NEAR(request.envelope.v1, 4.0, 1e-9);
    CHECK_NEAR(request.envelope.v2, 5.0, 1e-9);
    CHECK_NEAR(request.envelope.v3, 6.0, 1e-9);
    CHECK_NEAR(request.envelope.v4, 7.0, 1e-9);
    CHECK_NEAR(request.envelope.p4, 9.0, 1e-9);
    CHECK_NEAR(request.envelope.p5, 10.0, 1e-9);
    CHECK_NEAR(request.envelope.v5, 11.0, 1e-9);

    const std::array<std::string_view, 5> minimal{"lessampler", "out.wav", "in.wav", "0", "100"};
    const auto defaults = WavtoolProtocol::Parse(minimal);
    CHECK_NEAR(defaults.overlap_ms, 0.0, 1e-9);
    CHECK_NEAR(defaults.envelope.p1, 0.0, 1e-9);
    CHECK_NEAR(defaults.envelope.p2, 5.0, 1e-9);
    CHECK_NEAR(defaults.envelope.p3, 35.0, 1e-9);
    CHECK_NEAR(defaults.envelope.v1, 0.0, 1e-9);
    CHECK_NEAR(defaults.envelope.v2, 100.0, 1e-9);
    CHECK_NEAR(defaults.envelope.v3, 100.0, 1e-9);
    CHECK_NEAR(defaults.envelope.v4, 0.0, 1e-9);
    CHECK_NEAR(defaults.envelope.p4, 0.0, 1e-9);
    CHECK_NEAR(defaults.envelope.p5, 10.0, 1e-9);
    CHECK_NEAR(defaults.envelope.v5, 100.0, 1e-9);

    CHECK_NEAR(WavtoolProtocol::ParseDuration("100.5"), 100.5, 1e-9);
    CHECK_NEAR(WavtoolProtocol::ParseDuration("480@120+100"), 600.0, 1e-9);
    CHECK_NEAR(WavtoolProtocol::ParseDuration("480@120-50"), 450.0, 1e-9);
    CHECK_NEAR(WavtoolProtocol::ParseDuration("240@60"), 500.0, 1e-9);

    bool rejected = false;
    try {
        (void) WavtoolProtocol::Parse(std::array<std::string_view, 4>{"lessampler", "out.wav", "in.wav", "0"});
    } catch (const parameter_error &) {
        rejected = true;
    }
    CHECK(rejected);

    rejected = false;
    try {
        (void) WavtoolProtocol::ParseDuration("nope");
    } catch (const parameter_error &) {
        rejected = true;
    }
    CHECK(rejected);

    return test::finish();
}
