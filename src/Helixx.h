#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/DelayFM.h"

namespace helixx
{

namespace ID
{
    const juce::String FMCoarseFreqID   = "fmFreq";
    const juce::String FMDepthID        = "FMDepthID";
} // namespace ID

class Helixx
{
public:
    explicit Helixx(juce::AudioProcessor& processor);
    ~Helixx();

    void prepare(double sampleRate, int samplesPerBlock, unsigned int numChannels);
    void process(juce::AudioBuffer<float>& buffer);

    juce::AudioProcessorValueTreeState& getAPVTS();

private:
    juce::AudioProcessorValueTreeState parameters;
    DelayFM delayFM;
    
    void updateParameters();
};

} // namespace helixx