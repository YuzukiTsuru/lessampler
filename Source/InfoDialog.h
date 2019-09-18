/*
  ==============================================================================

    InfoDialog.h
    Created: 18 Sep 2019 1:09:54pm
    Author:  Ghost Gloomy

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class InfoDialog    : public Component
{
public:
    InfoDialog();
    ~InfoDialog();

    void paint (Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InfoDialog)
};
