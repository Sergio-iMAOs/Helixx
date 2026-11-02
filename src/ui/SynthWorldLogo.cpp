#include "SynthWorldLogo.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <vector>

namespace helixx
{
namespace
{
    constexpr float PI           = juce::MathConstants<float>::pi;
    constexpr float QUARTER_TURN = 0.5f * PI;
    constexpr float FULL_TURN    = 2.f * PI;

    // --- The mark, in units of the planet's radius, the planet's centre the origin ---
    // The ring: a circle round the planet seen from a little above, with a sine wave out of its plane on its near side
    constexpr float WAVE_HEIGHT    = 0.36f;
    constexpr float WAVE_RATE      = 4.3f;     // The wave's phase, for the angle round the orbit: one cycle across the planet's face
    constexpr float WAVE_FADE      = 2.2f;     // How fast the wave dies out towards the orbit's sides: a power of their nearness
    constexpr float ROLL           = -10.f * PI / 180.f;     // The whole orbit tilted, its right side up
    constexpr float RING_WIDTH     = 0.15f;    // At its nearest
    constexpr float RING_FAR_WIDTH = 0.62f;    // At its furthest, for its nearest
    constexpr float RING_STEPS     = 120.f;    // Points along it, per radian round the orbit
    constexpr float RING_OVERLAP   = 0.04f;    // Radians the far half goes on under the near one at each side: no seam between them

    // Shaded: the planet lit from the top left
    constexpr juce::Point<float> LIGHT { -0.38f, -0.42f };
    constexpr float SPHERE_REACH    = 1.55f;   // From the light to where the planet is its darkest
    constexpr float HIGHLIGHT_REACH = 0.75f;   // From the light to where its brightest has faded out
    constexpr float LIGHT_BRIGHTEN  = 1.2f;    // The planet's colour at the light
    constexpr float DARK_DARKEN     = 1.3f;    // Its darkest: darker,
    constexpr float DARK_VIVIDNESS  = 1.6f;    // then further from grey

    // Shaded: the cut, the planet under the wave in the ring's shadow, deepest just under the ring
    constexpr float SHADE_DARKEN      = 1.5f;  // The shadow's colour, from the planet's darkest
    constexpr float SHADE_ALPHA       = 0.36f;
    constexpr float SHADOW_BAND_ALPHA = 0.25f;
    constexpr juce::Point<float> SHADOW_OFFSET { 0.012f, 0.05f };              // Where the ring's shadow falls, from the ring
    constexpr std::array<float, 4> SHADOW_WIDTHS { 0.36f, 0.3f, 0.24f, 0.18f };  // Bands one over the other: soft, darkest in their middle

    // Shaded: the ring going towards the planet's colour as it turns away, then fading as it goes behind
    constexpr float RING_FAR_TINT   = 0.3f;
    constexpr float NEAR_TINT_START = 0.6f;    // Of the way across to where the near half meets the far one: the ring's own colour until there
    constexpr float FAR_FADE_ALPHA  = 0.1f;    // Where the far half goes behind the planet

    constexpr int PLANET_SIDES    = 180;       // The planet as a polygon, where shapes are clipped to it
    constexpr float ENVELOPE_STEP = 0.01f;     // Across, between the points of what a line covers

    // --- The name, in units of its capitals' height: EARL's letters, bars this thick ---
    constexpr float NAME_HEIGHT = 1.25f;       // In the planet's radii
    constexpr float NAME_GAP    = 0.875f;      // From the mark, likewise
    constexpr float STROKE      = 0.17f;
    constexpr float TRACKING    = 0.22f;       // Closer than EARL's four letters: ten that far apart fall into groups
    constexpr float S_WIDTH     = 0.62f;
    constexpr float Y_WIDTH     = 0.8f;
    constexpr float Y_FORK      = 0.6f;        // From the top to where the arms meet outside
    constexpr float N_WIDTH     = 0.7f;
    constexpr float T_WIDTH     = 0.64f;
    constexpr float H_WIDTH     = 0.64f;
    constexpr float W_WIDTH     = 1.44f;       // Two of EARL's A's peaks upside down
    constexpr float O_RADIUS    = 0.5f;
    constexpr float R_WIDTH     = 0.62f;
    constexpr float R_BOWL      = 0.56f;       // From the top to the bowl's bottom
    constexpr float R_LEG       = 0.02f;       // Left of the bowl's centre: where its leg's left edge meets the counter's bottom
    constexpr float L_WIDTH     = 0.46f;
    constexpr float D_WIDTH     = 0.72f;

