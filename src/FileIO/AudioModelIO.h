//
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_AUDIOMODELIO_H
#define LESSAMPLER_AUDIOMODELIO_H

#include <iostream>
#include <fstream>
#include <filesystem>
#include <parameterio.h>
#include "AudioModel/lessAudioModel.h"

class AduioModelIO {
public:
    AduioModelIO(const std::string &Path, lessAudioModel audioModel);

    explicit AduioModelIO(const std::string &Path);

    ~AduioModelIO();

    [[maybe_unused]] void SetFilePath(const std::string &Path);

    [[maybe_unused]] void SetAudioModel(lessAudioModel audioModel);

    [[maybe_unused]] std::string GetFilePath();

    [[maybe_unused]] lessAudioModel GetAudioModel();

    void SaveAudioModel();

    lessAudioModel ReadAudioModel();

private:
    lessAudioModel _audioModel{};
    std::filesystem::path RootFilePath{};
    std::filesystem::path F0FilePath{};
    std::filesystem::path SPFilePath{};
    std::filesystem::path APFilePath{};

protected:
    const std::string F0FileExt = "lessaudiof0";
    const std::string SPFileExt = "lessaudiosp";
    const std::string APFileExt = "lessaudioap";
    const char F0LengthHeader[4] = {'F', '0', 'L', ' '};            // number of samples (int)
    const char FramePeridoHeader[4] = {'F', 'P', ' ', ' '};         // frame perido (double)
    const char FFTSizeHeader[4] = {'F', 'F', 'T', ' '};             //  FFT size (int)
    const char DimensionsNumberHeader[4] = {'N', 'O', 'D', ' '};    // number of dimensions (int)
    const char FSHeader[4] = {'F', 'S', ' ', ' '};                  // number of dimensions (int)
    const char F0Header[4] = {'5', '4', '0', '0'};                  // F0
    const char SPHeader[4] = {'5', '4', '0', '1'};                  // SP
    const char APHeader[4] = {'5', '4', '0', '2'};                  // AP

private:
    void WriteAudioContent();

    void ReadAudioContent();

    void GenerateFilePath();

private: // Writer
    void WriteF0();

    void WriteSP();

    void WriteAP();

private: // Reader
    [[noreturn]] void ReadF0();

private:
    static void WriteOneParameter(FILE *fp, const char *text, double parameter, int size);

    static void LoadParameters(FILE *fp, int *f0_length, int *fft_size, int *number_of_dimensions);

    static void CheckHeader(FILE *fp, const char *text);
};


#endif //LESSAMPLER_AUDIOMODELIO_H
