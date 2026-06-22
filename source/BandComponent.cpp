#include "BandComponent.h"

//==============================================================================
namespace
{
    void configureRotary (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    }

    void configureLabel (juce::Label& label, const juce::String& text, juce::Component& owner)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (12.0f));
        owner.addAndMakeVisible (label);
    }
}

//==============================================================================
BandComponent::BandComponent (juce::AudioProcessorValueTreeState& apvts, int index)
    : bandIndex (index)
{
    onButton.setButtonText ("Band " + juce::String (index + 1));
    addAndMakeVisible (onButton);
    onAttachment = std::make_unique<ButtonAttachment> (apvts, eq::bandOnId (index), onButton);

    typeBox.addItemList (eq::filterTypeNames(), 1);
    addAndMakeVisible (typeBox);
    typeAttachment = std::make_unique<ComboBoxAttachment> (apvts, eq::bandTypeId (index), typeBox);

    configureRotary (freqSlider);
    configureRotary (gainSlider);
    configureRotary (qSlider);
    freqSlider.setTextValueSuffix (" Hz");
    gainSlider.setTextValueSuffix (" dB");

    addAndMakeVisible (freqSlider);
    addAndMakeVisible (gainSlider);
    addAndMakeVisible (qSlider);

    configureLabel (freqLabel, "Freq", *this);
    configureLabel (gainLabel, "Gain", *this);
    configureLabel (qLabel,    "Q",    *this);

    freqAttachment = std::make_unique<SliderAttachment> (apvts, eq::bandFreqId (index), freqSlider);
    gainAttachment = std::make_unique<SliderAttachment> (apvts, eq::bandGainId (index), gainSlider);
    qAttachment    = std::make_unique<SliderAttachment> (apvts, eq::bandQId (index),    qSlider);
}

void BandComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 6.0f);

    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 6.0f, 1.0f);
}

void BandComponent::resized()
{
    auto area = getLocalBounds().reduced (6);

    onButton.setBounds (area.removeFromTop (24));
    area.removeFromTop (4);
    typeBox.setBounds (area.removeFromTop (24));
    area.removeFromTop (6);

    // Three labelled rotary knobs stacked vertically.
    const int knobBlock = area.getHeight() / 3;

    auto layoutKnob = [&] (juce::Slider& slider, juce::Label& label)
    {
        auto block = area.removeFromTop (knobBlock);
        label.setBounds (block.removeFromTop (14));
        slider.setBounds (block);
    };

    layoutKnob (freqSlider, freqLabel);
    layoutKnob (gainSlider, gainLabel);
    layoutKnob (qSlider,    qLabel);
}
