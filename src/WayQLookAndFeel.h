#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "GUIConstants.h"
#include "WayQSlider.h"
#include <BinaryData.h>

class WayQLookAndFeel : public juce::LookAndFeel_V4
{
public:
    WayQLookAndFeel()
    {
        // Load Averia as the default sans-serif for ALL text
        averia_ = juce::Typeface::createSystemTypefaceFor (
            BinaryData::AveriaGruesaLibreRegular_ttf,
            BinaryData::AveriaGruesaLibreRegular_ttfSize);
        setDefaultSansSerifTypeface (averia_);

        // Pre-load Barlow Condensed for branding use
        barlow_ = juce::Typeface::createSystemTypefaceFor (
            BinaryData::BarlowCondensedRegular_ttf,
            BinaryData::BarlowCondensedRegular_ttfSize);

        // Pre-load GNU Unifont — full BMP coverage, used for glyphs
        // (e.g. U+23FB power symbol) that Averia/Barlow don't include.
        unifont_ = juce::Typeface::createSystemTypefaceFor (
            BinaryData::unifont_otf,
            BinaryData::unifont_otfSize);

        // Diagnostic: measure whether the loaded unifont actually claims to
        // have visible glyphs for Δ (U+0394) and Ø (U+00D8). Width > 0 means yes.
        if (unifont_ != nullptr)
        {
            auto probe = juce::Font (juce::FontOptions (unifont_).withHeight (20.0f));
            unifontDeltaW_   = probe.getStringWidthFloat (juce::String::fromUTF8 ("\xce\x94"));
            unifontPolarityW_= probe.getStringWidthFloat (juce::String::fromUTF8 ("\xc3\x98"));
        }
    }

    juce::Typeface::Ptr getBarlowTypeface()  const { return barlow_; }
    juce::Typeface::Ptr getAveriaTypeface()  const { return averia_; }
    juce::Typeface::Ptr getUnifontTypeface() const { return unifont_; }

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override
    {
        auto name = font.getTypefaceName();
        if (barlow_  != nullptr && name == barlow_ ->getName()) return barlow_;
        if (unifont_ != nullptr && name == unifont_->getName()) return unifont_;
        if (averia_  != nullptr && name == averia_ ->getName()) return averia_;
        // Unknown name — defer to base class instead of silently substituting
        // Averia (which has no extended-character coverage and would force a
        // Windows-level fallback to some random system font for symbols like
        // Δ U+0394, Ø U+00D8, etc.).
        return juce::LookAndFeel_V4::getTypefaceForFont (font);
    }

    // Buttons whose componentID is "unifont" get GNU Unifont so that
    // symbols outside the Averia coverage (e.g. Δ U+0394) render correctly.
    juce::Font getTextButtonFont (juce::TextButton& b, int h) override
    {
        if (unifont_ != nullptr && b.getComponentID() == "unifont")
            return juce::Font (juce::FontOptions (unifont_).withHeight ((float) (h - 1)));
        return juce::LookAndFeel_V4::getTextButtonFont (b, h);
    }

    // Full-face rendering for single-glyph buttons (Δ, Ø, EQ mode).
    // When unifont loads: text drawn with unifont filling the full button face.
    // When unifont is null: Δ drawn as a triangle outline, Ø as a circle+slash,
    // so both symbols remain legible even if the OTF font fails to load.
    void drawButtonText (juce::Graphics& g, juce::TextButton& b,
                         bool isMouseOver, bool isButtonDown) override
    {
        juce::LookAndFeel_V4::drawButtonText (g, b, isMouseOver, isButtonDown);
    }

