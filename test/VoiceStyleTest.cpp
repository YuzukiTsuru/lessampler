#include "TestSupport.h"

#include "Core/VoiceStyle.h"

using namespace lessampler;

int main() {
    CHECK_EQ(VoiceStyleKindName(VoiceStyleKind::natural), "natural");
    CHECK_EQ(VoiceStyleKindName(VoiceStyleKind::clear), "clear");
    CHECK_EQ(VoiceStyleKindName(VoiceStyleKind::cry), "cry");
    CHECK_EQ(VoiceStyleKindName(VoiceStyleKind::growl), "growl");

    CHECK(ParseVoiceStyleKind("NATURAL") == VoiceStyleKind::natural);
    CHECK(ParseVoiceStyleKind("Clear") == VoiceStyleKind::clear);
    CHECK(ParseVoiceStyleKind("cRy") == VoiceStyleKind::cry);
    CHECK(ParseVoiceStyleKind("GROWL") == VoiceStyleKind::growl);
    CHECK(!ParseVoiceStyleKind("unknown").has_value());

    const auto clear = WeightsFor(VoiceStyleKind::clear, 0.5);
    CHECK_NEAR(clear.clear, 0.5, 1e-12);
    CHECK_EQ(clear.cry, 0.0);
    CHECK_EQ(clear.growl, 0.0);
    CHECK(WeightsFor(VoiceStyleKind::clear, 2.0).clear == 1.0);
    CHECK(WeightsFor(VoiceStyleKind::natural, 1.0).empty());

    {
        VoiceStyle style;
        CHECK(ParseVoiceStyleFlags("C100", style));
        CHECK(style.from_flags);
        CHECK_NEAR(style.start.clear, 1.0, 1e-12);
        CHECK(style.start == style.end);
        CHECK(!style.morphs());
    }

    {
        VoiceStyle style;
        CHECK(ParseVoiceStyleFlags("Y60G40", style));
        CHECK_NEAR(style.start.cry, 0.6, 1e-12);
        CHECK_NEAR(style.start.growl, 0.4, 1e-12);
        CHECK_EQ(style.start.clear, 0.0);
        CHECK(style.start == style.end);
    }

    {
        VoiceStyle style;
        CHECK(ParseVoiceStyleFlags("C100>G60", style));
        CHECK_NEAR(style.start.clear, 1.0, 1e-12);
        CHECK_NEAR(style.end.growl, 0.6, 1e-12);
        CHECK(style.morphs());
    }

    {
        VoiceStyle style;
        CHECK(ParseVoiceStyleFlags("C50Y50>C100", style));
        CHECK_NEAR(style.start.clear, 0.5, 1e-12);
        CHECK_NEAR(style.start.cry, 0.5, 1e-12);
        CHECK_NEAR(style.end.clear, 1.0, 1e-12);
        CHECK_EQ(style.end.cry, 0.0);
    }

    {
        VoiceStyle style;
        CHECK(ParseVoiceStyleFlags("C0Y0G0", style));
        CHECK(style.from_flags);
        CHECK(style.start.empty());
        CHECK(style.end.empty());
    }

    {
        VoiceStyle style;
        CHECK(ParseVoiceStyleFlags("g+10C50", style));
        CHECK_NEAR(style.start.clear, 0.5, 1e-12);
    }

    {
        VoiceStyle style;
        CHECK(!ParseVoiceStyleFlags("B50", style));
        CHECK(!style.from_flags);
        CHECK(!style.active());
    }

    {
        VoiceStyle style;
        CHECK(!ParseVoiceStyleFlags(">", style));
    }

    const auto midpoint = LerpWeights(StyleWeights{1.0, 0.0, 0.0}, StyleWeights{0.0, 0.0, 1.0}, 0.5);
    CHECK_NEAR(midpoint.clear, 0.5, 1e-12);
    CHECK_NEAR(midpoint.growl, 0.5, 1e-12);

    CHECK_NEAR(StyleMorphPosition(500.0, 1000.0, 0.0), 0.5, 1e-12);
    CHECK_NEAR(StyleMorphPosition(50.0, 1000.0, 100.0), 0.5, 1e-12);
    CHECK_NEAR(StyleMorphPosition(500.0, 1000.0, 100.0), 1.0, 1e-12);

    return test::finish();
}
