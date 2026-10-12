#include "Oscillator.h"
#include <cassert>
#include <cmath>

namespace helixx
{
    void Oscillator::prepare(double sampleRate)
    {
        assert(sampleRate > 0.0);
        samplePeriod = static_cast<float>(1.0 / sampleRate);
        WaveTables::instance(); // build the shared tables now, not on the audio thread
        reset();
    }

    void Oscillator::reset()
    {
        phase = 0.0f;
    }

    // Sets phaseIncr
    void Oscillator::setFrequency(float newFrequency)
    {
        assert(std::isfinite(newFrequency));
        phaseIncr = newFrequency * samplePeriod * static_cast<float>(WaveTables::TABLE_SIZE);
    }

    void Oscillator::setWaveform(Waveform newWaveform)
    {
        waveform = newWaveform;
    }

    // Get current sample and step phase
    float Oscillator::processSample()
    {
        constexpr auto tableSize = static_cast<float>(WaveTables::TABLE_SIZE);
        constexpr unsigned int tableMask = WaveTables::TABLE_SIZE - 1;

        const auto& table = WaveTables::instance().getTable(waveform);

        // Masking index0 as well guards against the phase rounding up to
        // exactly TABLE_SIZE after wrapping.
        const unsigned int phaseInt = static_cast<unsigned int>(phase);
        const unsigned int index0 = phaseInt & tableMask;
        const unsigned int index1 = (index0 + 1) & tableMask;
        const float frac = phase - static_cast<float>(phaseInt);

        const float sample = table[index0] + frac * (table[index1] - table[index0]);

        phase += phaseIncr;
        if (phase >= tableSize || phase < 0.0f)
            phase -= tableSize * std::floor(phase / tableSize);

        return sample;
    }

} // namespace helixx
