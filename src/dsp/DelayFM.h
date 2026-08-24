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

    void prepare(double sampleRate, unsigned int samplesPerBlock, unsigned int numChannels);
    void process(float* const* channelPtrs, unsigned int numSamples);

    void setCoarseFreq      (float newFreq);
    void setDepth           (float newDepth);
    void setModRangeMs      (float ms);
    void setModRangeRatio   (float st);
    void setRelativeToPitch (bool isRelative);

private:
    float coarseFreq        = 1.f;
    float depth             = 0.f;
    float coarseFreqPrev    = 1.f;
    float depthPrev         = 0.f;
    std::vector<FractionalDelay> delayLines;
    std::vector<float> pitches;
    std::vector<float> previousPitches;
    std::vector<float> modRangeSamples;
    std::vector<Oscillator> modulators;
    std::vector<PitchDetector> pitchDetectors;
    bool isRelativeToPitch;
    float modRangeMs;
    float modRangeRatio;
    float modRangeFactor;

    float sampleRate;
    float blockSizeInv;
    float samplePeriod;

    float getModRangeSamples(float f0);

    static constexpr unsigned int MAX_LENGTH = 4096;
};

} // namespace helixx