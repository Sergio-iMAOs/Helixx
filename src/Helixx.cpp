#include "Helixx.h"


namespace helixx
{

namespace {
    constexpr int VERSION_HINT = 1;
} // anonymous-namespace

Helixx::Helixx(juce::AudioProcessor& processor)
    :
    parameters (
        processor,
        nullptr,
        juce::Identifier ("APVTSHelixx"),
        {
            std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID(ID::FMCoarseFreqID, VERSION_HINT), // parameterID
                "FM Coarse Freq", // parameter name
                0.25f, // minimum value
                4.0f, // maximum value
                1.0f // default value
            ),
            std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID(ID::FMDepthID, VERSION_HINT), // parameterID
                "FM Depth", // parameter name
                0.f, // minimum value
                1.0f, // maximum value
                0.0f // default value
            )
        }
    )
{       
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
    const auto numFrames = static_cast<unsigned int>(buffer.getNumSamples());
    delayFM.process(buffer.getArrayOfWritePointers(), numFrames);
}

juce::AudioProcessorValueTreeState &Helixx::getAPVTS()
{
    return parameters;
}

void Helixx::updateParameters()
{
    delayFM .setCoarseFreq  (*parameters.getRawParameterValue(ID::FMCoarseFreqID));
    delayFM .setDepth       (*parameters.getRawParameterValue(ID::FMDepthID));
}

} // namespace helixx