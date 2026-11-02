// SPDX-FileCopyrightText: 2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <vector>

namespace helixx
{

/**
 * @brief Single-channel circular delay line with a fractional, modulatable delay time.
 *
 * Samples are read with linear interpolation, so the delay time can be changed
 * every sample (e.g. by an LFO or audio-rate modulator) without stepping.
 * The internal buffer length is a power of 2 so that index wrapping is a bitmask.
 *
 * Real-time safe after construction: no allocations in the processing methods.
 */
class FractionalDelay
{
public:
    /**
     * @brief Creates a delay line able to hold at least @p length samples of delay.
     * @param length Maximum delay time, in samples, the line must support.
     *               Internally rounded up to the next power of 2.
     */
    explicit FractionalDelay(unsigned int length);
    ~FractionalDelay();

    /** @brief Zeroes the stored audio and rewinds the write position. */
    void clear();

    /**
     * @brief Sets the current delay time.
     * @param newTime Delay in samples (may be fractional). Must be in
     *                [0, getMaxDelayInSamples()]; out-of-range values are clamped.
     */
    void setTimeInSamples(float newTime);

    /** @brief Returns the largest delay time, in samples, accepted by setTimeInSamples(). */
    float getMaxDelayInSamples() const;

    /**
     * @brief Pushes one sample into the line and reads the delayed output.
     * @param sample Input sample.
     * @return The linearly interpolated sample written `delay time` samples ago
     *         (the input itself when the delay time is 0).
     */
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
