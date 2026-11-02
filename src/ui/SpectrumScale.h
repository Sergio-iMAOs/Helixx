#pragma once

#include <vector>

#include "HelixxLookAndFeel.h"

namespace helixx
{

/**
 * @brief What the spectrum is drawn on, as in SynthWorld's: #minFrequency to
 *        #maxFrequency left to right, each octave as wide, #minDecibels to
 *        #maxDecibels upwards, as #numPoints points; and how a transform's
 *        frequencies become those points (measure()).
 *
 * An object of it is the lines marked behind the curve: 100, 1k and 10k with
 * their texts, and a fainter line at each whole multiple of a decade between.
 * Everything else is static.
 */
class SpectrumScale
{
public:
    static constexpr int numPoints = 192;
    static constexpr float minFrequency = 20.f;
    static constexpr float maxFrequency = 20000.f;
    static constexpr float minDecibels = -78.f;
    static constexpr float maxDecibels = 6.f;

    SpectrumScale();

    /** @brief Returns a point's frequency, in Hz. */
    static float getFrequencyOf(int pointIdx);

    /** @brief Returns where a frequency is: 0 at #minFrequency, 1 at #maxFrequency. */
    static float getPositionOf(float hertz);

    /** @brief Returns where a level is: 0 at #minDecibels, 1 at #maxDecibels. */
    static float getLevelOf(float decibels);

    /** @brief Returns the frequency at a place along the scale, in Hz. */
    static float getFrequencyAt(float position);

    /** @brief Returns the level at a place up the scale, in decibels. */
    static float getDecibelsAt(float level);

    /**
     * @brief Returns what's read off a spectrum at a place.
     * @param position Place along it, 0..1.
     * @param level    Place up it, 0..1.
     */
    static HelixxLookAndFeel::SpectrumReadout getReadout(float position, float level);

    /** @brief Returns a frequency as shown: "52.5 Hz", "440 Hz", "1.25 kHz", "12.5 kHz". */
    static juce::String getFrequencyText(float hertz);

    /** @brief Returns the equal-tempered note nearest a frequency (A4 at 440 Hz): "A4", "C#5". */
    static juce::String getNoteName(float hertz);

    /** @brief Returns how far a frequency is from its nearest note: "+0 ct", "-12 ct". */
    static juce::String getCentsText(float hertz);

    /**
     * @brief Works out the level of each point: the loudest of the transform's
     *        frequencies nearer to it than to its neighbours or, where the
     *        points are closer together than those frequencies, the level
     *        between the two around it.
     * @param magnitudes  How loud each frequency is, from 0 Hz up to half the
     *                    sample rate; 1 is a sine at full level.
     * @param hertzPerBin Distance between two of those frequencies.
     * @param decibels    Filled with #numPoints levels.
     */
    static void measure(const std::vector<float>& magnitudes, float hertzPerBin, std::vector<float>& decibels);

    const std::vector<HelixxLookAndFeel::FrequencyMarker>& getFrequencyMarkers() const { return frequencyMarkers; }
    const std::vector<float>& getFrequencyLines() const { return frequencyLines; }
    const std::vector<HelixxLookAndFeel::LevelMarker>& getLevelMarkers() const { return levelMarkers; }

private:
    std::vector<HelixxLookAndFeel::FrequencyMarker> frequencyMarkers;
    std::vector<float> frequencyLines;
    std::vector<HelixxLookAndFeel::LevelMarker> levelMarkers;
};

} // namespace helixx
