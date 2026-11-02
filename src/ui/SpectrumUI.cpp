// SPDX-FileCopyrightText: 2025-2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "SpectrumUI.h"

#include <algorithm>
#include <cmath>

namespace helixx
{

namespace
{
    constexpr auto UPDATE_HZ          = 30;
    constexpr auto MAX_UPDATE_SECONDS = 0.25;   // A timer held up doesn't drop the curve at once

    // How finely the spectrum is cut: long enough to tell apart the sidebands
    // of a low note, and as long in time at higher rates
    constexpr auto BASE_FFT_ORDER = 13;
    constexpr auto BASE_FFT_RATE  = 50000.0;
    constexpr auto MAX_FFT_ORDER  = 15;
    static_assert((1 << MAX_FFT_ORDER) <= VoiceMonitor::maxFrames, "The monitor keeps no more frames than that");

    // Marks over the spectrum
    constexpr auto HELD_MARK_STRENGTH  = 0.5f;  // A pitch no longer detected, which the modulator holds on to
    constexpr auto SIDEBAND_STRENGTH   = 0.55f; // The first sideband, relative to its pitch
    constexpr auto SIDEBAND_FADE       = 0.8f;  // From one sideband to the next
    constexpr auto SAME_FREQUENCY      = 0.001f;// As a part of a pitch: a sideband that lands on it

    int getFFTOrder(double sampleRate)
    {
        const auto octaves = static_cast<int>(std::ceil(std::log2(sampleRate / BASE_FFT_RATE)));
        return juce::jlimit(BASE_FFT_ORDER, MAX_FFT_ORDER, BASE_FFT_ORDER + octaves);
    }

    void toLevels(const std::vector<float>& decibels, std::vector<float>& levels)
    {
        std::transform(decibels.begin(), decibels.end(), levels.begin(), SpectrumScale::getLevelOf);
    }
} // namespace

SpectrumUI::SpectrumUI(Helixx& helixxDSP)
    :
    helixx(helixxDSP),
    monitor(helixxDSP.getVoiceMonitor()),
    measured(SpectrumScale::numPoints, SpectrumScale::minDecibels),
    outputDecibels(SpectrumScale::numPoints, SpectrumScale::minDecibels),
    inputDecibels(SpectrumScale::numPoints, SpectrumScale::minDecibels),
    outputLevels(SpectrumScale::numPoints, 0.f),
    inputLevels(SpectrumScale::numPoints, 0.f)
{
    lastUpdateMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(UPDATE_HZ);
}

SpectrumUI::~SpectrumUI()
{
    stopTimer();
    if (displayRequested)
        monitor.removeDisplay();
}

void SpectrumUI::timerCallback()
{
    const auto nowMs = juce::Time::getMillisecondCounterHiRes();
    update(juce::jmin((nowMs - lastUpdateMs) * 0.001, MAX_UPDATE_SECONDS));
    lastUpdateMs = nowMs;
}

void SpectrumUI::setTransformOrder(int order)
{
    transform = std::make_unique<juce::dsp::FFT>(order);

    const auto size = static_cast<size_t>(transform->getSize());

    frames.resize(size);
    transformed.resize(2 * size);
    magnitudes.resize(size / 2);

    // Hann's window: nothing at its ends, so the frames needn't be whole cycles
    window.resize(size);
    for (size_t frameIdx = 0; frameIdx < size; ++frameIdx)
        window[frameIdx] = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * static_cast<float>(frameIdx) / static_cast<float>(size));
}

void SpectrumUI::measureFrames(double sampleRate)
{
    const auto size = frames.size();

    auto windowSum = 0.f;
    std::fill(transformed.begin(), transformed.end(), 0.f);
    for (size_t frameIdx = 0; frameIdx < size; ++frameIdx)
    {
        transformed[frameIdx] = frames[frameIdx] * window[frameIdx];
        windowSum += window[frameIdx];
    }

    transform->performFrequencyOnlyForwardTransform(transformed.data(), true);

    // How loud each frequency is: a sine at full level reads 1 where it is
    const auto magnitudeScale = 2.f / windowSum;
    for (size_t bin = 0; bin < magnitudes.size(); ++bin)
        magnitudes[bin] = transformed[bin] * magnitudeScale;

    SpectrumScale::measure(magnitudes, static_cast<float>(sampleRate / static_cast<double>(size)), measured);
}

bool SpectrumUI::applyMeasured(std::vector<float>& shown, float fall) const
{
    auto moved = false;

    for (size_t pointIdx = 0; pointIdx < shown.size(); ++pointIdx)
    {
        const auto level = juce::jmax(measured[pointIdx], shown[pointIdx] - fall, SpectrumScale::minDecibels);
        moved = moved || ! juce::exactlyEqual(level, shown[pointIdx]);
        shown[pointIdx] = level;
    }

    return moved;
}

