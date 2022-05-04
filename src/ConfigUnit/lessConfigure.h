//
// Created by gloom on 2022/5/4.
//

#ifndef LESSAMPLER_LESSCONFIGURE_H
#define LESSAMPLER_LESSCONFIGURE_H

#include <iostream>

class lessConfigure {
public:
    std::string version;
    bool debug_mode;
    double audio_model_frame_period;
    bool custom_fft_size;
    int fft_size;
    std::string f0_mode;
    int f0_speed;
    double f0_dio_floor;
    double f0_harvest_floor;
    double f0_cheap_trick_floor;
    double f0_allow_range;
    double ap_threshold;
};

#endif //LESSAMPLER_LESSCONFIGURE_H