    // What's added to the tracking after each letter but the last: each pair's space evened out (measured from
    // the letters' edges), the O pulled further under the W's arm
    constexpr std::array<float, 9> KERNING { -0.127f, -0.024f, 0.009f, 0.009f, 0.021f, -0.152f, 0.076f, 0.117f, -0.025f };

    using Polygon = std::vector<juce::Point<float>>;

    // The circle the ring goes round, seen from a little above
    constexpr float ORBIT_RADIUS = 1.5f;
    constexpr float ORBIT_TILT   = 0.2f;       // How far under its centre its nearest point is, for its radius: how far above it's seen from

    /** @brief A point of the ring's middle line, before the roll, and how near it is: 1 in front, -1 behind. */
    struct OrbitPoint
    {
        juce::Point<float> point;
        float nearness;
    };

    OrbitPoint onOrbit(float angle)
    {
        const float nearness = std::cos(angle);
        const float wave = std::pow(std::max(nearness, 0.f), WAVE_FADE) * std::sin(WAVE_RATE * angle);
        return { { ORBIT_RADIUS * std::sin(angle), ORBIT_TILT * ORBIT_RADIUS * nearness - WAVE_HEIGHT * wave }, nearness };
    }

    /** @brief Half the ring's width where it's this near, for its width at its nearest. */
    float halfWidthAt(float nearness, float width)
    {
        return 0.5f * width * (RING_FAR_WIDTH + (1.f - RING_FAR_WIDTH) * 0.5f * (nearness + 1.f));
    }

    /** @brief Points along the ring's middle line between two angles round an orbit (0 its nearest, going right). */
    std::vector<OrbitPoint> alongOrbit(float from, float to)
    {
        const int steps = std::max(2, static_cast<int>(std::ceil(std::abs(to - from) * RING_STEPS)));
        std::vector<OrbitPoint> points;
        points.reserve(static_cast<size_t>(steps) + 1);
        for (int step = 0; step <= steps; ++step)
            points.push_back(onOrbit(from + (to - from) * static_cast<float>(step) / static_cast<float>(steps)));
        return points;
    }

    /** @brief At each point, square to the line and of length 1, on its right going along it (under it, going right). */
    std::vector<juce::Point<float>> normalsOf(const std::vector<OrbitPoint>& line)
    {
        std::vector<juce::Point<float>> normals(line.size());
        for (size_t i = 0; i < line.size(); ++i)
        {
            const auto before = line[i > 0 ? i - 1 : i].point, after = line[std::min(i + 1, line.size() - 1)].point;
            const juce::Point<float> along { after.x - before.x, after.y - before.y };
            const float length = std::max(std::hypot(along.x, along.y), 1e-6f);
            normals[i] = { -along.y / length, along.x / length };
        }
        return normals;
    }

    /** @brief The ring between two angles round an orbit, as an outline: `width` thick where nearest, thinner further away. */
    Polygon ring(float from, float to, float width)
    {
        const auto line = alongOrbit(from, to);
        const auto normals = normalsOf(line);
        Polygon outline(2 * line.size());
        for (size_t i = 0; i < line.size(); ++i)
        {
            const float half = halfWidthAt(line[i].nearness, width);
            outline[i] = { line[i].point.x - normals[i].x * half, line[i].point.y - normals[i].y * half };
            outline[outline.size() - 1 - i] = { line[i].point.x + normals[i].x * half, line[i].point.y + normals[i].y * half };
        }
        return outline;
    }

