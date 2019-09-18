/*
  ==============================================================================

    WarningDialog.h
    Created: 18 Sep 2019 1:09:37pm
    Author:  Ghost Gloomy

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class WarningDialog    : public Component
{
public:
    WarningDialog();
    ~WarningDialog();

    void paint (Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarningDialog)
};
