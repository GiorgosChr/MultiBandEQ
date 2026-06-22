#pragma once

#include <juce_core/juce_core.h>
#include <array>

//==============================================================================
// Shared configuration and parameter-ID helpers for the multi-band EQ.
//
// Plugin formats (VST3/AU) require a fixed parameter layout, so we pre-declare
// a fixed maximum number of bands. "Adding a band" simply enables the next
// dormant band; the plugin starts with the first few bands active.
//==============================================================================
namespace eq
{
    static constexpr int maxBands           = 8;
    static constexpr int defaultActiveBands = 3;

    static constexpr float minFreq  = 20.0f;
    static constexpr float maxFreq  = 20000.0f;
    static constexpr float minGainDb = -24.0f;
    static constexpr float maxGainDb = 24.0f;
    static constexpr float minQ     = 0.1f;
    static constexpr float maxQ     = 10.0f;
    static constexpr float defaultQ = 0.707f;

    // Filter type for a single band. The order must match the choice list
    // built in createParameterLayout().
    enum class FilterType
    {
        lowCut = 0,
        lowShelf,
        peak,
        highShelf,
        highCut
    };

    inline juce::StringArray filterTypeNames()
    {
        return { "Low Cut", "Low Shelf", "Peak", "High Shelf", "High Cut" };
    }

    // Sensible default centre frequency for band i, spread across the spectrum
    // on a roughly logarithmic scale (60 Hz .. ~15 kHz for 8 bands).
    inline float defaultFreqForBand (int index)
    {
        const float t = (maxBands > 1) ? (float) index / (float) (maxBands - 1) : 0.0f;
        return minFreq * std::pow (maxFreq / minFreq, 0.15f + 0.7f * t);
    }

    //==============================================================================
    // Parameter-ID helpers. Each band owns a small, stable set of parameters.
    inline juce::String bandOnId   (int i) { return "band" + juce::String (i) + "_on"; }
    inline juce::String bandTypeId (int i) { return "band" + juce::String (i) + "_type"; }
    inline juce::String bandFreqId (int i) { return "band" + juce::String (i) + "_freq"; }
    inline juce::String bandGainId (int i) { return "band" + juce::String (i) + "_gain"; }
    inline juce::String bandQId    (int i) { return "band" + juce::String (i) + "_q"; }
}
