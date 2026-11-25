#include "TestSupport.h"

#include <string>
#include <vector>

#include "Audio/Envelope.h"
#include "Config/Config.h"
#include "Core/Exception.h"
#include "Vocoder/FeatureTypes.h"
#include "Vocoder/VocoderBackend.h"

using namespace lessampler;

int main() {
    CHECK(backend::NeuralFeatureAssemblyAvailable());

    Config configure;
    configure.neural_model_path = "/nonexistent/lessampler-feature-model.onnx";

    std::vector<FeatureSegment> empty;
    bool rejected = false;
    try {
        (void) backend::AssembleFeatureTimeline(empty, 44100, 0.99, configure);
    } catch (const parameter_error &) {
        rejected = true;
    }
    CHECK(rejected);

    std::vector<FeatureSegment> one{
            FeatureSegment{"/tmp/lessampler-missing-feature.wav", 0.0, 0.0, 100.0, Envelope::Default()}};
    configure.neural_model_path.clear();
    rejected = false;
    try {
        (void) backend::AssembleFeatureTimeline(one, 44100, 0.99, configure);
    } catch (const parameter_error &) {
        rejected = true;
    }
    CHECK(rejected);

    return test::finish();
}
