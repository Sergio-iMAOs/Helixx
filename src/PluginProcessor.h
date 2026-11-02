// SPDX-FileCopyrightText: 2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "Helixx.h"

#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/**
 * @brief JUCE plugin entry point: hosts the Helixx effect.
 *
 * Handles the host-facing side (bus layouts, latency reporting, state
 * persistence) and delegates all audio processing to helixx::Helixx.
 * Accepts any layout with the same number of input and output channels,
 * from mono up to 32, each channel being processed as an independent voice.
 */
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** @brief Returns the effect, so the editor can attach to its parameters. */
    helixx::Helixx& getHelixx();

private:
    //==============================================================================
    helixx::Helixx helixx;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
