/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NeuralModel.h"

#include <array>
#include <cstdint>
#include <format>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include "Core/Exception.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Vocoder/Common/OnnxEnv.h"

namespace lessampler {

    namespace {

        std::string canonical_key(const std::filesystem::path &path) {
            std::error_code error;
            const auto absolute = std::filesystem::weakly_canonical(path, error);
            return error ? path.lexically_normal().string() : absolute.string();
        }

    }// namespace

    struct NeuralModel::Impl {
        Ort::Session session;
        NeuralVocoderSpec spec;
        std::string mel_name;
        std::string f0_name;
        std::string uv_name;
        std::string output_name;
        bool mel_bin_major = false;
        bool f0_2d = false;
        bool uv_2d = false;
        std::filesystem::path model_path;

        Impl(const std::filesystem::path &path, int threads, NeuralVocoderSpec vocoder)
            : session(OnnxEnvironment(), path.c_str(), make_options(threads)), spec(std::move(vocoder)),
              model_path(path) {
            collect_names();
        }

        static Ort::SessionOptions make_options(int threads) {
            Ort::SessionOptions options;
            if (threads > 0) {
                options.SetIntraOpNumThreads(threads);
            }
            options.SetInterOpNumThreads(1);
            options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            return options;
        }

        void collect_names() {
            Ort::AllocatorWithDefaultOptions allocator;
            const auto input_count = session.GetInputCount();
            std::size_t mel_index = static_cast<std::size_t>(-1);
            for (std::size_t i = 0; i < input_count; ++i) {
                auto name = session.GetInputNameAllocated(i, allocator);
                const std::string input_name(name.get());
                if (input_name == spec.mel_input) {
                    mel_name = input_name;
                    mel_index = i;
                } else if (input_name == spec.f0_input) {
                    f0_name = input_name;
                } else if (input_name == spec.uv_input) {
                    uv_name = input_name;
                }
            }

            if (mel_name.empty()) {
                throw parameter_error(std::format(
                        "Neural model {} does not expose the expected '{}' input", model_path.string(), spec.mel_input));
            }
            if (spec.has_f0 && f0_name.empty()) {
                throw parameter_error(std::format(
                        "Neural model {} does not expose the expected '{}' input", model_path.string(), spec.f0_input));
            }
            if (spec.has_uv && uv_name.empty()) {
                throw parameter_error(std::format(
                        "Neural model {} does not expose the expected '{}' input", model_path.string(), spec.uv_input));
            }

            const auto mel_type = session.GetInputTypeInfo(mel_index);
            const auto mel_shape = mel_type.GetTensorTypeAndShapeInfo().GetShape();
            if (mel_shape.size() != 3) {
                throw parameter_error(std::format("Neural model {} mel input must be rank 3", model_path.string()));
            }
            const auto mel_dim1 = mel_shape[1];
            const auto mel_dim2 = mel_shape[2];
            if (mel_dim2 == spec.mel_bins && mel_dim1 != spec.mel_bins) {
                mel_bin_major = false;
            } else if (mel_dim1 == spec.mel_bins && mel_dim2 != spec.mel_bins) {
                mel_bin_major = true;
            } else if (mel_dim1 == spec.mel_bins && mel_dim2 == spec.mel_bins) {
                mel_bin_major = spec.mel_layout == MelLayout::BinMajor;
            } else if (mel_dim1 <= 0 && mel_dim2 <= 0) {
                mel_bin_major = spec.mel_layout == MelLayout::BinMajor;
            } else {
                throw parameter_error(std::format(
                        "Neural model {} expects mel dims [{}, {}], lessampler config uses {} bins",
                        model_path.string(), mel_dim1, mel_dim2, spec.mel_bins));
            }

            if (!f0_name.empty()) {
                const auto f0_type = session.GetInputTypeInfo(input_index(f0_name, allocator));
                const auto f0_shape = f0_type.GetTensorTypeAndShapeInfo().GetShape();
                f0_2d = f0_shape.size() == 2;
            }
            if (!uv_name.empty()) {
                const auto uv_type = session.GetInputTypeInfo(input_index(uv_name, allocator));
                const auto uv_shape = uv_type.GetTensorTypeAndShapeInfo().GetShape();
                uv_2d = uv_shape.size() == 2;
            }

            const auto output_count = session.GetOutputCount();
            for (std::size_t i = 0; i < output_count; ++i) {
                auto name = session.GetOutputNameAllocated(i, allocator);
                if (std::string_view(name.get()) == spec.waveform_output) {
                    output_name = name.get();
                    break;
                }
            }
            if (output_name.empty()) {
                throw parameter_error(std::format(
                        "Neural model {} does not expose the expected '{}' output", model_path.string(), spec.waveform_output));
            }
        }

        std::size_t input_index(std::string_view target, const Ort::AllocatorWithDefaultOptions &allocator) const {
            for (std::size_t i = 0; i < session.GetInputCount(); ++i) {
                auto name = session.GetInputNameAllocated(i, allocator);
                if (std::string_view(name.get()) == target) {
                    return i;
                }
            }
            return static_cast<std::size_t>(-1);
        }
    };

