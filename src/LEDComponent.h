#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "GUIConstants.h"
#include <atomic>

class LEDComponent : public juce::Component, private juce::Timer
{
public:
    LEDComponent() { startTimerHz (30); }

    void setTrigger (std::atomic<uint32_t>* trig)
    {
        trigger_ = trig;
        if (trig) lastSeen_ = trig->load (std::memory_order_relaxed);
    }

    void setLitColour (juce::Colour c) { litColour_ = c; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        float sz = juce::jmin (b.getWidth(), b.getHeight());
        auto led = juce::Rectangle<float> (sz, sz).withCentre (b.getCentre());

        // Dim inactive glow -> full lit colour on trigger
        juce::Colour dim = litColour_.withMultipliedBrightness (0.35f);
        juce::Colour c   = dim.interpolatedWith (litColour_, brightness_);
        g.setColour (c);
        g.fillEllipse (led);

        // Outline: always visible at fixed brightness, independent of trigger
        g.setColour (litColour_.withMultipliedBrightness (0.65f));
        g.drawEllipse (led.reduced (0.5f), 1.0f);
    }

private:
    void timerCallback() override
    {
        if (trigger_)
        {
            uint32_t current = trigger_->load (std::memory_order_relaxed);
            if (current != lastSeen_)
            {
                lastSeen_ = current;
                brightness_ = 1.0f;
            }
            else
            {
                brightness_ *= 0.75f;  // ~4 frames decay
            }
        }
        else
        {
            brightness_ *= 0.75f;
        }

        repaint();
    }

    std::atomic<uint32_t>* trigger_ = nullptr;
    uint32_t lastSeen_ = 0;
    juce::Colour litColour_ { GC::LED_GREEN };
    float brightness_ = 0.0f;
};
