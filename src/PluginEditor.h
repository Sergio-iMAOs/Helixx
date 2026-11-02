// SPDX-FileCopyrightText: 2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "PluginProcessor.h"
#include "ui/HelixxComponent.h"
#include "ui/HelixxLookAndFeel.h"

//==============================================================================
/**
 * @brief Plugin window: a fixed-size panel around helixx::HelixxComponent,
 *        which sets the look of everything in it.
 */
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;

    helixx::HelixxLookAndFeel lookAndFeel; // before what uses it: it must outlive it
    helixx::HelixxComponent helixxUI;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
