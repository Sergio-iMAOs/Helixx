#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

#include "../VoiceMonitor.h"
#include "HelixxLookAndFeel.h"

namespace helixx
{

/**
 * @brief One cell per channel with the note it plays and whether its pitch is
 *        being tracked, plus one for all of them when there's more than one.
 *        Clicking a cell selects what the spectrum shows.
 *
 * A channel that lost its pitch shows it dimmed: its modulator holds on to it.
 *
 * Any channel count fits: up to where the cells get too narrow they share one
 * tall row, past that they wrap into shorter rows that only show the notes.
 */
class VoiceStripUI : public juce::Component, private juce::Timer
{
public:
    /**
     * @brief Starts displaying the voices of a monitor.
     * @param voiceMonitor Where to read them; must outlive this component.
     */
    explicit VoiceStripUI(VoiceMonitor& voiceMonitor);
    ~VoiceStripUI() override;

    void paint(juce::Graphics& g) override;

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;

    /**
     * @brief Returns the height the cells need.
     * @param width Width it's given.
     */
    int getIdealHeight(int width) const;

    /** @brief Reads the voices again. */
    void update();

    /** @brief Called when the number of cells changes, and with it the ideal height. */
    std::function<void()> onNumCellsChanged;

private:
    VoiceMonitor& monitor;

    std::vector<HelixxLookAndFeel::VoiceCell> cells;
    int hoveredCell = -1;

    std::uint32_t lastNumWritten = 0;
    int numUpdatesStale = 0;

    void timerCallback() override;

    /** @brief Returns the channel a cell shows, or VoiceMonitor::allChannels. */
    int getChannelOf(int cellIdx) const;

    /** @brief Returns how many cells fit side by side in a width. */
    int getNumColumns(int width) const;

    /** @brief Returns where a cell is. */
    juce::Rectangle<int> getCellBounds(int cellIdx) const;

    /** @brief Returns the cell at a place, or -1. */
    int getCellAt(juce::Point<int> position) const;

    /** @brief Works out what the cells show. */
    std::vector<HelixxLookAndFeel::VoiceCell> makeCells(bool isStale) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceStripUI)
};

} // namespace helixx
