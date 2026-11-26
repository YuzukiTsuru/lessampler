#include "TestSupport.h"

#include <stdexcept>
#include <string>
#include <string_view>

#include "Audio/Dsp/MasterChain.h"
#include "Config/NeuralVocoderSpec.h"

using namespace lessampler;

namespace {

    bool same_master_options(const MasterOptions &a, const MasterOptions &b) {
        return a.enabled == b.enabled &&
               a.highpass_hz == b.highpass_hz &&
               a.low_shelf_hz == b.low_shelf_hz &&
               a.low_shelf_db == b.low_shelf_db &&
               a.presence_hz == b.presence_hz &&
               a.presence_db == b.presence_db &&
               a.compressor_threshold_db == b.compressor_threshold_db &&
               a.compressor_ratio == b.compressor_ratio &&
               a.compressor_attack_ms == b.compressor_attack_ms &&
               a.compressor_release_ms == b.compressor_release_ms &&
               a.compressor_makeup_db == b.compressor_makeup_db &&
               a.saturation == b.saturation &&
               a.target_rms_db == b.target_rms_db &&
               a.ceiling_db == b.ceiling_db &&
               a.limiter_lookahead_ms == b.limiter_lookahead_ms &&
               a.limiter_release_ms == b.limiter_release_ms;
    }

}// namespace

int main() {
    // Neural vocoder presets.
    CHECK_EQ(NeuralVocoderPresetKindName(NeuralVocoderPresetKind::nhv), "nhv");
    CHECK_EQ(NeuralVocoderPresetKindName(NeuralVocoderPresetKind::pc_nsf_hifigan), "pc-nsf-hifigan");

    CHECK(ParseNeuralVocoderPresetKind("NHV") == NeuralVocoderPresetKind::nhv);
    CHECK(ParseNeuralVocoderPresetKind("PC-NSF-HiFiGAN") == NeuralVocoderPresetKind::pc_nsf_hifigan);
    CHECK(!ParseNeuralVocoderPresetKind("unknown").has_value());

    const auto default_spec = NeuralVocoderSpec{};
    const auto nhv = CreateNeuralVocoderPreset(NeuralVocoderPresetKind::nhv);
    CHECK_EQ(nhv.preset, "nhv");
    CHECK_EQ(nhv.fingerprint(), default_spec.fingerprint());

    const auto pc = CreateNeuralVocoderPreset(NeuralVocoderPresetKind::pc_nsf_hifigan);
    CHECK_EQ(pc.preset, "pc-nsf-hifigan");
    CHECK_EQ(pc.hop_size, 512);
    CHECK_EQ(pc.fft_size, 2048);
    CHECK_EQ(pc.mel_bins, 128);
    CHECK(!pc.has_uv);
    CHECK(pc.pitch_controllable);
    CHECK(!pc.normalize_mel);
    CHECK_NEAR(pc.f0_max, 2500.0, 1e-9);
    CHECK_EQ(pc.fingerprint(), NeuralVocoderSpec::Preset("pc-nsf-hifigan").fingerprint());

    CHECK_EQ(CreateNeuralVocoderPreset("").fingerprint(), nhv.fingerprint());
    CHECK_EQ(CreateNeuralVocoderPreset("NhV").fingerprint(), nhv.fingerprint());
    CHECK_EQ(CreateNeuralVocoderPreset("nhv").fingerprint(), nhv.fingerprint());

    {
        bool rejected = false;
        try {
            (void) CreateNeuralVocoderPreset("not-a-preset");
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        CHECK(rejected);
    }

    const std::string neural_names = AvailableNeuralVocoderPresets();
    CHECK(neural_names.find("nhv") != std::string::npos);
    CHECK(neural_names.find("pc-nsf-hifigan") != std::string::npos);
    {
        std::size_t start = 0;
        while (start < neural_names.size()) {
            const auto end = neural_names.find(' ', start);
            const auto name = neural_names.substr(start, end == std::string::npos ? std::string::npos : end - start);
            CHECK(ParseNeuralVocoderPresetKind(name).has_value());
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
    }

    // Master presets.
    CHECK_EQ(MasterPresetKindName(MasterPresetKind::none), "none");
    CHECK_EQ(MasterPresetKindName(MasterPresetKind::natural), "natural");
    CHECK_EQ(MasterPresetKindName(MasterPresetKind::warm), "warm");
    CHECK_EQ(MasterPresetKindName(MasterPresetKind::power), "power");

    CHECK(ParseMasterPresetKind("NONE") == MasterPresetKind::none);
    CHECK(ParseMasterPresetKind("Natural") == MasterPresetKind::natural);
    CHECK(ParseMasterPresetKind("WARM") == MasterPresetKind::warm);
    CHECK(ParseMasterPresetKind("PoWeR") == MasterPresetKind::power);
    CHECK(!ParseMasterPresetKind("unknown").has_value());

    CHECK(!CreateMasterOptions(MasterPresetKind::none).enabled);
    CHECK(same_master_options(CreateMasterOptions(MasterPresetKind::warm), MasterOptions::Preset("warm")));
    CHECK(same_master_options(CreateMasterOptions("WARM"), MasterOptions::Preset("warm")));
    CHECK(same_master_options(CreateMasterOptions(""), MasterOptions::Preset("")));
    CHECK_EQ(CreateMasterOptions(MasterPresetKind::power).saturation, 0.18);

    {
        bool rejected = false;
        try {
            (void) CreateMasterOptions("not-a-preset");
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        CHECK(rejected);
    }

    const std::string master_names = AvailableMasterPresets();
    {
        std::size_t start = 0;
        while (start < master_names.size()) {
            const auto end = master_names.find(' ', start);
            const auto name = master_names.substr(start, end == std::string::npos ? std::string::npos : end - start);
            CHECK(ParseMasterPresetKind(name).has_value());
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
    }

    return test::finish();
}
