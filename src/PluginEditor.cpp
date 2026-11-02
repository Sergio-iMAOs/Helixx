// SPDX-FileCopyrightText: 2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
    helixxUI(p.getHelixx())
{
    juce::ignoreUnused (processorRef);

    addAndMakeVisible(helixxUI);

    // After adding it: only the components already here are told, and the
    // sliders make their text boxes with the look's colours when they are.
    setLookAndFeel(&lookAndFeel);

    setSize (960, 600 + helixx::HelixxComponent::HEADER_HEIGHT);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    lookAndFeel.drawPanel(g, getLocalBounds().toFloat());
}

void AudioPluginAudioProcessorEditor::resized()
{
    helixxUI.setBounds(helixx::HelixxLookAndFeel::getPanelContentBounds(getLocalBounds()));
}
