#include "TestSupport.h"

#include "Shine/Binding/libUTAU/ScaleConvert.h"

int main() {
    const auto a4 = ScaleConvert("A4").GetScaleNum();
    REQUIRE(a4.has_value());
    CHECK_NEAR(*a4, 440.0, 1e-9);

    const auto a_sharp_3 = ScaleConvert("A#3").GetScaleNum();
    REQUIRE(a_sharp_3.has_value());
    CHECK_NEAR(*a_sharp_3, 233.08188075904496, 1e-9);

    const auto c4 = ScaleConvert("C4").GetScaleNum();
    REQUIRE(c4.has_value());
    CHECK_NEAR(*c4, 261.6255653005986, 1e-9);

    CHECK(!ScaleConvert("").GetScaleNum().has_value());
    CHECK(!ScaleConvert("H4").GetScaleNum().has_value());
    CHECK(!ScaleConvert("C#").GetScaleNum().has_value());
    CHECK(!ScaleConvert("E#4").GetScaleNum().has_value());

    return test::finish();
}
