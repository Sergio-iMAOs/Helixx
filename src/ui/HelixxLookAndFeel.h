#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

namespace helixx
{

/**
 * @brief The plugin's look, after SynthWorld's Light skin: pale panels, dark
 *        text in Oscilla, boxes with a faint outline, thin faders with guides
 *        and on/off as a bar that lights up.
 *
 * Covers what the plugin shows: vertical faders with their value below,
 * labels, combo boxes and their menus, and toggle buttons. Set on the editor,
 * so every component inside it gets it. Disabled controls are dimmed.
 */
class HelixxLookAndFeel: public juce::LookAndFeel_V4
{
public:
    HelixxLookAndFeel();

    /** @brief A panel: a rounded box a little lighter than the window's background. */
    void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds) const;

    /** @brief What's left inside a panel for what's on it. */
    static juce::Rectangle<int> getPanelContentBounds(juce::Rectangle<int> bounds);

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelixxLookAndFeel)
};

} // namespace helixx
