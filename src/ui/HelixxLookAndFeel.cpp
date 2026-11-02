#include "HelixxLookAndFeel.h"

#include "HelixxLogo.h"
#include "SynthWorldLogo.h"

#include <BinaryData.h>

#include <algorithm>
#include <cmath>

namespace helixx
{

namespace
{
    constexpr auto TEXT_HEIGHT      = 12.f;     // The whole line's (ascent plus descent)
    constexpr auto MIN_TEXT_SQUASH  = 0.75f;    // How narrow a text gets before it gets smaller
    constexpr auto DISABLED_ALPHA   = 0.35f;

    // Panels: the background left round one, then its own edge to what's on it
    constexpr auto PANEL_MARGIN     = 1;
    constexpr auto PANEL_PADDING    = 3;
    constexpr auto PANEL_LIGHTENING = 0.035f;   // Its fill: how far the background goes to the foreground
    constexpr auto PANEL_OUTLINE    = 0.1f;     // The surface's alpha

    // The header
    constexpr auto HELIXX_LOGO_HEIGHT     = 28.f;
    constexpr auto SYNTHWORLD_LOGO_HEIGHT = 18;
    constexpr auto HEADER_INSET           = 6;      // From its ends to the logos
    constexpr auto HEADER_LINE            = 1.f;

    // A control's box
    constexpr auto CONTROL_MARGIN           = 1.f;
    constexpr auto CONTROL_CORNER           = 4.f;
    constexpr auto CONTROL_OUTLINE          = 1.f;
    constexpr auto CONTROL_LIGHTENING       = 0.1f;     // Its fill: how far the background goes to the foreground
    constexpr auto CONTROL_HOVER_LIGHTENING = 0.06f;    // And how much further while hovered
    constexpr auto CONTROL_LIT_TINT         = 0.22f;    // How far a lit box's fill goes to its light
    constexpr auto CONTROL_OUTLINE_ALPHA    = 0.16f;    // The foreground's, at rest
    constexpr auto CONTROL_HOVER_ALPHA      = 0.6f;     // The foreground's, while hovered
    constexpr auto CONTROL_LIT_ALPHA        = 0.8f;     // Its light's, when not hovered
    constexpr auto PRESSED_DARKEN           = 0.2f;

    // A fader
    constexpr auto THUMB_THICKNESS  = 8.f;      // Along the track
    constexpr auto THUMB_LENGTH     = 18.f;     // Across the track: the fader's thickness
    constexpr auto THUMB_CORNER     = 2.f;
    constexpr auto TRACK_THICKNESS  = 1.5f;
    constexpr auto TRACK_ALPHA      = 0.3f;
    constexpr auto TEXT_BOX_PADDING = 3;        // Over and under the value's line
    constexpr auto TEXT_BOX_GAP     = 2;        // Between the value and the fader

    // Guides: GUIDE_COUNT long ticks, each span split in GUIDE_SUBDIVISIONS by short ones
    constexpr auto GUIDE_COUNT        = 5;
    constexpr auto GUIDE_SUBDIVISIONS = 4;
    constexpr auto GUIDE_TICKS        = (GUIDE_COUNT - 1) * GUIDE_SUBDIVISIONS;
    constexpr auto GUIDE_THICKNESS    = 1.f;
    constexpr auto GUIDE_LENGTH       = 5.5f;
    constexpr auto SUB_GUIDE_LENGTH   = 2.5f;
    constexpr auto GUIDE_GAP          = 2.f;    // Between the guides and the track

    // A switch's bar
    constexpr auto SWITCH_BAR_WIDTH  = 12.f;
    constexpr auto SWITCH_BAR_HEIGHT = 4.f;
    constexpr auto SWITCH_BAR_GLOW   = 2.f;     // How far a lit bar's glow reaches around it
    constexpr auto SWITCH_BAR_RIM    = 1.f;     // The slot showing around the light
    constexpr auto SWITCH_GLOW_ALPHA = 0.25f;
    constexpr auto SWITCH_DIM_ALPHA  = 0.35f;   // The rim of a slot that is off, so it's seen on the background

    // Combo boxes
    constexpr auto COMBO_ARROW_ROOM  = 22;      // Kept free of text on the right
    constexpr auto COMBO_ARROW_INSET = 7.f;     // Between the arrow and the box's right edge
    constexpr auto COMBO_ARROW_SIZE  = 8.f;     // At most

