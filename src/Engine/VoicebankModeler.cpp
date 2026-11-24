/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "VoicebankModeler.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <exception>
#include <format>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "Analysis/World/WorldModelBuilder.h"
#include "Analysis/World/WorldModel.h"
#include "Analysis/World/AutoAMP.h"
#include "Analysis/World/WorldModelCache.h"
#include "Audio/WavIO.h"
#include "Core/Log.h"
#include "Core/Exception.h"

namespace {

    bool has_wav_extension(const std::filesystem::path &path) {
        std::string extension = path.extension().string();
        std::ranges::transform(extension, extension.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return extension == ".wav";
    }

}// namespace

VoicebankModeler::VoicebankModeler(const std::filesystem::path &path, Config _configure)
    : target_voice_path(path), configure(std::move(_configure)) {
    if (!std::filesystem::is_directory(target_voice_path)) {
        throw parameter_error("Modeling input is not a directory: " + target_voice_path.string());
    }

    YALL_INFO_ << "Get the audio files in the directory";
    GetWavFileLists();
#ifdef DEBUG_MODE
    PrintWavFiles();
#endif
    YALL_INFO_ << "Now Generating the Model";
    GenerateModelFromFile();
}

void VoicebankModeler::GenerateModelForFile(const std::filesystem::path &wav_path,
                                              const Config &configure) {
    WavData wav = WavIO::ReadWav(wav_path);

    if (configure.model_amp != 0.0) {
        YALL_DEBUG_ << "Apply AMP Before Modeling";
        AutoAMP::ApplyInPlace(wav.samples, configure.model_amp);
    }

    const auto model = BuildWorldModel(wav.samples, wav.sample_rate, configure);
    WorldModelCache audioModelIO(wav_path, model, configure);
    audioModelIO.SaveAudioModel();
}

void VoicebankModeler::PrintWavFiles() const {
    for (const auto &file: wav_files) {
        YALL_DEBUG_ << file.string();
    }
}

void VoicebankModeler::GetWavFileLists() {
    YALL_INFO_ << "Working on folder: " + target_voice_path.string();
    for (const auto &entry: std::filesystem::directory_iterator(target_voice_path)) {
        if (entry.is_regular_file() && has_wav_extension(entry.path())) {
            wav_files.push_back(entry.path());
        }
    }
    std::ranges::sort(wav_files);
}

void VoicebankModeler::GenerateModelFromFile() {
    const auto hardware_threads = std::thread::hardware_concurrency();
    const auto requested_threads = std::max(1U, hardware_threads == 0 ? 1U : hardware_threads / 2U);
    const auto thread_count = std::min<std::size_t>(requested_threads,
                                                    std::max<std::size_t>(1, wav_files.size()));
    YALL_INFO_ << std::format("Modeling with {} threads", thread_count);

    std::atomic<std::size_t> next{0};
    std::mutex error_mutex;
    std::exception_ptr first_error;

    auto worker = [&]() {
        while (true) {
            const std::size_t index = next.fetch_add(1, std::memory_order_relaxed);
            if (index >= wav_files.size()) {
                return;
            }

            try {
                GenerateModelForFile(wav_files[index], configure);
            } catch (...) {
                std::lock_guard<std::mutex> lock(error_mutex);
                if (!first_error) {
                    first_error = std::current_exception();
                }
                next.store(wav_files.size(), std::memory_order_relaxed);
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
