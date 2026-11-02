// SPDX-FileCopyrightText: 2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <vector>
#include <cassert>

#include "FractionalDelay.h"
#include "Oscillator.h"
#include "PitchDetector.h"

namespace helixx
{

/**
 * @brief Adaptive distortion algorithms selectable in DelayFM.
 *
 * All of them are driven by the same sine modulator running at
 * `detected f0 * coarse ratio`; they differ in what the modulator acts on.
 * The order matches the choices of the plugin parameter.
 */
enum class Algorithm
{
    /** @brief Modulated delay line (Lazzarini, Timoney & Lysaght, DAFx-07). */
    AdaptiveFM,
    /** @brief Coefficient-modulated first-order allpass filter (Pekonen, DAFx-08). */
    Allpass,
    /** @brief Coefficient-modulated one-pole filter, y(n) = x(n) + beta m(n) y(n-1)
     *         (Kleimola, Lazzarini, Valimaki & Timoney, EURASIP JASP 2011). */
    FeedbackAM
};

/**
 * @brief Multichannel adaptive modulation effect: frequency modulation built
 *        on modulated delay lines, plus two filter-based alternatives.
 *
 * Every channel is treated as an independent monophonic voice (e.g. one string
 * of a hexaphonic pickup) with its own pitch detector, sine modulator and
 * fractional delay line. The modulator runs at `detected f0 * coarse ratio`.
 * What it modulates depends on the Algorithm:
 *
 * - AdaptiveFM: it sweeps the delay time between 0 and `range * depth`; the
 *   changing delay Doppler-shifts the signal, i.e. frequency-modulates it.
 *   The modulation range is either a fixed time in milliseconds or, in
 *   relative mode, a fraction of the detected period, which keeps the
 *   modulation index independent of the played pitch.
 * - Allpass: it is the coefficient of a first-order allpass filter, scaled by
 *   `depth`, which phase-distorts mostly the low end of the signal.
 * - FeedbackAM: it amplitude-modulates a one-sample feedback path whose gain
 *   is set by `depth`.
 *
 * The range settings only affect AdaptiveFM.
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

    /** @brief Peak allpass coefficient at full depth. The filter is stable up to 1. */
    static constexpr float maxAllpassCoeff = 0.999f;

    /** @brief Feedback amount (beta) at full depth. Kept below 1, where the
     *         gain of the feedback loop grows without bound at low pitches. */
    static constexpr float maxFeedback = 0.9f;

    DelayFM();
    ~DelayFM();

    /**
     * @brief Allocates and resets the per-channel state.
     * @param sampleRate  Rate, in Hz, of the audio passed to process().
     * @param numChannels Maximum number of channels process() will be given.
     */
    void prepare(double sampleRate, unsigned int numChannels);

    /**
     * @brief Selects the algorithm; takes effect on the next processed block.
     * @param newAlgorithm Algorithm to run. Changing it clears the audio held
     *                     in the delay lines and filters.
     */
    void setAlgorithm       (Algorithm newAlgorithm);

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
     * @brief Sets the amount of modulation.
     * @param newDepth Depth in [0, 1]. In AdaptiveFM it is how much of the
     *                 modulation range is used, in Allpass it sets the peak
     *                 filter coefficient (up to #maxAllpassCoeff), in FeedbackAM
     *                 the feedback amount relative to #maxFeedback. 0 leaves
     *                 the signal unprocessed (delayed by one sample in Allpass).
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

    /**
     * @brief Returns a channel's latest pitch estimate. Call from the audio thread.
     * @param channel Channel index, below the count given to prepare().
     * @return Frequency in Hz, or 0 if the latest analysis found no pitch.
     */
    float getDetectedPitch(unsigned int channel) const;

private:
    // Smoothed parameters: the value reached at the end of the last block and
    // the one to reach at the end of the next.
    float coarseFreq        = 1.f;
    float coarseFreqTarget  = 1.f;
    float depth             = 0.f;
    float depthTarget       = 0.f;

    Algorithm algorithm        = Algorithm::AdaptiveFM;
    Algorithm pendingAlgorithm = Algorithm::AdaptiveFM;

    // State of the filter-based algorithms, one per channel.
    struct FilterState
    {
        float allpass   = 0.f; // w(n-1), the allpass delay element
        float feedback  = 0.f; // y(n-1) of the feedback AM loop
        float dcIn      = 0.f; // DC blocker, x(n-1)
        float dcOut     = 0.f; // DC blocker, y(n-1)
    };

    // Per-channel state.
    std::vector<FilterState> filterStates;
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
    float dcBlockerCoeff    = 0.f;         // pole of the DC blocker

    /** @brief Clears the audio held in the delay lines and filters. */
    void clearAudioState();

    /**
     * @brief Returns the peak allpass coefficient for a depth setting.
     * @param depth Depth in [0, 1].
     * @return Coefficient in [0, maxAllpassCoeff].
     */
    static float getAllpassCoeff(float depth);

    /**
     * @brief Returns the gain that evens out the level of the feedback AM loop.
     * @param beta Feedback amount, in [0, 1).
     * @return Inverse of the RMS gain of the loop for a slow modulator.
     */
    static float getFeedbackAMGain(float beta);

    /**
     * @brief Returns the peak-to-peak delay excursion at full depth.
     * @param f0 Fundamental frequency of the channel, in Hz (> 0).
     * @return Range in samples, for the current mode and range settings.
     */
    float getModRangeSamples(float f0) const;

    // Pitch assumed until a channel's detector produces its first estimate.
    static constexpr float initialPitchHz = PitchDetector::minFrequencyHz;

    // Cutoff of the DC blocker that follows the filter-based algorithms.
    static constexpr float dcBlockerCutoffHz = 5.f;
};

} // namespace helixx
