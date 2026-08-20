#include "PitchDetector.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace helixx
{

PitchDetector::PitchDetector() = default;
PitchDetector::~PitchDetector() = default;

void PitchDetector::prepare(double newSampleRate, unsigned int samplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlock;
    halfBufferSize = blockSize / 2;

    yinBuffer.assign(halfBufferSize, 0.0f);

    // Convert the frequency range of interest into a tau (lag) search range.
    // Higher frequency -> smaller tau, lower frequency -> larger tau.
    const float maxTauF = static_cast<float>(sampleRate) / minFrequencyHz;
    const float minTauF = static_cast<float>(sampleRate) / maxFrequencyHz;

    minTau = std::max(2u, static_cast<unsigned int>(minTauF));
    maxTau = std::min(halfBufferSize > 0 ? halfBufferSize - 1 : 0,
                       static_cast<unsigned int>(maxTauF));
}

void PitchDetector::computeDifference(float const* buffer)
{
    // d(tau) = sum_{j=0}^{W-1} (x[j] - x[j+tau])^2
    for (unsigned int tau = 0; tau < halfBufferSize; ++tau)
    {
        float sum = 0.0f;
        for (unsigned int j = 0; j < halfBufferSize; ++j)
        {
            const float delta = buffer[j] - buffer[j + tau];
            sum += delta * delta;
        }
        yinBuffer[tau] = sum;
    }
}

void PitchDetector::computeCumulativeMeanNormalizedDifference()
{
    // d'(0) = 1 by definition.
    // d'(tau) = d(tau) / ( (1/tau) * sum_{j=1}^{tau} d(j) ), for tau >= 1.
    yinBuffer[0] = 1.0f;

    float runningSum = 0.0f;
    for (unsigned int tau = 1; tau < halfBufferSize; ++tau)
    {
        runningSum += yinBuffer[tau];

        if (runningSum <= 0.0f)
            yinBuffer[tau] = 1.0f; // avoid divide-by-zero on silence
        else
            yinBuffer[tau] *= static_cast<float>(tau) / runningSum;
    }
}

int PitchDetector::findAbsoluteThresholdTau() const
{
    const unsigned int searchEnd = std::min(maxTau, halfBufferSize - 1);

    for (unsigned int tau = minTau; tau <= searchEnd; ++tau)
    {
        if (yinBuffer[tau] < threshold)
        {
            // Walk forward while the CMNDF keeps decreasing, to land on the
            // local minimum rather than the first sample under threshold.
            unsigned int localMinTau = tau;
            while (localMinTau + 1 <= searchEnd &&
                   yinBuffer[localMinTau + 1] < yinBuffer[localMinTau])
            {
                ++localMinTau;
            }
            return static_cast<int>(localMinTau);
        }
    }

    return -1; // no tau found under threshold -> unvoiced / no clear pitch
}

float PitchDetector::parabolicInterpolation(int tauEstimate) const
{
    const int tau = tauEstimate;

    // Guard the neighbors used for interpolation; fall back to the raw
    // estimate at the buffer edges where a neighbor is unavailable.
    if (tau <= 0 || static_cast<unsigned int>(tau) >= halfBufferSize - 1)
        return static_cast<float>(tau);

    const float s0 = yinBuffer[tau - 1];
    const float s1 = yinBuffer[tau];
    const float s2 = yinBuffer[tau + 1];

    const float denom = 2.0f * (2.0f * s1 - s2 - s0);
    if (std::abs(denom) < std::numeric_limits<float>::epsilon())
        return static_cast<float>(tau);

    const float delta = (s2 - s0) / denom;
    return static_cast<float>(tau) + delta;
}

float PitchDetector::getPitch(float const* buffer)
{
    if (halfBufferSize == 0)
        return 0.0f;

    computeDifference(buffer);
    computeCumulativeMeanNormalizedDifference();

    const int tau = findAbsoluteThresholdTau();
    if (tau < 0)
        return 0.0f; // no pitch detected

    const float refinedTau = parabolicInterpolation(tau);
    if (refinedTau <= 0.0f)
        return 0.0f;

    return static_cast<float>(sampleRate) / refinedTau;
}

} // namespace helixx