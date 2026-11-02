#include "HelixxLogo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <vector>

namespace helixx
{

namespace
{
    constexpr float PI = juce::MathConstants<float>::pi;

    // The mark, for a height of 1: each strand starts at an extreme and ends at one, a crossing for each half turn
    constexpr float HALF_TURNS       = 6.f;
    constexpr float TIGHTENING       = 0.2f;    // How much faster it turns at the right than at the left: 0 evenly
    constexpr float STRAND_WIDTH     = 0.16f;   // At its nearest
    constexpr float STRAND_FAR_WIDTH = 0.62f;   // At its furthest, for its nearest
    constexpr float STRAND_GAP       = 0.055f;   // Round the strand in front, where they cross
    constexpr float STRAND_CUT_REACH = -0.5f;   // Nearness under which it can be cut: past it, it has left its crossing behind
    constexpr int   STRAND_STEPS     = 480;     // Points along it

    // The name, in units of its capitals' height: letters of bars this thick, this far apart
    constexpr float STROKE   = 0.17f;
    constexpr float H_WIDTH  = 0.64f;
    constexpr float E_WIDTH  = 0.58f;
    constexpr float E_MIDDLE = 0.5f;            // The E's middle bar's length
    constexpr float L_WIDTH  = 0.46f;
    constexpr float X_WIDTH  = 0.74f;

    // The room after each letter but the last: the X's close, their corners leaving more than a stem's edge does
    constexpr std::array<float, 5> TRACKING { 0.27f, 0.27f, 0.21f, 0.21f, 0.09f };

    constexpr float NAME_ERROR = H_WIDTH + E_WIDTH + L_WIDTH + STROKE + 2.f * X_WIDTH
                               + TRACKING[0] + TRACKING[1] + TRACKING[2] + TRACKING[3] + TRACKING[4] - HelixxLogo::NAME_ASPECT;
    static_assert(NAME_ERROR < 1e-4f && NAME_ERROR > -1e-4f, "the letters must be as wide as NAME_ASPECT says");

    using Polygon = std::vector<juce::Point<float>>;

    /** @brief A point of a strand's middle line, and what's needed to go round it. */
    struct StrandPoint
    {
        juce::Point<float> point;
        juce::Point<float> normal;      ///< Square to the line and of length 1
        float nearness;                 ///< 1 in front, -1 behind
        float halfWidth;
    };

    float halfWidthAt(float nearness)
    {
        return 0.5f * STRAND_WIDTH * (STRAND_FAR_WIDTH + (1.f - STRAND_FAR_WIDTH) * 0.5f * (nearness + 1.f));
    }

    /** @brief A strand's point at `along`, 0..1 across the mark. `side` picks the strand: 1 starts at the top, -1 at the bottom. */
    StrandPoint onStrand(float along, float side)
    {
        const float turns = HALF_TURNS * PI;
        const float phase = 0.5f * PI + turns * ((1.f - TIGHTENING) * along + TIGHTENING * along * along);
        const float speed = turns * (1.f - TIGHTENING + 2.f * TIGHTENING * along);
        const float reach = 0.5f - halfWidthAt(0.f);        // At the extremes it's half-way round: neither near nor far
        const float slope = -side * reach * std::cos(phase) * speed / HelixxLogo::MARK_ASPECT;
        const float length = std::hypot(1.f, slope);
        const float nearness = side * std::cos(phase);
        return { { along * HelixxLogo::MARK_ASPECT, 0.5f - side * reach * std::sin(phase) }, { -slope / length, 1.f / length }, nearness, halfWidthAt(nearness) };
    }

