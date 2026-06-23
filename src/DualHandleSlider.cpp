#include "DualHandleSlider.h"
#include <cmath>

DualHandleSlider::DualHandleSlider()
{
}

void DualHandleSlider::setRange (double minVal, double maxVal, bool isLog)
{
    rangeMin_ = minVal;
    rangeMax_ = maxVal;
    logScale_ = isLog;
    repaint();
}

void DualHandleSlider::setValues (double hp, double lp, juce::NotificationType notification)
{
    hpVal_ = juce::jlimit (rangeMin_, rangeMax_, hp);
    lpVal_ = juce::jlimit (rangeMin_, rangeMax_, lp);

    if (hpVal_ > lpVal_)
        hpVal_ = lpVal_;

    repaint();

    if (notification != juce::dontSendNotification)
    {
        if (onHPChanged) onHPChanged();
        if (onLPChanged) onLPChanged();
    }
}

double DualHandleSlider::valToX (double val) const
{
    auto b = getLocalBounds().toFloat();
    float r = (float) thumbRadius();
    float trackL = b.getX() + r;
    float trackR = b.getRight() - r;

    double norm;
    if (logScale_)
        norm = std::log (val / rangeMin_) / std::log (rangeMax_ / rangeMin_);
    else
        norm = (val - rangeMin_) / (rangeMax_ - rangeMin_);

    return trackL + norm * (trackR - trackL);
}

double DualHandleSlider::xToVal (double x) const
{
    auto b = getLocalBounds().toFloat();
    float r = (float) thumbRadius();
    float trackL = b.getX() + r;
    float trackR = b.getRight() - r;

    double norm = (x - trackL) / (trackR - trackL);
    norm = juce::jlimit (0.0, 1.0, norm);

    if (logScale_)
        return rangeMin_ * std::pow (rangeMax_ / rangeMin_, norm);
    else
        return rangeMin_ + norm * (rangeMax_ - rangeMin_);
}

int DualHandleSlider::thumbRadius() const
{
    return juce::jmax (4, getHeight() * 2 / 5);
}

void DualHandleSlider::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    float r = (float) thumbRadius();

    // Track background
    float trackH = b.getHeight() * 0.6f;
    float trackY = b.getCentreY() - trackH * 0.5f;
    juce::Rectangle<float> track (b.getX() + r, trackY, b.getWidth() - 2 * r, trackH);

    g.setColour (trackBg_);
    g.fillRoundedRectangle (track, 3.0f);
    g.setColour (trackBorder_);
    g.drawRoundedRectangle (track, 3.0f, 1.0f);

    // Fill between HP and LP
    float hpX = (float) valToX (hpVal_);
    float lpX = (float) valToX (lpVal_);
    if (lpX > hpX)
    {
        g.setColour (fillColor_.withAlpha (0.5f));
        g.fillRect (hpX, trackY, lpX - hpX, trackH);
    }

    // HP thumb
    g.setColour (thumbColor_);
    g.fillEllipse (hpX - r, b.getCentreY() - r, r * 2.0f, r * 2.0f);

    // LP thumb
    g.fillEllipse (lpX - r, b.getCentreY() - r, r * 2.0f, r * 2.0f);
}

void DualHandleSlider::applyFafoColors (bool isFafo)
{
    if (isFafo)
    {
        trackBg_     = juce::Colour (GC::FAFO_TRACK_BG);
        trackBorder_ = juce::Colour (GC::FAFO_TRACK_BORDER);
        fillColor_   = juce::Colour (GC::FAFO_THUMB_FILL);
        thumbColor_  = juce::Colour (GC::FAFO_THUMB_FILL);
    }
    else
    {
        trackBg_     = juce::Colour (GC::SLIDER_BG);
        trackBorder_ = juce::Colour (GC::SLIDER_BORDER);
        fillColor_   = juce::Colour (GC::SLIDER_FILL);
        thumbColor_  = juce::Colour (GC::SLIDER_THUMB);
    }
    repaint();
}

void DualHandleSlider::mouseDown (const juce::MouseEvent& e)
{
    float hpX = (float) valToX (hpVal_);
    float lpX = (float) valToX (lpVal_);
    float mx = (float) e.position.x;

    float dHP = std::abs (mx - hpX);
    float dLP = std::abs (mx - lpX);

    dragging_ = (dHP <= dLP) ? HP : LP;
    moveActiveThumb (e);
}

void DualHandleSlider::mouseDrag (const juce::MouseEvent& e)
{
    moveActiveThumb (e);
}

void DualHandleSlider::mouseMove (const juce::MouseEvent& e)
{
    float hpX = (float) valToX (hpVal_);
    float lpX = (float) valToX (lpVal_);
    float mx  = (float) e.position.x;

    float dHP = std::abs (mx - hpX);
    float dLP = std::abs (mx - lpX);
    float r   = (float) thumbRadius();

    float nearest = std::min (dHP, dLP);
    if (nearest > r)
        hovered_ = None;
    else
        hovered_ = (dHP <= dLP) ? HP : LP;
}

void DualHandleSlider::mouseUp (const juce::MouseEvent&)
{
    dragging_ = None;
}

void DualHandleSlider::moveActiveThumb (const juce::MouseEvent& e)
{
    double val = xToVal (e.position.x);

    if (dragging_ == HP)
    {
        hpVal_ = juce::jlimit (rangeMin_, lpVal_, val);
        if (onHPChanged) onHPChanged();
    }
    else if (dragging_ == LP)
    {
        lpVal_ = juce::jlimit (hpVal_, rangeMax_, val);
        if (onLPChanged) onLPChanged();
    }

    repaint();
}

juce::String DualHandleSlider::getTooltip()
{
    if (hovered_ == None)
        return SettableTooltipClient::getTooltip();

    double val = (hovered_ == HP) ? hpVal_ : lpVal_;

    if (val >= 1000.0)
        return juce::String (val / 1000.0, 1) + " kHz";

    return juce::String ((int) val) + " Hz";
}

juce::String DualHandleSlider::getDragTooltip() const
{
    if (dragging_ == None) return {};
    double val = (dragging_ == HP) ? hpVal_ : lpVal_;
    if (val >= 1000.0) return juce::String (val / 1000.0, 1) + " kHz";
    return juce::String ((int) val) + " Hz";
}
