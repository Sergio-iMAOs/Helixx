#include "DelayFM.h"

namespace helixx
{

DelayFM::DelayFM()
{
}

DelayFM::~DelayFM()
{
}

void DelayFM::prepare(double sampleRate, int samplesPerBlock, unsigned int numChannels)
{
    assert(sampleRate > 0.0);
    assert(samplesPerBlock > 0);
    assert(numChannels > 0);

    delayLines.clear();
    modulators.clear();
    pitchDetectors.clear();

    for (auto chIdx = 0; chIdx < numChannels; ++chIdx)
    {
        delayLines.push_back(FractionalDelay(MAX_LENGTH));

        modulators.push_back(Oscillator());
        modulators.back().prepare(sampleRate);

        pitchDetectors.push_back(PitchDetector());
        pitchDetectors.back().prepare(sampleRate, samplesPerBlock);
    }
    pitches          .resize(numChannels, 0.f);
    previousPitches  .resize(numChannels, 0.f);

    blockSizeInv = 1.f / samplesPerBlock;
    this->sampleRate = static_cast<float>(sampleRate);
    samplePeriod = 1.f / sampleRate;
}

void DelayFM::process(float* const* channelPtrs, unsigned int numSamples)
{
    // Get pitches
    for (auto chIdx = 0; chIdx < pitches.size(); ++chIdx)
    {
        auto pitch = pitchDetectors[chIdx].getPitch(channelPtrs[chIdx]);
        pitches[chIdx] = pitch == 0 ? previousPitches[chIdx] : pitch;
    }

    for (auto frameIdx = 0; frameIdx < numSamples; ++frameIdx)
    {
        // Ramp parameters
        auto workCoarseFreq = coarseFreqPrev * (1.f - frameIdx * blockSizeInv) + coarseFreq * (frameIdx * blockSizeInv);
        auto workDepth = depthPrev * (1.f - frameIdx * blockSizeInv) + depth * (frameIdx * blockSizeInv);

        // Set delay times
        for (auto chIdx = 0; chIdx < delayLines.size(); ++chIdx)
        {   
            // Apply pitch ramp
            auto pitch = previousPitches[chIdx] * (1.f - frameIdx * blockSizeInv) + pitches[chIdx] * (frameIdx * blockSizeInv);

            // Set modulator frequency
            modulators[chIdx].setFrequency(pitch * workCoarseFreq);

            // Calulate delay time
            auto modRangeSamples = getModRangeSamples(pitch);
            auto baseDelay = modRangeSamples * 0.5f * workDepth; // Half mod range to keep original freq centered
            auto delayTime = baseDelay + modulators[chIdx].processSample() * baseDelay;
            
            // Process delay line
            auto& delayLine = delayLines[chIdx];
            auto sample = delayLine.processSample(channelPtrs[chIdx][frameIdx]);
            delayLine.setTimeInSamples(delayTime);

            // Set output
            channelPtrs[chIdx][frameIdx] = sample;
        }
    }

    // Update previous pitches
    for (auto chIdx = 0; chIdx < pitches.size(); ++chIdx)
    {
        previousPitches[chIdx] = pitches[chIdx];
    }
}

void DelayFM::setCoarseFreq(float newFreq)
{
    assert(newFreq > 0.f);
    coarseFreqPrev = coarseFreq;
    coarseFreq = newFreq;
}

void DelayFM::setDepth(float newDepth)
{
    assert(newDepth >= 0.f);
    depthPrev = depth;
    depth = newDepth;
}

void DelayFM::setModRangeMs(float ms)
{
    assert(ms >= 0.f);
    assert(ms * 0.001f < MAX_LENGTH * samplePeriod);
    modRangeMs = ms;
}

void DelayFM::setModRangeRel(float value)
{
    assert(value >= 0);
    assert(value < MAX_LENGTH);
    modRangeRel = value * samplePeriod;
}

void DelayFM::setRelativeToPitch(bool isRelative)
{
    isRelativeToPitch = isRelative;
}

float DelayFM::getModRangeSamples(float f0)
{
    if (isRelativeToPitch)
    {
        return f0 * modRangeRel;
    }
    else
    {
        return modRangeMs * 0.001f * sampleRate;
    } 
}

} // namespace helixx