    // Menus
    constexpr auto MENU_ITEM_HEIGHT     = 22;
    constexpr auto MENU_ITEM_MARGIN_X   = 2.f;
    constexpr auto MENU_ITEM_MARGIN_Y   = 1.f;
    constexpr auto MENU_HIGHLIGHT_ALPHA = 0.45f;    // The accent's, behind the item the mouse is over
    constexpr auto MENU_TEXT_INDENT     = 6.f;
    constexpr auto MENU_TICKED_INDENT   = 18.f;     // Past the tick
    constexpr auto MENU_TICK_INSET      = 4.f;
    constexpr auto MENU_TICK_SIZE       = 10.f;     // At most
    constexpr auto MENU_TICK_THICKNESS  = 1.5f;

    // A display's box
    constexpr auto DISPLAY_CORNER        = 2.f;
    constexpr auto DISPLAY_OUTLINE_ALPHA = 0.3f;    // The surface's
    constexpr auto DISPLAY_FILL_ALPHA    = 0.45f;   // The accent's, under a curve at its most

    // A spectrum
    constexpr auto SPECTRUM_MARGIN             = 5.f;
    constexpr auto SPECTRUM_CURVE_THICKNESS    = 1.5f;
    constexpr auto SPECTRUM_INPUT_THICKNESS    = 1.f;
    constexpr auto SPECTRUM_INPUT_ALPHA        = 0.6f;     // The accent's
    constexpr auto SPECTRUM_UNITY_ALPHA        = 0.25f;
    constexpr auto SPECTRUM_LINE_ALPHA         = 0.12f;
    constexpr auto SPECTRUM_MINOR_LINE_ALPHA   = 0.06f;
    constexpr auto SPECTRUM_TEXT_ALPHA         = 0.55f;
    constexpr auto SPECTRUM_TEXT_HEIGHT        = 11.f;
    constexpr auto SPECTRUM_TEXT_GAP           = 3.f;
    constexpr auto SPECTRUM_LEVEL_TEXT_WIDTH   = 30.f;
    constexpr auto SPECTRUM_READOUT_LINE_ALPHA = 0.18f;
    constexpr auto SPECTRUM_READOUT_TEXT_ALPHA = 0.7f;
    constexpr auto SPECTRUM_READOUT_WIDTH      = 96.f;
    constexpr auto SPECTRUM_READOUT_GAP        = 6.f;
    constexpr auto SPECTRUM_CORNER_WIDTH       = 90.f;     // What's written in its top right corner
    constexpr auto SPECTRUM_LEGEND_WIDTH       = 28.f;     // Each of the curves' names
    constexpr auto PITCH_MARK_ALPHA            = 0.8f;     // The highlight's, for a pitch at full strength
    constexpr auto PITCH_MARK_THICKNESS        = 1.5f;
    constexpr auto PITCH_MARK_TEXT_WIDTH       = 34.f;

    // A cell of the voice strip
    constexpr auto VOICE_CELL_PADDING     = 6.f;
    constexpr auto VOICE_NOTE_HEIGHT      = 20.f;
    constexpr auto VOICE_TALL_MIN_HEIGHT  = 48.f;   // Under it only the note shows
    constexpr auto VOICE_LIGHT_WIDTH      = 10.f;
    constexpr auto VOICE_LIGHT_HEIGHT     = 3.f;

    /** @brief Whether a slider is what the skin draws: a vertical track with its value below. */
    bool isFader(const juce::Slider& slider)
    {
        return slider.getSliderStyle() == juce::Slider::LinearVertical
            && slider.getTextBoxPosition() == juce::Slider::TextBoxBelow;
    }

    /** @brief Whether a tick's a long one. */
    bool isMajorGuide(int tickIdx)
    {
        return tickIdx % GUIDE_SUBDIVISIONS == 0;
    }