    /** @brief The outlines of a strand, in pieces: each broken where it goes behind the other, each edge cut where it comes too close to it. */
    juce::Path makeStrand(float side)
    {
        std::vector<StrandPoint> other(static_cast<size_t>(STRAND_STEPS) + 1);
        for (int step = 0; step <= STRAND_STEPS; ++step)
            other[static_cast<size_t>(step)] = onStrand(static_cast<float>(step) / static_cast<float>(STRAND_STEPS), -side);

        const auto isClear = [&](juce::Point<float> point) {
            return std::all_of(other.begin(), other.end(), [&](const StrandPoint& front) {
                return front.nearness < 0.f || point.getDistanceFrom(front.point) >= front.halfWidth + STRAND_GAP;
            });
        };

        // --- Each edge in runs, along the strand ---
        std::array<std::vector<Polygon>, 2> edges;
        for (size_t edge = 0; edge < edges.size(); ++edge)
        {
            bool wasClear = false;
            for (int step = 0; step <= STRAND_STEPS; ++step)
            {
                const auto strand = onStrand(static_cast<float>(step) / static_cast<float>(STRAND_STEPS), side);
                auto point = strand.point + strand.normal * (edge == 0 ? -strand.halfWidth : strand.halfWidth);
                point.x = juce::jlimit(0.f, HelixxLogo::MARK_ASPECT, point.x);      // Its ends are flat cuts
                const bool clear = strand.nearness >= STRAND_CUT_REACH || isClear(point);
                if (clear && !wasClear)
                    edges[edge].emplace_back();
                if (clear)
                    edges[edge].back().push_back(point);
                wasClear = clear;
            }
        }

        // --- A piece: along one edge, back along the other ---
        juce::Path path;
        for (size_t piece = 0; piece < std::min(edges[0].size(), edges[1].size()); ++piece)
        {
            path.startNewSubPath(edges[0][piece].front());
            for (const auto point : edges[0][piece])
                path.lineTo(point);
            for (auto point = edges[1][piece].rbegin(); point != edges[1][piece].rend(); ++point)
                path.lineTo(*point);
            path.closeSubPath();
        }
        return path;
    }

    /** @brief How wide a slanted bar is across, for it to be `thickness` thick, going `dx` across for `dy` down. */
    float slantedWidth(float thickness, float dx, float dy)
    {
        return thickness * std::hypot(dx, dy) / dy;
    }

    /** @brief A closed outline through `corners`, each moved `x` across. A letter is one outline, never pieces: where they'd meet, their edges would show. */
    void addOutline(juce::Path& path, float x, std::initializer_list<juce::Point<float>> corners)
    {
        path.startNewSubPath(corners.begin()->x + x, corners.begin()->y);
        for (auto corner = corners.begin() + 1; corner != corners.end(); ++corner)
            path.lineTo(corner->x + x, corner->y);
        path.closeSubPath();
    }

    /** @brief The X, from `x`: two bars crossed in the middle. */
    void addX(juce::Path& path, float x)
    {
        // The bars' width across, for them to go across what they leave of the letter's width
        float bar = STROKE;
        for (int i = 0; i < 8; ++i)
            bar = slantedWidth(STROKE, X_WIDTH - bar, 1.f);
        const float middle = 0.5f * X_WIDTH;
        const float notch = (middle - bar) / (X_WIDTH - bar);            // How far down the top's notch is, and up the foot's
        addOutline(path, x, { { 0.f, 0.f }, { bar, 0.f }, { middle, notch }, { X_WIDTH - bar, 0.f }, { X_WIDTH, 0.f }, { 0.5f * (X_WIDTH + bar), 0.5f },
            { X_WIDTH, 1.f }, { X_WIDTH - bar, 1.f }, { middle, 1.f - notch }, { bar, 1.f }, { 0.f, 1.f }, { 0.5f * (X_WIDTH - bar), 0.5f } });
    }

