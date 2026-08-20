#pragma once

#include <vector>
#include <cassert>

#include "FractionalDelay.h"
#include "Oscillator.h"
#include "PitchDetector.h"

namespace helixx
{

class DelayFM
{
public:
    DelayFM();
    ~DelayFM();

    void prepare(double sampleRate, int samplesPerBlock, unsigned int numChannels);
    void process(float* const* channelPtrs, unsigned int numSamples);

    void setCoarseFreq(float newFreq);
    void setDepth(float newDepth);

private:
    float coarseFreq;
    float depth;
    float coarseFreqPrev;
    float depthPrev;
    std::vector<FractionalDelay> delayLines;
    std::vector<float> pitches;
    std::vector<float> previousPitches;
    std::vector<Oscillator> modulators;
    std::vector<PitchDetector> pitchDetectors;
    bool isRelativeToPitch = false;
    float modRangeMs = 20.f;
    float modRangeSemitones = 24.f;

    float sampleRate;
    float blockSizeInv;

    float getModRangeSamples(float f0);

    static constexpr unsigned int MAX_LENGTH = 4096;
};

} // namespace helixx