    /**
     * @brief The edge of what a line covers out to `reach` from each of its points, on one side of it (`side`
     *        -1 its top, 1 its bottom), from its left end to its right, moved by `shift`: what a stroke of the
     *        line would cover, which an offset of it isn't where the line bends tighter than `reach`.
     */
    Polygon envelope(const std::vector<OrbitPoint>& line, juce::Point<float> shift, float reach, float side)
    {
        const float from = line.front().point.x, to = line.back().point.x;
        const int steps = static_cast<int>(std::ceil((to - from) / ENVELOPE_STEP));
        Polygon edge;
        edge.reserve(static_cast<size_t>(steps) + 1);
        for (int step = 0; step <= steps; ++step)
        {
            const float x = from + (to - from) * static_cast<float>(step) / static_cast<float>(steps);
            float furthest = 0.f;
            bool covered = false;
            for (const auto& [point, nearness] : line)
            {
                const float dx = x - point.x;
                if (std::abs(dx) > reach)
                    continue;
                const float y = point.y + side * std::sqrt(reach * reach - dx * dx);
                furthest = !covered ? y : side < 0.f ? std::min(furthest, y) : std::max(furthest, y);
                covered = true;
            }
            if (covered)
                edge.push_back({ x + shift.x, furthest + shift.y });
        }
        return edge;
    }

    Polygon pointsOf(const std::vector<OrbitPoint>& line)
    {
        Polygon points;
        points.reserve(line.size());
        for (const auto& point : line)
            points.push_back(point.point);
        return points;
    }

    /** @brief What's past an edge running across the planet, on its side (`side` -1 over it, 1 under it), as far as past the planet. */
    Polygon beyond(Polygon edge, float side)
    {
        const float leftX = edge.front().x, rightX = edge.back().x;
        edge.push_back({ rightX, side * 3.f });
        edge.push_back({ leftX, side * 3.f });
        return edge;
    }

    /** @brief What a line covers out to half `width` from it, moved by `shift`. */
    Polygon band(const std::vector<OrbitPoint>& line, juce::Point<float> shift, float width)
    {
        auto outline = envelope(line, shift, 0.5f * width, -1.f);
        const auto bottom = envelope(line, shift, 0.5f * width, 1.f);
        outline.insert(outline.end(), bottom.rbegin(), bottom.rend());
        return outline;
    }

    /** @brief What of a shape is inside the planet: each side of the planet's polygon cutting it in turn. */
    Polygon clipToPlanet(Polygon shape)
    {
        for (int side = 0; side < PLANET_SIDES && !shape.empty(); ++side)
        {
            const float from = FULL_TURN * static_cast<float>(side) / PLANET_SIDES, to = FULL_TURN * static_cast<float>(side + 1) / PLANET_SIDES;
            const juce::Point<float> a { std::cos(from), std::sin(from) }, b { std::cos(to), std::sin(to) };
            const auto inside = [&](juce::Point<float> p) { return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x) >= 0.f; };
            const auto crossing = [&](juce::Point<float> p, juce::Point<float> q) {
                const float dp = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
                const float dq = (b.x - a.x) * (q.y - a.y) - (b.y - a.y) * (q.x - a.x);
                const float t = dp / (dp - dq);
                return juce::Point<float> { p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t };
            };

            Polygon kept;
            kept.reserve(shape.size() + 2);
            for (size_t i = 0; i < shape.size(); ++i)
            {
                const auto p = shape[i], q = shape[(i + 1) % shape.size()];
                if (inside(p))
                {
                    kept.push_back(p);
                    if (!inside(q))
                        kept.push_back(crossing(p, q));
                }
                else if (inside(q))
                    kept.push_back(crossing(p, q));
            }
            shape = std::move(kept);
        }
        return shape;
    }

    /** @brief The angle round the orbit, going from `from` towards `towards`, where the ring's middle line first comes within `reach(nearness)` of the planet's centre. */
    template <typename Reach>
    float firstWithin(Reach&& reach, float from, float towards)
    {
        constexpr int steps = 2000;
        for (int step = 1; step <= steps; ++step)
        {
            const float angle = from + (towards - from) * static_cast<float>(step) / static_cast<float>(steps);
            const auto [point, nearness] = onOrbit(angle);
            if (std::hypot(point.x, point.y) < reach(nearness))
                return angle;
        }
        return towards;
    }

