// SPDX-FileCopyrightText: 2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "HelixxComponent.h"

namespace helixx
{

namespace
{
    constexpr auto HELIXX_URL     = "https://github.com/Sergio-iMAOs/Helixx";
    constexpr auto SYNTHWORLD_URL = "https://synthworldaudio.com/";
}

HelixxComponent::LogoButton::LogoButton(const juce::String& name, const juce::URL& url, Draw drawLogo)
    : juce::Button(name), draw(drawLogo)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus(false);

    onClick = [url] { url.launchInDefaultBrowser(); };
}

void HelixxComponent::LogoButton::paintButton(juce::Graphics& g, bool /*isHighlighted*/, bool /*isDown*/)
{
    if (auto* look = dynamic_cast<HelixxLookAndFeel*>(&getLookAndFeel()))
        (look->*draw)(g, getLocalBounds().toFloat());
}

HelixxComponent::HelixxComponent(Helixx& helixxDSP)
    :
    helixx(helixxDSP),
    helixxButton("Helixx", juce::URL(HELIXX_URL), &HelixxLookAndFeel::drawHelixxLogo),
    synthWorldButton("SynthWorld", juce::URL(SYNTHWORLD_URL), &HelixxLookAndFeel::drawSynthWorldLogo),
    spectrum(helixxDSP),
    voiceStrip(helixxDSP.getVoiceMonitor())
{
    auto& apvts = helixx.getAPVTS();

    // The attachment selects by index: the items must be there before it is created.
    if (auto* algorithmParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(ID::AlgorithmID)))
        algorithmBox.addItemList(algorithmParam->choices, 1);

    algorithmBox.onChange = [this] { updateRangeEnablement(); };
    algorithmAttachment .reset(new ComboBoxAttachment(apvts, ID::AlgorithmID    , algorithmBox));
    updateRangeEnablement();

    coarseFreqAttachment.reset(new SliderAttachment(apvts, ID::FMCoarseFreqID   , coarseFreqSlider));
    depthAttachment     .reset(new SliderAttachment(apvts, ID::FMDepthID        , depthSlider));

    // Listen before attaching: the attachment sets the initial toggle state,
    // which is what selects the parameter edited by the range slider.
    relativeButton.addListener(this);
    relativeAttachment  .reset(new ButtonAttachment(apvts, ID::FMIsRelativeID   , relativeButton));
    updateRangeAttachment();

    coarseFreqLabel .setText("COARSE FREQ"  , juce::dontSendNotification);
    depthLabel      .setText("MOD DEPTH"    , juce::dontSendNotification);
    rangeLabel      .setText("MOD RANGE"    , juce::dontSendNotification);
    relativeLabel   .setText("RELATIVE"     , juce::dontSendNotification);
    coarseFreqLabel .setJustificationType(juce::Justification::centred);
    depthLabel      .setJustificationType(juce::Justification::centred);
    rangeLabel      .setJustificationType(juce::Justification::centred);
    relativeLabel   .setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(helixxButton);
    addAndMakeVisible(synthWorldButton);
    addAndMakeVisible(algorithmBox);

    addAndMakeVisible(coarseFreqLabel);
    addAndMakeVisible(depthLabel);
    addAndMakeVisible(rangeLabel);
    addAndMakeVisible(relativeLabel);
    addAndMakeVisible(coarseFreqSlider);
    addAndMakeVisible(depthSlider);
    addAndMakeVisible(rangeSlider);
    addAndMakeVisible(relativeButton);

    addAndMakeVisible(spectrum);
    addAndMakeVisible(voiceStrip);

    voiceStrip.onNumCellsChanged = [this] { resized(); };
}

HelixxComponent::~HelixxComponent()
{
    relativeButton.removeListener(this);
}

void HelixxComponent::paint(juce::Graphics& g)
{
    if (auto* look = dynamic_cast<HelixxLookAndFeel*>(&getLookAndFeel()))
        look->drawHeader(g, headerBounds.toFloat());
}

void HelixxComponent::resized()
{
    constexpr auto margin = 2;
    auto bounds = getLocalBounds().reduced(margin);

    constexpr auto headerGap = 6;

    headerBounds = bounds.removeFromTop(HEADER_HEIGHT - headerGap);
    bounds.removeFromTop(headerGap);
    helixxButton    .setBounds(HelixxLookAndFeel::getHelixxLogoBounds(headerBounds));
    synthWorldButton.setBounds(HelixxLookAndFeel::getSynthWorldLogoBounds(headerBounds));

    constexpr auto controlsWidth = 288;
    constexpr auto displaysGap = 8;

    // Displays at the right: the spectrum over the voice strip
    auto displayBounds = bounds.withTrimmedLeft(controlsWidth + displaysGap);
    bounds.setWidth(controlsWidth);

    voiceStrip.setBounds(displayBounds.removeFromBottom(voiceStrip.getIdealHeight(displayBounds.getWidth())));
    displayBounds.removeFromBottom(displaysGap);
    spectrum.setBounds(displayBounds);

    constexpr auto labelHeight = 20;
    constexpr auto algorithmHeight = 24;

    algorithmBox.setBounds(bounds.removeFromTop(algorithmHeight));
    bounds.removeFromTop(margin);
    const auto sliderWidth = bounds.getWidth() / 3;

    auto freqBounds     = bounds.removeFromLeft(sliderWidth);
    auto depthBounds    = bounds.removeFromLeft(sliderWidth);
    auto& rangeBounds   = bounds;

    coarseFreqLabel .setBounds(freqBounds   .removeFromTop(labelHeight));
    coarseFreqSlider.setBounds(freqBounds);
    depthLabel      .setBounds(depthBounds  .removeFromTop(labelHeight));
    depthSlider     .setBounds(depthBounds);

    constexpr auto relativeButtonWidth = 24;

    auto relativeBounds = rangeBounds.removeFromBottom(labelHeight);
    relativeButton  .setBounds(relativeBounds   .removeFromLeft(relativeButtonWidth));
    relativeLabel   .setBounds(relativeBounds);
    rangeLabel      .setBounds(rangeBounds      .removeFromTop(labelHeight));
    rangeSlider     .setBounds(rangeBounds);
}

void HelixxComponent::buttonClicked(juce::Button* /*button*/)
{
}

void HelixxComponent::buttonStateChanged(juce::Button* /*button*/)
{
    updateRangeAttachment();
}

void HelixxComponent::updateRangeAttachment()
{
    const auto isRelative = relativeButton.getToggleState();

    // This also runs for hover and press state changes: only re-attach when
    // the toggle actually flipped.
    if (rangeAttachment != nullptr && isRelative == rangeIsRelative)
        return;

    auto& apvts = helixx.getAPVTS();

    // Re-attach range slider (detach from the old parameter first)
    rangeAttachment.reset();
    rangeAttachment.reset(new SliderAttachment(apvts, isRelative ? ID::FMRangeRelID : ID::FMRangeMsID, rangeSlider));
    rangeIsRelative = isRelative;
}

void HelixxComponent::updateRangeEnablement()
{
    const auto usesRange = algorithmBox.getSelectedItemIndex() == static_cast<int>(Algorithm::AdaptiveFM);

    rangeSlider     .setEnabled(usesRange);
    relativeButton  .setEnabled(usesRange);
    rangeLabel      .setEnabled(usesRange);
    relativeLabel   .setEnabled(usesRange);
}

} // namespace helixx
