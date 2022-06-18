/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <fstream>
#include <utility>

#include "AudioModelIO.h"
#include "Utils/LOG.h"
#include "Utils/exception.h"

AudioModelIO::AudioModelIO(std::filesystem::path Path, lessAudioModel audioModel) :
        in_file_path(std::move(Path)), _audioModel(std::move(audioModel)) {
    root_file_path = in_file_path;
    GenerateFilePath();
}

AudioModelIO::AudioModelIO(std::filesystem::path Path) : in_file_path(std::move(Path)) {
    root_file_path = in_file_path;
    GenerateFilePath();
}

AudioModelIO::~AudioModelIO() = default;

[[maybe_unused]] void AudioModelIO::SetFilePath(const std::filesystem::path &Path) {
    root_file_path = Path;
    GenerateFilePath();
}

[[maybe_unused]] void AudioModelIO::SetAudioModel(lessAudioModel audioModel) {
    _audioModel = std::move(audioModel);
}

[[maybe_unused]] lessAudioModel AudioModelIO::GetAudioModel() {
    return _audioModel;
}

[[maybe_unused]] std::filesystem::path AudioModelIO::GetFilePath() {
    return root_file_path;
}

void AudioModelIO::GenerateFilePath() {
    f0_file_path = root_file_path.replace_extension(f0_file_ext);
    sp_file_path = root_file_path.replace_extension(sp_file_ext);
    ap_file_path = root_file_path.replace_extension(ap_file_ext);
}

void AudioModelIO::SaveAudioModel() {
    // TODO: impl LZ4Stream to compress files
    WriteAudioContent();
}

lessAudioModel AudioModelIO::ReadAudioModel() {
    ReadAudioContent();
    return _audioModel;
}

void AudioModelIO::WriteAudioContent() {
    WriteF0();
    WriteSP();
    WriteAP();
}

void AudioModelIO::ReadAudioContent() {
    ReadF0();
    ReadSP();
    ReadAP();
}

void AudioModelIO::WriteOneParameter(FILE *fp, const char *text, int parameter) {
    fwrite(text, 1, 4, fp);
    fwrite(&parameter, 4, 1, fp);
}

void AudioModelIO::WriteOneParameter(FILE *fp, const char *text, double parameter) {
    fwrite(text, 1, 4, fp);
    fwrite(&parameter, sizeof(double), 1, fp);
}

void AudioModelIO::LoadParameters(FILE *fp, int *f0_length, int *fft_size, double *frame_peroid, int *fs) {
    char data_check[5];
    // NOF
    fread(&data_check, 1, 4, fp);
    fread(f0_length, 4, 1, fp);

    // FP
    fread(&data_check, 1, 4, fp);
    *frame_peroid = 0.0;
    fread(frame_peroid, sizeof(double), 1, fp);

    // FFT
    fread(&data_check, 1, 4, fp);
    fread(fft_size, 4, 1, fp);

    // FS
    fread(&data_check, 1, 4, fp);
    fread(fs, 4, 1, fp);
}

int AudioModelIO::CheckHeader(FILE *fp, const char *text) {
    char data_check[5];
    fread(data_check, 1, 4, fp);
    data_check[4] = '\0';
    if (0 != strcmp(data_check, text)) {
        fclose(fp);
        throw header_check_error(data_check, text);
    }
    return 0;
}

void AudioModelIO::WriteF0() {
    FILE *fp = fopen(f0_file_path.string().c_str(), "wb");
    if (nullptr == fp)
        throw file_open_error(f0_file_path.string());

    // Header
    fwrite(f0_header, 1, 4, fp);

    // Parameters
    WriteOneParameter(fp, f0_length_header, _audioModel.f0_length);
    WriteOneParameter(fp, frame_period_header, _audioModel.frame_period);
    WriteOneParameter(fp, x_length_header, _audioModel.x_length);

    // Data
    fwrite(_audioModel.f0.data(), 8, _audioModel.f0_length, fp);
    fclose(fp);
}

void AudioModelIO::WriteSP() {
    FILE *fp = fopen(sp_file_path.string().c_str(), "wb");
    if (nullptr == fp)
        throw file_open_error(sp_file_path.string());
    // Header
    fwrite(sp_header, 1, 4, fp);

    // Parameters
    WriteOneParameter(fp, f0_length_header, _audioModel.f0_length);
    WriteOneParameter(fp, frame_period_header, _audioModel.frame_period);
    WriteOneParameter(fp, fft_size_header, _audioModel.fft_size);
    WriteOneParameter(fp, fs_header, _audioModel.fs);

    // Data
    for (int i = 0; i < _audioModel.f0_length; ++i)
        fwrite(_audioModel.spectrogram[i].data(), 8, _audioModel.w_length, fp);
    fclose(fp);
}