    juce::Path toPath(const Polygon& outline)
    {
        juce::Path path;
        if (outline.size() < 3)
            return path;
        path.startNewSubPath(outline.front());
        for (size_t i = 1; i < outline.size(); ++i)
            path.lineTo(outline[i]);
        path.closeSubPath();
        return path;
    }

    // --- The name ---

    /** @brief How wide a slanted bar is across, for it to be `thickness` thick, going `dx` across for `dy` down. */
    float slantedWidth(float thickness, float dx, float dy)
    {
        return thickness * std::hypot(dx, dy) / dy;
    }

    /** @brief The lower of the points where a line, through `point` along `direction`, crosses a circle: as an angle clockwise from 12 o'clock, from 0 to a full turn. */
    float lowerCrossing(juce::Point<float> point, juce::Point<float> direction, juce::Point<float> centre, float radius)
    {
        // |point + s direction - centre| = radius, the root further down
        const juce::Point<float> offset { point.x - centre.x, point.y - centre.y };
        const float a = direction.x * direction.x + direction.y * direction.y;
        const float b = 2.f * (offset.x * direction.x + offset.y * direction.y);
        const float c = offset.x * offset.x + offset.y * offset.y - radius * radius;
        const float root = std::sqrt(std::max(0.f, b * b - 4.f * a * c));
        const float s = direction.y > 0.f ? (-b + root) / (2.f * a) : (-b - root) / (2.f * a);
        const float angle = std::atan2(offset.x + s * direction.x, -(offset.y + s * direction.y));
        return angle < 0.f ? angle + FULL_TURN : angle;
    }

    /** @brief A closed outline through `corners`, each moved `x` across. */
    void addOutline(juce::Path& path, float x, std::initializer_list<juce::Point<float>> corners)
    {
        path.startNewSubPath(corners.begin()->x + x, corners.begin()->y);
        for (auto corner = corners.begin() + 1; corner != corners.end(); ++corner)
            path.lineTo(corner->x + x, corner->y);
        path.closeSubPath();
    }

    /** @brief The S, from `x`: bars at the top, middle and foot, joined by two half-round bowls, one outline. */
    void addS(juce::Path& path, float x)
    {
        const float radius = 0.25f * (1.f + STROKE);   // Two bowls sharing the middle bar make the height
        const float inner = radius - STROKE;
        const juce::Point<float> upper { x + radius, radius }, lower { x + S_WIDTH - radius, 1.f - radius };

        // --- Along the top, round the upper bowl's outside, along the middle bar's underside, round the lower bowl's inside ---
        path.startNewSubPath(x + S_WIDTH, 0.f);
        path.lineTo(upper.x, 0.f);
        path.addCentredArc(upper.x, upper.y, radius, radius, 0.f, 0.f, -PI, false);
        path.lineTo(lower.x, 2.f * radius);
        path.addCentredArc(lower.x, lower.y, inner, inner, 0.f, 0.f, PI, false);

        // --- The foot's end, then back round the lower bowl's outside, along the middle bar's top, round the upper bowl's inside ---
        path.lineTo(x, 1.f - STROKE);
        path.lineTo(x, 1.f);
        path.lineTo(lower.x, 1.f);
        path.addCentredArc(lower.x, lower.y, radius, radius, 0.f, PI, 0.f, false);
        path.lineTo(upper.x, 1.f - 2.f * radius);
        path.addCentredArc(upper.x, upper.y, inner, inner, 0.f, -PI, 0.f, false);
        path.lineTo(x + S_WIDTH, STROKE);
        path.closeSubPath();
    }

