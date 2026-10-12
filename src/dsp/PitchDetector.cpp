#include "PitchDetector.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

namespace helixx
{

PitchDetector::PitchDetector() = default;
PitchDetector::~PitchDetector() = default;

void PitchDetector::prepare(double sampleRate)
{
    assert(sampleRate > 0.0);

    decimationFactor = std::max(1u, static_cast<unsigned int>(std::lround(sampleRate / targetAnalysisRateHz)));
    analysisRate = static_cast<float>(sampleRate / decimationFactor);

    // Convert the frequency range of interest into a tau (lag) search range.
    // Higher frequency -> smaller tau, lower frequency -> larger tau.
    minTau = std::max(2u, static_cast<unsigned int>(std::floor(analysisRate / maxFrequencyHz)));
    maxTau = std::max(minTau + 1, static_cast<unsigned int>(std::ceil(analysisRate / minFrequencyHz)));

    // Integrate over one period of the lowest pitch. The window must also hold
    // the lagged samples up to maxTau + 1 (the +1 is the interpolation neighbour).
    integrationLength = maxTau;
    window.assign(integrationLength + maxTau + 1, 0.0f);
    yinBuffer.assign(maxTau + 2, 0.0f);

    hopSize = std::clamp(static_cast<unsigned int>(analysisRate * hopSeconds),
                         1u, static_cast<unsigned int>(window.size()));

    reset();
}

void PitchDetector::reset()
{
    std::fill(window.begin(), window.end(), 0.0f);
    fillCount = 0;
    decimationCount = 0;
    decimationSum = 0.0f;
    pitch = 0.0f;
}

float PitchDetector::process(float const* samples, unsigned int numSamples)
{
    if (window.empty())
        return 0.0f; // not prepared

    const auto windowSize = static_cast<unsigned int>(window.size());
    const float decimationGain = 1.0f / static_cast<float>(decimationFactor);

    for (unsigned int i = 0; i < numSamples; ++i)
    {
        decimationSum += samples[i];
        if (++decimationCount < decimationFactor)
            continue;

        window[fillCount++] = decimationSum * decimationGain;
        decimationSum = 0.0f;
        decimationCount = 0;

        if (fillCount == windowSize)
        {
            pitch = analyse();

            // Slide the window forward by one hop.
            std::copy(window.begin() + hopSize, window.end(), window.begin());
            fillCount -= hopSize;
        }
    }

    return pitch;
}

float PitchDetector::getPitch() const
{
    return pitch;
}

float PitchDetector::analyse()
{
    computeDifference();
    computeCumulativeMeanNormalizedDifference();

    const int tau = findAbsoluteThresholdTau();
    if (tau < 0)
        return 0.0f; // no pitch detected

    const float frequency = analysisRate / parabolicInterpolation(tau);
    if (! (frequency >= minFrequencyHz && frequency <= maxFrequencyHz))
        return 0.0f; // out of range (or NaN from a non-finite input)

    return frequency;
}

void PitchDetector::computeDifference()
{
    // d(tau) = sum_{j=0}^{W-1} (x[j] - x[j+tau])^2
    const float* x = window.data();

    for (unsigned int tau = 0; tau <= maxTau + 1; ++tau)
    {
        float sum = 0.0f;
        for (unsigned int j = 0; j < integrationLength; ++j)
        {
            const float delta = x[j] - x[j + tau];
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
    for (unsigned int tau = 1; tau <= maxTau + 1; ++tau)
    {
        runningSum += yinBuffer[tau];

        if (runningSum <= std::numeric_limits<float>::min())
            yinBuffer[tau] = 1.0f; // avoid divide-by-zero on silence
        else
            yinBuffer[tau] *= static_cast<float>(tau) / runningSum;
    }
}

int PitchDetector::findAbsoluteThresholdTau() const
{
    for (unsigned int tau = minTau; tau <= maxTau; ++tau)
    {
        if (yinBuffer[tau] < threshold)
        {
            // Walk forward while the CMNDF keeps decreasing, to land on the
            // local minimum rather than the first sample under threshold.
            unsigned int localMinTau = tau;
            while (localMinTau + 1 <= maxTau &&
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
    const auto tau = static_cast<unsigned int>(tauEstimate);

    // tau is in [minTau, maxTau] with minTau >= 2 and yinBuffer holding
    // maxTau + 2 values, so both neighbours are always available.
    const float s0 = yinBuffer[tau - 1];
    const float s1 = yinBuffer[tau];
    const float s2 = yinBuffer[tau + 1];

    const float denom = 2.0f * (2.0f * s1 - s2 - s0);
    if (std::abs(denom) < std::numeric_limits<float>::epsilon())
        return static_cast<float>(tau);

    // The vertex lies within half a sample of a true local minimum; clamp in
    // case tau sits on the edge of the search range and is not one.
    const float delta = std::clamp((s2 - s0) / denom, -1.0f, 1.0f);
    return static_cast<float>(tau) + delta;
}

} // namespace helixx