    /** @brief The guides at both sides of a vertical track, spread along its whole length. */
    juce::Path makeGuides(juce::Rectangle<float> track)
    {
        constexpr auto halfThickness = GUIDE_THICKNESS * 0.5f;

        juce::Path guides;
        for (int tickIdx = 0; tickIdx <= GUIDE_TICKS; ++tickIdx)
        {
            const auto length = isMajorGuide(tickIdx) ? GUIDE_LENGTH : SUB_GUIDE_LENGTH;
            const auto y = track.getY() + track.getHeight() * static_cast<float>(tickIdx) / GUIDE_TICKS - halfThickness;

            guides.addRoundedRectangle(track.getX() - GUIDE_GAP - length, y, length, GUIDE_THICKNESS, halfThickness);
            guides.addRoundedRectangle(track.getRight() + GUIDE_GAP,      y, length, GUIDE_THICKNESS, halfThickness);
        }
        return guides;
    }
} // namespace

HelixxLookAndFeel::HelixxLookAndFeel()
    :
    textFont(juce::FontOptions(juce::Typeface::createSystemTypefaceFor(BinaryData::OscillaRegular_ttf,
                                                                       BinaryData::OscillaRegular_ttfSize))
                 .withHeight(TEXT_HEIGHT))
{
    constexpr auto textHighlightAlpha = 0.5f;

    const auto textHighlight = palette.accent.withAlpha(textHighlightAlpha);

    setColour(juce::ResizableWindow::backgroundColourId , palette.background);
    setColour(juce::Label::textColourId                 , palette.foreground);
    setColour(juce::ComboBox::textColourId              , palette.foreground);
    setColour(juce::Slider::textBoxTextColourId         , palette.foreground);
    setColour(juce::Slider::textBoxBackgroundColourId   , juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId      , juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId    , textHighlight);
    setColour(juce::TextEditor::highlightColourId       , textHighlight);
    setColour(juce::TextEditor::highlightedTextColourId , palette.foreground);
    setColour(juce::CaretComponent::caretColourId       , palette.foreground);

    // Not opaque: a menu's window has to show past its box's rounded corners
    setColour(juce::PopupMenu::backgroundColourId, juce::Colours::transparentBlack);
}

void HelixxLookAndFeel::drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const auto panel = bounds.reduced(static_cast<float>(PANEL_MARGIN));

    g.setColour(palette.background.interpolatedWith(palette.foreground, PANEL_LIGHTENING));
    g.fillRoundedRectangle(panel, CONTROL_CORNER);
    g.setColour(palette.surface.withAlpha(PANEL_OUTLINE));
    g.drawRoundedRectangle(panel.reduced(CONTROL_OUTLINE * 0.5f), CONTROL_CORNER, CONTROL_OUTLINE);
}

juce::Rectangle<int> HelixxLookAndFeel::getPanelContentBounds(juce::Rectangle<int> bounds)
{
    return bounds.reduced(PANEL_MARGIN + PANEL_PADDING);
}

void HelixxLookAndFeel::drawHeader(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    // --- The line under it ---
    g.setColour(palette.foreground.withAlpha(CONTROL_OUTLINE_ALPHA));
    g.fillRect(bounds.removeFromBottom(HEADER_LINE));

    // --- Helixx's logo at the left, standing in the middle ---
    HelixxLogo::drawWide(g, bounds.reduced(static_cast<float>(HEADER_INSET), 0.f).removeFromLeft(HELIXX_LOGO_HEIGHT * HelixxLogo::WIDE_ASPECT),
                         palette.accent, palette.foreground, palette.foreground);
}

void HelixxLookAndFeel::drawSynthWorldLogo(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    SynthWorldLogo::drawWide(g, bounds, palette.accent, palette.foreground);
}

juce::Rectangle<int> HelixxLookAndFeel::getSynthWorldLogoBounds(juce::Rectangle<int> header)
{
    const auto width = static_cast<int>(std::ceil(static_cast<float>(SYNTHWORLD_LOGO_HEIGHT) * SynthWorldLogo::getWideAspect()));
    return header.reduced(HEADER_INSET, 0).removeFromRight(width).withSizeKeepingCentre(width, SYNTHWORLD_LOGO_HEIGHT);
}

/**
 * ============================================================================
 * =                                 DISPLAYS                                 =
 * ============================================================================
 */