    /** @brief The Y, from `x`: EARL's A's peak upside down, its point in a stem. */
    void addY(juce::Path& path, float x)
    {
        const float middle = 0.5f * Y_WIDTH;
        const float arm = slantedWidth(STROKE, middle, Y_FORK);
        const float crotch = Y_FORK * (1.f - 2.f * arm / Y_WIDTH);       // Where the arms' inner edges meet
        const float stemTop = Y_FORK * (1.f - STROKE / Y_WIDTH);         // Where the stem's edges meet the arms' outer ones
        const float stemLeft = middle - 0.5f * STROKE, stemRight = middle + 0.5f * STROKE;
        addOutline(path, x, { { 0.f, 0.f }, { arm, 0.f }, { middle, crotch }, { Y_WIDTH - arm, 0.f }, { Y_WIDTH, 0.f },
            { stemRight, stemTop }, { stemRight, 1.f }, { stemLeft, 1.f }, { stemLeft, stemTop } });
    }

    /** @brief The N, from `x`: two stems, and a diagonal from the left one's top to the right one's foot. */
    void addN(juce::Path& path, float x)
    {
        // The diagonal's width across, for it to go across what it leaves of the letter's width
        float diagonal = STROKE;
        for (int i = 0; i < 8; ++i)
            diagonal = slantedWidth(STROKE, N_WIDTH - diagonal, 1.f);
        const float meetsRight = (N_WIDTH - STROKE - diagonal) / (N_WIDTH - diagonal);
        const float meetsLeft = STROKE / (N_WIDTH - diagonal);
        addOutline(path, x, { { 0.f, 1.f }, { 0.f, 0.f }, { diagonal, 0.f }, { N_WIDTH - STROKE, meetsRight }, { N_WIDTH - STROKE, 0.f },
            { N_WIDTH, 0.f }, { N_WIDTH, 1.f }, { N_WIDTH - diagonal, 1.f }, { STROKE, meetsLeft }, { STROKE, 1.f } });
    }

    /** @brief The W, from `x`: two of EARL's A's peaks upside down, its middle peak as high as the letter. */
    void addW(juce::Path& path, float x)
    {
        const float quarter = 0.25f * W_WIDTH;
        const float stroke = slantedWidth(STROKE, quarter, 1.f);
        const float notch = stroke / quarter;                           // How far the inner corners are from the outer ones, up and down
        addOutline(path, x, { { 0.f, 0.f }, { stroke, 0.f }, { quarter, 1.f - notch }, { 2.f * quarter, 0.f }, { 3.f * quarter, 1.f - notch },
            { W_WIDTH - stroke, 0.f }, { W_WIDTH, 0.f }, { 3.f * quarter, 1.f }, { 2.f * quarter, notch }, { quarter, 1.f } });
    }

    /** @brief The O, from `x`: a ring. */
    void addO(juce::Path& path, float x)
    {
        path.addCentredArc(x + O_RADIUS, O_RADIUS, O_RADIUS, O_RADIUS, 0.f, 0.f, FULL_TURN, true);
        path.closeSubPath();
        path.addCentredArc(x + O_RADIUS, O_RADIUS, O_RADIUS - STROKE, O_RADIUS - STROKE, 0.f, 0.f, FULL_TURN, true);
        path.closeSubPath();
    }

    /** @brief The R, from `x`: one trace, up the stem, round the bowl and down into the leg, which only the bowl touches (EARL's). */
    void addR(juce::Path& path, float x)
    {
        const float bowlRadius = R_BOWL * 0.5f;
        const float innerRadius = bowlRadius - STROKE;
        const juce::Point<float> bowlCentre { x + R_WIDTH - bowlRadius, bowlRadius };
        const float legDy = 1.f - (R_BOWL - STROKE);
        const float legWidth = slantedWidth(STROKE, R_WIDTH - STROKE - (bowlCentre.x - x - R_LEG), legDy);
        const float legDx = R_WIDTH - legWidth - (bowlCentre.x - x - R_LEG);
        const juce::Point<float> legDirection { legDx, legDy };
        const float outerEnd = lowerCrossing({ x + R_WIDTH, 1.f }, legDirection, bowlCentre, bowlRadius);
        const float innerEnd = lowerCrossing({ x + R_WIDTH - legWidth, 1.f }, legDirection, bowlCentre, innerRadius);

        path.startNewSubPath(x, 1.f);
        path.lineTo(x, 0.f);
        path.lineTo(bowlCentre.x, 0.f);
        path.addCentredArc(bowlCentre.x, bowlCentre.y, bowlRadius, bowlRadius, 0.f, 0.f, outerEnd, false);
        path.lineTo(x + R_WIDTH, 1.f);
        path.lineTo(x + R_WIDTH - legWidth, 1.f);
        path.addCentredArc(bowlCentre.x, bowlCentre.y, innerRadius, innerRadius, 0.f, innerEnd, 0.f, false);
        path.lineTo(x + STROKE, STROKE);
        path.lineTo(x + STROKE, 1.f);
        path.closeSubPath();
    }

