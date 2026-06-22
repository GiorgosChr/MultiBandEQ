#pragma once

#include "EqConstants.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// Draws the combined magnitude response of all enabled bands and shows a
// draggable control dot for each one. Dragging a dot changes that band's
// frequency (horizontal) and gain (vertical); the mouse wheel changes its Q.
// All edits go through the APVTS, so the knobs and the audio update too.
class ResponseCurveComponent final : public juce::Component,
                                     private juce::Timer
{
public:
    ResponseCurveComponent (juce::AudioProcessorValueTreeState& apvts, double sampleRate);
    ~ResponseCurveComponent() override = default;

    void paint (juce::Graphics&) override;

    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;
    void mouseMove  (const juce::MouseEvent&) override;
    void mouseExit  (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;

    // --- coordinate <-> value mapping (shared by paint and mouse handling) ---
    juce::Rectangle<float> plotBounds() const;
    float xForFreq (float freq) const;
    float freqForX (float x) const;
    float yForDb   (float db) const;
    float dbForY   (float y) const;
    juce::Point<float> dotPosition (int band) const;

    int  bandAt (juce::Point<float> p) const;          // nearest enabled band dot, or -1
    void setParam (const juce::String& paramId, float value);
    bool isCutType (int band) const;

    juce::AudioProcessorValueTreeState& apvts;
    double sampleRate;

    int draggedBand = -1;
    int hoveredBand = -1;

    static constexpr float maxDb      = 24.0f; // vertical range of the display, +/- maxDb
    static constexpr float hitRadius  = 14.0f; // how close the mouse must be to grab a dot

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResponseCurveComponent)
};
