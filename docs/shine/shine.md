# Shine
Shine is a data class used to describe the parameters of individual audio transforms. Contains input and output audio files, audio interception and transformation parameters, audio pitch adjustment parameters, and elongation parameters. Shine also provides a convenient interface for describing timbre transformations, etc.

## Basic define

```c++
class ShinePara {
public:
    // Basic Transformation Parameters
    std::string input_file_name = {};
    std::string output_file_name = {};
    int time_percent = 0;
    double velocity = 0.0;
    double offset = 0.0;
    double required_length = 0.0;
    int required_frame = 0;
    double first_half_fixed_part = 0.0;
    double last_unused_part = 0.0;
    double volumes = 0;
    int modulation = 0;
    double wave_length = 0.0;
    double pre_cross_length = 0.0;
    double base_length = 0.0;
    double cross_length = 0.0;
    double stretch_length = 0.0;
    int output_samples = 0;
    double scale_num = 0.0;
    int tempo_num = 0;

public:
    // Pitch sections
    int *pitch_bend = nullptr;
    int pitch_length = 0;
    int pitch_step = 256;

public:
    // Options
    bool is_custom_pitch = false;
    bool is_gender = false;
}
```