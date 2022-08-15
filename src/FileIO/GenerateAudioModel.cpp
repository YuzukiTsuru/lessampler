/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/23.
//

#include <utility>
#include <future>
#include <thread>

#include "GenerateAudioModel.h"

#include "Utils/LOG.h"
#include "AudioModel/AudioModel.h"
#include "AudioModel/lessAudioModel.h"
#include "AudioProcess/AutoAMP.h"
#include "FileIO/AudioModelIO.h"
#include "FileIO/WavIO.h"

GenerateAudioModel::GenerateAudioModel(std::filesystem::path path, lessConfigure configure) : target_voice_path(std::move(path)),
                                                                                              configure(std::move(configure)) {
    YALL_INFO_ << "Get the audio files in the directory";
    GetWavFileLists();
#ifdef DEBUG_MODE
    PrintWavFiles();
#endif
    YALL_INFO_ << "Now Generating the Model";
    GenerateModelFromFile();
}

GenerateAudioModel::GenerateAudioModel(char *path, lessConfigure configure) : configure(std::move(configure)) {
    auto file_path = std::string(path);
    WavFileModel(file_path);
}

void GenerateAudioModel::PrintWavFiles() {
    for (const auto &file: wav_files) {
        YALL_DEBUG_ << file.string();
    }
}

void GenerateAudioModel::GetWavFileLists() {
    YALL_INFO_ << "Working on folder: " + target_voice_path.string();
    for (const auto &entry: std::filesystem::directory_iterator(target_voice_path)) {
        if (entry.path().extension().string() == ".wav") {
            wav_files.push_back(entry.path());
        }
    }
}

void GenerateAudioModel::WavFileModel(const std::filesystem::path &wav_path) {
    // Read Audio File
    auto x_length = WavIO::GetAudioLength(wav_path.string().c_str());
    auto x = new double[x_length];
    auto fs = WavIO::WavRead(wav_path.string().c_str(), x);

    // Check weather need to apply amp before modeling
    if (configure.model_amp != 0.0) {
        YALL_DEBUG_ << "Apply AMP Before Modeling";
        AutoAMP amp(x, x_length, configure.model_amp);
        x = amp.GetAMP();
    }

    AudioModel audioModel(x, x_length, fs, configure);
    auto model = audioModel.GetAudioModel();

    AudioModelIO audioModelIO(wav_path.string(), model, configure);
    audioModelIO.SaveAudioModel();
}

void GenerateAudioModel::GenerateModelFromFile() {
    YALL_INFO_ << "Modeling with " + std::to_string(std::thread::hardware_concurrency()) + " threads";
    for_each(wav_files.begin(), wav_files.end(), [&](const std::filesystem::path &wav_path) {
        WavFileModel(wav_path);
    });
}

template<class I, class F>
void GenerateAudioModel::for_each(size_t thread_count, I begin, I end, F f) {
    I it = begin;

    if (it == end)
        return;
    if (++it == end) {
        f(*begin);
        return;
    }

    // If std::thread::hardware_concurrency() does not get the number of threads, use 1 thread
    if (thread_count == 0) {
        thread_count = 1;
    }

    std::vector<std::thread> threads;
    threads.reserve(thread_count - 1);

    auto worker_ = [&begin, &end, &f] {
        while (true) {
            I it;
            it = begin;
            if (it == end)
                break;
            ++begin;
            f(*it);
        }
    };

    for (unsigned i = 0; i < thread_count - 1; ++i, ++it) {
        if (it == end)
            break;
        threads.emplace_back(std::thread(worker_));
    }

    worker_();
    for (auto &th: threads) {
        th.join();
    }
}

template<class I, class F>
void GenerateAudioModel::for_each(I begin, I end, F f) {
    for_each(std::lround(std::thread::hardware_concurrency() / 2), begin, end, f);
}

