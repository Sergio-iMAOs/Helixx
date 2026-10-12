#pragma once

#include <vector>
#include <cassert>

#include "FractionalDelay.h"
#include "Oscillator.h"
#include "PitchDetector.h"

namespace helixx
{

/**
 * @brief Multichannel frequency modulation effect built on modulated delay lines.
 *
 * Every channel is treated as an independent monophonic voice (e.g. one string
 * of a hexaphonic pickup) with its own pitch detector, sine modulator and
 * fractional delay line. The modulator runs at `detected f0 * coarse ratio` and
 * sweeps the delay time between 0 and `range * depth`; the changing delay
 * Doppler-shifts the signal, i.e. frequency-modulates it.
 *
 * The modulation range is either a fixed time in milliseconds or, in
 * relative mode, a fraction of the detected period, which keeps the modulation
 * index independent of the played pitch.
 *
 * Threading: prepare() allocates and must not run concurrently with process().
 * The setters and process() are meant to be called from the audio thread;
 * process() performs no allocations. Parameter changes are ramped linearly
 * over the next processed block.
 */
class DelayFM
{
public:
    /** @brief Largest modulation range accepted by setModRangeMs(), in milliseconds. */
    static constexpr float maxModRangeMs = 20.f;

    /** @brief Largest ratio accepted by setModRangeRatio() (4 = two octaves). */
    static constexpr float maxModRangeRatio = 4.f;

    DelayFM();
    ~DelayFM();

    /**
     * @brief Allocates and resets the per-channel state.
     * @param sampleRate  Rate, in Hz, of the audio passed to process().
     * @param numChannels Maximum number of channels process() will be given.
     */
    void prepare(double sampleRate, unsigned int numChannels);

    /**
     * @brief Processes a block of audio in place.
     * @param channelPtrs One pointer per channel, each to @p numSamples samples.
     * @param numChannels Number of entries in @p channelPtrs. Channels beyond
     *                    the count given to prepare() are left untouched.
     * @param numSamples  Number of samples per channel; any block size is accepted.
     */
    void process(float* const* channelPtrs, unsigned int numChannels, unsigned int numSamples);

    /**
     * @brief Sets the modulator frequency as a multiple of the detected pitch.
     * @param newFreq Ratio modulator / carrier, > 0 (1 = same frequency as the input).
     */
    void setCoarseFreq      (float newFreq);

    /**
     * @brief Sets how much of the modulation range is used.
     * @param newDepth Depth in [0, 1]; 0 leaves the signal unprocessed.
     */
    void setDepth           (float newDepth);

    /**
     * @brief Sets the modulation range used in absolute mode.
     * @param ms Peak-to-peak delay excursion at full depth, in [0, maxModRangeMs] milliseconds.
     */
    void setModRangeMs      (float ms);

    /**
     * @brief Sets the modulation range used in relative mode.
     * @param ratio Frequency ratio in [1, maxModRangeRatio] (2^(semitones / 12)).
     *              The peak-to-peak delay excursion at full depth is
     *              `period * (1 - 1 / ratio)`, so 1 means no modulation.
     */
    void setModRangeRatio   (float ratio);

    /**
     * @brief Chooses between the relative and the absolute modulation range.
     * @param isRelative true to use setModRangeRatio() (follows the pitch),
     *                   false to use setModRangeMs().
     */
    void setRelativeToPitch (bool isRelative);

private:
    // Smoothed parameters: the value reached at the end of the last block and
    // the one to reach at the end of the next.
    float coarseFreq        = 1.f;
    float coarseFreqTarget  = 1.f;
    float depth             = 0.f;
    float depthTarget       = 0.f;

    // Per-channel state.
    std::vector<FractionalDelay> delayLines;
    std::vector<Oscillator> modulators;
    std::vector<PitchDetector> pitchDetectors;
    std::vector<float> pitches;            // smoothed f0 at the end of the last block, Hz
    std::vector<float> modRangeSamples;    // smoothed modulation range at the end of the last block

    bool isRelativeToPitch  = false;
    float modRangeMs        = 0.f;
    float modRangeFactor    = 0.f;         // 1 - 1 / ratio: range as a fraction of the period

    float sampleRate        = 44100.f;
    float maxDelaySamples   = 0.f;

    /**
     * @brief Returns the peak-to-peak delay excursion at full depth.
     * @param f0 Fundamental frequency of the channel, in Hz (> 0).
     * @return Range in samples, for the current mode and range settings.
     */
    float getModRangeSamples(float f0) const;

    // Pitch assumed until a channel's detector produces its first estimate.
    static constexpr float initialPitchHz = PitchDetector::minFrequencyHz;
};

} // namespace helixx
