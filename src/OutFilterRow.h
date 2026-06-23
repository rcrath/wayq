#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DoubleClickTextButton.h"
#include "DragTooltipProvider.h"
#include "GUIConstants.h"
#include "OutputFilter.h"
#include "WayQSlider.h"

// Single-row widget replacing the OUT-filter DualHandleFilterRow.
// Layout left-to-right: [Label] [A/B tab stack] [ unified filter cell ] [L]
//
// All five params (HP, mid F, mid G, mid Q, LP) drive one unified visualization
// in a single cell. The cell renders the channel's full filter response — HP/LP
// edges with grab handles, passband shading between, and the bell curve drawn
// inside the passband.
//
// Gestures inside the unified cell:
//   • drag on bell area              → mid Freq (X-axis) + mid Gain (Y-axis)
//   • drag the gold HP / LP handle   → that cutoff
//   • mouse wheel anywhere           → mid Q (option 1: scroll-wheel)
//   • drag on bell's slope sideways  → mid Q (option 4: shoulder-grab)
//   • shift held during drag         → fine-tune (0.1× sensitivity)
//   • double-click on HP/LP handle   → reset that cutoff
//   • double-click anywhere else     → reset bell trio (F, G, Q)
//
// The 5 WayQSliders are kept (so SliderAttachments still bridge values to
// APVTS) but they have setInterceptsMouseClicks(false, false) — all input is
// caught by OutFilterRow's row-level mouse handlers and routed via Slider::
// setValue, which fires the attachment's listener.
//
// One channel's params drive the unified cell at any moment; switching A/B
// tabs swaps the SliderAttachments. When linked, B tab is disabled (grayed)
// and edits to A mirror to B; on link engage we stash B's 5 values; on
// disengage we restore them. Same grammar as existing per-row link buttons.
class OutFilterRow : public juce::Component,
                     public juce::TooltipClient,
                     public DragTooltipProvider,
                     private juce::AudioProcessorValueTreeState::Listener
{
public:
    OutFilterRow (const juce::String& labelText,
                  juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& hpA,          const juce::String& hpB,
                  const juce::String& hpQA,         const juce::String& hpQB,
                  const juce::String& fA,            const juce::String& fB,
                  const juce::String& gA,            const juce::String& gB,
                  const juce::String& qA,            const juce::String& qB,
                  const juce::String& lpA,           const juce::String& lpB,
                  const juce::String& lpQA,          const juce::String& lpQB,
                  const juce::String& deltaA,        const juce::String& deltaB,
                  const juce::String& bassTiltFreqA, const juce::String& bassTiltFreqB,
                  const juce::String& bassTiltGainA, const juce::String& bassTiltGainB,
                  const juce::String& trebTiltFreqA, const juce::String& trebTiltFreqB,
                  const juce::String& trebTiltGainA, const juce::String& trebTiltGainB,
                  const juce::String& eqModeXA,     const juce::String& eqModeXB);
    ~OutFilterRow() override;

    void resized() override;
    void paint (juce::Graphics& g) override;

    void mouseDown        (const juce::MouseEvent& e) override;
    void mouseDrag        (const juce::MouseEvent& e) override;
    void mouseUp          (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    // TooltipClient — position-aware: returns different text for HP / LP
    // handles vs. the bell-peak dot vs. anywhere else in the cell.
    juce::String getTooltip() override;

    // DragTooltipProvider — keep the handle readout visible while a handle is
    // being dragged (stock JUCE hides hover tooltips once a button is down).
    bool isDraggingForTooltip() const override { return dragMode_ != DragMode::None; }
    juce::String getDragTooltip() const override
        { return const_cast<OutFilterRow*> (this)->getTooltip(); }

    bool isLinked() const { return linked_; }
    void setLinked (bool linked);

    // Sane/FAFO color tweak hook (matches existing rows' API).
    void applyFafoColors (bool isFafo);

    std::function<void(bool)> onLinkChanged;

private:
    enum Knob { KHP = 0, KHpQ, KMidF, KMidG, KMidQ, KLP, KLpQ,
                KBassTiltF, KBassTiltG, KTrebTiltF, KTrebTiltG,
                NumKnobs };

    enum class DragMode
    {
        None,
        HpHandle,
        LpHandle,
        BellPosition,
        BellQBracket,
        BassTiltDot,
        TrebTiltDot
    };

    void selectChannel (int channel /* 0 = A, 1 = B */);
    void rebindAttachments();
    void mirrorAtoB();      // when linked, copy A param values into B
    void stashB();          // capture B's 5 current param values
    void restoreB();        // restore B from stashed values

    // Reset helpers (double-click handlers).
    void resetChannelDefaults (int channel);  // tab double-click: reset that channel's 5 params
    void resetSectionToDefaults();            // link double-click: reset both channels
    void resetBellTrio (int channel);         // reset just F, G, Q to defaults
    void resetSingleParam (Knob k, int channel);

    void copyChannelParams (int from, int to);

    // Paint + layout helpers for the unified cell.
    juce::Rectangle<int> cellBounds() const;
    void paintUnifiedFilterCell (juce::Graphics& g, juce::Rectangle<int> r);
    void paintCutoffHandle      (juce::Graphics& g, float x, float y0, float y1,
                                     bool isActive, float dotY);
    void paintBellPeakDot       (juce::Graphics& g, float x, float y);
    void paintTiltDot           (juce::Graphics& g, float x, float y, bool isActive);
    // Draw one half of the yinyang SVG ("yin" gold = bass, "yang" cream = treble)
    // centred on the tilt handle, mirroring RhythmEcho's split/scale/colour.
    void paintYinyangHandle     (juce::Graphics& g, const char* childId, float x, float y);
    void paintResponseCurve     (juce::Graphics& g, juce::Rectangle<float> r,
                                 float hp, float lp, float bellF, float bellG, float bellQ,
                                 float hpQ, float lpQ,
                                 juce::Colour curveColour, float strokeWidth = 1.5f);
    void paintModeXResponseCurve (juce::Graphics& g, juce::Rectangle<float> r,
                                  float bassTiltF, float bassTiltG,
                                  float bellF, float bellG, float bellQ,
                                  float trebTiltF, float trebTiltG,
                                  juce::Colour curveColour, float strokeWidth = 1.5f);
    void paintQBracket          (juce::Graphics& g, float anchorX, float anchorY,
                                 float armPx, int direction, bool isActive);

    DragMode hitZone (juce::Point<int> p, juce::Rectangle<int> cell,
                      float hp, float lp, float bellF, float bellG, float bellQ,
                      float hpQ, float lpQ) const;
    DragMode hitZoneModeX (juce::Point<int> p, juce::Rectangle<int> cell,
                           float bassTiltF, float bassTiltG,
                           float bellF, float bellG, float bellQ,
                           float trebTiltF, float trebTiltG) const;

    static float applyTiltSoftKnee (float rawGain);

    // Map a freq->x and x->freq within the cell using log scale 20 Hz..20 kHz.
    float freqToX (float hz, juce::Rectangle<int> cell) const;
    float xToFreq (int   x,  juce::Rectangle<int> cell) const;
    // Symmetric ±DB_RANGE_DISPLAY mapping.
    float gainToY (float db, juce::Rectangle<int> cell) const;
    float yToGain (int   y,  juce::Rectangle<int> cell) const;

    // Update a slider value and let SliderAttachment write it through to APVTS.
    // Honors linked-mode A→B mirroring via the slider's existing onValueChange.
    void writeKnob (Knob k, float rangeUnits);

    // Apply current visualization clamps so what we paint is what the engine
    // will use: hp ≤ bellF ≤ lp, all within 20..20k.
    static float clampInside (float v, float lo, float hi);

    juce::AudioProcessorValueTreeState& apvts_;
    juce::String labelText_;

    // 0 = A, 1 = B. paramIds_[knob][channel].
    juce::String paramIds_[NumKnobs][2];

    // Delta/polarity param IDs (one per channel). Bound to PID_OUT_FILT_DELTA_A/B.
    juce::String deltaPid_[2];

    // EQ mode X param IDs (one per channel). Bound to PID_OUT_FILT_EQ_MODE_X_A/B.
    juce::String eqModeXPid_[2];

    // AudioProcessorValueTreeState::Listener — picks up preset loads and host
    // automation on the delta params, updating button displays.
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void syncDeltaButtonsFromParam();
    void syncEqModeXButtonFromParam();

    // Baxandall tilt-handle art, loaded once from BinaryData (yinyang_svg).
    std::unique_ptr<juce::Drawable> yinyangDrawable_;

    WayQSlider knobs_[NumKnobs];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach_[NumKnobs];

    DoubleClickTextButton tabA_ { "A" }, tabB_ { "B" };
    DoubleClickTextButton linkBtn_ { "L" };
    UnifontDoubleClickTextButton deltaPolarityBtn_;     // cycles 0=off / 1=Δ delta / 2=Ø polarity — channel A
    UnifontDoubleClickTextButton deltaPolarityBtn_B_;   // channel B (hidden when linked)
    juce::TextButton eqModeXBtnA_ { "P" };        // P=Pass mode, X=tilt mode — channel A
    juce::TextButton eqModeXBtnB_ { "P" };        // channel B (hidden when linked)

    bool linked_     = false;   // constructor's setLinked(true) flips this and runs the full linked-state setup
    int  activeChan_ = 0;     // 0 = A, 1 = B
    float savedB_[NumKnobs] = {};

    bool fafo_ = false;

    DragMode         dragMode_         = DragMode::None;
    juce::Point<int> dragStart_;
    float dragStartHp_         = 0.0f, dragStartLp_    = 0.0f;
    float dragStartBellF_      = 0.0f, dragStartBellG_ = 0.0f, dragStartBellQ_ = 0.0f;
    float dragStartHpQ_        = 0.707f, dragStartLpQ_ = 0.707f;
    float dragStartBassTiltF_  = 0.0f, dragStartBassTiltG_ = 0.0f;
    float dragStartTrebTiltF_  = 0.0f, dragStartTrebTiltG_ = 0.0f;
    int   dragBellQSign_      = 0;
    float dragStartArm_       = 20.0f;
    float dragBracketArmMin_  =  9.0f;
    float dragBracketArmMax_  = 50.0f;
};
