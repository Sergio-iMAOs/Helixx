#include <array>
#include <cmath>

namespace helixx
{

enum class Waveform { Sine, Triangle, Saw, Square };

// Shared, read-only lookup tables — one instance for the whole program.
class WaveTables
{
public:
    static constexpr unsigned int TABLE_SIZE = 256; // power of 2

    static const WaveTables& instance()
    {
        static WaveTables tables; // thread-safe, lazy, initialized exactly once
        return tables;
    }

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
        for (unsigned int i = 0; i < TABLE_SIZE; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(TABLE_SIZE); // 0..1

            sineTable[i] = std::sin(2.0f * static_cast<float>(M_PI) * t);

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

// Per-instance oscillator state; tables are shared, only phase/freq are per-instance.
class Oscillator
{
public:
    Oscillator() = default;
    ~Oscillator() = default;

    void prepare(double sampleRate);

    // Sets phaseIncr
    void setFrequency(float newFrequency);

    void setWaveform(Waveform newWaveform);

    // Get current sample and step phase
    float processSample();
    
private:
    float samplePeriod = 0.0f;
    float phase = 0.0f;
    float phaseIncr = 0.0f;
    Waveform waveform = Waveform::Sine;
};

} // namespace helixx