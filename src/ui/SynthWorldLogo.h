#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace helixx
{

/**
 * @brief SynthWorld's logo, ported from its own code (SynthWorld draws with its
 *        own framework, not JUCE): a planet, and a ring orbiting it that is a
 *        sine wave, one cycle of it across the planet's face. Its name is a word
 *        mark of EARL's letters, set apart beside the mark.
 *
 * Drawn at any size, in the colours it's given. Nothing is drawn outside the
 * area given.
 */
struct SynthWorldLogo
{
    /** @brief The wide logo's width for a height of 1: the mark, then the name. */
    static float getWideAspect();

    /**
     * @brief The wide logo, as large as fits in `area` and centred there: the mark
     *        shaded, then the name, in the ring's colour. The planet's shades are
     *        worked out from its colour. For opaque colours.
     */
    static void drawWide(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour planet, juce::Colour ring);
};

} // namespace helixx
