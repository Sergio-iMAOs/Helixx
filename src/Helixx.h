#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/DelayFM.h"

namespace helixx
{

/** @brief Identifiers of the plugin parameters, as stored in the APVTS and in saved sessions. */
namespace ID
{
    /** @brief Algorithm applied to every channel, see helixx::Algorithm. */
    const juce::String AlgorithmID      = "algorithmID";
    /** @brief Modulator frequency as a multiple of the detected pitch. */
    const juce::String FMCoarseFreqID   = "fmFreq";
    /** @brief Modulation depth, 0..1. */
    const juce::String FMDepthID        = "fmDepthID";
    /** @brief Modulation range in milliseconds (absolute mode). */
    const juce::String FMRangeMsID      = "fmRangeMsID";
    /** @brief Modulation range as a frequency ratio (relative mode). */
    const juce::String FMRangeRelID     = "fmRangeRelID";
    /** @brief Switches between the relative and the absolute modulation range. */
    const juce::String FMIsRelativeID   = "fmIsRelID";
} // namespace ID

/**
 * @brief The Helixx effect: owns the parameters and runs DelayFM oversampled.
 *
 * Sits between the JUCE AudioProcessor and the framework-independent DSP in
 * `dsp/`: it declares the parameters, forwards their values to the DSP once
 * per block, and wraps the processing in an up/down-sampling stage to keep the
 * sidebands created by the modulation from aliasing.
 */
class Helixx
{
public:
    /**
     * @brief Creates the effect and registers its parameters.
     * @param processor The AudioProcessor that will host the parameters.
     */
    explicit Helixx(juce::AudioProcessor& processor);
    ~Helixx();

    /**
     * @brief Allocates everything needed for processing. Not real-time safe.
     * @param sampleRate      Host sample rate, in Hz.
     * @param samplesPerBlock Largest block size the host is expected to send.
     * @param numChannels     Number of channels to process.
     */
    void prepare(double sampleRate, unsigned int samplesPerBlock, unsigned int numChannels);

    /**
     * @brief Processes a block of audio in place. Call from the audio thread.
     * @param buffer Audio to process. Blocks of any size are accepted; channels
     *               beyond the count given to prepare() are left untouched.
     */
    void process(juce::AudioBuffer<float>& buffer);

    /** @brief Returns the parameter tree, for editor attachments and state saving. */
    juce::AudioProcessorValueTreeState& getAPVTS();

    /**
     * @brief Returns the latency introduced by the oversampling filters.
     * @return Latency in samples at the host rate, 0 before prepare().
     */
    int getLatencySamples() const;

private:
    juce::AudioProcessorValueTreeState parameters;

    // Raw parameter values, cached to avoid a lookup by name on every block.
    std::atomic<float>* algorithmParam  = nullptr;
    std::atomic<float>* coarseFreqParam = nullptr;
    std::atomic<float>* depthParam      = nullptr;
    std::atomic<float>* rangeMsParam    = nullptr;
    std::atomic<float>* rangeRelParam   = nullptr;
    std::atomic<float>* isRelativeParam = nullptr;

    DelayFM delayFM;

    static constexpr unsigned int oversamplingOrder = 2; // 2^2 = 4x oversampling
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    unsigned int maxBlockSize = 0;  // largest block the oversampler can take at once
    unsigned int numPreparedChannels = 0;

    std::vector<float*> channelPtrs;

    /** @brief Pushes the current parameter values to the DSP. */
    void updateParameters();
};

} // namespace helixx
