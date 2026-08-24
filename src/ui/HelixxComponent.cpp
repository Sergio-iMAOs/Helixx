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
    buttonStateChanged(&relativeButton);
    relativeAttachment  .reset(new ButtonAttachment(apvts, ID::FMIsRelativeID   , relativeButton));

    coarseFreqLabel .setText("COARSE FREQ"  , juce::dontSendNotification);
    depthLabel      .setText("MOD DEPTH"    , juce::dontSendNotification);
    rangeLabel      .setText("MOD RANGE"    , juce::dontSendNotification);
    relativeLabel   .setText("RELATIVE"     , juce::dontSendNotification);
    coarseFreqLabel .setJustificationType(juce::Justification::centred);
    depthLabel      .setJustificationType(juce::Justification::centred);
    rangeLabel      .setJustificationType(juce::Justification::centred);
    relativeLabel   .setJustificationType(juce::Justification::centred);

    relativeButton.addListener(this);

    addAndMakeVisible(coarseFreqLabel);
    addAndMakeVisible(depthLabel);
    addAndMakeVisible(rangeLabel);
    addAndMakeVisible(relativeLabel);
    addAndMakeVisible(coarseFreqSlider);
    addAndMakeVisible(depthSlider);
    addAndMakeVisible(rangeSlider);
    addAndMakeVisible(relativeButton);
}

HelixxComponent::~HelixxComponent()
{
    relativeButton.removeListener(this);
}

void HelixxComponent::paint(juce::Graphics& /*g*/)
{
}

void HelixxComponent::resized()
{
    constexpr auto margin = 2;
    auto bounds = getLocalBounds().reduced(margin);

    constexpr auto labelHeight = 20;
    const auto sliderWidth = bounds.getWidth() / 3;

    auto freqBounds     = bounds.removeFromLeft(sliderWidth);
    auto depthBounds    = bounds.removeFromLeft(sliderWidth);
    auto& rangeBounds   = bounds;

    coarseFreqLabel .setBounds(freqBounds   .removeFromTop(labelHeight));
    coarseFreqSlider.setBounds(freqBounds);
    depthLabel      .setBounds(depthBounds  .removeFromTop(labelHeight));
    depthSlider     .setBounds(depthBounds);

    const auto relativeHeight = rangeBounds.getHeight() / 4;

    auto relativeBounds = rangeBounds.removeFromBottom(relativeHeight);
    relativeLabel   .setBounds(relativeBounds   .removeFromTop(labelHeight));
    relativeButton  .setBounds(relativeBounds);
    rangeLabel      .setBounds(rangeBounds      .removeFromTop(labelHeight));
    rangeSlider     .setBounds(rangeBounds);
}

void HelixxComponent::buttonClicked(juce::Button *button)
{
}

void HelixxComponent::buttonStateChanged(juce::Button *button)
{
    auto& apvts = helixx.getAPVTS();

    // Re-attach range slider
    if (button->getToggleState())
    {
        rangeAttachment.reset(new SliderAttachment(apvts, ID::FMRangeRelID  , rangeSlider));
    }
    else
    {
        rangeAttachment.reset(new SliderAttachment(apvts, ID::FMRangeMsID   , rangeSlider));
    }
}

} // namespace helixx