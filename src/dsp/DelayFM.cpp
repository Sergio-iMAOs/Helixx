#include "DelayFM.h"

#include <algorithm>
#include <cmath>

namespace helixx
{

DelayFM::DelayFM()
{
}

DelayFM::~DelayFM()
{
}

void DelayFM::prepare(double newSampleRate, unsigned int numChannels)
{
    assert(newSampleRate > 0.0);

    sampleRate = static_cast<float>(newSampleRate);

    // Longest delay either mode can ask for: the full absolute range, or
    // (almost) one period of the lowest pitch a channel can hold.
    const auto maxDelaySeconds = std::max(maxModRangeMs * 0.001f, 1.f / initialPitchHz);
    const auto delayLength = static_cast<unsigned int>(std::ceil(maxDelaySeconds * sampleRate));

    delayLines      .clear();
    modulators      .clear();
    pitchDetectors  .clear();

    for (unsigned int chIdx = 0; chIdx < numChannels; ++chIdx)
    {
        delayLines.emplace_back(delayLength);

        modulators.emplace_back();
        modulators.back().prepare(newSampleRate);

        pitchDetectors.emplace_back();
        pitchDetectors.back().prepare(newSampleRate);
    }
    pitches         .assign(numChannels, initialPitchHz);
    modRangeSamples .assign(numChannels, 0.f);
    filterStates    .assign(numChannels, FilterState());

    maxDelaySamples = static_cast<float>(delayLength);
    dcBlockerCoeff  = 1.f - 6.283185307179586f * dcBlockerCutoffHz / sampleRate;

    algorithm = pendingAlgorithm;

    // Start from the settled parameter values rather than ramping to them.
    coarseFreq = coarseFreqTarget;
    depth = depthTarget;
}

void DelayFM::process(float* const* channelPtrs, unsigned int numChannels, unsigned int numSamples)
{
    const auto numActiveChannels = std::min(static_cast<size_t>(numChannels), delayLines.size());
    if (numSamples == 0 || numActiveChannels == 0)
        return;

    // Every smoothed value moves linearly from where the previous block left
    // it to its new target, reaching it on the last sample of this block.
    const auto rampStep       = 1.f / static_cast<float>(numSamples);
    const auto coarseFreqIncr = (coarseFreqTarget - coarseFreq) * rampStep;
    const auto depthIncr      = (depthTarget - depth) * rampStep;

    if (algorithm != pendingAlgorithm)
    {
        // Don't let the new algorithm start from audio left by an earlier one
        clearAudioState();
        algorithm = pendingAlgorithm;
    }

    // Peak allpass coefficient, ramped along with the depth
    const auto allpassCoeff     = getAllpassCoeff(depth);
    const auto allpassCoeffIncr = (getAllpassCoeff(depthTarget) - allpassCoeff) * rampStep;

    // Level compensation of the feedback AM loop, ramped along with the depth
    const auto feedbackGain     = getFeedbackAMGain(depth * maxFeedback);
    const auto feedbackGainIncr = (getFeedbackAMGain(depthTarget * maxFeedback) - feedbackGain) * rampStep;

    for (size_t chIdx = 0; chIdx < numActiveChannels; ++chIdx)
    {
        auto* samples    = channelPtrs[chIdx];
        auto& delayLine  = delayLines[chIdx];
        auto& modulator  = modulators[chIdx];
        auto& filter     = filterStates[chIdx];

        // Get pitch from the dry input; hold the last one while unvoiced
        const auto detectedPitch = pitchDetectors[chIdx].process(samples, numSamples);
        const auto targetPitch   = detectedPitch > 0.f ? detectedPitch : pitches[chIdx];
        const auto targetRange   = getModRangeSamples(targetPitch);

        const auto pitchIncr = (targetPitch - pitches[chIdx]) * rampStep;
        const auto rangeIncr = (targetRange - modRangeSamples[chIdx]) * rampStep;

        auto workPitch      = pitches[chIdx];
        auto workRange      = modRangeSamples[chIdx];
        auto workCoarseFreq = coarseFreq;
        auto workDepth      = depth;
        auto workAllpassCoeff = allpassCoeff;
        auto workFeedbackGain = feedbackGain;

        for (unsigned int frameIdx = 0; frameIdx < numSamples; ++frameIdx)
        {
            // Ramp parameters
            workPitch      += pitchIncr;
            workRange      += rangeIncr;
            workCoarseFreq += coarseFreqIncr;
            workDepth      += depthIncr;
            workAllpassCoeff += allpassCoeffIncr;
            workFeedbackGain += feedbackGainIncr;

            // Set modulator frequency
            modulator.setFrequency(workPitch * workCoarseFreq);
            const auto modSample = modulator.processSample();

            const auto input = samples[frameIdx];
            auto output = input;

            switch (algorithm)
            {
                case Algorithm::AdaptiveFM:
                {
                    // Calulate delay time
                    const auto baseDelay = workRange * 0.5f * workDepth; // Half mod range to keep original freq centered
                    const auto delayTime = baseDelay + modSample * baseDelay;

                    // Process delay line
                    delayLine.setTimeInSamples(std::clamp(delayTime, 0.f, maxDelaySamples));
                    output = delayLine.processSample(input);
                    break;
                }
                case Algorithm::Allpass:
                {
                    // y(n) = -m(n) x(n) + w(n-1),  w(n) = x(n) + m(n) y(n)
                    const auto coeff = modSample * workAllpassCoeff;
                    output = filter.allpass - coeff * input;
                    filter.allpass = input + coeff * output;
                    break;
                }
                case Algorithm::FeedbackAM:
                {
                    // y(n) = x(n) + beta m(n) y(n-1)
                    const auto beta = workDepth * maxFeedback;
                    filter.feedback = input + beta * modSample * filter.feedback;
                    output = filter.feedback * workFeedbackGain;
                    break;
                }
            }

            if (algorithm != Algorithm::AdaptiveFM)
            {
                // Modulating a filter coefficient at a multiple of the pitch
                // rectifies part of the signal: remove the resulting DC
                const auto blocked = output - filter.dcIn + dcBlockerCoeff * filter.dcOut;
                filter.dcIn  = output;
                filter.dcOut = blocked;
                output = blocked;
            }

            samples[frameIdx] = output;
        }

        // Land exactly on the targets so rounding errors don't accumulate
        pitches[chIdx]         = targetPitch;
        modRangeSamples[chIdx] = targetRange;
    }

    coarseFreq = coarseFreqTarget;
    depth      = depthTarget;
}

void DelayFM::setAlgorithm(Algorithm newAlgorithm)
{
    pendingAlgorithm = newAlgorithm;
}

void DelayFM::setCoarseFreq(float newFreq)
{
    assert(newFreq > 0.f);
    coarseFreqTarget = newFreq;
}

void DelayFM::setDepth(float newDepth)
{
    assert(newDepth >= 0.f && newDepth <= 1.f);
    depthTarget = std::clamp(newDepth, 0.f, 1.f);
}

void DelayFM::setModRangeMs(float ms)
{
    assert(ms >= 0.f && ms <= maxModRangeMs);
    modRangeMs = std::clamp(ms, 0.f, maxModRangeMs);
}

void DelayFM::setModRangeRatio(float ratio)
{
    assert(ratio >= 1.0f && ratio <= maxModRangeRatio);
    ratio = std::clamp(ratio, 1.0f, maxModRangeRatio);
    modRangeFactor = 1.0f - 1.0f / ratio; // division done once, here, not per-sample
}

void DelayFM::setRelativeToPitch(bool isRelative)
{
    isRelativeToPitch = isRelative;
}

void DelayFM::clearAudioState()
{
    for (auto& delayLine : delayLines)
        delayLine.clear();

    std::fill(filterStates.begin(), filterStates.end(), FilterState());
}

float DelayFM::getAllpassCoeff(float depth)
{
    // The delay of the filter is (1 - m) / (1 + m): nearly all of its range
    // is packed next to |m| = 1. Approaching 1 geometrically spreads it
    // evenly over the depth control.
    return 1.f - std::pow(1.f - maxAllpassCoeff, depth);
}

float DelayFM::getFeedbackAMGain(float beta)
{
    // For a modulator much slower than the sample rate the loop settles to
    // x / (1 - beta m); averaging the squared gain over one cycle of m gives
    // (1 - beta^2)^(-3/2).
    return std::pow(1.f - beta * beta, 0.75f);
}

float DelayFM::getModRangeSamples(float f0) const
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