void HelixxLookAndFeel::drawSpectrumDisplay(juce::Graphics& g, juce::Rectangle<float> bounds, const SpectrumDisplay& display) const
{
    drawDisplayBox(g, bounds);

    // --- The curves' place ---
    const auto area = getSpectrumDisplayArea(bounds);
    if (area.isEmpty())
        return;

    const auto pointAt = [&area](float position, float level)
    {
        return juce::Point<float> { area.getX() + position * area.getWidth(),
                                    area.getBottom() - juce::jlimit(0.f, 1.f, level) * area.getHeight() };
    };

    // --- Its levels: a line each, full level's the brightest, and its text over its left end ---
    for (const auto& marker : display.levelMarkers)
    {
        const auto y = pointAt(0.f, marker.level).y;

        g.setColour(palette.surface.withAlpha(juce::exactlyEqual(marker.level, display.unity) ? SPECTRUM_UNITY_ALPHA : SPECTRUM_LINE_ALPHA));
        g.drawLine(area.getX(), y, area.getRight(), y);

        g.setColour(palette.surface.withAlpha(SPECTRUM_TEXT_ALPHA));
        drawDisplayText(g, marker.text, { area.getX() + SPECTRUM_TEXT_GAP, y - SPECTRUM_TEXT_HEIGHT, SPECTRUM_LEVEL_TEXT_WIDTH, SPECTRUM_TEXT_HEIGHT },
                        juce::Justification::centredLeft);
    }

    // --- The frequencies between those marked: a fainter line each ---
    g.setColour(palette.surface.withAlpha(SPECTRUM_MINOR_LINE_ALPHA));
    for (const auto position : display.frequencyLines)
    {
        const auto top  = pointAt(position, 1.f);
        const auto base = pointAt(position, 0.f);
        g.drawLine(top.x, top.y, base.x, base.y);
    }

    // --- Its frequencies: a line each, and its text at the bottom, from the last one up to it ---
    auto previousAt = 0.f;
    for (const auto& marker : display.frequencyMarkers)
    {
        const auto top     = pointAt(marker.position, 1.f);
        const auto base    = pointAt(marker.position, 0.f);
        const auto textBox = juce::Rectangle<float> { pointAt(previousAt, 0.f).x + SPECTRUM_TEXT_GAP,
                                                      area.getBottom() - SPECTRUM_TEXT_HEIGHT,
                                                      juce::jmax((marker.position - previousAt) * area.getWidth() - 2.f * SPECTRUM_TEXT_GAP, 0.f),
                                                      SPECTRUM_TEXT_HEIGHT };

        g.setColour(palette.surface.withAlpha(SPECTRUM_LINE_ALPHA));
        g.drawLine(top.x, top.y, base.x, base.y);

        g.setColour(palette.surface.withAlpha(SPECTRUM_TEXT_ALPHA));
        drawDisplayText(g, marker.text, textBox, juce::Justification::centredRight);

        previousAt = marker.position;
    }

    // --- Where the modulator acts: a thin line at each mark, a thicker one at the selected pitch ---
    for (const auto& mark : display.pitchMarks)
    {
        const auto top  = pointAt(mark.position, 1.f);
        const auto base = pointAt(mark.position, 0.f);

        g.setColour(palette.highlight.withAlpha(PITCH_MARK_ALPHA * mark.strength));
        g.drawLine(top.x, top.y, base.x, base.y, mark.isMain ? PITCH_MARK_THICKNESS : CONTROL_OUTLINE);

        if (mark.text.isNotEmpty())
        {
            const auto hasRoom = top.x + SPECTRUM_TEXT_GAP + PITCH_MARK_TEXT_WIDTH <= area.getRight();
            drawDisplayText(g, mark.text,
                            { hasRoom ? top.x + SPECTRUM_TEXT_GAP : top.x - SPECTRUM_TEXT_GAP - PITCH_MARK_TEXT_WIDTH,
                              top.y, PITCH_MARK_TEXT_WIDTH, SPECTRUM_TEXT_HEIGHT },
                            hasRoom ? juce::Justification::centredLeft : juce::Justification::centredRight);
        }
    }

    // --- Where the mouse is: a cross through it, and beside it what's read there, on the side with room ---
    if (const auto* readout = display.readout)
    {
        const auto at = pointAt(readout->position, readout->level);

        g.setColour(palette.surface.withAlpha(SPECTRUM_READOUT_LINE_ALPHA));
        g.drawLine(at.x, area.getY(), at.x, area.getBottom());
        g.drawLine(area.getX(), at.y, area.getRight(), at.y);

        const auto boxHeight = 2.f * SPECTRUM_TEXT_HEIGHT;
        const auto right = at.x + SPECTRUM_READOUT_GAP + SPECTRUM_READOUT_WIDTH <= area.getRight();
        const auto above = at.y - SPECTRUM_READOUT_GAP - boxHeight >= area.getY();
        const auto box   = juce::Rectangle<float> { right ? at.x + SPECTRUM_READOUT_GAP : at.x - SPECTRUM_READOUT_GAP - SPECTRUM_READOUT_WIDTH,
                                                    above ? at.y - SPECTRUM_READOUT_GAP - boxHeight : at.y + SPECTRUM_READOUT_GAP,
                                                    SPECTRUM_READOUT_WIDTH, boxHeight };
        const auto justification = right ? juce::Justification::centredLeft : juce::Justification::centredRight;

        g.setColour(palette.surface.withAlpha(SPECTRUM_READOUT_TEXT_ALPHA));
        drawDisplayText(g, readout->frequencyText + "  " + readout->levelText, box.withHeight(SPECTRUM_TEXT_HEIGHT), justification);
        drawDisplayText(g, readout->noteText, box.withTrimmedTop(SPECTRUM_TEXT_HEIGHT), justification);
    }

    // --- The curves ---
    const auto makeCurve = [&pointAt](const std::vector<float>& levels)
    {
        const auto numSegments = static_cast<float>(levels.size() - 1);

        juce::Path curve;
        curve.startNewSubPath(pointAt(0.f, levels[0]));
        for (size_t pointIdx = 1; pointIdx < levels.size(); ++pointIdx)
            curve.lineTo(pointAt(static_cast<float>(pointIdx) / numSegments, levels[pointIdx]));
        return curve;
    };

    // What goes in, behind what comes out
    if (display.inputLevels.size() >= 2)
    {
        g.setColour(palette.accent.withAlpha(SPECTRUM_INPUT_ALPHA));
        g.strokePath(makeCurve(display.inputLevels), juce::PathStrokeType(SPECTRUM_INPUT_THICKNESS));
    }

    // What comes out, and what's under it down to the bottom
    if (display.outputLevels.size() >= 2)
    {
        const auto curve = makeCurve(display.outputLevels);

        auto under = curve;
        under.lineTo(area.getBottomRight());
        under.lineTo(area.getBottomLeft());
        under.closeSubPath();

        const auto fill = palette.accent.withAlpha(DISPLAY_FILL_ALPHA);
        g.setGradientFill(juce::ColourGradient(fill, area.getTopLeft(), fill.withAlpha(0.f), area.getBottomLeft(), false));
        g.fillPath(under);

        g.setColour(palette.foreground);
        g.strokePath(curve, juce::PathStrokeType(SPECTRUM_CURVE_THICKNESS));
    }

    // --- Its top right corner: which curve is which, and the index under them ---
    auto corner = area.withLeft(area.getRight() - SPECTRUM_CORNER_WIDTH).withHeight(2.f * SPECTRUM_TEXT_HEIGHT);
    auto legend = corner.removeFromTop(SPECTRUM_TEXT_HEIGHT);

    g.setColour(palette.foreground);
    drawDisplayText(g, "OUT", legend.removeFromRight(SPECTRUM_LEGEND_WIDTH), juce::Justification::centredRight);
    g.setColour(palette.accent);
    drawDisplayText(g, "IN", legend.removeFromRight(SPECTRUM_LEGEND_WIDTH), juce::Justification::centredRight);

    g.setColour(palette.foreground);
    drawDisplayText(g, display.indexText, corner, juce::Justification::centredRight);
}

