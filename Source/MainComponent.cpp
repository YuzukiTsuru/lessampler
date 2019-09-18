#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() {
    setSize(1024, 576);

    // Some platforms require permissions to open input channels so request that here
    if (RuntimePermissions::isRequired(RuntimePermissions::recordAudio) && !RuntimePermissions::isGranted(RuntimePermissions::recordAudio)) {
        RuntimePermissions::request(RuntimePermissions::recordAudio, [&](bool granted) { if (granted)  setAudioChannels (2, 2); });
    } else {
        // Specify the number of input and output channels that we want to open
        setAudioChannels(2, 2);
    }
}

MainComponent::~MainComponent() {
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
    
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) {
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources() {
    
}

//==============================================================================
void MainComponent::paint(Graphics &g) {
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    g.setColour(Colours::white);
    g.drawText(CharPointer_UTF8("\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x81"
                                "\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x81\xaf\xef\xbc\x81"
                                "Hello!"),
               getLocalBounds(), Justification::centred);
}

void MainComponent::resized(){
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}