void SpectrumUI::update(double seconds)
{
    // Ask for the audio only while the spectrum is on screen
    const auto showing = isShowing();
    if (showing != displayRequested)
    {
        displayRequested = showing;
        if (showing)
            monitor.addDisplay();
        else
            monitor.removeDisplay();
    }

    const auto sampleRate = monitor.getSampleRate();
    if (! showing || sampleRate <= 0.0)
        return;

    const auto order = getFFTOrder(sampleRate);
    if (transform == nullptr || transform->getSize() != 1 << order)
        setTransformOrder(order);

    const auto numFrames = transform->getSize();
    const auto fall = fallDecibelsPerSecond * static_cast<float>(seconds);

    // The latest frames: none new, the curves as they were for a moment, then silence
    const auto numWritten = monitor.readOutput(frames.data(), numFrames);
    const auto stale = numWritten == lastNumWritten;
    secondsStale = stale ? secondsStale + seconds : 0.0;
    lastNumWritten = numWritten;
    hasStaleFrames = stale && secondsStale >= staleSeconds;

    const auto hadMarks = ! pitchMarks.empty();
    const auto oldIndexText = indexText;
    updateMarks();

    auto moved = hadMarks || ! pitchMarks.empty() || indexText != oldIndexText;

    if (! stale || hasStaleFrames)
    {
        if (hasStaleFrames)
            std::fill(measured.begin(), measured.end(), SpectrumScale::minDecibels);
        else
            measureFrames(sampleRate);

        moved = applyMeasured(outputDecibels, fall) || moved;

        if (! hasStaleFrames)
        {
            monitor.readInput(frames.data(), numFrames);
            measureFrames(sampleRate);
        }

        moved = applyMeasured(inputDecibels, fall) || moved;
    }

    if (moved)
    {
        toLevels(outputDecibels, outputLevels);
        toLevels(inputDecibels, inputLevels);
        repaint();
    }
}

void SpectrumUI::updateMarks()
{
    pitchMarks.clear();
    indexText.clear();

    const auto selected = monitor.getSelectedChannel();

    // The modulator holds a pitch no longer detected, but not past the end of the audio
    if (! hasStaleFrames)
    {
        if (selected == VoiceMonitor::allChannels)
        {
            for (int channel = 0; channel < monitor.getNumChannels(); ++channel)
            {
                const auto voice = monitor.getVoice(channel);
                if (voice.hertz > 0.f)
                    pitchMarks.push_back({ SpectrumScale::getPositionOf(voice.hertz),
                                           voice.isTracked ? 1.f : HELD_MARK_STRENGTH, {}, false });
            }
        }
        else if (const auto voice = monitor.getVoice(selected); voice.hertz > 0.f)
        {
            const auto strength = voice.isTracked ? 1.f : HELD_MARK_STRENGTH;

            pitchMarks.push_back({ SpectrumScale::getPositionOf(voice.hertz), strength,
                                   SpectrumScale::getNoteName(voice.hertz), true });

            // The sidebands, those under 0 Hz coming back up
            if (helixx.getDepth() > 0.f)
            {
                const auto modulatorHz = voice.hertz * helixx.getCoarseFreq();
                auto sidebandStrength  = strength * SIDEBAND_STRENGTH;

                for (int sideband = 1; sideband <= numSidebandsMarked; ++sideband)
                {
                    const auto offset = static_cast<float>(sideband) * modulatorHz;

                    for (const auto hertz : { voice.hertz + offset, std::abs(voice.hertz - offset) })
                        if (hertz > SpectrumScale::minFrequency && hertz < SpectrumScale::maxFrequency
                            && std::abs(hertz - voice.hertz) > SAME_FREQUENCY * voice.hertz)
                            pitchMarks.push_back({ SpectrumScale::getPositionOf(hertz), sidebandStrength, {}, false });

                    sidebandStrength *= SIDEBAND_FADE;
                }
            }
        }
    }

    const auto pitch = selected == VoiceMonitor::allChannels ? 0.f : monitor.getVoice(selected).hertz;
    if (const auto index = helixx.getModulationIndex(pitch))
        indexText = "INDEX " + juce::String(*index, 2);
}

void SpectrumUI::mouseMove(const juce::MouseEvent& event)
{
    showReadoutAt(event.position);
}

void SpectrumUI::mouseDrag(const juce::MouseEvent& event)
{
    showReadoutAt(event.position);
}

void SpectrumUI::mouseExit(const juce::MouseEvent& /*event*/)
{
    showReadoutAt(std::nullopt);
}

void SpectrumUI::showReadoutAt(std::optional<juce::Point<float>> position)
{
    std::optional<HelixxLookAndFeel::SpectrumReadout> read;

    if (position)
    {
        const auto area = HelixxLookAndFeel::getSpectrumDisplayArea(getLocalBounds().toFloat());
        if (! area.isEmpty() && area.contains(*position))
            read = SpectrumScale::getReadout((position->x - area.getX()) / area.getWidth(),
                                             (area.getBottom() - position->y) / area.getHeight());
    }

    if (! read && ! readout)
        return;

    readout = std::move(read);
    repaint();
}

void SpectrumUI::paint(juce::Graphics& g)
{
    if (auto* look = dynamic_cast<HelixxLookAndFeel*>(&getLookAndFeel()))
    {
        look->drawSpectrumDisplay(g, getLocalBounds().toFloat(),
                                  { outputLevels, inputLevels, scale.getFrequencyMarkers(), scale.getFrequencyLines(),
                                    scale.getLevelMarkers(), SpectrumScale::getLevelOf(0.f), pitchMarks,
                                    readout ? &*readout : nullptr, indexText });
    }
}

} // namespace helixx
