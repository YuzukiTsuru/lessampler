/*
 * Copyright (c)  2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <fstream>

#include "AudioModelIO.h"
#include "Utils/LOG.h"
#include "Utils/exception.h"

AudioModelIO::AudioModelIO(const std::string &FilePath, lessAudioModel audioModel) : RootFilePath(FilePath), _audioModel(audioModel) {
    GenerateFilePath();
}

AudioModelIO::AudioModelIO(const std::string &FilePath) : RootFilePath(FilePath) {
    GenerateFilePath();
}

AudioModelIO::~AudioModelIO() {
    delete[] _audioModel.time_axis;
    delete[] _audioModel.f0;
    for (int i = 0; i < _audioModel.f0_length; ++i) {
        delete[] _audioModel.spectrogram[i];
        delete[] _audioModel.aperiodicity[i];
    }
    delete[] _audioModel.spectrogram;
    delete[] _audioModel.aperiodicity;
}

[[maybe_unused]] void AudioModelIO::SetFilePath(const std::string &Path) {
    RootFilePath = Path;
    GenerateFilePath();
}

[[maybe_unused]] void AudioModelIO::SetAudioModel(lessAudioModel audioModel) {
    _audioModel = audioModel;
}

[[maybe_unused]] lessAudioModel AudioModelIO::GetAudioModel() {
    return _audioModel;
}

[[maybe_unused]] std::string AudioModelIO::GetFilePath() {
    return RootFilePath.string();
}

void AudioModelIO::GenerateFilePath() {
    F0FilePath = RootFilePath.replace_extension(F0FileExt);
    SPFilePath = RootFilePath.replace_extension(SPFileExt);
    APFilePath = RootFilePath.replace_extension(APFileExt);
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

void AudioModelIO::WriteOneParameter(FILE *fp, const char *text, double parameter, int size) {
    fwrite(text, 1, 4, fp);
    if (size == 4) {
        int parameter_int = static_cast<int>(parameter);
        fwrite(&parameter_int, 4, 1, fp);
    } else {
        fwrite(&parameter, 8, 1, fp);
    }
}

void AudioModelIO::LoadParameters(FILE *fp, int *f0_length, int *fft_size) {
    char data_check[12];
    // NOF
    fread(&data_check, 1, 4, fp);
    fread(f0_length, 4, 1, fp);

    // FP (may skipped)
    fread(&data_check, 1, 12, fp);

    // FFT
    fread(&data_check, 1, 4, fp);
    fread(fft_size, 4, 1, fp);

    // FS (may skipped)
    fread(&data_check, 1, 8, fp);
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
    FILE *fp = fopen(F0FilePath.string().c_str(), "wb");
    if (nullptr == fp)
        throw file_open_error(F0FilePath.string());

    // Header
    fwrite(F0Header, 1, 4, fp);

    // Parameters
    WriteOneParameter(fp, F0LengthHeader, _audioModel.f0_length, 4);
    WriteOneParameter(fp, FramePeridoHeader, _audioModel.frame_period, 8);

    // Data
    fwrite(_audioModel.f0, 8, _audioModel.f0_length, fp);
    fclose(fp);
}

void AudioModelIO::WriteSP() {
    FILE *fp = fopen(SPFilePath.string().c_str(), "wb");
    if (nullptr == fp)
        throw file_open_error(SPFilePath.string());
    // Header
    fwrite(SPHeader, 1, 4, fp);

    // Parameters
    WriteOneParameter(fp, F0LengthHeader, _audioModel.f0_length, 4);
    WriteOneParameter(fp, FramePeridoHeader, _audioModel.frame_period, 8);
    WriteOneParameter(fp, FFTSizeHeader, _audioModel.fft_size, 4);
    WriteOneParameter(fp, FSHeader, _audioModel.fs, 4);

    // Data
    for (int i = 0; i < _audioModel.f0_length; ++i)
        fwrite(_audioModel.spectrogram[i], 8, _audioModel.w_length, fp);
    fclose(fp);
}

void AudioModelIO::WriteAP() {
    FILE *fp = fopen(APFilePath.string().c_str(), "wb");
    if (nullptr == fp)
        throw file_open_error(APFilePath.string());

    // Header
    fwrite(APHeader, 1, 4, fp);

    // Parameters
    WriteOneParameter(fp, F0LengthHeader, _audioModel.f0_length, 4);
    WriteOneParameter(fp, FramePeridoHeader, _audioModel.frame_period, 8);
    WriteOneParameter(fp, FFTSizeHeader, _audioModel.fft_size, 4);
    WriteOneParameter(fp, FSHeader, _audioModel.fs, 4);

    // Data
    for (int i = 0; i < _audioModel.f0_length; ++i)
        fwrite(_audioModel.aperiodicity[i], 8, _audioModel.w_length, fp);
    fclose(fp);
}

void AudioModelIO::ReadF0() {
    FILE *fp = fopen(F0FilePath.string().c_str(), "rb");
    if (nullptr == fp)
        throw file_open_error(F0FilePath.string());
    // Header
    try {
        CheckHeader(fp, F0Header);
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

    // Data
    _audioModel.f0 = new double[_audioModel.f0_length];
    fread(_audioModel.f0, 8, _audioModel.f0_length, fp);

    fclose(fp);

    _audioModel.time_axis = new double[_audioModel.f0_length];
    for (int i = 0; i < _audioModel.f0_length; ++i)
        _audioModel.time_axis[i] = i / 1000.0 * _audioModel.frame_period;
}

void AudioModelIO::ReadSP() {
    FILE *fp = fopen(SPFilePath.string().c_str(), "rb");
    if (nullptr == fp)
        throw file_open_error(SPFilePath.string());

    // Header
    try {
        CheckHeader(fp, SPHeader);
    } catch (header_check_error &error) {
        YALL_ERROR_ << error.what();
    }

    // Parameters
    LoadParameters(fp, &_audioModel.f0_length, &_audioModel.fft_size);

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
    FILE *fp = fopen(APFilePath.string().c_str(), "rb");
    if (nullptr == fp)
        throw file_open_error(APFilePath.string());

    // Header
    try {
        CheckHeader(fp, APHeader);
    } catch (header_check_error &error) {
        YALL_ERROR_ << error.what();
    }

    // Parameters
    int f0_length, fft_size;
    LoadParameters(fp, &f0_length, &fft_size);
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