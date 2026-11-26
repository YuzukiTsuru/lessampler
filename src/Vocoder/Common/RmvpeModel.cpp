/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RmvpeModel.h"

#include <algorithm>
#include <array>
#include <cctype>
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
#include "Vocoder/Common/OnnxEnv.h"

namespace lessampler {

    namespace {

        std::string canonical_key(const std::filesystem::path &path) {
            std::error_code error;
            const auto absolute = std::filesystem::weakly_canonical(path, error);
            return error ? path.lexically_normal().string() : absolute.string();
        }

        std::string lower(std::string_view text) {
            std::string value(text);
            std::ranges::transform(value, value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        constexpr std::size_t rmvpe_bins = 360;

        bool has_bin_dimension(const std::vector<std::int64_t> &shape) {
            return std::ranges::find(shape, static_cast<std::int64_t>(rmvpe_bins)) != shape.end();
        }

    }// namespace

    struct RmvpeModel::Impl {
        Ort::Session session;
        std::string input_name;
        std::string threshold_name;
        std::string salience_name;
        std::string f0_name;
        std::string uv_name;
        std::filesystem::path model_path;
        int input_rank = 2;
        int threshold_rank = 0;
        bool direct_f0 = false;

        Impl(const std::filesystem::path &path, int threads)
            : session(OnnxEnvironment(), path.c_str(), make_options(threads)), model_path(path) {
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
            if (input_count == 0) {
                throw parameter_error("RMVPE model has no inputs");
            }

            std::size_t input_index = input_count;
            std::size_t threshold_index = input_count;
            std::string waveform_name;
            for (std::size_t i = 0; i < input_count; ++i) {
                auto name = session.GetInputNameAllocated(i, allocator);
                const auto lowered = lower(name.get());
                if (lowered == "audio") {
                    input_index = i;
                    input_name = name.get();
                } else if (lowered == "waveform" && input_index == input_count) {
                    input_index = i;
                    waveform_name = name.get();
                } else if (lowered == "threshold") {
                    threshold_index = i;
                    threshold_name = name.get();
                }
            }
            if (input_index == input_count && !waveform_name.empty()) {
                input_name = waveform_name;
            }
            if (input_index == input_count) {
                if (input_count != 1) {
                    throw parameter_error("RMVPE model must expose 'audio' or 'waveform'");
                }
                auto name = session.GetInputNameAllocated(0, allocator);
                input_name = name.get();
                input_index = 0;
            }

            const auto input_info = session.GetInputTypeInfo(input_index).GetTensorTypeAndShapeInfo();
            if (input_info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                throw parameter_error("RMVPE model input must be float32");
            }
            const auto input_shape = input_info.GetShape();
            if (input_shape.size() == 1) {
                input_rank = 1;
            } else if (input_shape.size() == 2) {
                input_rank = 2;
            } else if (input_shape.size() == 3) {
                input_rank = 3;
            } else {
                throw parameter_error("RMVPE model input must be rank 1, 2, or 3");
            }

            if (threshold_index != input_count) {
                const auto threshold_shape = session.GetInputTypeInfo(threshold_index)
                                                     .GetTensorTypeAndShapeInfo()
                                                     .GetShape();
                if (threshold_shape.size() == 0) {
                    threshold_rank = 0;
                } else if (threshold_shape.size() == 1) {
                    threshold_rank = 1;
                } else {
                    throw parameter_error("RMVPE threshold input must be a scalar or a 1-D tensor");
                }
            }

            const auto output_count = session.GetOutputCount();
            if (output_count == 0) {
                throw parameter_error("RMVPE model has no outputs");
            }

            std::vector<std::string> names(output_count);
            std::vector<std::vector<std::int64_t>> shapes(output_count);
            for (std::size_t i = 0; i < output_count; ++i) {
                auto name = session.GetOutputNameAllocated(i, allocator);
                names[i] = name.get();
                shapes[i] = session.GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
            }

            std::size_t salience_index = output_count;
            std::size_t f0_index = output_count;
            std::size_t uv_index = output_count;
            for (std::size_t i = 0; i < output_count; ++i) {
                const auto lowered = lower(names[i]);
                if (lowered == "salience") {
                    salience_index = i;
                } else if (lowered == "f0") {
                    f0_index = i;
                } else if (lowered == "uv") {
                    uv_index = i;
                }
            }

            if (salience_index != output_count) {
                direct_f0 = false;
                salience_name = names[salience_index];
                const auto info = session.GetOutputTypeInfo(salience_index).GetTensorTypeAndShapeInfo();
                if (info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                    throw parameter_error("RMVPE salience output must be float32");
                }
                if ((info.GetShape().size() != 2 && info.GetShape().size() != 3) ||
                    !has_bin_dimension(info.GetShape())) {
                    throw parameter_error("RMVPE salience output must have a 360-bin dimension");
                }
                return;
            }

            if (f0_index == output_count || uv_index == output_count) {
                if (output_count == 2) {
                    f0_index = 0;
                    uv_index = 1;
                } else if (output_count == 1 && has_bin_dimension(shapes[0])) {
                    direct_f0 = false;
                    salience_name = names[0];
                    const auto info = session.GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo();
                    if (info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                        throw parameter_error("RMVPE salience output must be float32");
                    }
                    return;
                } else {
                    throw parameter_error("RMVPE model must expose salience or f0+uv outputs");
                }
            }

            direct_f0 = true;
            f0_name = names[f0_index];
            uv_name = names[uv_index];
            const auto f0_type = session.GetOutputTypeInfo(f0_index).GetTensorTypeAndShapeInfo().GetElementType();
            if (f0_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                throw parameter_error("RMVPE f0 output must be float32");
            }
            const auto uv_type = session.GetOutputTypeInfo(uv_index).GetTensorTypeAndShapeInfo().GetElementType();
            if (uv_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL &&
                uv_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT &&
                uv_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32 &&
                uv_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64 &&
                uv_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8) {
                throw parameter_error("RMVPE uv output must be bool, float, or an integer type");
            }
        }
    };

    RmvpeModel::RmvpeModel(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

    RmvpeModel::~RmvpeModel() = default;

    const std::filesystem::path &RmvpeModel::path() const noexcept {
        return impl_->model_path;
    }

    std::shared_ptr<RmvpeModel> RmvpeModel::Load(const std::filesystem::path &path, int threads) {
        if (path.empty()) {
            throw parameter_error("The neural F0 estimator requires [f0] neural_model_path");
        }
        if (!std::filesystem::exists(path)) {
            throw parameter_error(std::format("RMVPE model not found: {}", path.string()));
        }

        try {
            auto impl = std::make_unique<Impl>(path, threads);
            LOG_INFO << std::format("RMVPE Model: {}", path.string());
            return std::shared_ptr<RmvpeModel>(new RmvpeModel(std::move(impl)));
        } catch (const Ort::Exception &error) {
            throw parameter_error(std::format("Cannot load RMVPE model {}: {}", path.string(), error.what()));
        }
    }

    RmvpeOutput RmvpeModel::Infer(std::span<const float> audio, float threshold) const {
        if (audio.empty()) {
            throw parameter_error("RMVPE input is empty");
        }

        const Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::array<std::int64_t, 3> shape{};
        std::size_t rank = 0;
        if (impl_->input_rank == 1) {
            shape = {static_cast<std::int64_t>(audio.size()), 0, 0};
            rank = 1;
        } else if (impl_->input_rank == 2) {
            shape = {1, static_cast<std::int64_t>(audio.size()), 0};
            rank = 2;
        } else {
            shape = {1, 1, static_cast<std::int64_t>(audio.size())};
            rank = 3;
        }

        std::vector<Ort::Value> inputs;
        inputs.emplace_back(Ort::Value::CreateTensor<float>(
                memory_info, const_cast<float *>(audio.data()), audio.size(), shape.data(), rank));
        std::vector<const char *> input_names{impl_->input_name.c_str()};

        float threshold_value = threshold;
        if (!impl_->threshold_name.empty()) {
            if (impl_->threshold_rank == 0) {
                const std::array<std::int64_t, 0> scalar_shape{};
                inputs.emplace_back(Ort::Value::CreateTensor<float>(
                        memory_info, &threshold_value, 1, scalar_shape.data(), scalar_shape.size()));
            } else {
                std::array<std::int64_t, 1> threshold_shape{1};
                inputs.emplace_back(Ort::Value::CreateTensor<float>(
                        memory_info, &threshold_value, 1, threshold_shape.data(), 1));
            }
            input_names.push_back(impl_->threshold_name.c_str());
        }

        std::vector<const char *> output_names;
        if (impl_->direct_f0) {
            output_names.push_back(impl_->f0_name.c_str());
            output_names.push_back(impl_->uv_name.c_str());
        } else {
            output_names.push_back(impl_->salience_name.c_str());
        }

        std::vector<Ort::Value> outputs;
        try {
            outputs = impl_->session.Run(Ort::RunOptions{nullptr}, input_names.data(), inputs.data(),
                                         inputs.size(), output_names.data(), output_names.size());
        } catch (const Ort::Exception &error) {
            throw parameter_error(std::format("RMVPE inference failed: {}", error.what()));
        }

        RmvpeOutput result;
        result.direct_f0 = impl_->direct_f0;

        if (!impl_->direct_f0) {
            if (outputs.empty() || !outputs.front().IsTensor()) {
                throw parameter_error("RMVPE returned no salience tensor");
            }
            const auto info = outputs.front().GetTensorTypeAndShapeInfo();
            if (info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                throw parameter_error("RMVPE salience is not float32");
            }

            const auto output_shape = info.GetShape();
            std::size_t frames = 0;
            std::size_t bins = 0;
            bool bin_major = false;
            if (output_shape.size() == 2) {
                if (output_shape[1] == static_cast<std::int64_t>(rmvpe_bins)) {
                    frames = static_cast<std::size_t>(output_shape[0]);
                    bins = rmvpe_bins;
                } else if (output_shape[0] == static_cast<std::int64_t>(rmvpe_bins)) {
                    frames = static_cast<std::size_t>(output_shape[1]);
                    bins = rmvpe_bins;
                    bin_major = true;
                }
            } else if (output_shape.size() == 3) {
                if (output_shape[2] == static_cast<std::int64_t>(rmvpe_bins)) {
                    frames = static_cast<std::size_t>(output_shape[1]);
                    bins = rmvpe_bins;
                } else if (output_shape[1] == static_cast<std::int64_t>(rmvpe_bins)) {
                    frames = static_cast<std::size_t>(output_shape[2]);
                    bins = rmvpe_bins;
                    bin_major = true;
                }
            }
            if (frames == 0 || bins != rmvpe_bins) {
                throw parameter_error("RMVPE returned an invalid salience shape");
            }

            const auto *data = outputs.front().GetTensorData<float>();
            const auto count = static_cast<std::size_t>(info.GetElementCount());
            if (count < frames * bins) {
                throw parameter_error("RMVPE salience tensor is truncated");
            }

            result.frames = frames;
            result.bins = bins;
            result.salience.resize(frames * bins);
            if (bin_major) {
                for (std::size_t b = 0; b < bins; ++b) {
                    for (std::size_t t = 0; t < frames; ++t) {
                        result.salience[t * bins + b] = data[b * frames + t];
                    }
                }
            } else {
                result.salience.assign(data, data + frames * bins);
            }
            return result;
        }

        if (outputs.size() < 2 || !outputs[0].IsTensor() || !outputs[1].IsTensor()) {
            throw parameter_error("RMVPE returned an invalid f0/uv output");
        }
        const auto f0_info = outputs[0].GetTensorTypeAndShapeInfo();
        if (f0_info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            throw parameter_error("RMVPE f0 output is not float32");
        }
        const auto frames = static_cast<std::size_t>(f0_info.GetElementCount());
        const auto uv_info = outputs[1].GetTensorTypeAndShapeInfo();
        if (static_cast<std::size_t>(uv_info.GetElementCount()) < frames) {
            throw parameter_error("RMVPE uv output is shorter than f0");
        }

        const auto *f0_data = outputs[0].GetTensorData<float>();
        result.frames = frames;
        result.f0.assign(f0_data, f0_data + frames);
        result.uv.resize(frames, 0);
        switch (uv_info.GetElementType()) {
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL: {
                const auto *data = outputs[1].GetTensorData<bool>();
                for (std::size_t i = 0; i < frames; ++i) {
                    result.uv[i] = data[i] ? 1 : 0;
                }
                break;
            }
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: {
                const auto *data = outputs[1].GetTensorData<float>();
                for (std::size_t i = 0; i < frames; ++i) {
                    result.uv[i] = data[i] != 0.0f ? 1 : 0;
                }
                break;
            }
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32: {
                const auto *data = outputs[1].GetTensorData<std::int32_t>();
                for (std::size_t i = 0; i < frames; ++i) {
                    result.uv[i] = data[i] != 0 ? 1 : 0;
                }
                break;
            }
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64: {
                const auto *data = outputs[1].GetTensorData<std::int64_t>();
                for (std::size_t i = 0; i < frames; ++i) {
                    result.uv[i] = data[i] != 0 ? 1 : 0;
                }
                break;
            }
            case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8: {
                const auto *data = outputs[1].GetTensorData<std::uint8_t>();
                for (std::size_t i = 0; i < frames; ++i) {
                    result.uv[i] = data[i] != 0 ? 1 : 0;
                }
                break;
            }
            default:
                throw parameter_error("RMVPE uv output has an unsupported element type");
        }
        return result;
    }

    std::shared_ptr<RmvpeModel> LoadRmvpeModel(const std::filesystem::path &path, int threads) {
        if (path.empty()) {
            throw parameter_error("The neural F0 estimator requires [f0] neural_model_path");
        }
        if (!std::filesystem::exists(path)) {
            throw parameter_error(std::format("RMVPE model not found: {}", path.string()));
        }

        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        error.clear();
        const auto mtime = std::filesystem::last_write_time(path, error);

        std::string key = canonical_key(path) + "|" + std::to_string(threads) + "|rmvpe-v1";
        if (!error) {
            key += "|" + std::to_string(size);
            key += "|" + std::to_string(mtime.time_since_epoch().count());
        }

        static std::mutex mutex;
        static std::unordered_map<std::string, std::shared_ptr<RmvpeModel>> cache;

        std::lock_guard<std::mutex> lock(mutex);
        if (const auto iterator = cache.find(key); iterator != cache.end()) {
            return iterator->second;
        }

        auto model = RmvpeModel::Load(path, threads);
        cache.emplace(key, model);
        return model;
    }

}// namespace lessampler