    /** @brief HELIXX, its top left at the origin, its capitals 1 high. */
    juce::Path makeName()
    {
        juce::Path path;
        float x = 0.f;
        size_t letter = 0;
        const auto next = [&](float width) { x += width + (letter < TRACKING.size() ? TRACKING[letter] : 0.f); ++letter; };

        // --- H: two stems and a bar ---
        addOutline(path, x, { { 0.f, 0.f }, { STROKE, 0.f }, { STROKE, 0.5f * (1.f - STROKE) }, { H_WIDTH - STROKE, 0.5f * (1.f - STROKE) },
            { H_WIDTH - STROKE, 0.f }, { H_WIDTH, 0.f }, { H_WIDTH, 1.f }, { H_WIDTH - STROKE, 1.f }, { H_WIDTH - STROKE, 0.5f * (1.f + STROKE) },
            { STROKE, 0.5f * (1.f + STROKE) }, { STROKE, 1.f }, { 0.f, 1.f } });
        next(H_WIDTH);

        // --- E: a stem and three bars ---
        const float middleTop = 0.5f - STROKE * 0.5f, middleBottom = 0.5f + STROKE * 0.5f;
        addOutline(path, x, { { 0.f, 0.f }, { E_WIDTH, 0.f }, { E_WIDTH, STROKE }, { STROKE, STROKE }, { STROKE, middleTop }, { E_MIDDLE, middleTop },
            { E_MIDDLE, middleBottom }, { STROKE, middleBottom }, { STROKE, 1.f - STROKE }, { E_WIDTH, 1.f - STROKE }, { E_WIDTH, 1.f }, { 0.f, 1.f } });
        next(E_WIDTH);

        // --- L: a stem and a bar ---
        addOutline(path, x, { { 0.f, 0.f }, { STROKE, 0.f }, { STROKE, 1.f - STROKE }, { L_WIDTH, 1.f - STROKE }, { L_WIDTH, 1.f }, { 0.f, 1.f } });
        next(L_WIDTH);

        // --- I: a stem ---
        addOutline(path, x, { { 0.f, 0.f }, { STROKE, 0.f }, { STROKE, 1.f }, { 0.f, 1.f } });
        next(STROKE);

        // --- X, X ---
        addX(path, x);
        next(X_WIDTH);
        addX(path, x);
        return path;
    }

    /** @brief What's drawn, made once. */
    struct Shapes
    {
        juce::Path strand      = makeStrand(1.f);
        juce::Path otherStrand = makeStrand(-1.f);
        juce::Path name        = makeName();
    };

    const Shapes& theShapes()
    {
        static const Shapes shapes;
        return shapes;
    }

    /** @brief From the shapes' units, as large as fits in `area` and centred there. */
    juce::AffineTransform fitInto(float width, float height, juce::Rectangle<float> area)
    {
        const float unit = std::min(area.getWidth() / width, area.getHeight() / height);
        const auto placed = area.withSizeKeepingCentre(width * unit, height * unit);
        return juce::AffineTransform::scale(unit).translated(placed.getX(), placed.getY());
    }
}

void HelixxLogo::drawMark(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour strand, juce::Colour otherStrand)
{
    const auto transform = fitInto(MARK_ASPECT, 1.f, area);
    g.setColour(strand);
    g.fillPath(theShapes().strand, transform);
    g.setColour(otherStrand);
    g.fillPath(theShapes().otherStrand, transform);
}

void HelixxLogo::drawName(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour name)
{
    g.setColour(name);
    g.fillPath(theShapes().name, fitInto(NAME_ASPECT, 1.f, area));
}

void HelixxLogo::drawWide(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour strand, juce::Colour otherStrand, juce::Colour name)
{
    // As large as fits, centred
    const float height = std::min(area.getHeight(), area.getWidth() / WIDE_ASPECT);
    auto bounds = area.withSizeKeepingCentre(height * WIDE_ASPECT, height);

    // --- The mark, then the name ---
    drawMark(g, bounds.removeFromLeft(height * MARK_ASPECT), strand, otherStrand);
    drawName(g, bounds.removeFromRight(height * WIDE_NAME_HEIGHT * NAME_ASPECT), name);
}

} // namespace helixx
