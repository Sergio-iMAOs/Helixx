#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "../Helixx.h"

namespace helixx
{

class HelixxComponent: public juce::Component, private juce::Button::Listener
{
public:
    explicit HelixxComponent(Helixx& helixxDSP);
    ~HelixxComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;


    // Button listener methods
    void buttonClicked      (juce::Button* button) override;
    void buttonStateChanged (juce::Button* button) override;

private:
    Helixx& helixx;

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
    
    std::unique_ptr<SliderAttachment> coarseFreqAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> rangeAttachment;
    std::unique_ptr<ButtonAttachment> relativeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelixxComponent)
};

} // namespace helixx