#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/DelayFM.h"

namespace helixx
{

class Helixx
{
public:
    Helixx(juce::AudioProcessorValueTreeState& parameters);
    ~Helixx();

    void prepare(double sampleRate, int samplesPerBlock, unsigned int numChannels);
    void process(juce::AudioBuffer<float>& buffer);

private:
    juce::AudioProcessorValueTreeState& parameters;
    DelayFM delayFM;
    
    void updateParameters();
};

} // namespace helixx