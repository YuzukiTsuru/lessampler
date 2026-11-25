/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Config/NeuralVocoderSpec.h"

#include <cmath>
#include <format>
#include <stdexcept>

namespace lessampler {

    NeuralVocoderSpec CreateNeuralVocoderPreset(NeuralVocoderPresetKind kind) {
        NeuralVocoderSpec spec;
        switch (kind) {
            case NeuralVocoderPresetKind::nhv:
                spec.preset = std::string(NeuralVocoderPresetKindName(kind));
                return spec;
            case NeuralVocoderPresetKind::pc_nsf_hifigan:
                // OpenVPI PC-NSF-HiFiGAN 44.1k/hop512/128bin, 40-16 kHz.
                spec.preset = std::string(NeuralVocoderPresetKindName(kind));
                spec.hop_size = 512;
                spec.has_uv = false;
                spec.pitch_controllable = true;
                spec.normalize_mel = false;
                spec.f0_max = 2500.0;
                return spec;
        }
        throw std::invalid_argument("unknown neural vocoder preset kind");
    }

    NeuralVocoderSpec CreateNeuralVocoderPreset(std::string_view name) {
        if (name.empty()) {
            return CreateNeuralVocoderPreset(NeuralVocoderPresetKind::nhv);
        }
        const auto kind = ParseNeuralVocoderPresetKind(name);
        if (!kind) {
            throw std::invalid_argument(std::format(
                    "unknown neural vocoder preset: {} (expected {})", name, AvailableNeuralVocoderPresets()));
        }
        return CreateNeuralVocoderPreset(*kind);
    }

    NeuralVocoderSpec NeuralVocoderSpec::Preset(std::string_view name) {
        return CreateNeuralVocoderPreset(name);
    }

    std::string AvailableNeuralVocoderPresets() {
        return "nhv pc-nsf-hifigan";
    }

    std::string NeuralVocoderSpec::Validate() const {
        if (sample_rate <= 0) {
            return "sample_rate must be greater than zero";
        }
        if (hop_size <= 0) {
            return "hop_size must be greater than zero";
        }
        if (fft_size <= 0 || fft_size < hop_size) {
            return "fft_size must be greater than or equal to hop_size";
        }
        if (mel_bins <= 0) {
            return "mel_bins must be greater than zero";
        }
        if (mel_fmin < 0.0 || !(mel_fmax > mel_fmin)) {
            return "mel_fmax must be greater than mel_fmin";
        }
        if (mel_fmax > static_cast<double>(sample_rate) / 2.0 + 1e-9) {
            return "mel_fmax must not exceed the Nyquist frequency";
        }
        if (!(f0_min > 0.0) || !(f0_max > f0_min)) {
            return "f0_max must be greater than f0_min";
        }
        if (has_uv && !has_f0) {
            return "has_uv requires has_f0";
        }
        if (mel_input.empty() || f0_input.empty() || uv_input.empty() || waveform_output.empty()) {
            return "input and output names must not be empty";
        }
        return {};
    }

    std::string NeuralVocoderSpec::fingerprint() const {
        return std::format(
                "{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}",
                preset, sample_rate, hop_size, fft_size, mel_bins, mel_fmin, mel_fmax,
                mel_base_name(mel_base), mel_scale_name(mel_scale), mel_layout_name(mel_layout),
                has_f0, has_uv, pitch_controllable, normalize_mel, f0_min, f0_max,
                mel_input, f0_input, uv_input, waveform_output);
    }

    std::string_view NeuralVocoderSpec::mel_base_name(MelBase base) {
        return base == MelBase::Log10 ? "10" : "e";
    }

    std::string_view NeuralVocoderSpec::mel_scale_name(MelScale scale) {
        return scale == MelScale::Htk ? "htk" : "slaney";
    }

    std::string_view NeuralVocoderSpec::mel_layout_name(MelLayout layout) {
        return layout == MelLayout::BinMajor ? "bin-major" : "time-major";
    }

    MelBase NeuralVocoderSpec::parse_mel_base(std::string_view text) {
        if (text == "e" || text == "natural" || text == "ln") {
            return MelBase::Natural;
        }
        if (text == "10" || text == "log10") {
            return MelBase::Log10;
        }
        throw std::invalid_argument("mel_base must be \"e\" or \"10\"");
    }

    MelScale NeuralVocoderSpec::parse_mel_scale(std::string_view text) {
        if (text == "slaney") {
            return MelScale::Slaney;
        }
        if (text == "htk") {
            return MelScale::Htk;
        }
        throw std::invalid_argument("mel_scale must be \"slaney\" or \"htk\"");
    }

    MelLayout NeuralVocoderSpec::parse_mel_layout(std::string_view text) {
        if (text == "time-major" || text == "time") {
            return MelLayout::TimeMajor;
        }
        if (text == "bin-major" || text == "bin") {
            return MelLayout::BinMajor;
        }
        throw std::invalid_argument("mel_layout must be \"time-major\" or \"bin-major\"");
    }

}// namespace lessampler
