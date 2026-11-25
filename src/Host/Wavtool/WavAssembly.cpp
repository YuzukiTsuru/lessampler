/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Host/Wavtool/WavAssembly.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include "Audio/WavIO.h"
#include "Core/Exception.h"

namespace lessampler::wavtool {

    namespace {

        std::int64_t milliseconds_to_samples(double milliseconds, int sample_rate) {
            if (!std::isfinite(milliseconds)) {
                throw parameter_error("Wavtool segment length must be finite");
            }
            const double samples = std::round(std::max(0.0, milliseconds) * static_cast<double>(sample_rate) / 1000.0);
            constexpr double max_samples = static_cast<double>(std::numeric_limits<std::int64_t>::max() / 2);
            return static_cast<std::int64_t>(std::min(samples, max_samples));
        }

        double channel_value(const WavFrames &input, std::size_t frame, std::size_t output_channel, std::size_t output_channels) {
            const auto input_channels = static_cast<std::size_t>(input.channels);
            if (input_channels == 0) {
                return 0.0;
            }
            if (input_channels == 1) {
                return input.samples[frame];
            }
            if (output_channels == 1 || output_channel >= input_channels) {
                double sum = 0.0;
                for (std::size_t channel = 0; channel < input_channels; ++channel) {
                    sum += input.samples[frame * input_channels + channel];
                }
                return sum / static_cast<double>(input_channels);
            }
            return input.samples[frame * input_channels + output_channel];
        }

    }// namespace

    WavBuffer WavAssembly::LoadOrCreate(const std::filesystem::path &output, int fallback_rate, int fallback_channels) {
        std::error_code error;
        const bool exists = std::filesystem::exists(output, error);
        if (exists && !error) {
            const auto size = std::filesystem::file_size(output, error);
            if (!error && size > 0) {
                auto frames = WavIO::ReadWavFrames(output);
                if (frames.channels <= 0 || frames.sample_rate <= 0) {
                    throw audio_file_error("invalid output WAV layout: " + output.string());
                }
                return WavBuffer{std::move(frames.samples), frames.sample_rate, frames.channels};
            }
        }

        if (fallback_rate <= 0 || fallback_channels <= 0) {
            throw audio_file_error("no valid output layout for " + output.string());
        }
        return WavBuffer{{}, fallback_rate, fallback_channels};
    }

