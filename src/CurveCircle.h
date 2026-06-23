#pragma once
// =============================================================================
// CurveCircle.h  --  RhythmEcho fade/swell curve handle
// =============================================================================
// Round handle that overflows the right end of a Fade or Swell slider. Draws
// the actual SigCon curve (env = pow(u, 2^-c)) as a diagonal line inside the
// circle. Dragging the line up/down bows the curve and writes the matching
// fadeCurve* / swellCurve* parameter in [-1, +1].
//
//   Direction::Fade  -- diagonal upper-left -> lower-right (env: 1 -> 0)
//   Direction::Swell -- diagonal lower-left -> upper-right (env: 0 -> 1)
//
// Bow up   = +1 (convex)
// Bow down = -1 (concave)
//
// Drag hit zone is the middle 50% of the chord, with a generous perpendicular
// tolerance so the line is easy to grab. Double-click resets to 0 (linear).
//
// (c) 2026 Rich Rath / Way.Net
// =============================================================================

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include "GUIConstants.h"
#include "DragTooltipProvider.h"

class CurveCircle : public juce::Component,
                    public juce::SettableTooltipClient,
                    public DragTooltipProvider
{
public:
    enum Direction { Fade, Swell };

    explicit CurveCircle (Direction dir) : dir_ (dir)
    {
        setRepaintsOnMouseActivity (false);
        setTooltip ("Shape the curve by dragging the diagonal up or down. Double click resets.");
    }

    float getValue() const { return value_; }

    void setValue (float v, juce::NotificationType notify = juce::sendNotification)
    {
        v = juce::jlimit (-1.0f, 1.0f, v);
        if (std::abs (v - value_) < 1.0e-6f) return;
        value_ = v;
        repaint();
        if (notify == juce::sendNotification && onValueChange)
            onValueChange (value_);
    }

    std::function<void (float)> onValueChange;

    bool isDraggingForTooltip() const override { return dragging_; }
    juce::String getDragTooltip() const override
    {
        if (std::abs (value_) < 0.005f) return "linear";
        int pct = (int) std::round (value_ * 100.0f);
        return (pct > 0 ? "+" : "") + juce::String (pct) + "%";
    }

    bool hitTest (int x, int y) override
    {
        return isInDragZone ({ (float) x, (float) y });
    }

    void paint (juce::Graphics& g) override
    {
        Geom geom = computeGeom();

        juce::Colour outline (GC::FAFO_THUMB_STROKE);
        g.setColour (outline.withMultipliedBrightness (0.65f));
        g.drawEllipse (geom.rect.reduced (0.5f), 1.0f);

        // Symmetric parabolic bow whose peak magnitude matches the SigCon curve's
        // perpendicular distance from the chord at u=0.5. The actual function
        // pow(u, 2^-c) is geometrically asymmetric for c>0 (peak shifts toward
        // u=0); we trade exact shape for a centred visual indicator.
        const float gamma = std::exp2 (-value_);
        const float M     = std::pow (0.5f, gamma) - 0.5f;
        const int   N     = 48;

        juce::Path path;
        for (int i = 0; i <= N; ++i)
        {
            const float t   = (float) i / (float) N;
            const float bow = M * 4.0f * t * (1.0f - t);
            const auto  pt  = geom.p0
                              + (geom.p1 - geom.p0) * t
                              + geom.perpUp * (bow * geom.scale);
            if (i == 0) path.startNewSubPath (pt);
            else        path.lineTo (pt);
        }

        g.setColour (outline);
        g.strokePath (path, juce::PathStrokeType (2.5f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragging_ = isInDragZone (e.position);
        if (dragging_)
            setValue (perpToValue (e.position));
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging_) return;
        setValue (perpToValue (e.position));
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        dragging_ = false;
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        setValue (0.0f);
    }

private:
    struct Geom
    {
        juce::Rectangle<float> rect;
        juce::Point<float>     p0, p1;
        juce::Point<float>     perpUp;
        float                  chordLen;
        float                  scale;     // screen units per plot-space unit
    };

    Geom computeGeom() const
    {
        Geom g {};
        auto b = getLocalBounds().toFloat();
        const float d = juce::jmin (b.getWidth(), b.getHeight());
        g.rect = juce::Rectangle<float> (d, d).withCentre (b.getCentre());

        const float cx = g.rect.getCentreX();
        const float cy = g.rect.getCentreY();
        const float r  = d * 0.5f;
        const float q  = r * 0.70710678f;   // r / sqrt(2)

        if (dir_ == Fade)
        {
            g.p0 = { cx - q, cy - q };   // upper-left perimeter midpoint
            g.p1 = { cx + q, cy + q };   // lower-right perimeter midpoint
        }
        else
        {
            g.p0 = { cx - q, cy + q };   // lower-left
            g.p1 = { cx + q, cy - q };   // upper-right
        }

        const auto chord = g.p1 - g.p0;
        g.chordLen = chord.getDistanceFromOrigin();
        const auto dir = chord / g.chordLen;
        g.perpUp = { -dir.y, dir.x };
        if (g.perpUp.y > 0.0f)
            g.perpUp = -g.perpUp;          // negative y = screen up

        g.scale = g.chordLen / std::sqrt (2.0f);   // unit-square diagonal
        return g;
    }

    bool isInDragZone (juce::Point<float> p) const
    {
        const Geom g = computeGeom();
        if (g.chordLen < 1.0e-3f) return false;

        const auto chordDir = (g.p1 - g.p0) / g.chordLen;
        const auto rel      = p - g.p0;
        const float tpar    = (rel.x * chordDir.x + rel.y * chordDir.y) / g.chordLen;
        if (tpar < 0.25f || tpar > 0.75f) return false;

        const juce::Point<float> perp { -chordDir.y, chordDir.x };
        const float perpDist = std::abs (rel.x * perp.x + rel.y * perp.y);
        return perpDist <= 8.0f;
    }

    float perpToValue (juce::Point<float> p) const
    {
        const Geom g = computeGeom();
        if (g.chordLen < 1.0e-3f) return 0.0f;

        const auto mid = (g.p0 + g.p1) * 0.5f;
        const auto rel = p - mid;
        const float signedPerp = rel.x * g.perpUp.x + rel.y * g.perpUp.y;
        const float bowPlot    = signedPerp / g.scale;

        // bow at u=0.5 = 0.5^(2^-c) - 0.5; invert for c
        const float env = juce::jlimit (1.0e-4f, 1.0f - 1.0e-4f, bowPlot + 0.5f);
        const float c   = -std::log2 (std::log (env) / std::log (0.5f));
        return juce::jlimit (-1.0f, 1.0f, c);
    }

    Direction dir_;
    float value_    = 0.0f;
    bool  dragging_ = false;
};
