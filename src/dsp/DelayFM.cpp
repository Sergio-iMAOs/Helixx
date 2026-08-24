#include "DelayFM.h"

namespace helixx
{

DelayFM::DelayFM()
{
}

DelayFM::~DelayFM()
{
}

void DelayFM::prepare(double sampleRate, unsigned int samplesPerBlock, unsigned int numChannels)
{
    assert(sampleRate > 0.0);
    assert(samplesPerBlock > 0);
    assert(numChannels > 0);

    delayLines      .clear();
    modulators      .clear();
    pitchDetectors  .clear();

    for (auto chIdx = 0; chIdx < numChannels; ++chIdx)
    {
        delayLines.push_back(FractionalDelay(MAX_LENGTH));

        modulators.push_back(Oscillator());
        modulators.back().prepare(sampleRate);

        pitchDetectors.push_back(PitchDetector());
        pitchDetectors.back().prepare(sampleRate, samplesPerBlock);
    }
    pitches         .resize(numChannels, 55.f);
    previousPitches .resize(numChannels, 55.f);
    modRangeSamples .resize(numChannels, 0.f);

    blockSizeInv = 1.f / samplesPerBlock;
    this->sampleRate = static_cast<float>(sampleRate);
    samplePeriod = 1.f / sampleRate;
}

void DelayFM::process(float* const* channelPtrs, unsigned int numSamples)
{
    // Get pitches
    for (size_t chIdx = 0; chIdx < pitches.size(); ++chIdx)
    {
        auto pitch = pitchDetectors[chIdx].getPitch(channelPtrs[chIdx]);
        pitch = pitch == 0.f ? previousPitches[chIdx] : pitch;
        pitches[chIdx] = pitch;
        modRangeSamples[chIdx] = getModRangeSamples(pitch);
    }

    for (size_t frameIdx = 0; frameIdx < numSamples; ++frameIdx)
    {
        // Ramp parameters
        auto workCoarseFreq = coarseFreqPrev * (1.f - frameIdx * blockSizeInv) + coarseFreq * (frameIdx * blockSizeInv);
        auto workDepth = depthPrev * (1.f - frameIdx * blockSizeInv) + depth * (frameIdx * blockSizeInv);

        // Set delay times
        for (size_t chIdx = 0; chIdx < delayLines.size(); ++chIdx)
        {   
            // Apply pitch ramp
            auto pitch = previousPitches[chIdx] * (1.f - frameIdx * blockSizeInv) + pitches[chIdx] * (frameIdx * blockSizeInv);

            // Set modulator frequency
            modulators[chIdx].setFrequency(pitch * workCoarseFreq);

            // Calulate delay time
            auto baseDelay = modRangeSamples[chIdx] * 0.5f * workDepth; // Half mod range to keep original freq centered
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
    for (size_t chIdx = 0; chIdx < pitches.size(); ++chIdx)
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

void DelayFM::setModRangeRatio(float value)
{
    assert(value >= 1.0f && value <= 4.0f); // matches the parameter's NormalisableRange [0, 24] st
    modRangeRatio = value;
    modRangeFactor = 1.0f - 1.0f / modRangeRatio; // division done once, here, not per-sample
}

void DelayFM::setRelativeToPitch(bool isRelative)
{
    isRelativeToPitch = isRelative;
}

float DelayFM::getModRangeSamples(float f0)
{
    if (isRelativeToPitch)
    {
        return (sampleRate / f0) * modRangeFactor;
    }
    else
    {
        return modRangeMs * 0.001f * sampleRate;
    } 
}

} // namespace helixx