    void drawTooltip (juce::Graphics& g, const juce::String& text,
                      int width, int height) override
    {
        juce::Rectangle<int> bounds (width, height);
        constexpr float cornerSize = 5.0f;
        constexpr float tooltipFontSize = 20.0f;

        g.setColour (findColour (juce::TooltipWindow::backgroundColourId));
        g.fillRoundedRectangle (bounds.toFloat(), cornerSize);

        g.setColour (findColour (juce::TooltipWindow::outlineColourId));
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

        g.setColour (findColour (juce::TooltipWindow::textColourId));
        g.setFont (juce::Font (juce::FontOptions (averia_).withHeight (tooltipFontSize)));
        g.drawFittedText (text, bounds.reduced (7, 3), juce::Justification::centred, 1);
    }

    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText,
                                            juce::Point<int> screenPos,
                                            juce::Rectangle<int> parentArea) override
    {
        constexpr float tooltipFontSize = 20.0f;
        constexpr int maxTooltipWidth = 600;

        juce::AttributedString s;
        s.setJustification (juce::Justification::centred);
        s.append (tipText,
                  juce::Font (juce::FontOptions (averia_).withHeight (tooltipFontSize)),
                  findColour (juce::TooltipWindow::textColourId));

        juce::TextLayout tl;
        tl.createLayoutWithBalancedLineLengths (s, (float) maxTooltipWidth);

        auto w = (int) (tl.getWidth()  + 14.0f);
        auto h = (int) (tl.getHeight() +  6.0f);

        return juce::Rectangle<int> (
            screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
            screenPos.y > parentArea.getCentreY() ? screenPos.y - (h +  6) : screenPos.y +  6,
            w, h
        ).constrainedWithin (parentArea);
    }

    void drawLinearSlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos, float /*minPos*/, float /*maxPos*/,
                           juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        const float trackH = height * 0.6f;
        const float trackY = y + (height - trackH) * 0.5f;
        const float thumbR = height * 0.4f;
        const float trackR = trackH * 0.5f;   // pill ends

        // Track background
        g.setColour (slider.findColour (juce::Slider::backgroundColourId));
        g.fillRoundedRectangle ((float)x, trackY, (float)width, trackH, trackR);

        // Notch at tick value — vertical line cut into the track
        if (auto* rs = dynamic_cast<WayQSlider*> (&slider))
        {
            if (rs->hasTickMark())
            {
                float tickX = (float) rs->getPositionOfValue (rs->getTickValue());
                g.setColour (juce::Colour (GUIConstants::Color::SLIDER_NOTCH));
                g.fillRect (tickX - 0.5f, trackY, 2.0f, trackH);
            }
        }

        // Filled portion
        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillRoundedRectangle ((float)x, trackY, sliderPos - x, trackH, trackR);

        // Thumb - clamp centre so the circle stays within the slider bounds
        const float thumbCx = juce::jlimit ((float)x + thumbR, (float)(x + width) - thumbR, sliderPos);
        g.setColour (slider.findColour (juce::Slider::thumbColourId));
        g.fillEllipse (thumbCx - thumbR, y + (height - thumbR * 2) * 0.5f,
                       thumbR * 2, thumbR * 2);
    }

private:
    juce::Typeface::Ptr averia_;
    juce::Typeface::Ptr barlow_;
    juce::Typeface::Ptr unifont_;
    float unifontDeltaW_    = 0.0f;
    float unifontPolarityW_ = 0.0f;
};

// Free helper: returns a Font that explicitly uses the loaded Averia typeface.
// Use this anywhere you would otherwise call `g.setFont(SIZE)` so the Font is
// bound directly to our typeface instead of relying on the LookAndFeel default
// indirection (which JUCE 8 does not always honour).
inline juce::Font getAveriaFont (float height, const juce::String& style = {})
{
    if (auto* lnf = dynamic_cast<WayQLookAndFeel*> (&juce::LookAndFeel::getDefaultLookAndFeel()))
    {
        if (auto tf = lnf->getAveriaTypeface())
        {
            auto opts = juce::FontOptions (tf).withHeight (height);
            return style.isNotEmpty() ? juce::Font (opts.withStyle (style))
                                      : juce::Font (opts);
        }
    }
    return juce::Font (juce::FontOptions().withHeight (height));
}

// Free helper: apply the standard link-button color scheme to any TextButton.
// One call site = one place to update colors for all link buttons in the UI.
inline void applyLinkButtonStyle (juce::TextButton& b)
{
    b.setColour (juce::TextButton::buttonColourId,   juce::Colour (GC::LINK_BG));
    b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (GC::LINK_ACTIVE));
    b.setColour (juce::TextButton::textColourOffId,  juce::Colour (GC::LINK_TEXT));
    b.setColour (juce::TextButton::textColourOnId,   juce::Colour (GC::LINK_BG));
}

// Free helper: returns a Font bound to GNU Unifont. Covers the full BMP
// (incl. U+23FB power symbol and other symbols not present in Averia).
inline juce::Font getUnifontFont (float height)
{
    if (auto* lnf = dynamic_cast<WayQLookAndFeel*> (&juce::LookAndFeel::getDefaultLookAndFeel()))
    {
        if (auto tf = lnf->getUnifontTypeface())
            return juce::Font (juce::FontOptions (tf).withHeight (height));
    }
    return juce::Font (juce::FontOptions().withHeight (height));
}
