#include "TestSupport.h"

#include <vector>

#include "Config/Config.h"
#include "Core/Exception.h"
#include "Vocoder/VocoderBackend.h"

using namespace lessampler;

int main() {
    CHECK(backend::NeuralPolishAvailable());

    const std::vector<double> samples{0.1, -0.1, 0.05};
    Config configure;

    configure.neural_model_path.clear();
    bool rejected = false;
    try {
        (void) backend::PolishWaveform(samples, 44100, configure);
    } catch (const parameter_error &) {
        rejected = true;
    }
    CHECK(rejected);

    configure.neural_model_path = "/nonexistent/lessampler-polish-model.onnx";
    rejected = false;
    try {
        (void) backend::PolishWaveform(samples, 44100, configure);
    } catch (const parameter_error &) {
        rejected = true;
    }
    CHECK(rejected);

    return test::finish();
}
