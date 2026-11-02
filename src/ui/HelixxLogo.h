#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace helixx
{

/**
 * @brief Helixx's logo, after EARL's. Its mark is a double helix seen from the
 *        side: two strands crossing six times, once for each string of a
 *        hexaphonic guitar, their turns tightening to the right as a frequency
 *        sweeps up. Where one goes behind the other a gap is cut, so nothing
 *        overlaps. Its name is a word mark of EARL's letters: bars with sharp
 *        corners, the X's a pair of them crossed like the strands.
 *
 * Drawn at any size, in the colours it's given.
 */
struct HelixxLogo
{
    /** @brief The mark's width for a height of 1. */
    static constexpr float MARK_ASPECT = 3.f;

    /** @brief The name's width for a height of 1 (its capitals' height). */
    static constexpr float NAME_ASPECT = 4.38f;

    /** @brief In the wide logo, the room between the mark and the name beside it, for a height of 1. */
    static constexpr float WIDE_GAP = 0.4f;

    /** @brief In the wide logo, the name's height, for a height of 1 (the mark's). */
    static constexpr float WIDE_NAME_HEIGHT = 0.62f;

    /** @brief The wide logo's width for a height of 1: the mark beside the name, smaller. */
    static constexpr float WIDE_ASPECT = MARK_ASPECT + WIDE_GAP + WIDE_NAME_HEIGHT * NAME_ASPECT;

    /** @brief The mark alone, as large as fits in `area` and centred there, a strand in each colour. */
    static void drawMark(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour strand, juce::Colour otherStrand);

    /** @brief The name alone, as large as fits in `area` and centred there. */
    static void drawName(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour name);

    /** @brief The wide logo, as large as fits in `area` and centred there: the mark, then the name beside it, level with its middle. */
    static void drawWide(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour strand, juce::Colour otherStrand, juce::Colour name);
};

} // namespace helixx
