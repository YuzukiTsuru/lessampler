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

AduioModelIO::AduioModelIO(const std::string &FilePath, lessAudioModel audioModel) : RootFilePath(FilePath), _audioModel(audioModel) {
    GenerateFilePath();
}

AduioModelIO::AduioModelIO(const std::string &FilePath) : RootFilePath(FilePath) {
    GenerateFilePath();
}

AduioModelIO::~AduioModelIO() {
    delete[] this->_audioModel.time_axis;
    delete[] this->_audioModel.f0;
    for (int i = 0; i < this->_audioModel.f0_length; ++i) {
        delete[] this->_audioModel.spectrogram[i];
        delete[] this->_audioModel.aperiodicity[i];
    }
    delete[] this->_audioModel.spectrogram;
    delete[] this->_audioModel.aperiodicity;
}

[[maybe_unused]] void AduioModelIO::SetFilePath(const std::string &Path) {
    this->RootFilePath = Path;
    GenerateFilePath();
}

[[maybe_unused]] void AduioModelIO::SetAudioModel(lessAudioModel audioModel) {
    this->_audioModel = audioModel;
}

[[maybe_unused]] lessAudioModel AduioModelIO::GetAudioModel() {
    return this->_audioModel;
}

[[maybe_unused]] std::string AduioModelIO::GetFilePath() {
    return this->RootFilePath.string();
}

void AduioModelIO::GenerateFilePath() {
    this->F0FilePath = this->RootFilePath / this->F0FileExt;
    this->SPFilePath = this->RootFilePath / this->SPFilePath;
    this->APFilePath = this->RootFilePath / this->APFilePath;
}

void AduioModelIO::SaveAudioModel() {
    // TODO: impl LZ4Stream to compress files
    WriteAudioContent();
}

lessAudioModel AduioModelIO::ReadAudioModel() {
    ReadAudioContent();
    return this->_audioModel;
}

void AduioModelIO::WriteAudioContent() {

}

void AduioModelIO::ReadAudioContent() {

}

void AduioModelIO::WriteOneParameter(FILE *fp, const char *text, double parameter, int size) {
    fwrite(text, 1, 4, fp);
    if (size == 4) {
        int parameter_int = static_cast<int>(parameter);
        fwrite(&parameter_int, 4, 1, fp);
    } else {
        fwrite(&parameter, 8, 1, fp);
    }
}

void AduioModelIO::LoadParameters(FILE *fp, int *f0_length, int *fft_size, int *number_of_dimensions) {
    char data_check[12];
    // NOF
    fread(&data_check, 1, 4, fp);
    fread(f0_length, 4, 1, fp);

    // FP (may skipped)
    fread(&data_check, 1, 12, fp);

    // FFT
    fread(&data_check, 1, 4, fp);
    fread(fft_size, 4, 1, fp);

    // NOD
    fread(&data_check, 1, 4, fp);
    fread(number_of_dimensions, 4, 1, fp);
    *number_of_dimensions = *number_of_dimensions == 0 ? *fft_size / 2 + 1 : *number_of_dimensions;

    // FS (may skipped)
    fread(&data_check, 1, 8, fp);
}

void AduioModelIO::CheckHeader(FILE *fp, const char *text) {
    char data_check[5];
    fread(data_check, 1, 4, fp);
    data_check[4] = '\0';
    if (0 != strcmp(data_check, text)) {
        fclose(fp);
        throw header_check_error(data_check, text);
    }
}

void AduioModelIO::WriteF0() {
    FILE *fp = fopen(this->F0FilePath.string().c_str(), "wb");
    if (nullptr == fp)
        throw file_open_error(this->F0FilePath.string());

    // Header
    fwrite(this->F0Header, 1, 4, fp);

    // Parameters
    WriteOneParameter(fp, this->F0LengthHeader, this->_audioModel.f0_length, 4);
    WriteOneParameter(fp, this->FramePeridoHeader, this->_audioModel.frame_period, 8);

    // Data
    fwrite(this->_audioModel.f0, 8, this->_audioModel.f0_length, fp);
    fclose(fp);
}

void AduioModelIO::WriteSP() {

}

void AduioModelIO::WriteAP() {

}

[[noreturn]] void AduioModelIO::ReadF0() {
    FILE *fp = fopen(this->F0FilePath.string().c_str(), "rb");
    if (nullptr == fp)
        throw file_open_error(this->F0FilePath.string());
    // Header
    try {
        CheckHeader(fp, this->F0Header);
    } catch (header_check_error &error) {
        YALL_ERROR_ << error.what();
    }

    // Parameters
    char data_check[5];
    // "NOF "
    fread(data_check, 1, 4, fp);
    int f0_length = 0;
    fread(&f0_length, 4, 1, fp);

    // "FP  "
    fread(data_check, 1, 4, fp);
    double frame_period;
    fread(&frame_period, 8, 1, fp);

    // Data
    fread(this->_audioModel.f0, 8, f0_length, fp);

    fclose(fp);
    for (int i = 0; i < f0_length; ++i)
        this->_audioModel.time_axis[i] = i / 1000.0 * frame_period;
}

