// SPDX-FileCopyrightText: 2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "SpectrumScale.h"

#include <algorithm>
#include <cmath>

namespace helixx
{

namespace
{
    constexpr auto DECIBELS_PER_DECADE = 20.f;

    // The frequencies and the levels marked behind the curve
    struct MarkedFrequency { float hertz; const char* text; };
    constexpr MarkedFrequency MARKED_FREQUENCIES[] { { 100.f, "100" }, { 1000.f, "1k" }, { 10000.f, "10k" } };
    constexpr int MARKED_DECIBELS[] { 0, -24, -48, -72 };
    constexpr auto MULTIPLES_PER_DECADE = 10;

    // Notes: A4 is the 440 Hz one, middle C is C4
    constexpr auto A4_HERTZ         = 440.f;
    constexpr auto A4_NOTE          = 69;
    constexpr auto NOTES_PER_OCTAVE = 12;
    constexpr auto FIRST_OCTAVE     = -1;
    constexpr auto CENTS_PER_NOTE   = 100.f;
    constexpr const char* NOTE_NAMES[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    /** @brief A frequency as a note number, with its fraction. */
    float getNoteOf(float hertz)
    {
        return static_cast<float>(A4_NOTE) + static_cast<float>(NOTES_PER_OCTAVE) * std::log2(hertz / A4_HERTZ);
    }

    /** @brief A magnitude (1 is full level) in decibels, within the levels shown. */
    float toDecibels(float magnitude)
    {
        if (! (magnitude > 0.f))
            return SpectrumScale::minDecibels;

        return std::clamp(DECIBELS_PER_DECADE * std::log10(magnitude),
                          SpectrumScale::minDecibels, SpectrumScale::maxDecibels);
    }
} // namespace

SpectrumScale::SpectrumScale()
{
    for (const auto& marked : MARKED_FREQUENCIES)
        frequencyMarkers.push_back({ getPositionOf(marked.hertz), marked.text });

    for (const auto decibels : MARKED_DECIBELS)
        levelMarkers.push_back({ getLevelOf(static_cast<float>(decibels)), juce::String(decibels) });

    // The fainter lines: every multiple of a decade shown, but the marked decades
    for (auto decade = 10.f; decade < maxFrequency; decade *= static_cast<float>(MULTIPLES_PER_DECADE))
    {
        for (int multiple = 2; multiple < MULTIPLES_PER_DECADE; ++multiple)
        {
            const auto hertz = decade * static_cast<float>(multiple);
            if (hertz > minFrequency && hertz < maxFrequency)
                frequencyLines.push_back(getPositionOf(hertz));
        }
    }
}

float SpectrumScale::getFrequencyOf(int pointIdx)
{
    return getFrequencyAt(static_cast<float>(pointIdx) / static_cast<float>(numPoints - 1));
}

float SpectrumScale::getPositionOf(float hertz)
{
    return std::clamp(std::log(hertz / minFrequency) / std::log(maxFrequency / minFrequency), 0.f, 1.f);
}

float SpectrumScale::getLevelOf(float decibels)
{
    return std::clamp((decibels - minDecibels) / (maxDecibels - minDecibels), 0.f, 1.f);
}

float SpectrumScale::getFrequencyAt(float position)
{
    return minFrequency * std::pow(maxFrequency / minFrequency, position);
}

float SpectrumScale::getDecibelsAt(float level)
{
    return minDecibels + level * (maxDecibels - minDecibels);
}

HelixxLookAndFeel::SpectrumReadout SpectrumScale::getReadout(float position, float level)
{
    position = std::clamp(position, 0.f, 1.f);
    level    = std::clamp(level, 0.f, 1.f);

    const auto hertz = getFrequencyAt(position);

    return { position, level, getFrequencyText(hertz),
             juce::String(getDecibelsAt(level), 1) + " dB",
             getNoteName(hertz) + " " + getCentsText(hertz) };
}

juce::String SpectrumScale::getFrequencyText(float hertz)
{
    if (hertz < 99.95f)
        return juce::String(hertz, 1) + " Hz";

    if (hertz < 999.5f)
        return juce::String(juce::roundToInt(hertz)) + " Hz";

    const auto kilohertz = hertz * 0.001f;

    return juce::String(kilohertz, kilohertz < 9.995f ? 2 : 1) + " kHz";
}

juce::String SpectrumScale::getNoteName(float hertz)
{
    const auto nearest = juce::roundToInt(getNoteOf(hertz));

    // Its name and octave, rounded down for the notes under C0
    const auto pitchClass = ((nearest % NOTES_PER_OCTAVE) + NOTES_PER_OCTAVE) % NOTES_PER_OCTAVE;
    const auto octave     = (nearest - pitchClass) / NOTES_PER_OCTAVE + FIRST_OCTAVE;

    return juce::String(NOTE_NAMES[pitchClass]) + juce::String(octave);
}

juce::String SpectrumScale::getCentsText(float hertz)
{
    const auto note  = getNoteOf(hertz);
    const auto cents = juce::roundToInt((note - std::round(note)) * CENTS_PER_NOTE);

    return (cents < 0 ? "" : "+") + juce::String(cents) + " ct";
}

void SpectrumScale::measure(const std::vector<float>& magnitudes, float hertzPerBin, std::vector<float>& decibels)
{
    jassert(magnitudes.size() >= 2);

    decibels.resize(numPoints);

    const auto lastBin    = static_cast<int>(magnitudes.size()) - 1;
    const auto pointRatio = std::sqrt(getFrequencyOf(1) / getFrequencyOf(0));

    for (int pointIdx = 0; pointIdx < numPoints; ++pointIdx)
    {
        const auto place = getFrequencyOf(pointIdx) / hertzPerBin;
        auto& pointLevel = decibels[static_cast<size_t>(pointIdx)];

        if (place > static_cast<float>(lastBin))
        {
            pointLevel = minDecibels;
            continue;
        }

        const auto firstBin = juce::jmax(static_cast<int>(std::ceil(place / pointRatio)), 1);
        const auto endBin   = juce::jmin(static_cast<int>(std::ceil(place * pointRatio)), lastBin + 1);

        auto magnitude = 0.f;
        if (firstBin < endBin)
        {
            for (int bin = firstBin; bin < endBin; ++bin)
                magnitude = juce::jmax(magnitude, magnitudes[static_cast<size_t>(bin)]);
        }
        else
        {
            const auto under   = juce::jlimit(0, lastBin - 1, static_cast<int>(place));
            const auto between = juce::jlimit(0.f, 1.f, place - static_cast<float>(under));

            magnitude = juce::jmap(between, magnitudes[static_cast<size_t>(under)], magnitudes[static_cast<size_t>(under + 1)]);
        }

        pointLevel = toDecibels(magnitude);
    }
}

} // namespace helixx