    void WavAssembly::AppendAt(WavBuffer &output, const WavSegment &segment, std::int64_t position_samples) {
        if (output.channels <= 0 || output.sample_rate <= 0) {
            throw audio_file_error("output buffer has no valid WAV layout");
        }
        if (segment.length_ms <= 0.0) {
            return;
        }

        const auto length_samples = milliseconds_to_samples(segment.length_ms, output.sample_rate);
        if (length_samples <= 0) {
            return;
        }

        std::optional<WavFrames> input;
        std::error_code error;
        if (std::filesystem::exists(segment.input, error) && !error) {
            input = WavIO::ReadWavFrames(segment.input);
            if (input->sample_rate != output.sample_rate) {
                throw audio_file_error("sample rate mismatch: " + segment.input.string() + " is " + std::to_string(input->sample_rate) +
                                       " Hz, output is " + std::to_string(output.sample_rate) + " Hz");
            }
        }

        const auto gains = segment.envelope.GainCurve(static_cast<std::size_t>(length_samples), output.sample_rate);
        const auto skip_samples = milliseconds_to_samples(segment.skip_ms, output.sample_rate);

        const std::int64_t drop = position_samples < 0 ? -position_samples : 0;
        if (drop >= length_samples) {
            return;
        }
        const std::int64_t start = std::max<std::int64_t>(0, position_samples);
        const std::int64_t count = length_samples - drop;

        const auto output_channels = static_cast<std::size_t>(output.channels);
        const auto old_frames = output.frames();
        const auto required_frames = static_cast<std::size_t>(start) + static_cast<std::size_t>(count);
        if (required_frames > old_frames) {
            output.resize_frames(required_frames);
        }

        const auto requested_declick = segment.declick_ms > 0.0
                                               ? static_cast<std::int64_t>(std::llround(
                                                         segment.declick_ms * static_cast<double>(output.sample_rate) / 1000.0))
                                               : 0;
        const std::int64_t declick = std::clamp<std::int64_t>(requested_declick, 0, count / 2);
        const auto declick_gain = [declick](std::int64_t position) {
            if (declick < 2) {
                return 1.0;
            }
            const double phase = static_cast<double>(position) / static_cast<double>(declick - 1);
            return 0.5 * (1.0 - std::cos(std::numbers::pi * phase));
        };

        for (std::int64_t offset = 0; offset < count; ++offset) {
            const std::int64_t envelope_index = drop + offset;
            const std::int64_t input_index = skip_samples + envelope_index;
            double gain = gains[static_cast<std::size_t>(envelope_index)];
            if (declick >= 2) {
                if (drop == 0 && offset < declick) {
                    gain *= declick_gain(offset);
                }
                const std::int64_t from_tail = count - 1 - offset;
                if (from_tail < declick) {
                    gain *= declick_gain(from_tail);
                }
            }
            const auto output_frame = static_cast<std::size_t>(start + offset);

            double source = 0.0;
            if (input && input_index >= 0 && static_cast<std::size_t>(input_index) < input->frames()) {
                source = channel_value(*input, static_cast<std::size_t>(input_index), 0, output_channels);
            }

            if (output_channels == 1) {
                double sum = source;
                if (input && input->channels > 1 && input_index >= 0 && static_cast<std::size_t>(input_index) < input->frames()) {
                    sum = 0.0;
                    const auto input_channels = static_cast<std::size_t>(input->channels);
                    for (std::size_t channel = 0; channel < input_channels; ++channel) {
                        sum += input->samples[static_cast<std::size_t>(input_index) * input_channels + channel];
                    }
                    sum /= static_cast<double>(input_channels);
                }
                output.samples[output_frame] += sum * gain;
                continue;
            }

            for (std::size_t channel = 0; channel < output_channels; ++channel) {
                double value = 0.0;
                if (input && input_index >= 0 && static_cast<std::size_t>(input_index) < input->frames()) {
                    value = channel_value(*input, static_cast<std::size_t>(input_index), channel, output_channels);
                }
                output.samples[output_frame * output_channels + channel] += value * gain;
            }
        }
    }

    void WavAssembly::AppendRelative(WavBuffer &output, const WavSegment &segment, double overlap_ms) {
        if (output.channels <= 0 || output.sample_rate <= 0) {
            throw audio_file_error("output buffer has no valid WAV layout");
        }
        const auto overlap_samples = static_cast<std::int64_t>(
                std::llround(overlap_ms * static_cast<double>(output.sample_rate) / 1000.0));
        const auto position = static_cast<std::int64_t>(output.frames()) - overlap_samples;
        AppendAt(output, segment, position);
    }

    double WavAssembly::Peak(std::span<const double> samples) {
        double peak = 0.0;
        for (double value: samples) {
            if (std::isfinite(value)) {
                peak = std::max(peak, std::abs(value));
            }
        }
        return peak;
    }

    void WavAssembly::Write(const WavBuffer &output, const std::filesystem::path &path, double peak_limit) {
        if (output.samples.empty() || output.channels <= 0 || output.sample_rate <= 0) {
            throw audio_file_error("refusing to write an empty WAV: " + path.string());
        }

        double gain = 1.0;
        if (peak_limit > 0.0) {
            const double peak = Peak(output.samples);
            if (peak > peak_limit) {
                gain = peak_limit / peak;
            }
        }

        std::vector<double> samples(output.samples.size());
        for (std::size_t i = 0; i < samples.size(); ++i) {
            const double value = std::isfinite(output.samples[i]) ? output.samples[i] * gain : 0.0;
            samples[i] = std::clamp(value, -1.0, 1.0);
        }

        WavIO::WriteWavFrames(path, samples, output.sample_rate, output.channels);
    }

}// namespace lessampler::wavtool
