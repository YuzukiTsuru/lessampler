/*
  ==============================================================================

    OptionSetter.h
    Created: 18 Sep 2019 1:10:43pm
    Author:  Ghost Gloomy

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class OptionSetter    : public Component
{
public:
    OptionSetter();
    ~OptionSetter();

    void paint (Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OptionSetter)
};
