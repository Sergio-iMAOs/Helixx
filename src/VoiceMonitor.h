#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace helixx
{

/**
 * @brief What the audio thread leaves for the displays: each channel's pitch,
 *        and the latest audio going in and out of the effect.
 *
 * The audio is kept for one thing at a time, chosen with setSelectedChannel():
 * a channel, or all of them summed, so a sine at full level on any one channel
 * reads the same either way.
 *
 * It costs the audio thread nothing while no display is on screen: nothing is
 * copied unless a display asked for it with addDisplay().
 *
 * Threading: everything is lock-free and allocated up front, so what the
 * displays read stays valid across a prepare(). A frame read while it's being
 * overwritten only costs the display a slightly wrong picture for one update.
 */
class VoiceMonitor
{
public:
    /** @brief Most channels the plugin accepts. */
    static constexpr int maxChannels = 32;

    /** @brief A selection of every channel at once, see setSelectedChannel(). */
    static constexpr int allChannels = -1;

    /** @brief Most frames read at once. A power of two. */
    static constexpr int maxFrames = 1 << 15;

    /** @brief What's known of a channel's pitch. */
    struct Voice
    {
        float hertz     = 0.f;      ///< Last pitch detected, which the modulator holds on to; 0 if there never was one
        bool isTracked  = false;    ///< Whether the latest analysis found a pitch
    };

    /**
     * @brief Starts over with a new configuration. Call along with the effect's prepare.
     * @param newSampleRate  Rate, in Hz, of the audio that will be pushed.
     * @param newNumChannels Number of channels that will be processed.
     */
    void prepare(double newSampleRate, int newNumChannels);

    /** @name Audio thread */
    ///@{
    /** @brief Keeps the selected channel of a block entering the effect. */
    void pushInput(const juce::AudioBuffer<float>& buffer);

    /** @brief Keeps the selected channel of a block leaving the effect. */
    void pushOutput(const juce::AudioBuffer<float>& buffer);

    /**
     * @brief Stores the latest analysis of a channel.
     * @param channel Channel index.
     * @param hertz   Detected pitch, or 0 if there was none.
     */
    void setDetectedPitch(int channel, float hertz);
    ///@}

    /** @name Displays */
    ///@{
    /** @brief Asks for the audio to be kept; each call needs its removeDisplay(). */
    void addDisplay();

    /** @brief Undoes addDisplay(). */
    void removeDisplay();

    /** @brief Chooses what audio is kept: a channel index, or #allChannels. */
    void setSelectedChannel(int channel);

    /**
     * @brief Returns what audio is kept.
     * @return A channel index, or #allChannels; a lone channel is always selected.
     */
    int getSelectedChannel() const;

    /** @brief Returns the number of channels given to prepare(). */
    int getNumChannels() const;

    /** @brief Returns the rate given to prepare(), in Hz, 0 before it. */
    double getSampleRate() const;

    /** @brief Returns what's known of a channel's pitch. */
    Voice getVoice(int channel) const;

    /** @brief Returns the frames kept so far, wrapping around: the same again means no audio ran. */
    std::uint32_t getNumFramesWritten() const;

    /**
     * @brief Copies the latest frames that entered the effect, oldest first.
     * @param frames    Where to write them.
     * @param numFrames How many, up to #maxFrames.
     * @return Frames kept so far, see getNumFramesWritten().
     */
    std::uint32_t readInput(float* frames, int numFrames) const;

    /** @brief Same as readInput(), for what left the effect. */
    std::uint32_t readOutput(float* frames, int numFrames) const;
    ///@}

private:
    static constexpr std::uint32_t frameMask = maxFrames - 1;

    // A frame goes in as a float and comes out as one, whoever gets there first
    struct FrameRing
    {
        std::vector<std::atomic<float>> frames = std::vector<std::atomic<float>>(maxFrames);
        std::atomic<std::uint32_t> numWritten { 0 };
    };

    struct PitchState
    {
        std::atomic<float> detected { 0.f };    // latest analysis, 0 = no pitch
        std::atomic<float> held     { 0.f };    // latest one that found a pitch
    };

    FrameRing input;
    FrameRing output;
    std::array<PitchState, maxChannels> pitches;

    std::atomic<int> numChannels { 0 };
    std::atomic<int> selectedChannel { allChannels };
    std::atomic<int> numDisplays { 0 };
    std::atomic<double> sampleRate { 0.0 };

    /** @brief Appends the selected channel of a block to a ring. */
    void push(FrameRing& ring, const juce::AudioBuffer<float>& buffer) const;

    /** @brief Copies the latest frames of a ring, see readInput(). */
    static std::uint32_t read(const FrameRing& ring, float* frames, int numFrames);
};

} // namespace helixx