juce::Rectangle<float> HelixxLookAndFeel::getSpectrumDisplayArea(juce::Rectangle<float> bounds)
{
    return bounds.reduced(SPECTRUM_MARGIN);
}

void HelixxLookAndFeel::drawVoiceCell(juce::Graphics& g, juce::Rectangle<float> bounds, const VoiceCell& cell, bool isHighlighted) const
{
    // --- Its box: lit while selected ---
    g.setColour(cell.isSelected ? palette.recess.interpolatedWith(palette.accent, CONTROL_LIT_TINT) : palette.recess);
    g.fillRoundedRectangle(bounds, DISPLAY_CORNER);

    g.setColour(cell.isSelected ? palette.accent.withMultipliedAlpha(isHighlighted ? 1.f : CONTROL_LIT_ALPHA)
                : isHighlighted ? palette.foreground.withAlpha(CONTROL_HOVER_ALPHA)
                                : palette.surface.withAlpha(DISPLAY_OUTLINE_ALPHA));
    g.drawRoundedRectangle(bounds.reduced(CONTROL_OUTLINE * 0.5f), DISPLAY_CORNER, CONTROL_OUTLINE);

    auto area = bounds.reduced(VOICE_CELL_PADDING, 0.f);
    const auto isTall = bounds.getHeight() >= VOICE_TALL_MIN_HEIGHT;

    // --- Its name, and the light at the other side ---
    const auto nameLine = isTall ? area.removeFromTop(SPECTRUM_TEXT_HEIGHT + VOICE_CELL_PADDING).withTrimmedTop(VOICE_CELL_PADDING * 0.5f)
                                 : area;

    g.setColour(palette.surface);
    drawDisplayText(g, cell.name, nameLine, juce::Justification::centredLeft);

    if (cell.hasLight)
    {
        const auto light = nameLine.withLeft(nameLine.getRight() - VOICE_LIGHT_WIDTH)
                                   .withSizeKeepingCentre(VOICE_LIGHT_WIDTH, VOICE_LIGHT_HEIGHT);

        drawSwitchBar(g, light, cell.isTracked ? std::optional { palette.accent } : std::nullopt, false, false, 1.f);
    }

    // --- The note, and what's known of it under it ---
    const auto noteColour = cell.isSounding ? palette.foreground : palette.surface;

    if (! isTall)
    {
        g.setColour(noteColour);
        drawDisplayText(g, cell.note, area, juce::Justification::centred);
        return;
    }

    const auto detailLine = area.removeFromBottom(SPECTRUM_TEXT_HEIGHT + VOICE_CELL_PADDING * 0.5f).withTrimmedBottom(VOICE_CELL_PADDING * 0.5f);

    g.setColour(noteColour);
    g.setFont(textFont.withHeight(VOICE_NOTE_HEIGHT));
    g.drawFittedText(cell.note, area.toNearestInt(), juce::Justification::centred, 1, MIN_TEXT_SQUASH);

    g.setColour(palette.surface);
    drawDisplayText(g, cell.detail, detailLine, juce::Justification::centred);
}

