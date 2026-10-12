#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "../Helixx.h"

namespace helixx
{

/**
 * @brief Control panel of the effect: algorithm selector, coarse frequency,
 *        depth and range sliders plus the relative-range toggle.
 *
 * The single range slider edits either the millisecond or the relative range
 * parameter, following the state of the toggle. The range controls only act
 * on the Adaptive FM algorithm and are disabled for the others.
 */
class HelixxComponent: public juce::Component, private juce::Button::Listener
{
public:
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
    Helixx& helixx;

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