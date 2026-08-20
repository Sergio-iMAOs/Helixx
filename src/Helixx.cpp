#include "Helixx.h"


namespace helixx
{

namespace
{
    #define FMCoarseFreqID    "fmFreq"
    #define FMDepthID   "FMDepthID"
}

Helixx::Helixx(juce::AudioProcessorValueTreeState& parameters)
    :
    parameters(parameters)
{
    parameters.createAndAddParameter (
        std::make_unique<juce::AudioParameterFloat> (FMCoarseFreqID, // parameterID
            "FM Coarse Freq", // parameter name
            0.25f, // minimum value
            4.0f, // maximum value
            1.0f) // default value
    );
    parameters.createAndAddParameter (
        std::make_unique<juce::AudioParameterFloat> (FMDepthID, // parameterID
            "FM Depth", // parameter name
            0.f, // minimum value
            1.0f, // maximum value
            0.0f) // default value
    );
}

Helixx::~Helixx()
{
}

void Helixx::prepare(double sampleRate, int samplesPerBlock, unsigned int numChannels)
{
    delayFM.prepare(sampleRate, samplesPerBlock, numChannels);
}

void Helixx::process(juce::AudioBuffer<float> &buffer)
{
    updateParameters();
    const auto numChannels = buffer.getNumChannels();
    const auto numFrames = buffer.getNumSamples();

    for (auto chIdx = 0; chIdx < numChannels; ++chIdx)
    {
        float* const* channels = buffer.getArrayOfWritePointers();
        delayFM.process(channels, numFrames);
    }
}

void Helixx::updateParameters()
{
    delayFM .setCoarseFreq  (*parameters.getRawParameterValue(FMCoarseFreqID));
    delayFM .setDepth       (*parameters.getRawParameterValue(FMDepthID));
}

} // namespace helixx