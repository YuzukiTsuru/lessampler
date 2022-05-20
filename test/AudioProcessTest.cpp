// Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

//
// Created by gloom on 2022/5/20.
//

#include "FileIO/AudioModelIO.h"
#include "AudioModel/AudioModel.h"
#include "ConfigUnit/lessConfigure.h"
#include "AudioProcess/AudioProcess.h"
#include "libUTAU/libUTAU.h"
#include "Utils/LOG.h"
#include "world/synthesis.h"

#include "../lib/World/tools/audioio.h"

// "../../test/ne.wav" "../../test/ne2.wav" A#3 100 "" 3.0 2100 80.0 66.0 100 0 !120 AA#203#AB#4#AA///+/9/8/7/6/5/4/3/3/4/4/6/7/9//ACAEAHAKAMAOAQAR#2#AQAPAMAKAGAC/+/7/3/0/x/v/t/s/s/t/u/w/y/1/5/9AAAEAIALAOARASAU#2#ATARAOALAIAEAB/9/5/2/z/w/u/t/s/s/t/v/x/0/3/6/+ACAGAJANAPARATAUAUATASAQANAKAHAD///7/4/0/x/v/t/s#2#/u/v/y/1/4/8AAAEAHALAOAQASATAUAUATARAPAMAJAFAB/+/6/2/z/w/u/t/s/s/t/u/w/z/2/6/9ABAFAJAMAOAQARARAQAQAOAMAKAHAFACAA/+/8/6/5/4#3#/5/6/7/8/9/+//AAAAAB#2#AA#2#

int main(int argc, char *argv[]) {
    YALL_DEBUG_.EnableDebug();

    std::string FileName;

    if (argc < 2) {
        FileName = "../../test/vaiueo2d.wav";
    } else {
        FileName = argv[1];
    }
    // Open wav File

    int x_length = GetAudioLength(FileName.c_str());
    if (x_length <= 0) {
        if (x_length == 0)
            std::cerr << "error: File \"" << FileName << "\" not found" << std::endl;
        else
            std::cerr << "error: File \"" << FileName << "\" is not a .wav format" << std::endl;
        return EXIT_FAILURE;
    }
    auto *x = new double[x_length];

    // wavread() must be called after GetAudioLength().
    int fs, nbit;
    wavread(FileName.c_str(), &fs, &nbit, x);

    lessConfigure configure{};

    AudioModel audioModel(x, x_length, fs, configure);

    auto less_i = audioModel.GetAudioModel();

    libUTAU utau(argc, argv);

    utau.CheckPara(less_i);

    utau.printUTAUPara();

    auto utauPara = utau.getUTAUPara();

    UTAUFlags utauFlags;

    AduioProcess aduioProcess(less_i, utauPara, utauFlags);

    auto less_t = aduioProcess.GetTransAudioModel();

    int y_length = static_cast<int>((less_t.t_f0_length - 1) * less_i.frame_period / 1000.0 * less_i.fs) + 1;
    auto *y = new double[y_length];
    for (int i = 0; i < y_length; ++i) y[i] = 0.0;
    Synthesis(less_t.t_f0, less_t.t_f0_length, less_t.t_spectrogram, less_t.t_aperiodicity,
              less_i.fft_size, less_i.frame_period, less_i.fs, y_length, y);

    wavwrite(y, y_length, less_i.fs, nbit, argv[2]);
}