/**
 * ============================================================================
 * =                                   TEXT                                   =
 * ============================================================================
 */
juce::Font HelixxLookAndFeel::getLabelFont(juce::Label& /*label*/)
{
    return textFont;
}

juce::Font HelixxLookAndFeel::getComboBoxFont(juce::ComboBox& /*box*/)
{
    return textFont;
}

juce::Font HelixxLookAndFeel::getPopupMenuFont()
{
    return textFont;
}

/**
 * ============================================================================
 * =                                 SLIDERS                                  =
 * ============================================================================
 */
juce::Slider::SliderLayout HelixxLookAndFeel::getSliderLayout(juce::Slider& slider)
{
    if (! isFader(slider))
        return LookAndFeel_V4::getSliderLayout(slider);

    constexpr auto halfThumb = static_cast<int>(THUMB_THICKNESS) / 2;
    constexpr auto thickness = static_cast<int>(THUMB_LENGTH);

    auto bounds = slider.getLocalBounds();

    // Its line of text, but no more than a quarter of the slider
    const auto textHeight    = static_cast<int>(std::ceil(textFont.getHeight()));
    const auto textBoxHeight = juce::jmin(bounds.getHeight() / 4, textHeight + 2 * TEXT_BOX_PADDING);

    juce::Slider::SliderLayout layout;
    layout.textBoxBounds = bounds.removeFromBottom(textBoxHeight);

    // The bounds are the thumb's centre's travel, so the thumb stays in the slider
    bounds = bounds.reduced(0, TEXT_BOX_GAP + halfThumb);
    layout.sliderBounds = bounds.withSizeKeepingCentre(juce::jmin(thickness, bounds.getWidth()), bounds.getHeight());

    return layout;
}

juce::Label* HelixxLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* textBox = LookAndFeel_V4::createSliderTextBox(slider);

    // No box around the value, but a field to type it in
    textBox->setColour(juce::TextEditor::backgroundColourId, palette.recess);
    textBox->setMinimumHorizontalScale(MIN_TEXT_SQUASH);

    return textBox;
}

void HelixxLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (! isFader(slider))
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    const auto alpha  = slider.isEnabled() ? 1.f : DISABLED_ALPHA;

    // --- Track ---
    const auto track = bounds.withSizeKeepingCentre(TRACK_THICKNESS, bounds.getHeight());

    g.setColour(palette.surface.withMultipliedAlpha(TRACK_ALPHA * alpha));
    g.fillRoundedRectangle(track, TRACK_THICKNESS);

    // --- Guides ---
    g.setColour(palette.surface.withMultipliedAlpha(alpha));
    g.fillPath(makeGuides(track));

    // --- Thumb ---
    const auto thumb = juce::Rectangle<float>(THUMB_LENGTH, THUMB_THICKNESS).withCentre({ bounds.getCentreX(), sliderPos });

    g.setGradientFill(juce::ColourGradient(palette.surface.withMultipliedAlpha(alpha), thumb.getCentre(),
                                           palette.foreground.withMultipliedAlpha(alpha),
                                           { thumb.getX(), thumb.getCentreY() }, true));
    g.fillRoundedRectangle(thumb, THUMB_CORNER);
}

