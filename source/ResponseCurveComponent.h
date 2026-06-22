#pragma once

#include "EqConstants.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// Draws the combined magnitude response of all enabled bands. Polls the APVTS
// on a timer and rebuilds the curve so it tracks parameter changes live (from
// the GUI or from host automation).
class ResponseCurveComponent final : public juce::Component,
                                     private juce::Timer
{
public:
    ResponseCurveComponent (juce::AudioProcessorValueTreeState& apvts, double sampleRate);
    ~ResponseCurveComponent() override = default;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    double sampleRate;

    static constexpr float maxDb = 24.0f; // vertical range of the display, +/- maxDb

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResponseCurveComponent)
};
