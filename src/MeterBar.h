#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "GUIConstants.h"
#include <atomic>

class MeterBar : public juce::Component, private juce::Timer
{
public:
    MeterBar() { startTimerHz (30); }

    void setSource (std::atomic<float>* src) { source_ = src; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        if (level_ > 0.0f)
        {
            float db   = juce::Decibels::gainToDecibels (level_, -60.0f);
            float norm = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -60.0f, 12.0f, 0.0f, 1.0f));

            juce::Colour c;
            if (db < -12.0f)      c = juce::Colour (GC::METER_GREEN);
            else if (db < -6.0f)  c = juce::Colour (GC::METER_YELLOW);
            else if (db < 0.0f)   c = juce::Colour (GC::METER_ORANGE);
            else                  c = juce::Colour (GC::METER_RED);

            g.setColour (c);
            g.fillRoundedRectangle (b.withWidth (b.getWidth() * norm), b.getHeight() * 0.5f);
        }
    }

private:
    void timerCallback() override
    {
        if (source_ == nullptr) return;
        float raw = source_->load (std::memory_order_relaxed);
        // Fast attack, slow release
        level_ = (raw > level_) ? raw : level_ * 0.92f + raw * 0.08f;
        repaint();
    }

    std::atomic<float>* source_ = nullptr;
    float level_ = 0.0f;
};
