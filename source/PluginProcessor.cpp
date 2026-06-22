#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::dsp::IIR::Coefficients<float>::Ptr makeBandCoefficients (eq::FilterType type,
                                                               float freq,
                                                               float q,
                                                               float gainDb,
                                                               double sampleRate)
{
    freq = juce::jlimit (eq::minFreq, (float) (sampleRate * 0.5 - 1.0), freq);
    q    = juce::jmax (0.01f, q);
    const float gainLinear = juce::Decibels::decibelsToGain (gainDb);

    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    switch (type)
    {
        case eq::FilterType::lowCut:    return Coeffs::makeHighPass  (sampleRate, freq, q);
        case eq::FilterType::lowShelf:  return Coeffs::makeLowShelf  (sampleRate, freq, q, gainLinear);
        case eq::FilterType::peak:      return Coeffs::makePeakFilter (sampleRate, freq, q, gainLinear);
        case eq::FilterType::highShelf: return Coeffs::makeHighShelf (sampleRate, freq, q, gainLinear);
        case eq::FilterType::highCut:   return Coeffs::makeLowPass   (sampleRate, freq, q);
    }

    return Coeffs::makePeakFilter (sampleRate, freq, q, gainLinear);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Logarithmic frequency range so the slider feels natural across 20 Hz–20 kHz.
    juce::NormalisableRange<float> freqRange { eq::minFreq, eq::maxFreq, 1.0f };
    freqRange.setSkewForCentre (1000.0f);

    juce::NormalisableRange<float> gainRange { eq::minGainDb, eq::maxGainDb, 0.1f };
    juce::NormalisableRange<float> qRange    { eq::minQ, eq::maxQ, 0.01f };
    qRange.setSkewForCentre (1.0f);

    for (int i = 0; i < eq::maxBands; ++i)
    {
        const bool activeByDefault = i < eq::defaultActiveBands;

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { eq::bandOnId (i), 1 },
            "Band " + juce::String (i + 1) + " On",
            activeByDefault));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { eq::bandTypeId (i), 1 },
            "Band " + juce::String (i + 1) + " Type",
            eq::filterTypeNames(),
            (int) eq::FilterType::peak));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { eq::bandFreqId (i), 1 },
            "Band " + juce::String (i + 1) + " Freq",
            freqRange,
            eq::defaultFreqForBand (i)));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { eq::bandGainId (i), 1 },
            "Band " + juce::String (i + 1) + " Gain",
            gainRange,
            0.0f));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { eq::bandQId (i), 1 },
            "Band " + juce::String (i + 1) + " Q",
            qRange,
            eq::defaultQ));
    }

    return layout;
}

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = (juce::uint32) juce::jmax (1, getTotalNumOutputChannels());

    for (int i = 0; i < eq::maxBands; ++i)
    {
        bands[(size_t) i].prepare (spec);
        bands[(size_t) i].reset();
        bandStates[(size_t) i] = {};   // force a coefficient refresh on the next block
        updateBand (i);
    }
}

void AudioPluginAudioProcessor::releaseResources()
{
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::updateBand (int index)
{
    const auto i = (size_t) index;

    const bool  on     = apvts.getRawParameterValue (eq::bandOnId (index))->load()   > 0.5f;
    const int   type   = (int) apvts.getRawParameterValue (eq::bandTypeId (index))->load();
    const float freq   = apvts.getRawParameterValue (eq::bandFreqId (index))->load();
    const float gainDb = apvts.getRawParameterValue (eq::bandGainId (index))->load();
    const float q      = apvts.getRawParameterValue (eq::bandQId (index))->load();

    auto& state = bandStates[i];

    // Only rebuild the (relatively expensive) coefficients when a value changed.
    if (state.on == on && state.type == type && state.freq == freq
        && state.gainDb == gainDb && state.q == q)
        return;

    state = { on, type, freq, gainDb, q };

    auto coeffs = makeBandCoefficients ((eq::FilterType) type, freq, q, gainDb, currentSampleRate);
    *bands[i].state = *coeffs;
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    for (int i = 0; i < eq::maxBands; ++i)
    {
        updateBand (i);

        if (bandStates[(size_t) i].on)
            bands[(size_t) i].process (context);
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
