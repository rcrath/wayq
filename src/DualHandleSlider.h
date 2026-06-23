#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "GUIConstants.h"
#include "DragTooltipProvider.h"

// Two-thumb horizontal slider for HP/LP filter range.
// Each thumb is independently draggable. The fill region between them is drawn.
class DualHandleSlider : public juce::Component,
                         public juce::SettableTooltipClient,
                         public DragTooltipProvider
{
public:
    DualHandleSlider();

    void setRange (double minVal, double maxVal, bool isLog = true);
    void setValues (double hp, double lp, juce::NotificationType notification = juce::sendNotificationSync);

    double getHPValue() const { return hpVal_; }
    double getLPValue() const { return lpVal_; }

    std::function<void()> onHPChanged;
    std::function<void()> onLPChanged;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    juce::String getTooltip() override;

    bool isDraggingForTooltip() const override { return dragging_ != None; }
    juce::String getDragTooltip() const override;

    void applyFafoColors (bool isFafo);

private:
    double valToX (double val) const;
    double xToVal (double x) const;
    int thumbRadius() const;
    void moveActiveThumb (const juce::MouseEvent& e);

    double rangeMin_ = 20.0, rangeMax_ = 20000.0;
    double hpVal_ = 20.0, lpVal_ = 20000.0;
    bool logScale_ = true;

    // Paintable colors — set via applyFafoColors()
    juce::Colour trackBg_     { GC::SLIDER_BG };
    juce::Colour trackBorder_ { GC::SLIDER_BORDER };
    juce::Colour fillColor_   { GC::SLIDER_FILL };
    juce::Colour thumbColor_  { GC::SLIDER_THUMB };

    enum DragTarget { None, HP, LP };
    DragTarget dragging_ = None;
    DragTarget hovered_  = None;
};