    NeuralModel::NeuralModel(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

    NeuralModel::~NeuralModel() = default;

    const std::filesystem::path &NeuralModel::path() const noexcept {
        return impl_->model_path;
    }

    const NeuralVocoderSpec &NeuralModel::spec() const noexcept {
        return impl_->spec;
    }

    std::shared_ptr<NeuralModel> NeuralModel::Load(const std::filesystem::path &path, int threads,
                                                   const NeuralVocoderSpec &spec) {
        if (path.empty()) {
            throw parameter_error("The neural backend requires [neural] model_path");
        }
        if (!std::filesystem::exists(path)) {
            throw parameter_error(std::format("Neural model not found: {}", path.string()));
        }
        const auto error = spec.Validate();
        if (!error.empty()) {
            throw parameter_error("Invalid neural vocoder spec: " + error);
        }

        Timer timer;
        timer.SetTimer();
        try {
            auto impl = std::make_unique<Impl>(path, threads, spec);
            LOG_EVAL << timer.GetTimer("Neural Model Load: ");
            LOG_INFO << std::format("Neural Model: {}", path.string());
            return std::shared_ptr<NeuralModel>(new NeuralModel(std::move(impl)));
        } catch (const Ort::Exception &error) {
            throw parameter_error(std::format("Cannot load neural model {}: {}", path.string(), error.what()));
        }
    }

    std::vector<float> NeuralModel::Synthesize(std::span<const float> mel, std::size_t frames,
                                               std::span<const float> f0, std::span<const float> uv) const {
        if (frames == 0) {
            return {};
        }
        const auto bins = static_cast<std::size_t>(impl_->spec.mel_bins);
        const auto expected_mel = frames * bins;
        if (mel.size() < expected_mel) {
            throw parameter_error("Neural vocoder mel input size mismatch");
        }
        if (impl_->spec.has_f0 && f0.size() < frames) {
            throw parameter_error("Neural vocoder F0 input size mismatch");
        }
        if (impl_->spec.has_uv && uv.size() < frames) {
            throw parameter_error("Neural vocoder uv input size mismatch");
        }

        const Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<float> transposed;
        const float *mel_data = mel.data();
        std::array<std::int64_t, 3> mel_shape{
                1, static_cast<std::int64_t>(frames), static_cast<std::int64_t>(bins)};
        if (impl_->mel_bin_major) {
            transposed.resize(expected_mel);
            for (std::size_t frame = 0; frame < frames; ++frame) {
                for (std::size_t bin = 0; bin < bins; ++bin) {
                    transposed[bin * frames + frame] = mel[frame * bins + bin];
                }
            }
            mel_data = transposed.data();
            mel_shape = {1, static_cast<std::int64_t>(bins), static_cast<std::int64_t>(frames)};
        }

        std::array<std::int64_t, 3> track_shape_2d{1, static_cast<std::int64_t>(frames)};
        std::array<std::int64_t, 3> track_shape_3d{1, 1, static_cast<std::int64_t>(frames)};

        std::vector<Ort::Value> inputs;
        inputs.reserve(3);
        inputs.emplace_back(Ort::Value::CreateTensor<float>(
                memory_info, const_cast<float *>(mel_data), expected_mel, mel_shape.data(), mel_shape.size()));

        std::vector<const char *> input_names;
        input_names.reserve(3);
        input_names.push_back(impl_->mel_name.c_str());

        if (impl_->spec.has_f0) {
            auto *shape = impl_->f0_2d ? track_shape_2d.data() : track_shape_3d.data();
            const auto rank = impl_->f0_2d ? std::size_t{2} : std::size_t{3};
            inputs.emplace_back(Ort::Value::CreateTensor<float>(
                    memory_info, const_cast<float *>(f0.data()), frames, shape, rank));
            input_names.push_back(impl_->f0_name.c_str());
        }
        if (impl_->spec.has_uv) {
            auto *shape = impl_->uv_2d ? track_shape_2d.data() : track_shape_3d.data();
            const auto rank = impl_->uv_2d ? std::size_t{2} : std::size_t{3};
            inputs.emplace_back(Ort::Value::CreateTensor<float>(
                    memory_info, const_cast<float *>(uv.data()), frames, shape, rank));
            input_names.push_back(impl_->uv_name.c_str());
        }

        const std::array<const char *, 1> output_names{impl_->output_name.c_str()};

        std::vector<Ort::Value> outputs;
        try {
            outputs = impl_->session.Run(Ort::RunOptions{nullptr}, input_names.data(), inputs.data(),
                                         inputs.size(), output_names.data(), output_names.size());
        } catch (const Ort::Exception &error) {
            throw parameter_error(std::format("Neural vocoder inference failed: {}", error.what()));
        }

        if (outputs.empty() || !outputs.front().IsTensor()) {
            throw parameter_error("Neural vocoder returned no waveform tensor");
        }
        const auto info = outputs.front().GetTensorTypeAndShapeInfo();
        if (info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            throw parameter_error("Neural vocoder waveform is not float32");
        }

        const auto *data = outputs.front().GetTensorData<float>();
        const auto count = info.GetElementCount();
        return std::vector<float>(data, data + count);
    }

    std::shared_ptr<NeuralModel> LoadNeuralModel(const std::filesystem::path &path, int threads,
                                                 const NeuralVocoderSpec &spec) {
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        error.clear();
        const auto mtime = std::filesystem::last_write_time(path, error);

        std::string key = canonical_key(path) + "|" + std::to_string(threads) + "|" + spec.fingerprint();
        if (!error) {
            key += "|" + std::to_string(size);
            key += "|" + std::to_string(mtime.time_since_epoch().count());
        }

        static std::mutex mutex;
        static std::unordered_map<std::string, std::shared_ptr<NeuralModel>> cache;

        std::lock_guard<std::mutex> lock(mutex);
        if (const auto iterator = cache.find(key); iterator != cache.end()) {
            return iterator->second;
        }

        auto model = NeuralModel::Load(path, threads, spec);
        cache.emplace(key, model);
        return model;
    }

}// namespace lessampler
