#pragma once

#include <vector>

namespace helixx
{

class PitchDetector
{
public:
    PitchDetector();
    ~PitchDetector();

    void prepare(double sampleRate, unsigned int samplesPerBlock);

    // Returns fundamental frequency estimation in Hz (0.0f if no pitch could
    // be confidently detected). Real-time safe: performs no allocations.
    // Expects `samplesPerBlock` samples in `buffer` (mono).
    float getPitch(float const* buffer);

private:
    // Fills yinBuffer[0..halfBufferSize-1] with the YIN difference function
    // for the given input block.
    void computeDifference(float const* buffer);

    // Converts yinBuffer in-place from the difference function to the
    // cumulative mean normalized difference function (CMNDF).
    void computeCumulativeMeanNormalizedDifference();

    // Finds the smallest tau whose CMNDF value dips below threshold and is
    // a local minimum. Returns -1 if none is found (unvoiced/no pitch).
    int findAbsoluteThresholdTau() const;

    // Refines an integer-tau estimate to sub-sample precision via parabolic
    // interpolation of its neighbors in yinBuffer.
    float parabolicInterpolation(int tauEstimate) const;

    double sampleRate = 44100.0;
    unsigned int blockSize = 0;
    unsigned int halfBufferSize = 0; // max lag searched = blockSize / 2

    std::vector<float> yinBuffer; // difference function / CMNDF, size == halfBufferSize

    // Search range in samples of lag (tau), derived from a plausible pitch
    // range to avoid wasted computation and reduce octave errors.
    unsigned int minTau = 2;
    unsigned int maxTau = 0;

    static constexpr float threshold = 0.15f; // YIN absolute threshold
    static constexpr float minFrequencyHz = 60.0f;   // lowest detectable pitch
    static constexpr float maxFrequencyHz = 1500.0f; // highest detectable pitch
};

} // namespace helixx