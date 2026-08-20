#pragma once

#include <vector>

namespace helixx
{

#pragma once

#include <vector>

class FractionalDelay
{
public:
    // length: maximum delay time in samples the line must support.
    // Internally rounded up to the next power of 2 so index wrapping
    // can use a bitmask instead of modulo.
    explicit FractionalDelay(unsigned int length);
    ~FractionalDelay();

    // Sets the current delay time, in samples (can be fractional).
    // Must be in [0, bufferLength - 1].
    void setTimeInSamples(float newTime);

    // Introduces `sample` into the delay line and returns the
    // interpolated sample read from `delayTimeSamples` samples ago.
    float processSample(float sample);

private:
    static unsigned int nextPowerOfTwo(unsigned int value);

    std::vector<float> buffer;       // circular delay-line buffer, size is a power of 2
    unsigned int bufferLength;       // buffer.size(), always a power of 2
    unsigned int bufferMask;         // bufferLength - 1, used for fast index wrapping
    unsigned int writeIndex = 0;     // next write position

    float delayTimeSamples = 0.0f;   // current delay time, in samples (may be fractional)
};

} // namespace helixx