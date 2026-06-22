#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      responseCurve (p.apvts, p.getSampleRate() > 0.0 ? p.getSampleRate() : 44100.0)
{
    addAndMakeVisible (responseCurve);

    addButton.onClick    = [this] { addBand(); };
    removeButton.onClick = [this] { removeBand(); };
    addAndMakeVisible (addButton);
    addAndMakeVisible (removeButton);

    rebuildBandStrips();

    setResizable (true, true);
    setResizeLimits (520, 360, 1400, 900);
    setSize (780, 480);

    startTimerHz (10);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
int AudioPluginAudioProcessorEditor::bandOnMask() const
{
    int mask = 0;
    for (int i = 0; i < eq::maxBands; ++i)
        if (processorRef.apvts.getRawParameterValue (eq::bandOnId (i))->load() > 0.5f)
            mask |= (1 << i);

    return mask;
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    // Rebuild the visible strips if the set of enabled bands changed elsewhere
    // (preset load, host automation, undo, etc.).
    const int mask = bandOnMask();
    if (mask != lastOnMask)
        rebuildBandStrips();
}

void AudioPluginAudioProcessorEditor::rebuildBandStrips()
{
    bandStrips.clear();

    for (int i = 0; i < eq::maxBands; ++i)
    {
        if (processorRef.apvts.getRawParameterValue (eq::bandOnId (i))->load() > 0.5f)
        {
            auto* strip = new BandComponent (processorRef.apvts, i);
            addAndMakeVisible (strip);
            bandStrips.add (strip);
        }
    }

    lastOnMask = bandOnMask();

    const bool atMax = bandStrips.size() >= eq::maxBands;
    const bool atMin = bandStrips.size() <= 1;
    addButton.setEnabled (! atMax);
    removeButton.setEnabled (! atMin);

    resized();
}

void AudioPluginAudioProcessorEditor::addBand()
{
    // Enable the lowest currently-disabled band, resetting it to its defaults.
    for (int i = 0; i < eq::maxBands; ++i)
    {
        if (processorRef.apvts.getRawParameterValue (eq::bandOnId (i))->load() <= 0.5f)
        {
            auto resetToDefault = [this] (const juce::String& paramId)
            {
                if (auto* param = processorRef.apvts.getParameter (paramId))
                    param->setValueNotifyingHost (param->getDefaultValue());
            };

            resetToDefault (eq::bandTypeId (i));
            resetToDefault (eq::bandFreqId (i));
            resetToDefault (eq::bandGainId (i));
            resetToDefault (eq::bandQId (i));

            if (auto* onParam = processorRef.apvts.getParameter (eq::bandOnId (i)))
                onParam->setValueNotifyingHost (1.0f);

            break;
        }
    }

    rebuildBandStrips();
}

void AudioPluginAudioProcessorEditor::removeBand()
{
    // Disable the highest currently-enabled band (keep at least one).
    if (bandOnMask() == 0)
        return;

    for (int i = eq::maxBands - 1; i >= 0; --i)
    {
        if (processorRef.apvts.getRawParameterValue (eq::bandOnId (i))->load() > 0.5f)
        {
            if (auto* onParam = processorRef.apvts.getParameter (eq::bandOnId (i)))
                onParam->setValueNotifyingHost (0.0f);

            break;
        }
    }

    rebuildBandStrips();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff15171c));

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.drawText ("Multi-Band EQ", getLocalBounds().removeFromTop (30).reduced (12, 0),
                juce::Justification::centredLeft, false);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (8);

    area.removeFromTop (26); // title

    // Top: frequency response curve.
    responseCurve.setBounds (area.removeFromTop (juce::jmax (120, area.getHeight() / 3)));

    area.removeFromTop (6);

    // Bottom: add/remove controls.
    auto buttonRow = area.removeFromBottom (30);
    addButton.setBounds (buttonRow.removeFromLeft (120));
    buttonRow.removeFromLeft (8);
    removeButton.setBounds (buttonRow.removeFromLeft (120));

    area.removeFromBottom (6);

    // Middle: a row of band control strips.
    if (! bandStrips.isEmpty())
    {
        const int stripWidth = area.getWidth() / bandStrips.size();
        auto row = area;
        for (auto* strip : bandStrips)
            strip->setBounds (row.removeFromLeft (stripWidth));
    }
}
