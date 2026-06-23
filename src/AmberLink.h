/*
  ==============================================================================
    AmberLink.h
    RhythmEcho VST3 Plugin

    Amber-colored hyperlink button family — `AmberLink` is the base class with
    palette-consistent color states and a glow effect; `GetItFreeLink` is the
    concrete top-bar instance pointing at https://way.net/rhythmecho/get. Add
    new amber-link variants here.

    (c) 2026 Rich Rath / Way.Net
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "GUIConstants.h"
#include "WayQLookAndFeel.h"

// Shared amber hyperlink base class — unified visual spec for all clickable
// links in the editor:
//   Plain:  TITLE_ECHO text, subtle glow always on (shows link affordance).
//   Hover:  text brightens, glow strengthens.
//   Click:  text darkens, glow dims (press feel).
// Overrides paintButton entirely so JUCE's default darker() math never fires.
class AmberLink : public juce::HyperlinkButton
{
public:
    AmberLink (const juce::String& text, const juce::URL& url)
        : juce::HyperlinkButton (text, url)
    {
        setColour (juce::HyperlinkButton::textColourId,
                   juce::Colour (GUIConstants::Color::TITLE_ECHO));
        setJustificationType (juce::Justification::centredLeft);
        updateGlow (false, false);
        setComponentEffect (&glow_);
    }

    void paintButton (juce::Graphics& g,
                      bool highlighted,
                      bool down) override
    {
        // Bypass JUCE's default paintButton (which applies .darker(0.4) on
        // hover and .darker(1.3) on press) and drive colour ourselves.
        const juce::Colour base (GUIConstants::Color::TITLE_ECHO);
        const juce::Colour textColor = down      ? base.darker  (0.35f)
                                     : highlighted ? base.brighter (0.35f)
                                                   : base;

        updateGlow (highlighted, down);

        g.setColour (textColor);
        // getAveriaFont must be called HERE (not cached as a member) so it
        // sees the editor's WayQLookAndFeel — which is installed AFTER
        // AmberLink construction. A cached member captures the fallback
        // font, whose wider metrics overflow the layout's reserved width
        // and trigger JUCE's ellipsis truncation ("Get it Fr...").
        g.setFont (getAveriaFont (15.0f));
        g.drawText (getButtonText(), getLocalBounds(),
                    getJustificationType(), true);
    }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        juce::HyperlinkButton::mouseEnter (e);
        repaint();
    }

    void mouseExit (const juce::MouseEvent& e) override
    {
        juce::HyperlinkButton::mouseExit (e);
        repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        juce::HyperlinkButton::mouseDown (e);
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        juce::HyperlinkButton::mouseUp (e);
        repaint();
    }

private:
    void updateGlow (bool highlighted, bool down)
    {
        const juce::Colour base (GUIConstants::Color::TITLE_ECHO);
        if (down)
            glow_.setGlowProperties (1.0f, base.withAlpha (0.20f));
        else if (highlighted)
            glow_.setGlowProperties (4.0f, base.withAlpha (0.45f));
        else
            glow_.setGlowProperties (2.0f, base.withAlpha (0.25f));
    }

    juce::GlowEffect glow_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmberLink)
};

// Clickable amber link that opens the RhythmEcho product page. Sits just
// left of DemoCountdownLabel in the preset bar's middle gap.
class GetItFreeLink : public AmberLink
{
public:
    explicit GetItFreeLink (const juce::URL& url)
        : AmberLink ("Get it Free", url)
    {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GetItFreeLink)
};
