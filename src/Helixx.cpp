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
                    DelayFM::maxModRangeMs, // maximum value
                    0.1f,   // Interval
                    0.4f,   // Skew factor
                    false   // Symmetric skew
                ),
                10.0f,   // default value
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction
                    (
                        [](float value, int) { return juce::String(value, 1) + " ms"; }
                    )
            ),
            std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID(ID::FMRangeRelID, VERSION_HINT), // parameterID
                "FM Range st", // parameter name
                juce::NormalisableRange<float>(
                    1.0f,                       // minimum value (0 st)
                    DelayFM::maxModRangeRatio,  // maximum value (24 st)
                    0.001f, // Interval
                    0.5f,   // Skew factor
                    false   // Symmetric skew
                ),
                2.0f,   // default value (12 st)
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction
                    (
                        [](float value, int)
                            {
                                return juce::String(std::log2(value) * 12.f, 2) + " st";
                            }
                    )
                    .withValueFromStringFunction
                    (
                        [](const juce::String& text)
                            {
                                return std::exp2(text.getFloatValue() / 12.f);
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
    coarseFreqParam = parameters.getRawParameterValue(ID::FMCoarseFreqID);
    depthParam      = parameters.getRawParameterValue(ID::FMDepthID);
    rangeMsParam    = parameters.getRawParameterValue(ID::FMRangeMsID);
    rangeRelParam   = parameters.getRawParameterValue(ID::FMRangeRelID);
    isRelativeParam = parameters.getRawParameterValue(ID::FMIsRelativeID);
}

Helixx::~Helixx()
{
}

void Helixx::prepare(double sampleRate, unsigned int samplesPerBlock, unsigned int numChannels)
{
    oversampling.reset();
    maxBlockSize = samplesPerBlock;
    numPreparedChannels = numChannels;

    if (numChannels == 0 || samplesPerBlock == 0)
        return;

    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        numChannels,
        oversamplingOrder,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,  // isMaximumQuality
        false  // isIntegerLatency (set true if you need integer-sample latency reporting)
    );

    oversampling->initProcessing(samplesPerBlock);

    // Load the parameters first so the DSP starts on their current values
    // instead of ramping to them from its defaults.
    updateParameters();
    delayFM.prepare(sampleRate * static_cast<double>(oversampling->getOversamplingFactor()), numChannels);

    channelPtrs.resize(numChannels);
}

void Helixx::process(juce::AudioBuffer<float> &buffer)
{
    if (oversampling == nullptr)
        return;

    // Update parameters
    updateParameters();

    const auto numChannels = juce::jmin(static_cast<size_t>(buffer.getNumChannels()),
                                        static_cast<size_t>(numPreparedChannels));
    const auto numSamples  = static_cast<size_t>(buffer.getNumSamples());
    if (numChannels == 0)
        return;

    auto block = juce::dsp::AudioBlock<float>(buffer).getSubsetChannelBlock(0, numChannels);

    // The oversampler can't take more than maxBlockSize samples at once, and
    // hosts are allowed to send larger blocks than announced: split them.
    for (size_t offset = 0; offset < numSamples; offset += maxBlockSize)
    {
        auto subBlock = block.getSubBlock(offset, juce::jmin(static_cast<size_t>(maxBlockSize), numSamples - offset));

        // Upsample
        auto oversampledBlock = oversampling->processSamplesUp(subBlock);

        // Get pointers
        for (size_t ch = 0; ch < numChannels; ++ch)
            channelPtrs[ch] = oversampledBlock.getChannelPointer(ch);

        // Process
        delayFM.process(channelPtrs.data(),
                        static_cast<unsigned int>(numChannels),
                        static_cast<unsigned int>(oversampledBlock.getNumSamples()));

        // Downsample
        oversampling->processSamplesDown(subBlock);
    }
}

juce::AudioProcessorValueTreeState &Helixx::getAPVTS()
{
    return parameters;
}

int Helixx::getLatencySamples() const
{
    return oversampling ? juce::roundToInt(oversampling->getLatencyInSamples()) : 0;
}

void Helixx::updateParameters()
{
    delayFM .setCoarseFreq      (coarseFreqParam->load());
    delayFM .setDepth           (depthParam->load());
    delayFM .setModRangeMs      (rangeMsParam->load());
    delayFM .setModRangeRatio   (rangeRelParam->load());
    delayFM .setRelativeToPitch (isRelativeParam->load() >= 0.5f);
}

} // namespace helixx
