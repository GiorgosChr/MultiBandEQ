#include "ResponseCurveComponent.h"
#include "PluginProcessor.h"   // for makeBandCoefficients()

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent (juce::AudioProcessorValueTreeState& s, double sr)
    : apvts (s), sampleRate (sr)
{
    startTimerHz (30);
}

void ResponseCurveComponent::timerCallback()
{
    repaint();
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (juce::Colour (0xff20242b));
    g.fillRoundedRectangle (bounds, 6.0f);

    const auto left   = bounds.getX();
    const auto right  = bounds.getRight();
    const auto top    = bounds.getY();
    const auto bottom = bounds.getBottom();
    const auto width  = bounds.getWidth();

    auto yForDb = [&] (float db)
    {
        return juce::jmap (db, -maxDb, maxDb, bottom, top);
    };

    // --- grid -----------------------------------------------------------------
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    for (float db = -maxDb + 6.0f; db < maxDb; db += 6.0f)
    {
        const float y = yForDb (db);
        g.drawHorizontalLine ((int) y, left, right);
    }

    // 0 dB reference line
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    const float zeroY = yForDb (0.0f);
    g.drawHorizontalLine ((int) zeroY, left, right);

    // vertical decade markers (100 Hz, 1 kHz, 10 kHz)
    auto xForFreq = [&] (float freq)
    {
        const float t = std::log (freq / eq::minFreq) / std::log (eq::maxFreq / eq::minFreq);
        return left + t * width;
    };

    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = xForFreq (f);
        g.drawVerticalLine ((int) x, top, bottom);
    }

    // --- combined magnitude response -----------------------------------------
    if (sampleRate <= 0.0)
        return;

    const int numPoints = juce::jmax (2, (int) width);
    std::vector<double> mags ((size_t) numPoints, 1.0);

    for (int b = 0; b < eq::maxBands; ++b)
    {
        if (apvts.getRawParameterValue (eq::bandOnId (b))->load() <= 0.5f)
            continue;

        const auto type   = (eq::FilterType) (int) apvts.getRawParameterValue (eq::bandTypeId (b))->load();
        const float freq  = apvts.getRawParameterValue (eq::bandFreqId (b))->load();
        const float gain  = apvts.getRawParameterValue (eq::bandGainId (b))->load();
        const float q     = apvts.getRawParameterValue (eq::bandQId (b))->load();

        auto coeffs = makeBandCoefficients (type, freq, q, gain, sampleRate);

        for (int i = 0; i < numPoints; ++i)
        {
            const float t = (float) i / (float) (numPoints - 1);
            const double freqAtX = eq::minFreq * std::pow (eq::maxFreq / eq::minFreq, (double) t);
            mags[(size_t) i] *= coeffs->getMagnitudeForFrequency (freqAtX, sampleRate);
        }
    }

    juce::Path response;
    for (int i = 0; i < numPoints; ++i)
    {
        const float db = juce::Decibels::gainToDecibels ((float) mags[(size_t) i], -100.0f);
        const float x  = left + (float) i;
        const float y  = juce::jlimit (top, bottom, yForDb (juce::jlimit (-maxDb, maxDb, db)));

        if (i == 0)
            response.startNewSubPath (x, y);
        else
            response.lineTo (x, y);
    }

    g.setColour (juce::Colour (0xff4ea1ff));
    g.strokePath (response, juce::PathStrokeType (2.0f));
}
