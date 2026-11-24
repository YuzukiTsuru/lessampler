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

#include <onnxruntime_cxx_api.h>

#include "Core/Exception.h"
#include "Core/Log.h"
#include "Core/Timer.h"

namespace lessampler {

    namespace {

        Ort::Env &environment() {
            static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "lessampler");
            return env;
        }

        std::string canonical_key(const std::filesystem::path &path) {
            std::error_code error;
            const auto absolute = std::filesystem::weakly_canonical(path, error);
            return error ? path.lexically_normal().string() : absolute.string();
        }

    }// namespace

    struct NeuralModel::Impl {
        Ort::Session session;
        std::string mel_name;
        std::string f0_name;
        std::string uv_name;
        std::string output_name;
        std::filesystem::path model_path;

        Impl(const std::filesystem::path &path, int threads)
            : session(environment(), path.c_str(), make_options(threads)), model_path(path) {
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
            std::size_t mel_index = static_cast<std::size_t>(-1);
            const auto input_count = session.GetInputCount();
            for (std::size_t i = 0; i < input_count; ++i) {
                auto name = session.GetInputNameAllocated(i, allocator);
                const std::string input_name(name.get());
                if (input_name == "mel") {
                    mel_name = input_name;
                    mel_index = i;
                } else if (input_name == "f0") {
                    f0_name = input_name;
                } else if (input_name == "uv") {
                    uv_name = input_name;
                }
            }

            if (mel_name.empty() || f0_name.empty() || uv_name.empty()) {
                throw parameter_error(std::format(
                        "Neural model {} does not expose the expected mel/f0/uv inputs", model_path.string()));
            }

            const auto output_count = session.GetOutputCount();
            for (std::size_t i = 0; i < output_count; ++i) {
                auto name = session.GetOutputNameAllocated(i, allocator);
                if (std::string_view(name.get()) == "waveform") {
                    output_name = name.get();
                    break;
                }
            }
            if (output_name.empty()) {
                throw parameter_error(std::format(
                        "Neural model {} does not expose the expected waveform output", model_path.string()));
            }

            const auto mel_shape = session.GetInputTypeInfo(mel_index).GetTensorTypeAndShapeInfo().GetShape();
            if (mel_shape.size() == 3 && mel_shape[2] > 0 && mel_shape[2] != mel_bins) {
                throw parameter_error(std::format(
                        "Neural model {} expects {} mel bins, lessampler feeds {}", model_path.string(),
                        mel_shape[2], mel_bins));
            }
        }
    };

    NeuralModel::NeuralModel(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

    NeuralModel::~NeuralModel() = default;

    const std::filesystem::path &NeuralModel::path() const noexcept {
        return impl_->model_path;
    }

    std::shared_ptr<NeuralModel> NeuralModel::Load(const std::filesystem::path &path, int threads) {
        if (path.empty()) {
            throw parameter_error("The neural backend requires [neural] model_path");
        }
        if (!std::filesystem::exists(path)) {
            throw parameter_error(std::format("Neural model not found: {}", path.string()));
        }

        Timer timer;
        timer.SetTimer();
        try {
            auto impl = std::make_unique<Impl>(path, threads);
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
        const auto expected_mel = frames * static_cast<std::size_t>(mel_bins);
        if (mel.size() < expected_mel || f0.size() < frames || uv.size() < frames) {
            throw parameter_error("Neural vocoder input size mismatch");
        }

        const Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::array<std::int64_t, 3> mel_shape{1, static_cast<std::int64_t>(frames), mel_bins};
        std::array<std::int64_t, 3> track_shape{1, 1, static_cast<std::int64_t>(frames)};

        std::array<Ort::Value, 3> inputs{
                Ort::Value::CreateTensor<float>(memory_info, const_cast<float *>(mel.data()), expected_mel,
                                                mel_shape.data(), mel_shape.size()),
                Ort::Value::CreateTensor<float>(memory_info, const_cast<float *>(f0.data()), frames,
                                                track_shape.data(), track_shape.size()),
                Ort::Value::CreateTensor<float>(memory_info, const_cast<float *>(uv.data()), frames,
                                                track_shape.data(), track_shape.size()),
        };

        const std::array<const char *, 3> input_names{
                impl_->mel_name.c_str(), impl_->f0_name.c_str(), impl_->uv_name.c_str()};
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

    std::shared_ptr<NeuralModel> LoadNeuralModel(const std::filesystem::path &path, int threads) {
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        error.clear();
        const auto mtime = std::filesystem::last_write_time(path, error);

        std::string key = canonical_key(path) + "|" + std::to_string(threads) + "|" + std::to_string(size);
        if (!error) {
            key += "|" + std::to_string(mtime.time_since_epoch().count());
        }

        static std::mutex mutex;
        static std::unordered_map<std::string, std::shared_ptr<NeuralModel>> cache;

        std::lock_guard<std::mutex> lock(mutex);
        if (const auto iterator = cache.find(key); iterator != cache.end()) {
            return iterator->second;
        }

        auto model = NeuralModel::Load(path, threads);
        cache.emplace(key, model);
        return model;
    }

}// namespace lessampler
