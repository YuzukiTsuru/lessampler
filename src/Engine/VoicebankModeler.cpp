/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Engine/VoicebankModeler.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <exception>
#include <format>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Audio/SampleSource.h"
#include "Core/Exception.h"
#include "Core/Log.h"
#include "Vocoder/Common/F0Estimator.h"
#include "Vocoder/World/WorldModelStore.h"

namespace lessampler {

    namespace {

        bool has_wav_extension(const std::filesystem::path &path) {
            std::string extension = path.extension().string();
            std::ranges::transform(extension, extension.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return extension == ".wav";
        }

    }// namespace

    VoicebankModeler::VoicebankModeler(const std::filesystem::path &path, Config configure)
        : target_voice_path_(path), configs_(std::move(configure)) {
        if (!std::filesystem::is_directory(target_voice_path_)) {
            throw parameter_error("Modeling input is not a directory: " + target_voice_path_.string());
        }

        LOG_INFO << "Get the audio files in the directory";
        GetWavFileLists();
#ifdef DEBUG_MODE
        PrintWavFiles();
#endif
        LOG_INFO << "Now Generating the Model";
        GenerateModelFromFile();
    }

    void VoicebankModeler::PrintWavFiles() const {
        for (const auto &file: wav_files_) {
            LOG_DEBUG << file.string();
        }
    }

    void VoicebankModeler::GetWavFileLists() {
        LOG_INFO << "Working on folder: " + target_voice_path_.string();
        for (const auto &entry: std::filesystem::directory_iterator(target_voice_path_)) {
            if (entry.is_regular_file() && has_wav_extension(entry.path())) {
                wav_files_.push_back(entry.path());
            }
        }
        std::ranges::sort(wav_files_);
    }

    void VoicebankModeler::GenerateModelFromFile() {
        const auto hardware_threads = std::thread::hardware_concurrency();
        const auto requested_threads = std::max(1U, hardware_threads == 0 ? 1U : hardware_threads / 2U);
        const auto thread_count = std::min<std::size_t>(requested_threads,
                                                        std::max<std::size_t>(1, wav_files_.size()));
        const auto hardware = std::max(1U, hardware_threads == 0 ? 1U : hardware_threads);
        const auto workers_per_file = std::max(1U, hardware / static_cast<unsigned>(thread_count));
        LOG_INFO << std::format("Modeling with {} threads, {} analysis workers each", thread_count, workers_per_file);

        std::atomic<std::size_t> next{0};
        std::mutex error_mutex;
        std::exception_ptr first_error;

        auto worker = [&]() {
            while (true) {
                const std::size_t index = next.fetch_add(1, std::memory_order_relaxed);
                if (index >= wav_files_.size()) {
                    return;
                }

                try {
                    const SampleSource source(wav_files_[index]);
                    auto configure = configs_.Resolve(source.path());
                    configure = ResolveF0EstimatorConfig(configure, configure.vocoder);
                    static_cast<void>(LoadOrBuildWorldModel(source, configure, static_cast<int>(workers_per_file)));
                } catch (...) {
                    std::lock_guard<std::mutex> lock(error_mutex);
                    if (!first_error) {
                        first_error = std::current_exception();
                    }
                    next.store(wav_files_.size(), std::memory_order_relaxed);
                    return;
                }
            }
        };

        std::vector<std::jthread> threads;
        threads.reserve(thread_count - 1);
        for (unsigned i = 1; i < thread_count; ++i) {
            threads.emplace_back(worker);
        }

        worker();
        threads.clear();

        if (first_error) {
            std::rethrow_exception(first_error);
        }
    }

}// namespace lessampler
