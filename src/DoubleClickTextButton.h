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

// Variant that paints its glyph using GNU Unifont (full BMP coverage) — same
// pattern as UnifontTextButton in TopPanel.h. Bypasses the LookAndFeel
// drawButtonText path entirely so the typeface lookup can't substitute another
// font for glyphs like Δ (U+0394) and Ø (U+00D8).
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
        const float glyphH = (float) juce::jmin (getWidth(), getHeight()) * 0.7f;
        g.setFont (getUnifontFont (glyphH));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);
    }
};
