#pragma once

#include "EqConstants.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
// Builds the IIR coefficients for a single EQ band. Shared by the audio thread
// (PluginProcessor) and the GUI (ResponseCurveComponent) so the drawn response
// curve matches the processed audio exactly.
juce::dsp::IIR::Coefficients<float>::Ptr makeBandCoefficients (eq::FilterType type,
                                                               float freq,
                                                               float q,
                                                               float gainDb,
                                                               double sampleRate);

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMS", createParameterLayout() };

private:
    //==============================================================================
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    // Cached parameter values per band, used to detect changes and only rebuild
    // coefficients when something actually moved (avoids zipper noise).
    struct BandState
    {
        bool  on       = false;
        int   type     = -1;
        float freq     = 0.0f;
        float gainDb   = 0.0f;
        float q        = 0.0f;
    };

    void updateBand (int index);

    std::array<Filter, eq::maxBands>     bands;
    std::array<BandState, eq::maxBands>  bandStates;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
