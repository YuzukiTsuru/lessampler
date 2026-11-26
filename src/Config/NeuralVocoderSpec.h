/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_NEURALVOCODERSPEC_H
#define LESSAMPLER_NEURALVOCODERSPEC_H

#include <string>
#include <string_view>

#include "Config/NeuralVocoderPresetKind.h"

namespace lessampler {

    enum class MelBase {
        Natural,
        Log10,
    };

    enum class MelScale {
        Slaney,
        Htk,
    };

    enum class MelLayout {
        TimeMajor,
        BinMajor,
    };

    /*
     * Everything the neural front end needs to match a vocoder graph. The
     * defaults are NHVSing-compatible; "pc-nsf-hifigan" switches to the
     * OpenVPI 44.1 kHz / hop 512 / log10 mel contract and lets every field be
     * overridden in [neural].
     */
    struct NeuralVocoderSpec {
        std::string preset = "nhv";
        int sample_rate = 44100;
        int hop_size = 256;
        int fft_size = 2048;
        int mel_bins = 128;
        double mel_fmin = 40.0;
        double mel_fmax = 16000.0;
        MelBase mel_base = MelBase::Natural;
        MelScale mel_scale = MelScale::Slaney;
        MelLayout mel_layout = MelLayout::TimeMajor;
        bool has_f0 = true;
        bool has_uv = true;
        bool pitch_controllable = false;
        bool normalize_mel = true;
        double f0_min = 40.0;
        double f0_max = 1200.0;
        std::string mel_input = "mel";
        std::string f0_input = "f0";
        std::string uv_input = "uv";
        std::string waveform_output = "waveform";

        [[nodiscard]] static NeuralVocoderSpec Preset(std::string_view name);
        [[nodiscard]] std::string Validate() const;
        [[nodiscard]] std::string fingerprint() const;

        [[nodiscard]] static std::string_view mel_base_name(MelBase base);
        [[nodiscard]] static std::string_view mel_scale_name(MelScale scale);
        [[nodiscard]] static std::string_view mel_layout_name(MelLayout layout);

        [[nodiscard]] static MelBase parse_mel_base(std::string_view text);
        [[nodiscard]] static MelScale parse_mel_scale(std::string_view text);
        [[nodiscard]] static MelLayout parse_mel_layout(std::string_view text);
    };

    /*
     * Preset factory. CreateNeuralVocoderPreset(kind) is the primary entry
     * point; the string overload parses a config value and throws
     * std::invalid_argument for an unknown name. Per-field overrides are
     * applied by the caller after the preset spec is created.
     */
    [[nodiscard]] NeuralVocoderSpec CreateNeuralVocoderPreset(NeuralVocoderPresetKind kind);

    [[nodiscard]] NeuralVocoderSpec CreateNeuralVocoderPreset(std::string_view name);

    [[nodiscard]] std::string AvailableNeuralVocoderPresets();

}// namespace lessampler

#endif//LESSAMPLER_NEURALVOCODERSPEC_H
