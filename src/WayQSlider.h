#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <functional>
#include "DragTooltipProvider.h"

class WayQSlider : public juce::Slider,
                   public DragTooltipProvider
{
public:
    WayQSlider()
    {
        setSliderStyle (juce::Slider::LinearHorizontal);
        setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    }

    void setDefaultValue (double def) { defaultVal_ = def; }
    void setShowDbTooltip (bool show) { showDbTooltip_ = show; }
    void setShowPanTooltip (bool show) { showPanTooltip_ = show; }
    void setTickValue (float v) { tickValue_ = v; repaint(); }
    float getTickValue() const { return tickValue_; }
    bool hasTickMark() const { return tickValue_ >= 0.0f; }

    // Relative-inertia (Mode B) integration. When relativeMode is true, the
    // slider snaps drag positions to one of N evenly-spaced stops along the
    // visible track. The stop index is reported via onRelativeStopChanged so
    // the owner can write it to the matching APVTS choice param. The slider's
    // displayed value is set by the owner (via setValue) so the thumb sits
    // wherever the owner wants to show it (typically D × fraction).
    void setRelativeMode (bool on, int stopCount = 7)
    {
        relativeMode_ = on;
        relativeStops_ = juce::jmax (2, stopCount);
        repaint();
    }
    bool getRelativeMode() const { return relativeMode_; }
    std::function<void (int /*stopIndex*/)> onRelativeStopChanged;
    std::function<juce::String()> tooltipOverride;
    // Used when tooltipOverride is null. Lets owners set a Mode A tooltip
    // without disturbing the Mode B tooltipOverride wiring.
    std::function<juce::String()> defaultTooltipOverride;

    bool isDraggingForTooltip() const override { return dragging_; }

    juce::String getDragTooltip() const override
    {
        if (tooltipOverride)        return tooltipOverride();
        if (defaultTooltipOverride) return defaultTooltipOverride();

        if (showDbTooltip_)
        {
            double val = getValue();
            if (val <= 0.001) return "-inf dB";
            double db = 20.0 * std::log10 (val);
            return juce::String (db, 1) + " dB";
        }

        if (showPanTooltip_)
        {
            double val = getValue();
            int pct = (int) std::round (std::abs (val) * 100.0);
            if (pct == 0) return "0";
            return juce::String (pct) + (val < 0.0 ? "% L" : "% R");
        }

        return juce::String (getValue(), 2);
    }

    juce::String getTooltip() override
    {
        if (dragging_)
            return getDragTooltip();

        auto mousePos = getMouseXYRelative();
        float thumbX = (float) getPositionOfValue (getValue());
        const bool nearThumb = std::abs (mousePos.x - thumbX) <= getHeight() * 0.5f;

        if (nearThumb)
        {
            if (tooltipOverride)         return tooltipOverride();
            if (defaultTooltipOverride)  return defaultTooltipOverride();

            if (showDbTooltip_)
            {
                double val = getValue();
                if (val <= 0.001) return "-inf dB";
                double db = 20.0 * std::log10 (val);
                return juce::String (db, 1) + " dB";
            }

            if (showPanTooltip_)
            {
                double val = getValue();
                int pct = (int) std::round (std::abs (val) * 100.0);
                if (pct == 0) return "0";
                return juce::String (pct) + (val < 0.0 ? "% L" : "% R");
            }
        }

        return juce::Slider::getTooltip();
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        if (relativeMode_)
        {
            // In relative mode, double-click resets to stop 0 (OFF).
            if (onRelativeStopChanged) onRelativeStopChanged (0);
            return;
        }
        setValue (defaultVal_, juce::sendNotificationSync);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragging_ = true;
        if (relativeMode_)
        {
            handleRelativeDrag (e);
            return;
        }
        if (e.mods.isShiftDown())
            setVelocityBasedMode (true);
        else
            setVelocityBasedMode (false);

        juce::Slider::mouseDown (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        dragging_ = false;
        juce::Slider::mouseUp (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (relativeMode_)
        {
            handleRelativeDrag (e);
            return;
        }
        if (e.mods.isShiftDown())
        {
            setVelocityBasedMode (true);
            setVelocityModeParameters (0.1, 1, 0.0, true);
        }
        else
        {
            setVelocityBasedMode (false);
        }
        juce::Slider::mouseDrag (e);
    }

private:
    void handleRelativeDrag (const juce::MouseEvent& e)
    {
        const int w = getWidth();
        if (w <= 0) return;
        const float prop = juce::jlimit (0.0f, 1.0f, (float) e.x / (float) w);
        // Map proportion to nearest stop 0..(N-1) with even spacing.
        const int stop = juce::jlimit (0, relativeStops_ - 1,
                                       (int) std::round (prop * (float) (relativeStops_ - 1)));
        if (onRelativeStopChanged) onRelativeStopChanged (stop);
    }

    double defaultVal_ = 0.0;
    bool showDbTooltip_ = false;
    bool showPanTooltip_ = false;
    float tickValue_ = -1.0f;
    bool relativeMode_ = false;
    int  relativeStops_ = 7;
    bool dragging_ = false;
};
