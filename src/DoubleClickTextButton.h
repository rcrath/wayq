#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "WayQLookAndFeel.h"

// TextButton that fires onDoubleClick when double-clicked.
// After a double-click, suppresses the next onClick to avoid conflicting actions.
class DoubleClickTextButton : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;

    std::function<void()> onDoubleClick;

    // Optional callback: if set and returns true for the given button-local
    // point, the button refuses the hit so the click falls through to the
    // parent. Used to keep an underlying graph element (e.g. the bell peak)
    // grabbable when the button overlaps it.
    std::function<bool(juce::Point<int>)> shouldFallThrough;

    bool hitTest (int x, int y) override
    {
        if (shouldFallThrough && shouldFallThrough ({ x, y }))
            return false;
        return juce::TextButton::hitTest (x, y);
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        suppressNextClick_ = true;
        if (onDoubleClick)
            onDoubleClick();
    }

    void clicked() override
    {
        if (suppressNextClick_)
        {
            suppressNextClick_ = false;
            return;
        }
        juce::TextButton::clicked();
    }

private:
    bool suppressNextClick_ = false;
};

// Variant for the delta / polarity glyphs. Δ (U+0394) and Ø (U+00D8) are drawn
// as crisp VECTOR shapes (an upward triangle for Δ, a circle-with-slash for Ø)
// rather than as font text — the GNU Unifont versions of these glyphs are a
// bitmap face that blocks up and looks pixelated when scaled to button size.
// Any other glyph still falls back to Unifont. Bypasses the LookAndFeel
// drawButtonText path entirely.
class UnifontDoubleClickTextButton : public DoubleClickTextButton
{
public:
    using DoubleClickTextButton::DoubleClickTextButton;

    void paintButton (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override
    {
        auto& lf = getLookAndFeel();
        lf.drawButtonBackground (g, *this,
                                 findColour (getToggleState() ? buttonOnColourId
                                                              : buttonColourId),
                                 shouldDrawButtonAsHighlighted,
                                 shouldDrawButtonAsDown);

        g.setColour (findColour (getToggleState() ? textColourOnId : textColourOffId));

        const auto  txt    = getButtonText();
        const auto  bounds = getLocalBounds().toFloat();
        const float side   = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const float glyph  = side * 0.55f;            // glyph extent
        const float cx     = bounds.getCentreX();
        const float cy     = bounds.getCentreY();
        const float lw     = juce::jmax (1.5f, side * 0.10f);   // stroke width

        if (txt == juce::String::fromUTF8 ("\xce\x94"))            // Δ → triangle outline
        {
            const float h        = glyph;
            const float halfBase = glyph * 0.62f;
            juce::Path tri;
            tri.startNewSubPath (cx,            cy - h * 0.5f);
            tri.lineTo          (cx + halfBase, cy + h * 0.5f);
            tri.lineTo          (cx - halfBase, cy + h * 0.5f);
            tri.closeSubPath();
            g.strokePath (tri, juce::PathStrokeType (lw, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }
        else if (txt == juce::String::fromUTF8 ("\xc3\x98"))       // Ø → circle + slash
        {
            const float r = glyph * 0.5f;
            g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, lw);
            const float s = r * 1.18f;                 // slash overshoots the ring a touch
            g.drawLine (cx - s * 0.7f, cy + s * 0.7f,
                        cx + s * 0.7f, cy - s * 0.7f, lw);
        }
        else if (txt.isNotEmpty())                                 // anything else → Unifont
        {
            g.setFont (getUnifontFont (side * 0.7f));
            g.drawText (txt, getLocalBounds(), juce::Justification::centred, false);
        }
    }
};
