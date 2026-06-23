#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "WayQSlider.h"
#include "GUIConstants.h"

class MonoSliderRow : public juce::Component
{
public:
    MonoSliderRow (const juce::String& labelText,
                   juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramId);

    void resized() override;
    void paint (juce::Graphics& g) override;

    WayQSlider& getSlider() { return slider_; }

private:
    juce::String labelText_;
    WayQSlider slider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att_;
};
