#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DualHandleSlider.h"
#include "DoubleClickTextButton.h"
#include "GUIConstants.h"

class DualHandleFilterRow : public juce::Component
{
public:
    DualHandleFilterRow (const juce::String& labelText,
                         juce::AudioProcessorValueTreeState& apvts,
                         const juce::String& hpIdA, const juce::String& lpIdA,
                         const juce::String& hpIdB, const juce::String& lpIdB);

    void resized() override;
    void paint (juce::Graphics& g) override;

    bool isLinked() const { return linked_; }
    void setLinked (bool linked);

    void applyFafoColors (bool isFafo);

    std::function<void(bool)> onLinkChanged;

    DualHandleSlider& getSliderA() { return sliderA_; }
    DualHandleSlider& getSliderB() { return sliderB_; }

private:
    void syncBtoA();
    void syncAtoB();

    void resetSectionToDefaults();

    juce::String labelText_;
    DualHandleSlider sliderA_, sliderB_;
    DoubleClickTextButton linkBtn_ { "L" };
    bool linked_ = true;
    bool mirroring_ = false;       // re-entry guard for A↔B mirror
    float savedHpB_ = 0.0f;
    float savedLpB_ = 0.0f;

    // Manual parameter binding (DualHandleSlider is not a juce::Slider)
    juce::RangedAudioParameter* hpParamA_ = nullptr;
    juce::RangedAudioParameter* lpParamA_ = nullptr;
    juce::RangedAudioParameter* hpParamB_ = nullptr;
    juce::RangedAudioParameter* lpParamB_ = nullptr;
};