void AudioModelIO::WriteAP() {
    FILE *fp = fopen(ap_file_path.string().c_str(), "wb");
    if (nullptr == fp)
        throw file_open_error(ap_file_path.string());

    // Header
    fwrite(ap_header, 1, 4, fp);

    // Parameters
    WriteOneParameter(fp, f0_length_header, _audioModel.f0_length);
    WriteOneParameter(fp, frame_period_header, _audioModel.frame_period);
    WriteOneParameter(fp, fft_size_header, _audioModel.fft_size);
    WriteOneParameter(fp, fs_header, _audioModel.fs);

    // Data
    for (int i = 0; i < _audioModel.f0_length; ++i)
        fwrite(_audioModel.aperiodicity[i].data(), 8, _audioModel.w_length, fp);
    fclose(fp);
}

void AudioModelIO::ReadF0() {
    FILE *fp = fopen(f0_file_path.string().c_str(), "rb");
    if (nullptr == fp)
        throw file_open_error(f0_file_path.string());
    // Header
    try {
        CheckHeader(fp, f0_header);
    } catch (header_check_error &error) {
        YALL_ERROR_ << error.what();
    }

    // Parameters
    char data_check[5];
    // "NOF "
    fread(data_check, 1, 4, fp);
    fread(&_audioModel.f0_length, 4, 1, fp);

    // "FP  "
    fread(data_check, 1, 4, fp);
    fread(&_audioModel.frame_period, 8, 1, fp);

    // "XL  "
    fread(data_check, 1, 4, fp);
    fread(&_audioModel.x_length, 4, 1, fp);

    // Data
    _audioModel.f0.reserve(_audioModel.f0_length);
    fread(_audioModel.f0, 8, _audioModel.f0_length, fp);

    fclose(fp);

    _audioModel.time_axis = new double[_audioModel.f0_length];
    for (int i = 0; i < _audioModel.f0_length; ++i)
        _audioModel.time_axis[i] = i / 1000.0 * _audioModel.frame_period;
}

void AudioModelIO::ReadSP() {
    FILE *fp = fopen(sp_file_path.string().c_str(), "rb");
    if (nullptr == fp)
        throw file_open_error(sp_file_path.string());

    // Header
    try {
        CheckHeader(fp, sp_header);
    } catch (header_check_error &error) {
        YALL_ERROR_ << error.what();
    }

    // Parameters
    LoadParameters(fp, &_audioModel.f0_length, &_audioModel.fft_size, &_audioModel.frame_period, &_audioModel.fs);

    _audioModel.w_length = _audioModel.fft_size / 2 + 1;

    _audioModel.spectrogram = new double *[_audioModel.f0_length];
    for (int i = 0; i < _audioModel.f0_length; ++i)
        _audioModel.spectrogram[i] = new double[_audioModel.fft_size / 2 + 1];
    // Data
    for (int i = 0; i < _audioModel.f0_length; ++i)
        fread(_audioModel.spectrogram[i], 8, _audioModel.w_length, fp);

    fclose(fp);
}

void AudioModelIO::ReadAP() {
    FILE *fp = fopen(ap_file_path.string().c_str(), "rb");
    if (nullptr == fp)
        throw file_open_error(ap_file_path.string());

    // Header
    try {
        CheckHeader(fp, ap_header);
    } catch (header_check_error &error) {
        YALL_ERROR_ << error.what();
    }

    // Parameters
    int f0_length, fft_size, fs;
    double frame_peroid;
    LoadParameters(fp, &f0_length, &fft_size, &frame_peroid, &fs);

    if (f0_length != _audioModel.f0_length) {
        throw parameter_error("SP AP File diff F0 LENGTH OR FFT Size, Broken Modeling");
    }

    _audioModel.aperiodicity = new double *[_audioModel.f0_length];
    for (int i = 0; i < _audioModel.f0_length; ++i)
        _audioModel.aperiodicity[i] = new double[_audioModel.fft_size / 2 + 1];
    // Data
    for (int i = 0; i < f0_length; ++i)
        fread(_audioModel.aperiodicity[i], 8, _audioModel.w_length, fp);

    fclose(fp);
}

bool AudioModelIO::CheckAudioModelFile(const std::filesystem::path &path) {
    return std::filesystem::exists(path);
}

bool AudioModelIO::CheckAudioModel() {
    YALL_DEBUG_ << "Check AudioModel File: " + in_file_path.string();

    if (!CheckAudioModelFile(f0_file_path) || !CheckAudioModelFile(sp_file_path) || !CheckAudioModelFile(ap_file_path)) {
        YALL_DEBUG_ << "Audio Model NOT Exist.";
        return false;
    } else {
        YALL_DEBUG_ << "Audio Model Exist.";
        return true;
    }
}