/**
 * ============================================================================
 * =                                 BUTTONS                                  =
 * ============================================================================
 */
void HelixxLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool isHighlighted, bool isDown)
{
    const auto room = button.getLocalBounds().toFloat().reduced(CONTROL_MARGIN + SWITCH_BAR_GLOW);
    const auto bar  = room.withSizeKeepingCentre(juce::jmin(SWITCH_BAR_WIDTH, room.getWidth()),
                                                 juce::jmin(SWITCH_BAR_HEIGHT, room.getHeight()));

    drawSwitchBar(g, bar, button.getToggleState() ? std::optional { palette.accent } : std::nullopt,
                  isHighlighted, isDown, button.isEnabled() ? 1.f : DISABLED_ALPHA);
}

/**
 * ============================================================================
 * =                        COMBO BOXES AND POPUP MENUS                       =
 * ============================================================================
 */
void HelixxLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                     juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int>(width, height).toFloat().reduced(CONTROL_MARGIN);
    const auto isOpen = box.isPopupActive();
    const auto isHighlighted = box.isMouseOver();

    // --- Box: lit while its menu is open ---
    drawControlBox(g, bounds, isOpen ? std::optional { palette.accent } : std::nullopt, isHighlighted, isButtonDown);

    // --- Arrow: quiet until the mouse is over the box ---
    constexpr auto arrowHeightRatio = 0.7f;

    const auto arrowSize  = juce::jmin(COMBO_ARROW_SIZE, bounds.getHeight() * 0.4f);
    const auto halfHeight = arrowSize * arrowHeightRatio * 0.5f;
    const auto arrowX     = bounds.getRight() - COMBO_ARROW_INSET - arrowSize * 0.5f;
    const auto arrowY     = bounds.getCentreY();

    juce::Path arrow;
    arrow.addTriangle(arrowX - arrowSize * 0.5f, arrowY - halfHeight,
                      arrowX + arrowSize * 0.5f, arrowY - halfHeight,
                      arrowX                   , arrowY + halfHeight);

    g.setColour(isOpen ? palette.accent : isHighlighted ? palette.foreground : palette.surface);
    g.fillPath(arrow);
}

void HelixxLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(box.getLocalBounds().withTrimmedRight(COMBO_ARROW_ROOM).reduced(static_cast<int>(CONTROL_MARGIN)));
    label.setFont(getComboBoxFont(box));

    // Over most of the box: taking the mouse, it would keep the box from being hovered
    label.setInterceptsMouseClicks(false, false);
}

void HelixxLookAndFeel::getIdealPopupMenuItemSize(const juce::String& text, bool /*isSeparator*/, int /*standardMenuItemHeight*/,
                                                  int& idealWidth, int& idealHeight)
{
    idealHeight = MENU_ITEM_HEIGHT;
    idealWidth  = static_cast<int>(std::ceil(juce::GlyphArrangement::getStringWidth(textFont, text))) + idealHeight * 2;
}

void HelixxLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<int>(width, height).toFloat().reduced(CONTROL_MARGIN);

    g.setColour(getControlColour());
    g.fillRoundedRectangle(bounds, CONTROL_CORNER);
    g.setColour(palette.surface.withAlpha(CONTROL_HOVER_ALPHA));
    g.drawRoundedRectangle(bounds.reduced(CONTROL_OUTLINE * 0.5f), CONTROL_CORNER, CONTROL_OUTLINE);
}

void HelixxLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool /*isSeparator*/, bool isActive, bool isHighlighted, bool isTicked, bool /*hasSubMenu*/,
                                          const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                          const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    const auto bounds = area.toFloat().reduced(MENU_ITEM_MARGIN_X, MENU_ITEM_MARGIN_Y);

    // --- Highlight ---
    if (isHighlighted && isActive)
    {
        g.setColour(palette.accent.withAlpha(MENU_HIGHLIGHT_ALPHA));
        g.fillRoundedRectangle(bounds, CONTROL_CORNER);
    }

    // --- Tick ---
    if (isTicked)
    {
        const auto tickSize = juce::jmin(MENU_TICK_SIZE, bounds.getHeight() * 0.5f);
        const auto tickX    = bounds.getX() + MENU_TICK_INSET;
        const auto tickY    = bounds.getCentreY() - tickSize * 0.5f;

        juce::Path tick;
        tick.startNewSubPath(tickX,                    tickY + tickSize * 0.5f);
        tick.lineTo         (tickX + tickSize * 0.35f, tickY + tickSize);
        tick.lineTo         (tickX + tickSize,         tickY);

        g.setColour(isHighlighted ? palette.foreground : palette.accent);
        g.strokePath(tick, juce::PathStrokeType(MENU_TICK_THICKNESS, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- Text ---
    g.setColour(palette.foreground.withMultipliedAlpha(isActive ? 1.f : DISABLED_ALPHA));
    g.setFont(textFont);
    g.drawFittedText(text, bounds.withTrimmedLeft(isTicked ? MENU_TICKED_INDENT : MENU_TEXT_INDENT).toNearestInt(),
                     juce::Justification::centredLeft, 1, MIN_TEXT_SQUASH);
}

/**
 * ============================================================================
 * =                                 HELPERS                                  =
 * ============================================================================
 */
juce::Colour HelixxLookAndFeel::getControlColour() const
{
    return palette.background.interpolatedWith(palette.foreground, CONTROL_LIGHTENING);
}

void HelixxLookAndFeel::drawControlBox(juce::Graphics& g, juce::Rectangle<float> bounds, std::optional<juce::Colour> light,
                                       bool isHighlighted, bool isDown) const
{
    // Background: tinted with its light, a little darker while hovered
    auto fill = getControlColour();
    if (light)
        fill = fill.interpolatedWith(*light, CONTROL_LIT_TINT);
    if (isHighlighted)
        fill = fill.interpolatedWith(palette.foreground, CONTROL_HOVER_LIGHTENING);

    g.setColour(isDown ? fill.darker(PRESSED_DARKEN) : fill);
    g.fillRoundedRectangle(bounds, CONTROL_CORNER);

    // Outline: its light's, or a faint one that shows while hovered
    const auto outline = light ? light->withMultipliedAlpha(isHighlighted ? 1.f : CONTROL_LIT_ALPHA)
                               : palette.foreground.withAlpha(isHighlighted ? CONTROL_HOVER_ALPHA : CONTROL_OUTLINE_ALPHA);
    g.setColour(outline);
    g.drawRoundedRectangle(bounds.reduced(CONTROL_OUTLINE * 0.5f), CONTROL_CORNER, CONTROL_OUTLINE);
}

void HelixxLookAndFeel::drawDisplayBox(juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    g.setColour(palette.recess);
    g.fillRoundedRectangle(bounds, DISPLAY_CORNER);

    g.setColour(palette.surface.withAlpha(DISPLAY_OUTLINE_ALPHA));
    g.drawRoundedRectangle(bounds.reduced(CONTROL_OUTLINE * 0.5f), DISPLAY_CORNER, CONTROL_OUTLINE);
}

void HelixxLookAndFeel::drawDisplayText(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> bounds,
                                        juce::Justification justification) const
{
    g.setFont(textFont.withHeight(SPECTRUM_TEXT_HEIGHT));
    g.drawFittedText(text, bounds.toNearestInt(), justification, 1, MIN_TEXT_SQUASH);
}

void HelixxLookAndFeel::drawSwitchBar(juce::Graphics& g, juce::Rectangle<float> bar, std::optional<juce::Colour> colour,
                                      bool isHighlighted, bool isDown, float alpha) const
{
    const auto corner = juce::jmin(bar.getWidth(), bar.getHeight()) * 0.5f;

    // --- Its glow ---
    if (colour)
    {
        g.setColour(colour->withAlpha(SWITCH_GLOW_ALPHA * alpha));
        g.fillRoundedRectangle(bar.expanded(SWITCH_BAR_GLOW), corner + SWITCH_BAR_GLOW);
    }

    // --- The slot, and the light in it ---
    g.setColour(colour ? palette.recess.withAlpha(alpha) : palette.surface.withAlpha(SWITCH_DIM_ALPHA * alpha));
    g.fillRoundedRectangle(bar.expanded(SWITCH_BAR_RIM), corner + SWITCH_BAR_RIM);

    const auto fill = colour.value_or(palette.recess);
    g.setColour((isDown ? fill.darker(PRESSED_DARKEN) : fill).withAlpha(alpha));
    g.fillRoundedRectangle(bar, corner);

    // --- Outline while hovered ---
    if (isHighlighted)
    {
        g.setColour(palette.foreground);
        g.drawRoundedRectangle(bar.expanded(SWITCH_BAR_RIM), corner + SWITCH_BAR_RIM, CONTROL_OUTLINE);
    }
}

} // namespace helixx
