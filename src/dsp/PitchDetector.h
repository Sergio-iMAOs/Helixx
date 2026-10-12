#pragma once

#include <vector>

namespace helixx
{

/**
 * @brief Streaming monophonic fundamental-frequency (f0) estimator based on YIN.
 *
 * Audio is pushed in blocks of any size through process(). Internally the
 * signal is decimated to roughly #targetAnalysisRateHz and collected in a
 * sliding analysis window whose length depends only on #minFrequencyHz, so the
 * detectable pitch range and the CPU cost do not depend on the host block size
 * or on any oversampling applied upstream. A new estimate is produced every
 * #hopSeconds.
 *
 * Real-time safe after prepare(): process() performs no allocations.
 *
 * Reference: A. de Cheveigné and H. Kawahara, "YIN, a fundamental frequency
 * estimator for speech and music", JASA 111(4), 2002.
 */
class PitchDetector
{
public:
    /** @brief Lowest detectable pitch, in Hz. Sets the analysis window length. */
    static constexpr float minFrequencyHz = 60.0f;

    /** @brief Highest detectable pitch, in Hz. */
    static constexpr float maxFrequencyHz = 1500.0f;

    PitchDetector();
    ~PitchDetector();

    /**
     * @brief Allocates the analysis buffers and resets the detector.
     * @param sampleRate Rate, in Hz, of the audio that will be passed to process().
     */
    void prepare(double sampleRate);

    /** @brief Discards buffered audio and the current estimate. */
    void reset();

    /**
     * @brief Feeds a block of mono audio into the detector.
     * @param samples    Input samples.
     * @param numSamples Number of samples in @p samples; any block size is accepted.
     * @return The most recent estimate, see getPitch().
     */
    float process(float const* samples, unsigned int numSamples);

    /**
     * @brief Returns the most recent fundamental-frequency estimate.
     * @return Frequency in Hz within [minFrequencyHz, maxFrequencyHz], or 0.0f
     *         if the latest analysis window had no clear pitch (silence, noise,
     *         or not enough audio received yet).
     */
    float getPitch() const;

private:
    /**
     * @brief Runs YIN on the current analysis window.
     * @return Estimated frequency in Hz, or 0.0f if unvoiced.
     */
    float analyse();

    /**
     * @brief Fills yinBuffer with the YIN difference function of the window:
     *        d(tau) = sum_{j=0}^{W-1} (x[j] - x[j+tau])^2, with W = integrationLength.
     */
    void computeDifference();

    /**
     * @brief Converts yinBuffer in place from the difference function to the
     *        cumulative mean normalized difference function (CMNDF).
     */
    void computeCumulativeMeanNormalizedDifference();

    /**
     * @brief Finds the smallest lag whose CMNDF dips below #threshold, then
     *        walks to the bottom of that dip.
     * @return The lag in analysis samples, or -1 if none is found (unvoiced).
     */
    int findAbsoluteThresholdTau() const;

    /**
     * @brief Refines an integer lag to sub-sample precision by fitting a
     *        parabola through it and its two neighbours in yinBuffer.
     * @param tauEstimate Integer lag returned by findAbsoluteThresholdTau().
     * @return The refined lag, within one sample of @p tauEstimate.
     */
    float parabolicInterpolation(int tauEstimate) const;

    float analysisRate = 44100.0f;         // sample rate after decimation, Hz

    // Decimation (boxcar average of decimationFactor input samples).
    unsigned int decimationFactor = 1;
    unsigned int decimationCount = 0;
    float decimationSum = 0.0f;

    // Sliding analysis window, at the analysis rate.
    std::vector<float> window;
    unsigned int fillCount = 0;            // valid samples currently in window
    unsigned int hopSize = 1;              // new samples between two analyses
    unsigned int integrationLength = 0;    // W in the difference function

    std::vector<float> yinBuffer;          // difference function / CMNDF, indexed by tau

    // Lag search range, in analysis samples, derived from the pitch range.
    unsigned int minTau = 2;
    unsigned int maxTau = 0;

    float pitch = 0.0f;                    // latest estimate, 0 = unvoiced

    static constexpr float threshold = 0.15f;               // YIN absolute threshold
    static constexpr double targetAnalysisRateHz = 24000.0; // decimate down to about this rate
    static constexpr double hopSeconds = 0.01;              // time between two estimates
};

} // namespace helixx
