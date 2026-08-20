#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "../Helixx.h"

namespace helixx
{

class HelixxComponent: public juce::Component
{
public:
    explicit HelixxComponent(Helixx& helixx);
    ~HelixxComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    Helixx& helixx;

    juce::Slider coarseFreqSlider;
    juce::Slider depthSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelixxComponent)
};

} // namespace helixx