#include "FractionalDelay.h"
#include <cassert>
#include <cmath> // std::floor

namespace helixx
{

unsigned int FractionalDelay::nextPowerOfTwo(unsigned int value)
{
    if (value == 0)
        return 1;

    // If already a power of 2, keep it as-is.
    if ((value & (value - 1)) == 0)
        return value;

    unsigned int result = 1;
    while (result < value)
        result <<= 1;
    return result;
}

FractionalDelay::FractionalDelay(unsigned int length)
    : buffer(nextPowerOfTwo(length), 0.0f),
      bufferLength(nextPowerOfTwo(length)),
      bufferMask(bufferLength - 1)
{
}

FractionalDelay::~FractionalDelay() = default;

void FractionalDelay::setTimeInSamples(float newTime)
{
    assert(newTime >= 0.0f && newTime <= static_cast<float>(bufferLength - 1));
    delayTimeSamples = newTime;
}

float FractionalDelay::processSample(float sample)
{
    // Write the new sample into the line first.
    buffer[writeIndex] = sample;

    // Compute the (possibly fractional) read position, `delayTimeSamples`
    // behind the write pointer, wrapped into the buffer.
    float readPos = static_cast<float>(writeIndex) - delayTimeSamples;
    while (readPos < 0.0f)
        readPos += static_cast<float>(bufferLength);

    const unsigned int index0 = static_cast<unsigned int>(readPos) & bufferMask;
    const unsigned int index1 = (index0 + 1) & bufferMask;
    const float frac = readPos - std::floor(readPos);

    // Linear interpolation between the two nearest samples.
    const float delayedSample = buffer[index0] + frac * (buffer[index1] - buffer[index0]);

    // Advance the write pointer, wrapping around the buffer.
    writeIndex = (writeIndex + 1) & bufferMask;

    return delayedSample;
}

} // namespace helixx