    /** @brief The D, from `x`: a stem, and a bowl round a counter. */
    void addD(juce::Path& path, float x)
    {
        const float bowlX = x + D_WIDTH - O_RADIUS;
        const float innerRadius = O_RADIUS - STROKE;

        path.startNewSubPath(x, 0.f);
        path.lineTo(bowlX, 0.f);
        path.addCentredArc(bowlX, O_RADIUS, O_RADIUS, O_RADIUS, 0.f, 0.f, PI, false);
        path.lineTo(x, 1.f);
        path.closeSubPath();

        path.startNewSubPath(x + STROKE, STROKE);
        path.lineTo(bowlX, STROKE);
        path.addCentredArc(bowlX, O_RADIUS, innerRadius, innerRadius, 0.f, 0.f, PI, false);
        path.lineTo(x + STROKE, 1.f - STROKE);
        path.closeSubPath();
    }

    /** @brief SYNTHWORLD, its top left at the origin, its capitals 1 high. Returns its width. */
    float addName(juce::Path& path)
    {
        float x = 0.f;
        size_t letter = 0;
        const auto next = [&](float width) { x += width + (letter < KERNING.size() ? TRACKING + KERNING[letter] : 0.f); ++letter; };

        addS(path, x);
        next(S_WIDTH);
        addY(path, x);
        next(Y_WIDTH);
        addN(path, x);
        next(N_WIDTH);
        addOutline(path, x, { { 0.f, 0.f }, { T_WIDTH, 0.f }, { T_WIDTH, STROKE }, { 0.5f * (T_WIDTH + STROKE), STROKE }, { 0.5f * (T_WIDTH + STROKE), 1.f },
            { 0.5f * (T_WIDTH - STROKE), 1.f }, { 0.5f * (T_WIDTH - STROKE), STROKE }, { 0.f, STROKE } });
        next(T_WIDTH);
        addOutline(path, x, { { 0.f, 0.f }, { STROKE, 0.f }, { STROKE, 0.5f * (1.f - STROKE) }, { H_WIDTH - STROKE, 0.5f * (1.f - STROKE) },
            { H_WIDTH - STROKE, 0.f }, { H_WIDTH, 0.f }, { H_WIDTH, 1.f }, { H_WIDTH - STROKE, 1.f }, { H_WIDTH - STROKE, 0.5f * (1.f + STROKE) },
            { STROKE, 0.5f * (1.f + STROKE) }, { STROKE, 1.f }, { 0.f, 1.f } });
        next(H_WIDTH);
        addW(path, x);
        next(W_WIDTH);
        addO(path, x);
        next(2.f * O_RADIUS);
        addR(path, x);
        next(R_WIDTH);
        addOutline(path, x, { { 0.f, 0.f }, { STROKE, 0.f }, { STROKE, 1.f - STROKE }, { L_WIDTH, 1.f - STROKE }, { L_WIDTH, 1.f }, { 0.f, 1.f } });
        next(L_WIDTH);
        addD(path, x);
        next(D_WIDTH);
        return x;
    }

    // --- The shapes, made once ---

    /** @brief What's drawn, in the planet's units, before the orbit's roll (but the bounds, after it). */
    struct Shapes
    {
        // Shaded
        juce::Path nearRing, farRight, farLeft;
        juce::Point<float> rightMeets, rightGoesBehind, leftMeets, leftGoesBehind;   // Where each far side meets the near half and goes behind the planet
        float nearTintEnd = 0.f;                                                      // Across from the centre: where the near half meets the far one
        juce::Path shade;
        std::vector<juce::Path> shadow;      // Bands one over the other, each filled on its own

