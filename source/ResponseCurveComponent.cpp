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

//==============================================================================
// Coordinate / value mapping
juce::Rectangle<float> ResponseCurveComponent::plotBounds() const
{
    return getLocalBounds().toFloat().reduced (2.0f);
}

float ResponseCurveComponent::xForFreq (float freq) const
{
    const auto b = plotBounds();
    const float t = std::log (freq / eq::minFreq) / std::log (eq::maxFreq / eq::minFreq);
    return b.getX() + t * b.getWidth();
}

float ResponseCurveComponent::freqForX (float x) const
{
    const auto b = plotBounds();
    const float t = juce::jlimit (0.0f, 1.0f, (x - b.getX()) / b.getWidth());
    return eq::minFreq * std::pow (eq::maxFreq / eq::minFreq, t);
}

float ResponseCurveComponent::yForDb (float db) const
{
    const auto b = plotBounds();
    return juce::jmap (db, -maxDb, maxDb, b.getBottom(), b.getY());
}

float ResponseCurveComponent::dbForY (float y) const
{
    const auto b = plotBounds();
    return juce::jmap (y, b.getBottom(), b.getY(), -maxDb, maxDb);
}

bool ResponseCurveComponent::isCutType (int band) const
{
    const auto type = (eq::FilterType) (int) apvts.getRawParameterValue (eq::bandTypeId (band))->load();
    return type == eq::FilterType::lowCut || type == eq::FilterType::highCut;
}

juce::Point<float> ResponseCurveComponent::dotPosition (int band) const
{
    const float freq = apvts.getRawParameterValue (eq::bandFreqId (band))->load();
    float gain = isCutType (band) ? 0.0f
                                  : apvts.getRawParameterValue (eq::bandGainId (band))->load();
    return { xForFreq (freq), yForDb (juce::jlimit (-maxDb, maxDb, gain)) };
}

int ResponseCurveComponent::bandAt (juce::Point<float> p) const
{
    int   best     = -1;
    float bestDist = hitRadius;

    for (int b = 0; b < eq::maxBands; ++b)
    {
        if (apvts.getRawParameterValue (eq::bandOnId (b))->load() <= 0.5f)
            continue;

        const float d = dotPosition (b).getDistanceFrom (p);
        if (d < bestDist)
        {
            bestDist = d;
            best     = b;
        }
    }

    return best;
}

void ResponseCurveComponent::setParam (const juce::String& paramId, float value)
{
    if (auto* param = apvts.getParameter (paramId))
        param->setValueNotifyingHost (param->convertTo0to1 (value));
}

//==============================================================================
// Mouse interaction
void ResponseCurveComponent::mouseDown (const juce::MouseEvent& e)
{
    draggedBand = bandAt (e.position);
    repaint();
}

void ResponseCurveComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedBand < 0)
        return;

    setParam (eq::bandFreqId (draggedBand),
              juce::jlimit (eq::minFreq, eq::maxFreq, freqForX (e.position.x)));

    if (! isCutType (draggedBand))
        setParam (eq::bandGainId (draggedBand),
                  juce::jlimit (eq::minGainDb, eq::maxGainDb, dbForY (e.position.y)));

    repaint();
}

void ResponseCurveComponent::mouseUp (const juce::MouseEvent&)
{
    draggedBand = -1;
    repaint();
}

void ResponseCurveComponent::mouseMove (const juce::MouseEvent& e)
{
    const int previous = hoveredBand;
    hoveredBand = bandAt (e.position);

    setMouseCursor (hoveredBand >= 0 ? juce::MouseCursor::DraggingHandCursor
                                     : juce::MouseCursor::NormalCursor);

    if (hoveredBand != previous)
        repaint();
}

void ResponseCurveComponent::mouseExit (const juce::MouseEvent&)
{
    if (hoveredBand != -1)
    {
        hoveredBand = -1;
        repaint();
    }
}

void ResponseCurveComponent::mouseWheelMove (const juce::MouseEvent& e,
                                             const juce::MouseWheelDetails& wheel)
{
    const int band = draggedBand >= 0 ? draggedBand : bandAt (e.position);
    if (band < 0)
        return;

    const float q = apvts.getRawParameterValue (eq::bandQId (band))->load();
    setParam (eq::bandQId (band),
              juce::jlimit (eq::minQ, eq::maxQ, q * (1.0f + wheel.deltaY)));

    repaint();
}

//==============================================================================
void ResponseCurveComponent::paint (juce::Graphics& g)
{
    const auto bounds = plotBounds();

    g.setColour (juce::Colour (0xff20242b));
    g.fillRoundedRectangle (bounds, 6.0f);

    const auto left   = bounds.getX();
    const auto right  = bounds.getRight();
    const auto top    = bounds.getY();
    const auto bottom = bounds.getBottom();

    // --- grid -----------------------------------------------------------------
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    for (float db = -maxDb + 6.0f; db < maxDb; db += 6.0f)
        g.drawHorizontalLine ((int) yForDb (db), left, right);

    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawHorizontalLine ((int) yForDb (0.0f), left, right);

    for (float f : { 100.0f, 1000.0f, 10000.0f })
        g.drawVerticalLine ((int) xForFreq (f), top, bottom);

    // --- combined magnitude response -----------------------------------------
    if (sampleRate <= 0.0)
        return;

    const int numPoints = juce::jmax (2, (int) bounds.getWidth());
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

    // --- draggable band dots --------------------------------------------------
    for (int b = 0; b < eq::maxBands; ++b)
    {
        if (apvts.getRawParameterValue (eq::bandOnId (b))->load() <= 0.5f)
            continue;

        const auto pos    = dotPosition (b);
        const bool active = (b == draggedBand || b == hoveredBand);
        const float r     = active ? 9.0f : 7.0f;

        g.setColour (active ? juce::Colour (0xff7fbcff) : juce::Colour (0xff4ea1ff));
        g.fillEllipse (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colours::white);
        g.drawEllipse (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f, 1.5f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText (juce::String (b + 1),
                    juce::Rectangle<float> (pos.x - r, pos.y - r, r * 2.0f, r * 2.0f),
                    juce::Justification::centred);
    }

    // --- hint -----------------------------------------------------------------
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.setFont (juce::Font (11.0f));
    g.drawText ("Drag dots: freq / gain   •   wheel: Q",
                bounds.reduced (6.0f).removeFromBottom (14.0f),
                juce::Justification::bottomLeft, false);
}
