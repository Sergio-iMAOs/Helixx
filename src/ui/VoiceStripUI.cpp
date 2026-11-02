#include "VoiceStripUI.h"

#include <algorithm>

#include "SpectrumScale.h"

namespace helixx
{

namespace
{
    constexpr auto UPDATE_HZ = 30;
    constexpr auto STALE_UPDATES = 8;   // With no audio for this long a pitch no longer counts as tracked

    constexpr auto CELL_GAP         = 3;
    constexpr auto CELL_MIN_WIDTH   = 62;
    constexpr auto TALL_ROW_HEIGHT  = 64;
    constexpr auto SHORT_ROW_HEIGHT = 26;

    bool isSameCell(const HelixxLookAndFeel::VoiceCell& a, const HelixxLookAndFeel::VoiceCell& b)
    {
        return a.name == b.name && a.note == b.note && a.detail == b.detail
            && a.isTracked == b.isTracked && a.isSounding == b.isSounding
            && a.isSelected == b.isSelected;
    }
} // namespace

VoiceStripUI::VoiceStripUI(VoiceMonitor& voiceMonitor)
    :
    monitor(voiceMonitor)
{
    update();
    startTimerHz(UPDATE_HZ);
}

VoiceStripUI::~VoiceStripUI()
{
    stopTimer();
}

void VoiceStripUI::timerCallback()
{
    update();
}

void VoiceStripUI::update()
{
    // No new audio for a while: nothing's being tracked any more
    const auto numWritten = monitor.getNumFramesWritten();
    numUpdatesStale = numWritten == lastNumWritten ? numUpdatesStale + 1 : 0;
    lastNumWritten = numWritten;

    auto newCells = makeCells(numUpdatesStale >= STALE_UPDATES);

    const auto numCellsChanged = newCells.size() != cells.size();
    const auto changed = numCellsChanged || ! std::equal(newCells.begin(), newCells.end(), cells.begin(), isSameCell);

    cells = std::move(newCells);

    if (numCellsChanged && onNumCellsChanged != nullptr)
        onNumCellsChanged();

    if (changed)
        repaint();
}

std::vector<HelixxLookAndFeel::VoiceCell> VoiceStripUI::makeCells(bool isStale) const
{
    const auto numChannels = monitor.getNumChannels();
    const auto selected    = monitor.getSelectedChannel();

    std::vector<HelixxLookAndFeel::VoiceCell> newCells;

    if (numChannels > 1)
    {
        HelixxLookAndFeel::VoiceCell all;
        all.note       = "ALL";
        all.isSelected = selected == VoiceMonitor::allChannels;
        all.isSounding = true;

        auto numTracked = 0;
        for (int channel = 0; channel < numChannels; ++channel)
            if (monitor.getVoice(channel).isTracked && ! isStale)
                ++numTracked;

        all.detail = juce::String(numTracked) + " / " + juce::String(numChannels);
        newCells.push_back(std::move(all));
    }

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto voice = monitor.getVoice(channel);

        HelixxLookAndFeel::VoiceCell cell;
        cell.name       = juce::String(channel + 1);
        cell.isTracked  = voice.isTracked && ! isStale;
        cell.isSounding = cell.isTracked;
        cell.isSelected = channel == selected;
        cell.hasLight   = true;

        if (voice.hertz <= 0.f)
        {
            cell.note   = "-";
            cell.detail = "NO PITCH";
        }
        else
        {
            cell.note   = SpectrumScale::getNoteName(voice.hertz);
            cell.detail = cell.isTracked ? SpectrumScale::getFrequencyText(voice.hertz) + "  " + SpectrumScale::getCentsText(voice.hertz)
                                         : juce::String("HELD");
        }

        newCells.push_back(std::move(cell));
    }

    return newCells;
}

int VoiceStripUI::getChannelOf(int cellIdx) const
{
    return cells.size() > 1 ? cellIdx - 1 : cellIdx;
}

int VoiceStripUI::getNumColumns(int width) const
{
    const auto fitting = (width + CELL_GAP) / (CELL_MIN_WIDTH + CELL_GAP);
    return juce::jlimit(1, juce::jmax(1, static_cast<int>(cells.size())), fitting);
}

int VoiceStripUI::getIdealHeight(int width) const
{
    if (cells.empty())
        return 0;

    const auto numCells   = static_cast<int>(cells.size());
    const auto numColumns = getNumColumns(width);
    const auto numRows    = (numCells + numColumns - 1) / numColumns;

    if (numRows == 1)
        return TALL_ROW_HEIGHT;

    return numRows * SHORT_ROW_HEIGHT + (numRows - 1) * CELL_GAP;
}

juce::Rectangle<int> VoiceStripUI::getCellBounds(int cellIdx) const
{
    const auto numCells   = static_cast<int>(cells.size());
    const auto numColumns = getNumColumns(getWidth());
    const auto numRows    = juce::jmax(1, (numCells + numColumns - 1) / numColumns);

    const auto column = cellIdx % numColumns;
    const auto row    = cellIdx / numColumns;

    const auto rowHeight = (getHeight() - (numRows - 1) * CELL_GAP) / numRows;

    // Spread over the whole width, the rounding left out on no side in particular
    const auto cellStep = [this, numColumns](int edge) { return edge * (getWidth() + CELL_GAP) / numColumns; };

    return { cellStep(column), row * (rowHeight + CELL_GAP), cellStep(column + 1) - cellStep(column) - CELL_GAP, rowHeight };
}

int VoiceStripUI::getCellAt(juce::Point<int> position) const
{
    for (int cellIdx = 0; cellIdx < static_cast<int>(cells.size()); ++cellIdx)
        if (getCellBounds(cellIdx).contains(position))
            return cellIdx;

    return -1;
}

void VoiceStripUI::mouseMove(const juce::MouseEvent& event)
{
    const auto cellIdx = getCellAt(event.getPosition());
    if (cellIdx != hoveredCell)
    {
        hoveredCell = cellIdx;
        repaint();
    }
}

void VoiceStripUI::mouseExit(const juce::MouseEvent& /*event*/)
{
    hoveredCell = -1;
    repaint();
}

void VoiceStripUI::mouseDown(const juce::MouseEvent& event)
{
    const auto cellIdx = getCellAt(event.getPosition());
    if (cellIdx < 0)
        return;

    monitor.setSelectedChannel(getChannelOf(cellIdx));
    update();
}

void VoiceStripUI::paint(juce::Graphics& g)
{
    auto* look = dynamic_cast<HelixxLookAndFeel*>(&getLookAndFeel());
    if (look == nullptr)
        return;

    for (int cellIdx = 0; cellIdx < static_cast<int>(cells.size()); ++cellIdx)
        look->drawVoiceCell(g, getCellBounds(cellIdx).toFloat(), cells[static_cast<size_t>(cellIdx)], cellIdx == hoveredCell);
}

} // namespace helixx
