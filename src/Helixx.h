#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/DelayFM.h"

namespace helixx
{

namespace ID
{
    const juce::String FMCoarseFreqID   = "fmFreq";
    const juce::String FMDepthID        = "fmDepthID";
    const juce::String FMRangeMsID      = "fmRangeMsID";
    const juce::String FMRangeRelID     = "fmRangeRelID";
    const juce::String FMIsRelativeID   = "fmIsRelID";
} // namespace ID

class Helixx
{
public:
    explicit Helixx(juce::AudioProcessor& processor);
    ~Helixx();

    void prepare(double sampleRate, unsigned int samplesPerBlock, unsigned int numChannels);
    void process(juce::AudioBuffer<float>& buffer);

    juce::AudioProcessorValueTreeState& getAPVTS();
    int getLatencySamples() const;

private:
    juce::AudioProcessorValueTreeState parameters;
    DelayFM delayFM;

    unsigned int oversamplingFactor = 2; // 2^2 = 4x oversampling
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    std::vector<float*> channelPtrs;
    
    void updateParameters();
};

} // namespace helixx