#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "WayQSlider.h"
#include "MeterBar.h"
#include "LEDComponent.h"
#include "CurveCircle.h"
#include "DoubleClickTextButton.h"
#include "GUIConstants.h"

class StereoSliderRow : public juce::Component,
                        private juce::Timer
{
public:
    StereoSliderRow (const juce::String& labelText,
                     juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& paramIdA,
                     const juce::String& paramIdB,
                     bool showLinkButton = true);

    void resized() override;
    void paint (juce::Graphics& g) override;
    void paintOverChildren (juce::Graphics& g) override;

    // Optional overlays — call after construction
    void addMeterOverlay (std::atomic<float>* srcA, std::atomic<float>* srcB);
    void addLEDOverlay (std::atomic<uint32_t>* trigA, std::atomic<uint32_t>* trigB);
    void setLEDColour (juce::Colour c);

    enum class CurveSide { Left, Right };

    // Add curve-shape circles in the row label area (one of three subcolumns
    // to the left or right of the label). Ch A sits in the top half, Ch B in
    // the bottom half, almost touching at the row centre. Each circle drives
    // the matching curve param in [-1, +1]. When linked, A's circle gangs
    // B's param to match.
    void addCurveCircleOverlay (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String&                  paramIdA,
                                const juce::String&                  paramIdB,
                                CurveCircle::Direction              direction,
                                CurveSide                            side);

    // Link state
    bool isLinked() const { return linked_; }
    void setLinked (bool linked);

    // Apply Sane or FAFO color scheme
    void applyFafoColors (bool isFafo);

    // Remove fill/thumb — background only (let meter overlay do the coloring)
    void applyMeterOnlyStyle();

    // Pan mode: no fill track, center-tick triangles at midpoint of each slider
    void setPanMode (bool enable);

    // Value tick: downward-pointing triangle above sliderA at a specific value
    void setTickAtValue (float value);

    // Called when link state changes — use to notify processor
    std::function<void(bool)> onLinkChanged;

    WayQSlider& getSliderA() { return sliderA_; }
    WayQSlider& getSliderB() { return sliderB_; }

    // Reserve the right-column circle space in the label area without adding
    // an actual curve circle. Causes paint() to use the compact Decay-style
    // layout so an external control (e.g. mode button) can occupy that column.
    void reserveRightColumn() { reserveLabelSpace_ = true; curveSide_ = CurveSide::Right; }

    // Mode B (relative inertia). When enabled for a channel, the slider snaps
    // to 7 stops, writes the fraction choice param instead of behaving as a
    // continuous ms slider, and shows "D × frac = X ms (delay = D ms)" in the
    // tooltip. Pass nullptr fractionParamId to disable for that channel.
    // delayMsSource is the atomic the audio thread updates with the current
    // delay time D for that channel; it drives the slider's displayed value
    // and the tooltip's "delay = D ms" readout.
    void setRelativeModeForChannel (int channel /*0=A,1=B*/,
                                    bool                                on,
                                    juce::AudioProcessorValueTreeState* apvts,
                                    const juce::String&                 fractionParamId,
                                    std::atomic<float>*                 delayMsSource);

private:
    void timerCallback() override;
    void updateRelativeSliderValue (int channel);
    static juce::String formatFraction (int idx);
    static float fractionMultiplier (int idx);

    juce::String labelText_;
    WayQSlider sliderA_, sliderB_;
    DoubleClickTextButton linkBtn_ { "L" };
    bool showLinkBtn_;
    juce::AudioProcessorValueTreeState* apvts_ = nullptr;
    juce::String curveParamIdA_, curveParamIdB_;

    void resetSectionToDefaults();
    bool  linked_      = true;
    bool  fafo_        = false;
    bool  panMode_     = false;
    float tickValue_   = -1.0f;
    float savedBValue_ = 0.0f;
    bool  mirroring_        = false;  // re-entry guard for A↔B mirror while linked
    bool  reserveLabelSpace_ = false;

    std::unique_ptr<MeterBar> meterA_, meterB_;
    std::unique_ptr<LEDComponent> ledA_, ledB_;
    std::unique_ptr<CurveCircle>  curveA_, curveB_;

    juce::String paramIdA_, paramIdB_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attA_, attB_;
    std::unique_ptr<juce::ParameterAttachment> curveAttA_, curveAttB_;
    bool      curveMirroring_   = false;
    float     savedBCurveValue_ = 0.0f;
    CurveSide curveSide_        = CurveSide::Left;

    // Relative-mode (Mode B) wiring — per channel.
    bool                relMode_[2]    = { false, false };
    juce::AudioProcessorValueTreeState* relApvts_[2] = { nullptr, nullptr };
    juce::String        relFracId_[2];
    std::atomic<float>* relDMs_[2]     = { nullptr, nullptr };
    std::unique_ptr<juce::ParameterAttachment> relFracAtt_[2];
    int                 relCurrentIdx_[2] = { 0, 0 };
};
