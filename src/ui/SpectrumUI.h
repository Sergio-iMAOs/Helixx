// SPDX-FileCopyrightText: 2025-2026 Sergio Ramos Galindo
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <optional>
#include <vector>

#include "../Helixx.h"
#include "HelixxLookAndFeel.h"
#include "SpectrumScale.h"

namespace helixx
{

/**
 * @brief Spectrum analyser of the channel selected in the VoiceMonitor, or of
 *        all of them summed: the output as a filled curve, the input as a
 *        faint one behind it. A sine at full level reads 0 dB.
 *
 * Over the spectrum, it marks where the modulator is acting: each selected
 * pitch and, for a single channel, the sidebands the modulation puts around
 * it. Its corner shows the modulation index when it's known. Where the mouse
 * is, it shows the frequency, the level and the nearest note.
 *
 * The transforms run here on the message thread, on a timer, and only while it
 * is on screen; the audio thread only keeps the frames for them. A point goes
 * up at once and falls at #fallDecibelsPerSecond. When the frames stop coming
 * they count as silence after #staleSeconds, so the curve comes down.
 */
class SpectrumUI : public juce::Component, private juce::Timer
{
public:
    static constexpr float fallDecibelsPerSecond = 60.f;
    static constexpr double staleSeconds = 0.25;

    /** @brief Number of sidebands marked at each side of a pitch. */
    static constexpr int numSidebandsMarked = 6;

    /**
     * @brief Starts displaying an effect.
     * @param helixxDSP The effect to read; must outlive this component.
     */
    explicit SpectrumUI(Helixx& helixxDSP);
    ~SpectrumUI() override;

    void paint(juce::Graphics& g) override;

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    /**
     * @brief Works the spectra of the latest frames out and lets the points fall.
     * @param seconds Time since the last update.
     */
    void update(double seconds);

private:
    Helixx& helixx;
    VoiceMonitor& monitor;
    SpectrumScale scale;

    std::unique_ptr<juce::dsp::FFT> transform;
    std::vector<float> window;      // Hann's, as long as the transform
    std::vector<float> frames;      // the latest read
    std::vector<float> transformed; // scratch of the transform, twice as long
    std::vector<float> magnitudes;  // how loud each frequency is, up to half the rate
    std::vector<float> measured;    // what the points measure now, in decibels

    std::vector<float> outputDecibels;  // what's shown, in decibels
    std::vector<float> inputDecibels;
    std::vector<float> outputLevels;    // the same, as places up the scale
    std::vector<float> inputLevels;

    std::vector<HelixxLookAndFeel::PitchMark> pitchMarks;
    juce::String indexText;
    std::optional<HelixxLookAndFeel::SpectrumReadout> readout;

    std::uint32_t lastNumWritten = 0;
    double secondsStale = 0.0;
    bool hasStaleFrames = false;
    bool displayRequested = false;
    double lastUpdateMs = 0.0;

    void timerCallback() override;

    /** @brief Makes room for a transform of a size, and its window. */
    void setTransformOrder(int order);

    /** @brief Measures the frames just read into #measured, see SpectrumScale::measure(). */
    void measureFrames(double sampleRate);

    /**
     * @brief Moves shown levels to what's measured: up at once, down by the time gone.
     * @return Whether any of them moved.
     */
    bool applyMeasured(std::vector<float>& shown, float fall) const;

    /** @brief Works out the marks and the index text from the current pitches. */
    void updateMarks();

    /** @brief Shows what's read at a place in the component; none to show nothing. */
    void showReadoutAt(std::optional<juce::Point<float>> position);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumUI)
};

} // namespace helixx
