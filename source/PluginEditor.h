#pragma once

#include "PluginProcessor.h"
#include "BandComponent.h"
#include "ResponseCurveComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;          // rebuilds strips if the on/off set changed
    int  bandOnMask() const;                // bit i set when band i is enabled
    void rebuildBandStrips();
    void addBand();
    void removeBand();

    AudioPluginAudioProcessor& processorRef;

    ResponseCurveComponent responseCurve;

    juce::OwnedArray<BandComponent> bandStrips;
    juce::TextButton addButton    { "+ Add Band" };
    juce::TextButton removeButton { "- Remove Band" };

    int lastOnMask = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
