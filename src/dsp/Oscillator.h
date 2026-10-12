#pragma once

#include <array>
#include <cmath>

namespace helixx
{

/** @brief Waveform shapes available to Oscillator. */
enum class Waveform { Sine, Triangle, Saw, Square };

/**
 * @brief Shared, read-only single-cycle lookup tables, one per Waveform.
 *
 * Singleton: one instance for the whole program, built lazily on first use.
 * Touch instance() once outside the audio thread (Oscillator::prepare() does)
 * so the tables are never built from a real-time context.
 *
 * The tables are naive (not band-limited), which is fine for sub-audio and
 * modulation use but will alias if used as an audible high-pitched source.
 */
class WaveTables
{
public:
    /** @brief Number of samples per table. Must be a power of 2. */
    static constexpr unsigned int TABLE_SIZE = 256;

    /** @brief Returns the program-wide table set (thread-safe initialisation). */
    static const WaveTables& instance()
    {
        static WaveTables tables; // thread-safe, lazy, initialized exactly once
        return tables;
    }

    /**
     * @brief Returns the single-cycle table for a waveform.
     * @param waveform Shape to look up.
     * @return TABLE_SIZE samples in [-1, 1] covering one period.
     */
    const std::array<float, TABLE_SIZE>& getTable(Waveform waveform) const
    {
        switch (waveform)
        {
            case Waveform::Triangle: return triangleTable;
            case Waveform::Saw:      return sawTable;
            case Waveform::Square:   return squareTable;
            case Waveform::Sine:
            default:                 return sineTable;
        }
    }

    WaveTables(const WaveTables&) = delete;
    WaveTables& operator=(const WaveTables&) = delete;

private:
    WaveTables()
    {
        constexpr float twoPi = 6.283185307179586f;

        for (unsigned int i = 0; i < TABLE_SIZE; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(TABLE_SIZE); // 0..1

            sineTable[i] = std::sin(twoPi * t);

            if (t < 0.25f)
                triangleTable[i] = 4.0f * t;
            else if (t < 0.75f)
                triangleTable[i] = 2.0f - 4.0f * t;
            else
                triangleTable[i] = 4.0f * t - 4.0f;

            sawTable[i] = 2.0f * t - 1.0f;

            squareTable[i] = (i < TABLE_SIZE / 2) ? 1.0f : -1.0f;
        }
    }

    std::array<float, TABLE_SIZE> sineTable{};
    std::array<float, TABLE_SIZE> triangleTable{};
    std::array<float, TABLE_SIZE> sawTable{};
    std::array<float, TABLE_SIZE> squareTable{};
};

/**
 * @brief Wavetable oscillator with linear interpolation.
 *
 * Holds only per-instance state (phase, increment, waveform); the tables are
 * shared through WaveTables. Output is in [-1, 1].
 */
class Oscillator
{
public:
    Oscillator() = default;
    ~Oscillator() = default;

    /**
     * @brief Sets the sample rate and resets the phase.
     * @param sampleRate Rate, in Hz, at which processSample() will be called.
     */
    void prepare(double sampleRate);

    /** @brief Rewinds the phase to the start of the cycle. */
    void reset();

    /**
     * @brief Sets the oscillation frequency.
     * @param newFrequency Frequency in Hz. Cheap enough to call every sample.
     */
    void setFrequency(float newFrequency);

    /** @brief Selects the waveform; takes effect on the next sample. */
    void setWaveform(Waveform newWaveform);

    /**
     * @brief Returns the current sample and advances the phase.
     * @return Output sample in [-1, 1].
     */
    float processSample();

private:
    float samplePeriod = 0.0f;
    float phase = 0.0f;         // in table samples, [0, TABLE_SIZE)
    float phaseIncr = 0.0f;     // table samples per output sample
    Waveform waveform = Waveform::Sine;
};

} // namespace helixx
