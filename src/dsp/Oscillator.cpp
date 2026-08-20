#include "Oscillator.h"
#include <cmath>

namespace helixx
{
    void Oscillator::prepare(double sampleRate)
    {
        samplePeriod = static_cast<float>(1.0 / sampleRate);
    }

    // Sets phaseIncr
    void Oscillator::setFrequency(float newFrequency)
    {
        phaseIncr = newFrequency * samplePeriod * static_cast<float>(WaveTables::TABLE_SIZE);
    }

    void Oscillator::setWaveform(Waveform newWaveform)
    {
        waveform = newWaveform;
    }

    // Get current sample and step phase
    float Oscillator::processSample()
    {
        const auto& table = WaveTables::instance().getTable(waveform);

        const unsigned int index0 = static_cast<unsigned int>(phase);
        const unsigned int index1 = (index0 + 1) & (WaveTables::TABLE_SIZE - 1); // bitmask wrap
        const float frac = phase - static_cast<float>(index0);

        const float sample = table[index0] + frac * (table[index1] - table[index0]);

        phase += phaseIncr;
        while (phase >= static_cast<float>(WaveTables::TABLE_SIZE))
            phase -= static_cast<float>(WaveTables::TABLE_SIZE);
        while (phase < 0.0f)
            phase += static_cast<float>(WaveTables::TABLE_SIZE);

        return sample;
    }

} // namespace helixx