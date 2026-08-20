#include "HelixxComponent.h"

namespace helixx
{

HelixxComponent::HelixxComponent(Helixx& helixxDSP)
    :
    helixx(helixxDSP)
{
    auto& apvts = helixx.getAPVTS();

    coarseFreqAttachment.reset(new SliderAttachment(apvts, ID::FMCoarseFreqID   , coarseFreqSlider));
    depthAttachment     .reset(new SliderAttachment(apvts, ID::FMDepthID        , depthSlider));

    coarseFreqLabel .setText("COARSE FREQ"  , juce::dontSendNotification);
    depthLabel      .setText("MOD DEPTH"    , juce::dontSendNotification);
    coarseFreqLabel .setJustificationType(juce::Justification::centred);
    depthLabel      .setJustificationType(juce::Justification::centred);

    addAndMakeVisible(coarseFreqLabel);
    addAndMakeVisible(depthLabel);
    addAndMakeVisible(coarseFreqSlider);
    addAndMakeVisible(depthSlider);
}

HelixxComponent::~HelixxComponent()
{
}

void HelixxComponent::paint(juce::Graphics& /*g*/)
{
}

void HelixxComponent::resized()
{
    constexpr auto margin = 2;
    auto bounds = getLocalBounds().reduced(margin);

    constexpr auto labelHeight = 20;

    auto freqBounds = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto& depthBounds = bounds;

    coarseFreqLabel .setBounds(freqBounds   .removeFromTop(labelHeight));
    coarseFreqSlider.setBounds(freqBounds);
    depthLabel      .setBounds(depthBounds  .removeFromTop(labelHeight));
    depthSlider     .setBounds(depthBounds);
}

} // namespace helixx