        juce::Rectangle<float> markBounds;  // Rolled
        juce::Path name;                     // In place beside the mark, not rolled
        juce::Rectangle<float> wideBounds;
    };

    /** @brief The box round the planet and the paths given, once rolled. */
    juce::Rectangle<float> rolledBounds(std::initializer_list<const juce::Path*> paths)
    {
        const auto roll = juce::AffineTransform::rotation(ROLL);
        auto bounds = juce::Rectangle<float>(-1.f, -1.f, 2.f, 2.f);
        for (const auto* path : paths)
            bounds = bounds.getUnion(path->getBoundsTransformed(roll));
        return bounds;
    }

    Shapes makeShapes()
    {
        Shapes shapes;
        const auto nearLine = alongOrbit(-QUARTER_TURN, QUARTER_TURN);

        // --- Shaded: the ring's halves, the far one's sides each fading to where they go behind the planet ---
        shapes.nearRing = toPath(ring(-QUARTER_TURN, QUARTER_TURN, RING_WIDTH));
        shapes.farRight = toPath(ring(QUARTER_TURN - RING_OVERLAP, PI + RING_OVERLAP, RING_WIDTH));
        shapes.farLeft = toPath(ring(PI - RING_OVERLAP, FULL_TURN - QUARTER_TURN + RING_OVERLAP, RING_WIDTH));
        const auto planetEdge = [](float) { return 1.f; };
        shapes.rightMeets = onOrbit(QUARTER_TURN).point;
        shapes.rightGoesBehind = onOrbit(firstWithin(planetEdge, QUARTER_TURN, PI)).point;
        shapes.leftMeets = onOrbit(-QUARTER_TURN).point;
        shapes.leftGoesBehind = onOrbit(firstWithin(planetEdge, -QUARTER_TURN, -PI)).point;
        shapes.nearTintEnd = ORBIT_RADIUS - halfWidthAt(0.f, RING_WIDTH);

        // --- Shaded: the planet under the wave, and the ring's shadow on it ---
        shapes.shade = toPath(clipToPlanet(beyond(pointsOf(nearLine), 1.f)));
        for (const float width : SHADOW_WIDTHS)
            shapes.shadow.push_back(toPath(clipToPlanet(band(nearLine, SHADOW_OFFSET, width))));

        // --- The mark's bounds, rolled: the planet and every part of its ring ---
        shapes.markBounds = rolledBounds({ &shapes.nearRing, &shapes.farRight, &shapes.farLeft });
        const float left = shapes.markBounds.getX(), right = shapes.markBounds.getRight();
        const float top = shapes.markBounds.getY(), bottom = shapes.markBounds.getBottom();

        // --- The name beside it, its middle level with the planet's centre ---
        const float nameWidth = addName(shapes.name) * NAME_HEIGHT;
        const float nameLeft = right + NAME_GAP;
        shapes.name.setUsingNonZeroWinding(false);    // The O's and the D's counters are holes
        shapes.name.applyTransform(juce::AffineTransform::scale(NAME_HEIGHT).translated(nameLeft, -0.5f * NAME_HEIGHT));
        shapes.wideBounds = juce::Rectangle<float>(left, std::min(top, -0.5f * NAME_HEIGHT), nameLeft + nameWidth - left,
                                                   std::max(bottom, 0.5f * NAME_HEIGHT) - std::min(top, -0.5f * NAME_HEIGHT));
        return shapes;
    }

    const Shapes& theShapes()
    {
        static const Shapes shapes = makeShapes();
        return shapes;
    }

    /** @brief Draws from now on in a shape's units, `bounds` of them as large as fits in `area` and centred there. False if nothing would show. */
    bool fitInto(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Rectangle<float> area)
    {
        if (area.isEmpty())
            return false;
        const float unit = std::min(area.getWidth() / bounds.getWidth(), area.getHeight() / bounds.getHeight());
        const auto placed = area.withSizeKeepingCentre(bounds.getWidth() * unit, bounds.getHeight() * unit);
        g.addTransform(juce::AffineTransform::translation(-bounds.getX(), -bounds.getY()).scaled(unit).translated(placed.getX(), placed.getY()));
        return true;
    }

