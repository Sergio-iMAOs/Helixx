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
                0.25f,  // minimum value
                4.0f,   // maximum value
                1.0f    // default value
            ),
            std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID(ID::FMDepthID, VERSION_HINT), // parameterID
                "FM Depth", // parameter name
                juce::NormalisableRange<float>(
                    0.f,        // minimum value
                    1.0f,       // maximum value
                    0.0001f,    // Interval
                    0.4f,       // Skew factor
                    false       // Symmetric skew
                ),
                0.0f   // default value
            ),
            std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID(ID::FMRangeMsID, VERSION_HINT), // parameterID
                "FM Range ms", // parameter name
                juce::NormalisableRange<float>(
                    0.f,    // minimum value
                    20.f,   // maximum value
                    0.1f,   // Interval
                    0.4f,   // Skew factor
                    false   // Symmetric skew
                ),
                10.0f,   // default value
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction
                    (
                        [](float value, int) { return juce::String(value) + " ms"; }
                    )
            ),
            std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID(ID::FMRangeRelID, VERSION_HINT), // parameterID
                "FM Range st", // parameter name
                juce::NormalisableRange<float>(
                    0.f,    // minimum value
                    std::exp2(24.0f / 12.f),  // maximum value
                    0.001f,   // Interval
                    0.5f,   // Skew factor
                    false   // Symmetric skew
                ),
                std::exp2(12.0f / 12.0f),   // default value
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction
                    (
                        [](float value, int)
                            {
                                if (value == 0.f)
                                    return juce::String("0");
                                else
                                    return juce::String(std::log2(value) * 12.f) + " st";
                            }
                    )
                    .withValueFromStringFunction
                    (
                        [](const juce::String& text)
                            {
                                auto value = text.getFloatValue();
                                return std::exp2(value * 0.08333333333);
                            }
                    )
            ),
            std::make_unique<juce::AudioParameterBool> (
                juce::ParameterID(ID::FMIsRelativeID, VERSION_HINT), // parameter ID
                "FM Relative Range",    // parameter name
                false                   // default value
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
    delayFM .setCoarseFreq      (*parameters.getRawParameterValue(ID::FMCoarseFreqID));
    delayFM .setDepth           (*parameters.getRawParameterValue(ID::FMDepthID));
    delayFM .setModRangeMs      (*parameters.getRawParameterValue(ID::FMRangeMsID));
    delayFM .setModRangeRel     (*parameters.getRawParameterValue(ID::FMRangeRelID));
    delayFM .setRelativeToPitch (static_cast<bool>(*parameters.getRawParameterValue(ID::FMIsRelativeID)));
}

} // namespace helixx