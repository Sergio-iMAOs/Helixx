#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "../Helixx.h"
#include "SpectrumUI.h"
#include "VoiceStripUI.h"

namespace helixx
{

/**
 * @brief Everything in the plugin's window: the header with the logos, each
 *        a link to its site, and under it, at the left the controls (algorithm selector, coarse
 *        frequency, depth and range sliders plus the relative-range toggle),
 *        at the right the displays (the spectrum over the voice strip).
 *
 * The single range slider edits either the millisecond or the relative range
 * parameter, following the state of the toggle. The range controls only act
 * on the Adaptive FM algorithm and are disabled for the others.
 */
class HelixxComponent: public juce::Component, private juce::Button::Listener
{
public:
    /** @brief The header's height, with the room under it: what it adds to the window's. */
    static constexpr int HEADER_HEIGHT = 44;

    /**
     * @brief Builds the controls and attaches them to the effect's parameters.
     * @param helixxDSP The effect to control; must outlive this component.
     */
    explicit HelixxComponent(Helixx& helixxDSP);
    ~HelixxComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;


    // Button listener methods
    void buttonClicked      (juce::Button* button) override;
    void buttonStateChanged (juce::Button* button) override;

private:
    /** @brief A logo in the header: a click opens its site in the web browser. */
    class LogoButton final: public juce::Button
    {
    public:
        /** @brief What draws the logo. */
        using Draw = void (HelixxLookAndFeel::*)(juce::Graphics&, juce::Rectangle<float>) const;

        LogoButton(const juce::String& name, const juce::URL& url, Draw drawLogo);

        void paintButton(juce::Graphics& g, bool isHighlighted, bool isDown) override;

    private:
        const Draw draw;
    };

    Helixx& helixx;

    juce::Rectangle<int> headerBounds;
    LogoButton helixxButton;
    LogoButton synthWorldButton;

    juce::ComboBox algorithmBox;

    juce::Slider coarseFreqSlider
    {
        juce::Slider::SliderStyle::LinearVertical,
        juce::Slider::TextBoxBelow
    };
    juce::Slider depthSlider
    {
        juce::Slider::SliderStyle::LinearVertical,
        juce::Slider::TextBoxBelow
    };
    juce::Slider rangeSlider
    {
        juce::Slider::SliderStyle::LinearVertical,
        juce::Slider::TextBoxBelow
    };

    juce::ToggleButton relativeButton;

    juce::Label coarseFreqLabel;
    juce::Label depthLabel;
    juce::Label rangeLabel;
    juce::Label relativeLabel;

    SpectrumUI spectrum;
    VoiceStripUI voiceStrip;

    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;
    typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;

    std::unique_ptr<ComboBoxAttachment> algorithmAttachment;
    
    std::unique_ptr<SliderAttachment> coarseFreqAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> rangeAttachment;
    std::unique_ptr<ButtonAttachment> relativeAttachment;

    bool rangeIsRelative = false; // which parameter rangeAttachment is bound to

    /**
     * @brief Binds the range slider to the parameter matching the toggle:
     *        relative range when it is on, milliseconds when it is off.
     */
    void updateRangeAttachment();

    /** @brief Enables the range controls only for the algorithm that uses them. */
    void updateRangeEnablement();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelixxComponent)
};

} // namespace helixx