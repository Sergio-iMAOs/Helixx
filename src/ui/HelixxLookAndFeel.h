#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>
#include <vector>

namespace helixx
{

/**
 * @brief The plugin's look, after SynthWorld's Light skin: pale panels, dark
 *        text in Oscilla, boxes with a faint outline, thin faders with guides
 *        and on/off as a bar that lights up.
 *
 * Covers what the plugin shows: vertical faders with their value below,
 * labels, combo boxes and their menus, toggle buttons, and the displays, sunk
 * into the panel like SynthWorld's. Set on the editor, so every component
 * inside it gets it. Disabled controls are dimmed.
 */
class HelixxLookAndFeel: public juce::LookAndFeel_V4
{
public:
    HelixxLookAndFeel();

    /** @brief A panel: a rounded box a little lighter than the window's background. */
    void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    /** @brief What's left inside a panel for what's on it. */
    static juce::Rectangle<int> getPanelContentBounds(juce::Rectangle<int> bounds);

    /** @brief The window's header: Helixx's logo at the left, a line under all of it. */
    void drawHeader(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    /** @brief SynthWorld's logo, as large as fits in `bounds`. */
    void drawSynthWorldLogo(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    /** @brief Where SynthWorld's logo goes in a header: at its right end. */
    static juce::Rectangle<int> getSynthWorldLogoBounds(juce::Rectangle<int> header);

    /** @brief A frequency marked behind a spectrum, with its text at the bottom. */
    struct FrequencyMarker
    {
        float position = 0.f;   ///< Along the frequencies shown, 0..1
        juce::String text;
    };

    /** @brief A level marked behind a spectrum, with its text at the left. */
    struct LevelMarker
    {
        float level = 0.f;      ///< Up the levels shown, 0..1
        juce::String text;
    };

    /** @brief A frequency the modulator acts on: a pitch or one of its sidebands. */
    struct PitchMark
    {
        float position = 0.f;       ///< Along the frequencies shown, 0..1
        float strength = 1.f;       ///< How strongly it's drawn, 0..1
        juce::String text;          ///< At its top; may be empty
        bool isMain = false;        ///< The selected channel's pitch: a thicker line
    };

    /** @brief What's read off a spectrum where the mouse is. */
    struct SpectrumReadout
    {
        float position = 0.f;       ///< Along the frequencies shown, 0..1
        float level = 0.f;          ///< Up the levels shown, 0..1
        juce::String frequencyText;
        juce::String levelText;
        juce::String noteText;
    };

    /** @brief What a spectrum display shows (drawSpectrumDisplay()). */
    struct SpectrumDisplay
    {
        const std::vector<float>& outputLevels;     ///< The curve: places up the levels shown, the lowest frequency first
        const std::vector<float>& inputLevels;      ///< The faint curve behind it
        const std::vector<FrequencyMarker>& frequencyMarkers;
        const std::vector<float>& frequencyLines;   ///< Fainter lines, along the frequencies shown
        const std::vector<LevelMarker>& levelMarkers;
        float unity = 0.f;                          ///< The level marker drawn brightest
        const std::vector<PitchMark>& pitchMarks;
        const SpectrumReadout* readout = nullptr;   ///< None while the mouse isn't there
        juce::String indexText;                     ///< In its corner; may be empty
    };

    /** @brief What a cell of the voice strip shows (drawVoiceCell()). */
    struct VoiceCell
    {
        juce::String name;          ///< In its corner; may be empty
        juce::String note;
        juce::String detail;        ///< Under the note, when there's room
        bool isTracked  = false;    ///< Lights up its light
        bool isSounding = false;    ///< Dimmed otherwise
        bool isSelected = false;
        bool hasLight   = false;
    };

    /** @brief A spectrum: its levels and frequencies, the marks, then its curves over them. */
    void drawSpectrumDisplay(juce::Graphics& g, juce::Rectangle<float> bounds, const SpectrumDisplay& display) const;

    /** @brief Where a spectrum's curve goes inside its box. */
    static juce::Rectangle<float> getSpectrumDisplayArea(juce::Rectangle<float> bounds);

    /** @brief A cell of the voice strip. Under a certain height only its note shows. */
    void drawVoiceCell(juce::Graphics& g, juce::Rectangle<float> bounds, const VoiceCell& cell, bool isHighlighted) const;

    // Text
    juce::Font getLabelFont     (juce::Label& label) override;
    juce::Font getComboBoxFont  (juce::ComboBox& box) override;
    juce::Font getPopupMenuFont () override;

    // Sliders
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override;

    juce::Label* createSliderTextBox(juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // Buttons
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool isHighlighted, bool isDown) override;

    // Combo boxes and their menus
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;

    void getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator, int standardMenuItemHeight,
                                   int& idealWidth, int& idealHeight) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

private:
    /** @brief The Light skin's colours, by role. Every colour drawn derives from these. */
    struct Palette
    {
        juce::Colour background { 0xffeceaf0 }; // #eceaf0     ///< Behind everything
        juce::Colour recess     { 0xffdad7e0 }; // #dad7e0     ///< What's sunk into a panel: slots, value fields
        juce::Colour surface    { 0xff77747f }; // #77747f     ///< Guides; inactive or secondary elements
        juce::Colour foreground { 0xff232125 }; // #232125     ///< Text, outlines
        juce::Colour accent     { 0xff7648c4 }; // #7648c4     ///< What's on or selected
        juce::Colour highlight  { 0xffc77700 }; // #c77700     ///< What's drawn over a display against another axis
    };

    const Palette palette;
    const juce::Font textFont;

    /** @brief What a control's box is filled with: a little darker than a panel. */
    juce::Colour getControlColour() const;

    /**
     * @brief The rounded box of a control.
     * @param light What it's lit with while on: its outline is in that colour
     *              and its fill tinted with it. None for a box at rest.
     */
    void drawControlBox(juce::Graphics& g, juce::Rectangle<float> bounds, std::optional<juce::Colour> light,
                        bool isHighlighted, bool isDown) const;

    /**
     * @brief A switch: a thin slot, its light glowing in it while on.
     * @param colour Its light; none for a switch that is off.
     * @param alpha  How opaque all of it is.
     */
    void drawSwitchBar(juce::Graphics& g, juce::Rectangle<float> bar, std::optional<juce::Colour> colour,
                       bool isHighlighted, bool isDown, float alpha) const;

    /** @brief The box of a display: sunk into the panel, with a faint outline. */
    void drawDisplayBox(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    /** @brief A line of text in a display, narrowed to fit. */
    void drawDisplayText(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> bounds,
                         juce::Justification justification) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelixxLookAndFeel)
};

} // namespace helixx