    /** @brief A colour further from the grey of its own brightness. */
    juce::Colour moreVivid(juce::Colour colour, float factor)
    {
        const float grey = (static_cast<float>(colour.getRed()) + static_cast<float>(colour.getGreen()) + static_cast<float>(colour.getBlue())) / 3.f;
        const auto away = [&](uint8_t channel) {
            return static_cast<uint8_t>(std::clamp(grey + (static_cast<float>(channel) - grey) * factor + 0.5f, 0.f, 255.f));
        };
        return juce::Colour(away(colour.getRed()), away(colour.getGreen()), away(colour.getBlue()), colour.getAlpha());
    }

    /** @brief The mark shaded, in the planet's units, centred on the origin. */
    void drawShaded(juce::Graphics& g, juce::Colour planet, juce::Colour ringColour)
    {
        const auto& shapes = theShapes();
        const auto light = planet.brighter(LIGHT_BRIGHTEN);
        const auto dark = moreVivid(planet.darker(DARK_DARKEN), DARK_VIVIDNESS);
        const auto shade = dark.darker(SHADE_DARKEN);
        const auto far = ringColour.interpolatedWith(planet.withAlpha(ringColour.getFloatAlpha()), RING_FAR_TINT);
        const juce::Graphics::ScopedSaveState state(g);
        g.addTransform(juce::AffineTransform::rotation(ROLL));

        // --- The ring's far half: each side fading from where it meets the near half to where it goes behind the planet ---
        g.setGradientFill(juce::ColourGradient(far, shapes.rightMeets, far.withMultipliedAlpha(FAR_FADE_ALPHA), shapes.rightGoesBehind, false));
        g.fillPath(shapes.farRight);
        g.setGradientFill(juce::ColourGradient(far, shapes.leftMeets, far.withMultipliedAlpha(FAR_FADE_ALPHA), shapes.leftGoesBehind, false));
        g.fillPath(shapes.farLeft);

        // --- The planet, lit from the top left ---
        const juce::Rectangle<float> disc(-1.f, -1.f, 2.f, 2.f);
        g.setGradientFill(juce::ColourGradient(planet, LIGHT, dark, { LIGHT.x + SPHERE_REACH, LIGHT.y }, true));
        g.fillEllipse(disc);
        g.setGradientFill(juce::ColourGradient(light, LIGHT, light.withAlpha(0.f), { LIGHT.x + HIGHLIGHT_REACH, LIGHT.y }, true));
        g.fillEllipse(disc);

        // --- The cut: under the wave the planet is in the ring's shadow, deepest just under the ring ---
        g.setColour(shade.withMultipliedAlpha(SHADE_ALPHA));
        g.fillPath(shapes.shade);
        g.setColour(shade.withMultipliedAlpha(SHADOW_BAND_ALPHA));
        for (const auto& shadowBand : shapes.shadow)
            g.fillPath(shadowBand);

        // --- The ring's near half: its own colour in front, the far half's at either side, where they meet ---
        g.setColour(ringColour);
        g.fillPath(shapes.nearRing);
        for (const float side : { -1.f, 1.f })
        {
            g.setGradientFill(juce::ColourGradient(far.withAlpha(0.f), { side * NEAR_TINT_START * shapes.nearTintEnd, 0.f }, far, { side * shapes.nearTintEnd, 0.f }, false));
            g.fillPath(shapes.nearRing);
        }
    }
} // namespace

float SynthWorldLogo::getWideAspect()
{
    return theShapes().wideBounds.getWidth() / theShapes().wideBounds.getHeight();
}

void SynthWorldLogo::drawWide(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour planet, juce::Colour ring)
{
    const auto& shapes = theShapes();
    const juce::Graphics::ScopedSaveState state(g);
    if (!fitInto(g, shapes.wideBounds, area))
        return;

    // --- The mark, then the name ---
    drawShaded(g, planet, ring);
    g.setColour(ring);
    g.fillPath(shapes.name);
}

} // namespace helixx
