#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "../Helixx.h"

namespace helixx
{

class HelixxComponent: public juce::Component
{
public:
    explicit HelixxComponent(Helixx& helixxDSP);
    ~HelixxComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

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

    juce::Label coarseFreqLabel;
    juce::Label depthLabel;

    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;
    
    std::unique_ptr<SliderAttachment> coarseFreqAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelixxComponent)
};

} // namespace helixx