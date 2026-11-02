// SPDX-FileCopyrightText: 2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "FractionalDelay.h"
#include <algorithm>
#include <cassert>

namespace helixx
{

unsigned int FractionalDelay::nextPowerOfTwo(unsigned int value)
{
    unsigned int result = 1;
    while (result < value)
        result <<= 1;
    return result;
}

FractionalDelay::FractionalDelay(unsigned int length)
    // Two extra samples: one because the newest sample is written before
    // reading, one for the interpolation neighbour at the maximum delay.
    : buffer(nextPowerOfTwo(length + 2), 0.0f),
      bufferLength(static_cast<unsigned int>(buffer.size())),
      bufferMask(bufferLength - 1)
{
}

FractionalDelay::~FractionalDelay() = default;

void FractionalDelay::clear()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}

void FractionalDelay::setTimeInSamples(float newTime)
{
    assert(newTime >= 0.0f && newTime <= getMaxDelayInSamples());
    delayTimeSamples = std::clamp(newTime, 0.0f, getMaxDelayInSamples());
}

float FractionalDelay::getMaxDelayInSamples() const
{
    return static_cast<float>(bufferLength - 2);
}

float FractionalDelay::processSample(float sample)
{
    // Write the new sample into the line first.
    buffer[writeIndex] = sample;

    // Split the delay into integer and fractional parts. Working on the delay
    // (rather than on an absolute float read position) keeps the fractional
    // precision independent of the buffer length.
    const unsigned int delayInt = static_cast<unsigned int>(delayTimeSamples);
    const float frac = delayTimeSamples - static_cast<float>(delayInt);

    // Unsigned wrap-around is fine here because bufferLength is a power of 2.
    const unsigned int index0 = (writeIndex - delayInt) & bufferMask; // delayInt samples ago
    const unsigned int index1 = (index0 - 1) & bufferMask;            // one sample older

    // Linear interpolation between the two nearest samples.
    const float delayedSample = buffer[index0] + frac * (buffer[index1] - buffer[index0]);

    // Advance the write pointer, wrapping around the buffer.
    writeIndex = (writeIndex + 1) & bufferMask;

    return delayedSample;
}

} // namespace helixx
