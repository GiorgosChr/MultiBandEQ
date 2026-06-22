#pragma once

#include "EqConstants.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// A single vertical control strip for one EQ band: an on/off toggle, a filter
// type selector, and Freq / Gain / Q rotary sliders. Each control is bound to
// the matching APVTS parameter for the given band index.
class BandComponent final : public juce::Component
{
public:
    BandComponent (juce::AudioProcessorValueTreeState& apvts, int bandIndex);
    ~BandComponent() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    int bandIndex;

    juce::ToggleButton onButton;
    juce::ComboBox     typeBox;
    juce::Slider       freqSlider, gainSlider, qSlider;
    juce::Label        freqLabel, gainLabel, qLabel;

    std::unique_ptr<ButtonAttachment>   onAttachment;
    std::unique_ptr<ComboBoxAttachment> typeAttachment;
    std::unique_ptr<SliderAttachment>   freqAttachment, gainAttachment, qAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandComponent)
};
