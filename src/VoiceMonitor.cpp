// SPDX-FileCopyrightText: 2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "VoiceMonitor.h"

namespace helixx
{

void VoiceMonitor::prepare(double newSampleRate, int newNumChannels)
{
    jassert(newNumChannels <= maxChannels);

    for (auto& pitch : pitches)
    {
        pitch.detected.store(0.f, std::memory_order_relaxed);
        pitch.held    .store(0.f, std::memory_order_relaxed);
    }

    sampleRate.store(newSampleRate, std::memory_order_relaxed);
    numChannels.store(juce::jmin(newNumChannels, maxChannels), std::memory_order_relaxed);
}

void VoiceMonitor::pushInput(const juce::AudioBuffer<float>& buffer)
{
    push(input, buffer);
}

void VoiceMonitor::pushOutput(const juce::AudioBuffer<float>& buffer)
{
    push(output, buffer);
}

void VoiceMonitor::setDetectedPitch(int channel, float hertz)
{
    if (! juce::isPositiveAndBelow(channel, maxChannels))
        return;

    auto& pitch = pitches[static_cast<size_t>(channel)];

    pitch.detected.store(hertz, std::memory_order_relaxed);
    if (hertz > 0.f)
        pitch.held.store(hertz, std::memory_order_relaxed);
}

void VoiceMonitor::addDisplay()
{
    numDisplays.fetch_add(1, std::memory_order_relaxed);
}

void VoiceMonitor::removeDisplay()
{
    numDisplays.fetch_sub(1, std::memory_order_relaxed);
}

void VoiceMonitor::setSelectedChannel(int channel)
{
    selectedChannel.store(channel, std::memory_order_relaxed);
}

int VoiceMonitor::getSelectedChannel() const
{
    const auto count    = numChannels.load(std::memory_order_relaxed);
    const auto selected = selectedChannel.load(std::memory_order_relaxed);

    if (count == 1)
        return 0;

    return selected < count ? selected : allChannels;
}

int VoiceMonitor::getNumChannels() const
{
    return numChannels.load(std::memory_order_relaxed);
}

double VoiceMonitor::getSampleRate() const
{
    return sampleRate.load(std::memory_order_relaxed);
}

VoiceMonitor::Voice VoiceMonitor::getVoice(int channel) const
{
    if (! juce::isPositiveAndBelow(channel, maxChannels))
        return {};

    const auto& pitch = pitches[static_cast<size_t>(channel)];

    return { pitch.held.load(std::memory_order_relaxed),
             pitch.detected.load(std::memory_order_relaxed) > 0.f };
}

std::uint32_t VoiceMonitor::getNumFramesWritten() const
{
    return output.numWritten.load(std::memory_order_relaxed);
}

std::uint32_t VoiceMonitor::readInput(float* frames, int numFrames) const
{
    return read(input, frames, numFrames);
}

std::uint32_t VoiceMonitor::readOutput(float* frames, int numFrames) const
{
    return read(output, frames, numFrames);
}

void VoiceMonitor::push(FrameRing& ring, const juce::AudioBuffer<float>& buffer) const
{
    if (numDisplays.load(std::memory_order_relaxed) == 0)
        return;

    const auto count     = juce::jmin(buffer.getNumChannels(), numChannels.load(std::memory_order_relaxed));
    const auto selected  = getSelectedChannel();
    const auto firstChannel = selected == allChannels ? 0 : selected;
    const auto endChannel   = selected == allChannels ? count : juce::jmin(selected + 1, count);
    const auto numSamples   = buffer.getNumSamples();

    auto writeIdx = ring.numWritten.load(std::memory_order_relaxed);

    for (int frameIdx = 0; frameIdx < numSamples; ++frameIdx)
    {
        auto frame = 0.f;
        for (int channel = firstChannel; channel < endChannel; ++channel)
            frame += buffer.getReadPointer(channel)[frameIdx];

        ring.frames[writeIdx++ & frameMask].store(frame, std::memory_order_relaxed);
    }

    ring.numWritten.store(writeIdx, std::memory_order_release);
}

std::uint32_t VoiceMonitor::read(const FrameRing& ring, float* frames, int numFrames)
{
    jassert(numFrames <= maxFrames);

    const auto numWritten = ring.numWritten.load(std::memory_order_acquire);

    auto readIdx = numWritten - static_cast<std::uint32_t>(numFrames);
    for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx)
        frames[frameIdx] = ring.frames[readIdx++ & frameMask].load(std::memory_order_relaxed);

    return numWritten;
}

} // namespace helixx
