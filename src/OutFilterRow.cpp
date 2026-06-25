#include "OutFilterRow.h"
#include "WayQLookAndFeel.h"

namespace { constexpr int kTabWidth = 36; }

// Display constants — kept in one place so paint and hit-test stay consistent.
namespace
{
    constexpr float kDisplayDbRange  = 15.0f;   // ±dB visible vertical span
    constexpr int   kHandleHitPx     = 10;      // grab radius for HP/LP/bell dots
    constexpr int   kHandleDotR      = 5;       // radius of the gold grab dot
    constexpr float kFreqEdgeInsetPx = 10.0f;   // inset of 20 Hz / 20 kHz from cell edges
                                                // — matches how slider thumbs travel
                                                // inside their tracks, not edge-to-edge.

    constexpr float kBracketArmGapPx     =  4.0f;   // min gap from bell dot centre to arm
    constexpr float kBracketArmEdgePadPx =  8.0f;   // min gap from arm tip to cell edge
    constexpr float kBracketHitH         = 12.0f;
    constexpr float kBracketCapH         = 12.0f;
    constexpr float kBracketQPxPerOctave = 70.0f;
    constexpr float kFineSensitivity     = 0.1f;   // shift-held fine-tune multiplier

    // Baxandall (modeX) tilt handles drawn from yinyang.svg.
    constexpr float kYinyangDispR        = 14.0f;   // SVG outer radius 95 → this many screen px
}

// Physical arm limits: from just outside the bell dot to just inside the cell edge.
static void bellArmLimits (float xF, juce::Rectangle<int> cell, float& armMin, float& armMax)
{
    armMin = (float) kHandleDotR + kBracketArmGapPx;
    const float distL = xF - (float) cell.getX();
    const float distR = (float) cell.getRight() - xF;
    const float fullSpan = std::min (distL, distR) - kBracketArmEdgePadPx;
    armMax = fullSpan / 3.0f;
    armMax = std::max (armMax, armMin + 1.0f);
}

// Q → arm length. Inverted: wide bell (low Q) → long arm; narrow (high Q) → short arm.
static float qToArmPx (float q, float qMin, float qMax, float armMin, float armMax)
{
    const float lo   = std::log10 (qMin);
    const float hi   = std::log10 (qMax);
    const float norm = juce::jlimit (0.0f, 1.0f, (std::log10 (q) - lo) / (hi - lo));
    return armMin + (1.0f - norm) * (armMax - armMin);
}

// Arm length → Q (inverse of qToArmPx).
static float armPxToQ (float arm, float qMin, float qMax, float armMin, float armMax)
{
    const float norm = juce::jlimit (0.0f, 1.0f, (arm - armMin) / (armMax - armMin));
    const float lo   = std::log10 (qMin);
    const float hi   = std::log10 (qMax);
    return std::pow (10.0f, hi + norm * (lo - hi));
}

// HP/LP Q → vertical dot y-position within cell. Default Q maps to cell centre.
static float hpLpQToY (float q, juce::Rectangle<int> cell)
{
    const float logDefault = std::log10 (OutputFilter::HP_LP_Q_DEFAULT);
    const float logQ       = std::log10 (std::max (q, OutputFilter::HP_LP_Q_MIN));
    const float logMin     = std::log10 (OutputFilter::HP_LP_Q_MIN);
    const float logMax     = std::log10 (OutputFilter::HP_LP_Q_MAX);
    const float maxRange   = std::max (logDefault - logMin, logMax - logDefault);
    const float cy         = (float) cell.getCentreY();
    const float halfH      = (float) cell.getHeight() * 0.5f;
    const float dy         = -(logQ - logDefault) / maxRange * halfH;
    return juce::jlimit ((float) cell.getY(), (float) cell.getBottom(), cy + dy);
}

OutFilterRow::OutFilterRow (const juce::String& labelText,
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
                            const juce::String& eqModeXA,     const juce::String& eqModeXB)
    : apvts_ (apvts), labelText_ (labelText)
{
    paramIds_[KHP]        [0] = hpA;          paramIds_[KHP]        [1] = hpB;
    paramIds_[KHpQ]       [0] = hpQA;         paramIds_[KHpQ]       [1] = hpQB;
    paramIds_[KMidF]      [0] = fA;           paramIds_[KMidF]      [1] = fB;
    paramIds_[KMidG]      [0] = gA;           paramIds_[KMidG]      [1] = gB;
    paramIds_[KMidQ]      [0] = qA;           paramIds_[KMidQ]      [1] = qB;
    paramIds_[KLP]        [0] = lpA;          paramIds_[KLP]        [1] = lpB;
    paramIds_[KLpQ]       [0] = lpQA;         paramIds_[KLpQ]       [1] = lpQB;
    paramIds_[KBassTiltF] [0] = bassTiltFreqA; paramIds_[KBassTiltF] [1] = bassTiltFreqB;
    paramIds_[KBassTiltG] [0] = bassTiltGainA; paramIds_[KBassTiltG] [1] = bassTiltGainB;
    paramIds_[KTrebTiltF] [0] = trebTiltFreqA; paramIds_[KTrebTiltF] [1] = trebTiltFreqB;
    paramIds_[KTrebTiltG] [0] = trebTiltGainA; paramIds_[KTrebTiltG] [1] = trebTiltGainB;
    deltaPid_[0]    = deltaA;     deltaPid_[1]    = deltaB;
    eqModeXPid_[0] = eqModeXA;   eqModeXPid_[1] = eqModeXB;

    static const char* labels[NumKnobs] = { "HP", "HpQ", "F", "G", "Q", "LP", "LpQ",
                                             "BassTiltF", "BassTiltG", "TrebTiltF", "TrebTiltG" };

    for (int k = 0; k < NumKnobs; ++k)
    {
        knobs_[k].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knobs_[k].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knobs_[k].setName (labels[k]);
        knobs_[k].setComponentID (labels[k]);

        // Default value used by WayQSlider's mouseDoubleClick = setValue(defaultVal_).
        // Read straight from APVTS so it stays in lockstep with createParameterLayout.
        if (auto* p = apvts_.getParameter (paramIds_[k][0]))
            knobs_[k].setDefaultValue (p->convertFrom0to1 (p->getDefaultValue()));

        // Plumbing-only: the row owns all mouse handling for the unified cell.
        knobs_[k].setInterceptsMouseClicks (false, false);

        // Make the slider invisible — it exists only to bridge APVTS values.
        knobs_[k].setColour (juce::Slider::backgroundColourId,         juce::Colours::transparentBlack);
        knobs_[k].setColour (juce::Slider::trackColourId,              juce::Colours::transparentBlack);
        knobs_[k].setColour (juce::Slider::thumbColourId,              juce::Colours::transparentBlack);
        knobs_[k].setColour (juce::Slider::rotarySliderFillColourId,   juce::Colours::transparentBlack);
        knobs_[k].setColour (juce::Slider::rotarySliderOutlineColourId,juce::Colours::transparentBlack);
        knobs_[k].setColour (juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);

        knobs_[k].onValueChange = [this, k]
        {
            if (linked_)
            {
                // mirror to B in real time
                if (auto* p = apvts_.getParameter (paramIds_[k][1]))
                    p->setValueNotifyingHost (
                        p->convertTo0to1 ((float) knobs_[k].getValue()));
            }
            repaint();
        };
        addAndMakeVisible (knobs_[k]);
    }

    // Tab buttons: visually transparent so OutFilterRow::paint() owns rendering.
    // outlineColourId override hides the LookAndFeel default rounded-rect border
    // that's drawn around every TextButton.
    auto styleTab = [] (DoubleClickTextButton& b)
    {
        b.setClickingTogglesState (true);
        b.setColour (juce::TextButton::buttonColourId,    juce::Colours::transparentBlack);
        b.setColour (juce::TextButton::buttonOnColourId,  juce::Colours::transparentBlack);
        b.setColour (juce::TextButton::textColourOffId,   juce::Colours::transparentBlack);
        b.setColour (juce::TextButton::textColourOnId,    juce::Colours::transparentBlack);
        b.setColour (juce::ComboBox::outlineColourId,     juce::Colours::transparentBlack);
    };
    styleTab (tabA_);
    styleTab (tabB_);

    // Bell peak (mid freq+gain thumb) gets z-priority over the channel buttons:
    // when a click lands within the peak's grab radius, the button refuses the
    // hit and the click falls through to OutFilterRow::mouseDown for bell drag.
    auto onBellPeak = [this] (DoubleClickTextButton& btn, juce::Point<int> local) -> bool
    {
        const juce::Point<int> rowPt = local + btn.getBoundsInParent().getPosition();
        const auto cell = cellBounds();
        if (! cell.contains (rowPt)) return false;
        const float bellF = *apvts_.getRawParameterValue (paramIds_[KMidF][activeChan_]);
        const float bellG = *apvts_.getRawParameterValue (paramIds_[KMidG][activeChan_]);
        const int xB = (int) freqToX (bellF, cell);
        const int yB = (int) gainToY (bellG, cell);
        const int dx = rowPt.x - xB, dy = rowPt.y - yB;
        const int r  = kHandleHitPx + kHandleDotR;
        return (dx * dx + dy * dy) <= (r * r);
    };
    tabA_.shouldFallThrough = [this, onBellPeak] (juce::Point<int> p) { return onBellPeak (tabA_, p); };
    tabB_.shouldFallThrough = [this, onBellPeak] (juce::Point<int> p) { return onBellPeak (tabB_, p); };

    tabA_.setRadioGroupId (1001);
    tabB_.setRadioGroupId (1001);
    tabA_.setToggleState (true, juce::dontSendNotification);
    tabA_.onClick = [this] { selectChannel (0); };
    tabB_.onClick = [this] { if (! linked_) selectChannel (1); };
    tabA_.onDoubleClick = [this] { resetChannelDefaults (0); };
    tabB_.onDoubleClick = [this] { if (! linked_) resetChannelDefaults (1); };
    tabA_.setTooltip ("Channel A. Double-click to reset A's filter to defaults.");
    tabB_.setTooltip ("Channel B. Double-click to reset B's filter to defaults.");
    addAndMakeVisible (tabA_);
    addAndMakeVisible (tabB_);

    linkBtn_.setClickingTogglesState (true);
    linkBtn_.setToggleState (linked_, juce::dontSendNotification);
    applyLinkButtonStyle (linkBtn_);
    linkBtn_.setTooltip ("on: link channels. off: restore previous. double click reset just these to their default");
    linkBtn_.onClick = [this]
    {
        setLinked (linkBtn_.getToggleState());
    };
    linkBtn_.onDoubleClick = [this]
    {
        resetSectionToDefaults();
    };
    addAndMakeVisible (linkBtn_);

    // Combined delta/polarity button per channel: single click cycles 0 (off) → 1 (Δ) → 2 (Ø) → 1 → 2...
    // Double-click resets to state 0. B button hidden when linked.
    auto setupDeltaPolBtn = [&] (DoubleClickTextButton& btn, bool isB)
    {
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (GC::FAFO_TRACK_BG));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (GC::TAB_ACTIVE));
        btn.setColour (juce::TextButton::textColourOffId,  juce::Colour (GC::TAB_ACTIVE));
        btn.setColour (juce::TextButton::textColourOnId,   juce::Colour (GC::BG));
        btn.setComponentID ("unifont");
        btn.setTooltip (isB ? "Ch B: click to cycle — delta monitor / polarity flip. Double-click to reset."
                            : "Click to cycle — delta monitor / polarity flip. Double-click to reset normal.");
        addAndMakeVisible (btn);
    };
    setupDeltaPolBtn (deltaPolarityBtn_,   false);
    setupDeltaPolBtn (deltaPolarityBtn_B_, true);

    deltaPolarityBtn_.onClick = [this]
    {
        const int cur  = (int) *apvts_.getRawParameterValue (deltaPid_[0]);
        const int next = (cur == 0) ? 1 : (cur == 1) ? 2 : 1;
        auto write = [this, next] (int chan)
        {
            if (auto* p = apvts_.getParameter (deltaPid_[chan]))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) next));
        };
        write (0);
        if (linked_) write (1);
    };
    deltaPolarityBtn_.onDoubleClick = [this]
    {
        auto write = [this] (int chan)
        {
            if (auto* p = apvts_.getParameter (deltaPid_[chan]))
                p->setValueNotifyingHost (p->convertTo0to1 (0.0f));
        };
        write (0);
        if (linked_) write (1);
    };
    deltaPolarityBtn_B_.onClick = [this]
    {
        const int cur  = (int) *apvts_.getRawParameterValue (deltaPid_[1]);
        const int next = (cur == 0) ? 1 : (cur == 1) ? 2 : 1;
        if (auto* p = apvts_.getParameter (deltaPid_[1]))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) next));
    };
    deltaPolarityBtn_B_.onDoubleClick = [this]
    {
        if (auto* p = apvts_.getParameter (deltaPid_[1]))
            p->setValueNotifyingHost (p->convertTo0to1 (0.0f));
    };

    // EQ mode X toggle (P = HP/LP pass mode, X = tilt mode) — per-channel A+B pair.
    auto setupModeXBtn = [&] (juce::TextButton& btn)
    {
        btn.setClickingTogglesState (true);
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (GC::FAFO_TRACK_BG));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (GC::TAB_ACTIVE));
        btn.setColour (juce::TextButton::textColourOffId,  juce::Colour (GC::TAB_ACTIVE));
        btn.setColour (juce::TextButton::textColourOnId,   juce::Colour (GC::BG));
        btn.setTooltip ("P=HP/LP Pass filters, X=tilt EQ");
        addAndMakeVisible (btn);
    };
    setupModeXBtn (eqModeXBtnA_);
    setupModeXBtn (eqModeXBtnB_);
    eqModeXBtnA_.onClick = [this]
    {
        const bool on = eqModeXBtnA_.getToggleState();
        if (auto* p = apvts_.getParameter (eqModeXPid_[0]))
            p->setValueNotifyingHost (on ? 1.0f : 0.0f);
        if (linked_)
            if (auto* p = apvts_.getParameter (eqModeXPid_[1]))
                p->setValueNotifyingHost (on ? 1.0f : 0.0f);
    };
    eqModeXBtnB_.onClick = [this]
    {
        const bool on = eqModeXBtnB_.getToggleState();
        if (auto* p = apvts_.getParameter (eqModeXPid_[1]))
            p->setValueNotifyingHost (on ? 1.0f : 0.0f);
    };

    // Listen for external changes (preset load, host automation) on both
    // channels' delta and EQ-mode params.
    apvts_.addParameterListener (deltaPid_[0], this);
    apvts_.addParameterListener (deltaPid_[1], this);
    apvts_.addParameterListener (eqModeXPid_[0], this);
    apvts_.addParameterListener (eqModeXPid_[1], this);

    for (int k = 0; k < NumKnobs; ++k)
    {
        apvts_.addParameterListener (paramIds_[k][0], this);
        apvts_.addParameterListener (paramIds_[k][1], this);
    }

    // The non-obvious gesture (Q) gets a persistent painted chip in the cell;
    // see paintUnifiedFilterCell. No row-level tooltip — juce::Component isn't
    // a SettableTooltipClient and the chip is always visible, not hover-only.

    rebindAttachments();
    setLinked (true);
    syncDeltaButtonsFromParam();
    syncEqModeXButtonFromParam();

    yinyangDrawable_ = juce::Drawable::createFromImageData (
        BinaryData::yinyang_svg, (size_t) BinaryData::yinyang_svgSize);
}

juce::Rectangle<int> OutFilterRow::cellBounds() const
{
    const float kW = (float) getWidth() / 626.0f;

    // Symmetric left/right graph margins, both equal to the right "L" margin
    // (the gap from the graph cell's right edge to the window edge, where the
    // link button sits). The 2x2 button block lives inside the left margin
    // (see resized()); the graph fills the wide middle.
    const int cellRight = (int)(GUIConstants::WORKAREA_END * kW);   // unchanged right edge
    const int margin    = getWidth() - cellRight;                   // = L-margin width
    const int cellLeft  = margin;                                   // mirror on the left

    return { cellLeft, 0, cellRight - cellLeft, getHeight() };
}

void OutFilterRow::resized()
{
    const float kW = (float) getWidth() / 626.0f;
    const int workareaH = getHeight();

    // Link button: centred at LINK_CX * kW, vertically centred in the row.
    {
        const int lnkSz = GUIConstants::LINK_BTN_SIZE;
        const int lx    = (int)(GUIConstants::LINK_CX * kW) - lnkSz / 2;
        const int ly    = workareaH / 2 - lnkSz / 2;
        linkBtn_.setBounds (lx, ly, lnkSz, lnkSz);
    }

    // Channel selector buttons (unlinked only): "Ch A" above the cell's vertical
    // centerline, "Ch B" below, with a buffer that keeps the centerline open as
    // a horizontal travel zone for the HP/LP thumb circle.
    {
        auto cb = cellBounds();
        const int marginL = 10;
        const int btnW = (int)(60.0f * kW);
        const int halfH = cb.getHeight() / 2;
        const int centerBuffer = 6;

        if (! linked_)
        {
            tabA_.setVisible (true);
            tabB_.setVisible (true);
            tabA_.setBounds (cb.getX() + marginL, cb.getY() + 2,
                             btnW, halfH - 2 - centerBuffer);
            tabB_.setBounds (cb.getX() + marginL, cb.getY() + halfH + centerBuffer,
                             btnW, halfH - 2 - centerBuffer);
        }
        else
        {
            tabA_.setVisible (false);
            tabB_.setVisible (false);
        }
    }

    // Combined delta/polarity + EQ-mode buttons — 2 rows x 2 columns, sized to
    // fit inside the LEFT graph margin (which equals the right "L" margin width).
    // Shrunk ~50% from the old height/7 squares; the block is centred both ways
    // inside that margin.
    {
        const int margin = cellBounds().getX();         // = left margin width
        const int gap    = juce::jmax (2, (int)(2.0f * kW));
        int sz           = (margin - 3 * gap) / 2;       // two columns + edge padding
        sz               = juce::jmin (sz, workareaH / 8);
        sz               = juce::jmax (sz, 8);

        const int blockW = 2 * sz + gap;
        const int blockH = 2 * sz + gap;
        const int bx     = (margin     - blockW) / 2;    // centre in the left margin
        const int by0    = (workareaH  - blockH) / 2;

        deltaPolarityBtn_.setBounds  (bx,            by0,            sz, sz);
        deltaPolarityBtn_B_.setBounds(bx + sz + gap, by0,            sz, sz);
        eqModeXBtnA_.setBounds       (bx,            by0 + sz + gap, sz, sz);
        eqModeXBtnB_.setBounds       (bx + sz + gap, by0 + sz + gap, sz, sz);

        deltaPolarityBtn_B_.setVisible (! linked_);
        eqModeXBtnB_.setVisible        (! linked_);
    }

    // The 5 sliders are plumbing only — interceptsMouseClicks is off — so
    // their bounds don't matter for input. Park them all on the cell so the
    // attachment value pipeline keeps working in any debug layout pass.
    auto cb = cellBounds();
    for (int k = 0; k < NumKnobs; ++k)
        knobs_[k].setBounds (cb);
}

// ---------------------------------------------------------------------------
// freq/gain mapping helpers (cell-relative)
// ---------------------------------------------------------------------------
float OutFilterRow::freqToX (float hz, juce::Rectangle<int> cell) const
{
    const float lo = std::log10 (OutputFilter::FREQ_MIN);
    const float hi = std::log10 (OutputFilter::FREQ_MAX);
    const float t  = (std::log10 (juce::jlimit (OutputFilter::FREQ_MIN,
                                                OutputFilter::FREQ_MAX, hz)) - lo) / (hi - lo);
    const float xL = (float) cell.getX()      + kFreqEdgeInsetPx;
    const float xR = (float) cell.getRight()  - kFreqEdgeInsetPx;
    return xL + t * (xR - xL);
}

float OutFilterRow::xToFreq (int x, juce::Rectangle<int> cell) const
{
    const float lo = std::log10 (OutputFilter::FREQ_MIN);
    const float hi = std::log10 (OutputFilter::FREQ_MAX);
    const float xL = (float) cell.getX()     + kFreqEdgeInsetPx;
    const float xR = (float) cell.getRight() - kFreqEdgeInsetPx;
    const float t  = juce::jlimit (0.0f, 1.0f, ((float) x - xL) / (xR - xL));
    return std::pow (10.0f, lo + t * (hi - lo));
}

float OutFilterRow::gainToY (float db, juce::Rectangle<int> cell) const
{
    const float cy   = (float) cell.getCentreY();
    const float half = (float) cell.getHeight() * 0.5f;
    return cy - (db / kDisplayDbRange) * half;
}

float OutFilterRow::yToGain (int y, juce::Rectangle<int> cell) const
{
    const float cy   = (float) cell.getCentreY();
    const float half = (float) cell.getHeight() * 0.5f;
    return -((float) y - cy) / half * kDisplayDbRange;
}

float OutFilterRow::clampInside (float v, float lo, float hi)
{
    return std::max (lo, std::min (v, hi));
}

// ---------------------------------------------------------------------------
// paint
// ---------------------------------------------------------------------------
void OutFilterRow::paint (juce::Graphics& g)
{
    const float kW = (float) getWidth() / 626.0f;

    const int workareaX = (int)(GUIConstants::WORKAREA_X * kW);
    const int workareaW = (int)(GUIConstants::WORKAREA_W * kW);
    auto wa = juce::Rectangle<int> (workareaX, 0, workareaW, getHeight()).toFloat();

    // --- Tab fills (only when unlinked).
    // GUI WIP: tabs hidden in unlinked mode; A/B logic still intact, view matches linked.
    if (false && ! linked_)
    {
        const int tabW  = (int)(kTabWidth * kW);
        const int halfH = getHeight() / 2;

        auto tabARect = juce::Rectangle<int> (workareaX, 0, tabW, halfH).toFloat();
        auto tabBRect = juce::Rectangle<int> (workareaX, halfH, tabW, getHeight() - halfH).toFloat();

        if (activeChan_ == 0)
        {
            g.setColour (juce::Colour (GC::FRAC_SELECT));
            g.fillRect (tabARect);
            g.setColour (juce::Colour (GC::FRAC_BG));
            g.fillRect (tabBRect);
        }
        else
        {
            g.setColour (juce::Colour (GC::FRAC_BG));
            g.fillRect (tabARect);
            g.setColour (juce::Colour (GC::FRAC_SELECT));
            g.fillRect (tabBRect);
        }

        // Horizontal divider between A and B.
        g.setColour (juce::Colour (GC::FAFO_TEXT));
        g.fillRect ((float)workareaX, (float)halfH - 0.5f, (float)tabW, 1.0f);

        // Vertical divider between tab column and unified cell.
        g.fillRect ((float)(workareaX + tabW) - 0.5f, wa.getY(), 1.0f, wa.getHeight());

        // Tab letters.
        g.setFont (getAveriaFont (22.0f));
        for (int t = 0; t < 2; ++t)
        {
            auto& r = (t == 0) ? tabARect : tabBRect;
            const char* s = (t == 0) ? "A" : "B";
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawText (s, r.translated (1.0f, 1.0f).toNearestInt(),
                        juce::Justification::centred, false);
            g.setColour (juce::Colour (GC::FRAC_TEXT));
            g.drawText (s, r.toNearestInt(), juce::Justification::centred, false);
        }
    }

    // --- Unified filter cell.
    paintUnifiedFilterCell (g, cellBounds());

    // --- Channel selector labels (unlinked only). Active = glowing FAFO_TEXT
    //     gold; inactive = dim but readable. Click-catcher buttons (tabA_/tabB_)
    //     are positioned over these in resized().
    if (! linked_)
    {
        auto cell = cellBounds();
        const auto font = getAveriaFont (GF::SUB_LABEL_SIZE);
        const int marginL = 10;
        const float fontSize = (float) GF::SUB_LABEL_SIZE;
        const float xL = (float) (cell.getX() + marginL);
        const float halfH = (float) cell.getHeight() * 0.5f;
        // Each label sits at the vertical center of its half (upper / lower),
        // leaving the centerline clear for the HP/LP thumb circle's travel zone.
        const float yA = (float) cell.getY() + halfH * 0.5f + fontSize * 0.35f;
        const float yB = (float) cell.getY() + halfH * 1.5f + fontSize * 0.35f;

        const juce::Colour chALabelCol = juce::Colour (GC::TITLE_RHYTHM).brighter (0.15f);
        const juce::Colour chBLabelCol = juce::Colour (GC::FAFO_TEXT);

        auto drawCh = [&] (const juce::String& txt, float yBaseline, bool active, juce::Colour activeCol)
        {
            juce::GlyphArrangement ga;
            ga.addLineOfText (font, txt, xL, yBaseline);
            juce::Path path;
            ga.createPath (path);
            if (active)
            {
                juce::DropShadow ds (activeCol.withAlpha (0.85f),
                                     6, juce::Point<int> (0, 0));
                ds.drawForPath (g, path);
                g.setColour (activeCol);
            }
            else
            {
                g.setColour (juce::Colour (GC::LABEL).withAlpha (0.55f));
            }
            g.fillPath (path);
        };

        drawCh ("Ch A", yA, activeChan_ == 0, chALabelCol);
        drawCh ("Ch B", yB, activeChan_ == 1, chBLabelCol);
    }

    // --- Active outline (only when unlinked): single closed path in FAFO_TEXT gold.
    // GUI WIP: outline hidden in unlinked mode (matches linked view).
    if (false && ! linked_)
    {
        const float tabW = (float)(int)(kTabWidth * kW);
        const float r    = 3.0f;

        juce::Path outline;

        if (activeChan_ == 0)
        {
            outline.startNewSubPath (wa.getX() + r,    wa.getY());
            outline.lineTo          (wa.getRight(),    wa.getY());
            outline.lineTo          (wa.getRight(),    wa.getBottom());
            outline.lineTo          (wa.getX() + tabW, wa.getBottom());
            outline.lineTo          (wa.getX() + tabW, wa.getY() + wa.getHeight() * 0.5f);
            outline.lineTo          (wa.getX() + r,    wa.getY() + wa.getHeight() * 0.5f);
            outline.quadraticTo     (wa.getX(), wa.getY() + wa.getHeight() * 0.5f,
                                     wa.getX(), wa.getY() + wa.getHeight() * 0.5f - r);
            outline.lineTo          (wa.getX(), wa.getY() + r);
            outline.quadraticTo     (wa.getX(), wa.getY(),
                                     wa.getX() + r, wa.getY());
        }
        else
        {
            outline.startNewSubPath (wa.getX() + r,    wa.getBottom());
            outline.lineTo          (wa.getRight(),    wa.getBottom());
            outline.lineTo          (wa.getRight(),    wa.getY());
            outline.lineTo          (wa.getX() + tabW, wa.getY());
            outline.lineTo          (wa.getX() + tabW, wa.getY() + wa.getHeight() * 0.5f);
            outline.lineTo          (wa.getX() + r,    wa.getY() + wa.getHeight() * 0.5f);
            outline.quadraticTo     (wa.getX(), wa.getY() + wa.getHeight() * 0.5f,
                                     wa.getX(), wa.getY() + wa.getHeight() * 0.5f + r);
            outline.lineTo          (wa.getX(), wa.getBottom() - r);
            outline.quadraticTo     (wa.getX(), wa.getBottom(),
                                     wa.getX() + r, wa.getBottom());
        }

        outline.closeSubPath();
        g.setColour (juce::Colour (GC::FAFO_TEXT));
        g.strokePath (outline, juce::PathStrokeType (1.0f));
    }
}

// ---------------------------------------------------------------------------
// Unified-cell painter
// ---------------------------------------------------------------------------
void OutFilterRow::paintUnifiedFilterCell (juce::Graphics& g, juce::Rectangle<int> r)
{
    if (r.isEmpty()) return;

    // Inner strip: where the freq range 20 Hz..20 kHz actually maps. Outside
    // this (the kFreqEdgeInsetPx margins on each side) we paint nothing, so
    // the parent panel background shows through — same visual idiom as the
    // standard slider rows where the thumb-travel range insets from the row.
    const juce::Rectangle<int>   inner   = r.reduced ((int) kFreqEdgeInsetPx, 0);
    const juce::Rectangle<float> innerF  = inner.toFloat();
    auto rf = r.toFloat();
    const float cy = innerF.getCentreY();

    const int inactiveChan = 1 - activeChan_;
    const bool modeX   = *apvts_.getRawParameterValue (eqModeXPid_[activeChan_]) > 0.5f;

    const float hpHz   = *apvts_.getRawParameterValue (paramIds_[KHP]        [activeChan_]);
    const float hpQ    = *apvts_.getRawParameterValue (paramIds_[KHpQ]       [activeChan_]);
    const float lpHz   = *apvts_.getRawParameterValue (paramIds_[KLP]        [activeChan_]);
    const float lpQ    = *apvts_.getRawParameterValue (paramIds_[KLpQ]       [activeChan_]);
    const float bellF  = *apvts_.getRawParameterValue (paramIds_[KMidF]      [activeChan_]);
    const float bellG  = *apvts_.getRawParameterValue (paramIds_[KMidG]      [activeChan_]);
    const float bellQ  = *apvts_.getRawParameterValue (paramIds_[KMidQ]      [activeChan_]);
    const float bsTF   = *apvts_.getRawParameterValue (paramIds_[KBassTiltF] [activeChan_]);
    const float bsTG   = *apvts_.getRawParameterValue (paramIds_[KBassTiltG] [activeChan_]);
    const float trTF   = *apvts_.getRawParameterValue (paramIds_[KTrebTiltF] [activeChan_]);
    const float trTG   = *apvts_.getRawParameterValue (paramIds_[KTrebTiltG] [activeChan_]);

    const float hpHzInactive  = *apvts_.getRawParameterValue (paramIds_[KHP]        [inactiveChan]);
    const float hpQInactive   = *apvts_.getRawParameterValue (paramIds_[KHpQ]       [inactiveChan]);
    const float lpHzInactive  = *apvts_.getRawParameterValue (paramIds_[KLP]        [inactiveChan]);
    const float lpQInactive   = *apvts_.getRawParameterValue (paramIds_[KLpQ]       [inactiveChan]);
    const float bellFInactive = *apvts_.getRawParameterValue (paramIds_[KMidF]      [inactiveChan]);
    const float bellGInactive = *apvts_.getRawParameterValue (paramIds_[KMidG]      [inactiveChan]);
    const float bellQInactive = *apvts_.getRawParameterValue (paramIds_[KMidQ]      [inactiveChan]);
    const bool  modeXInactive = *apvts_.getRawParameterValue (eqModeXPid_[inactiveChan]) > 0.5f;
    const float bsTFInactive  = *apvts_.getRawParameterValue (paramIds_[KBassTiltF] [inactiveChan]);
    const float bsTGInactive  = *apvts_.getRawParameterValue (paramIds_[KBassTiltG] [inactiveChan]);
    const float trTFInactive  = *apvts_.getRawParameterValue (paramIds_[KTrebTiltF] [inactiveChan]);
    const float trTGInactive  = *apvts_.getRawParameterValue (paramIds_[KTrebTiltG] [inactiveChan]);

    // Channel colors: Chan A = teal, Chan B = gold
    const juce::Colour chanAColour = juce::Colour (GC::TITLE_RHYTHM);
    const juce::Colour chanBColour = juce::Colour (GC::TITLE_ECHO);
    const juce::Colour activeColour   = (activeChan_ == 0) ? chanAColour : chanBColour;
    const juce::Colour inactiveColour = ((activeChan_ == 0) ? chanBColour : chanAColour).withAlpha (0.65f);

    // Stroke widths: active channel 2× thicker
    constexpr float activeStroke   = 3.0f;
    constexpr float inactiveStroke = 1.5f;

    // Background at 60% opacity — parent panel shows through at 40%.
    g.setColour (juce::Colour (GC::FRAC_BG).withAlpha (0.60f));
    g.fillRect (innerF);

    // Passband shading: mode P only — shade OUTSIDE the active band so the
    // active range stays clear and each dot associates with its filter's range.
    if (! modeX)
    {
        const float xHp = freqToX (hpHz, r);
        const float xLp = freqToX (lpHz, r);
        g.setColour (juce::Colour (GC::FRAC_SELECT));
        // Left region (below HP cutoff)
        g.fillRect (innerF.getX(), innerF.getY(), xHp - innerF.getX(), innerF.getHeight());
        // Right region (above LP cutoff)
        g.fillRect (xLp, innerF.getY(), innerF.getRight() - xLp, innerF.getHeight());
    }

    // Faint 0 dB centerline — clipped to the inner strip.
    g.setColour (juce::Colour (GC::FAFO_TEXT).withAlpha (0.18f));
    g.fillRect (innerF.getX(), cy - 0.5f, innerF.getWidth(), 1.0f);

    // --- Inactive channel ---
    if (modeXInactive)
    {
        paintModeXResponseCurve (g, innerF, bsTFInactive, bsTGInactive,
                                 bellFInactive, bellGInactive, bellQInactive,
                                 trTFInactive, trTGInactive,
                                 inactiveColour, inactiveStroke);
        paintTiltDot (g, freqToX (bsTFInactive, r), gainToY (bsTGInactive, r), false);
        paintTiltDot (g, freqToX (trTFInactive, r), gainToY (trTGInactive, r), false);
    }
    else
    {
        paintResponseCurve (g, innerF, hpHzInactive, lpHzInactive, bellFInactive, bellGInactive, bellQInactive,
                            hpQInactive, lpQInactive,
                            inactiveColour, inactiveStroke);
        const float hpXI   = freqToX (hpHzInactive, r);
        const float lpXI   = freqToX (lpHzInactive, r);
        const float hpDotI = hpLpQToY (hpQInactive, r);
        const float lpDotI = hpLpQToY (lpQInactive, r);
        paintCutoffHandle (g, hpXI, innerF.getY(), innerF.getBottom(), false, hpDotI);
        paintCutoffHandle (g, lpXI, innerF.getY(), innerF.getBottom(), false, lpDotI);
    }
    {
        const float xFI = freqToX (bellFInactive, r);
        float bArmMinI, bArmMaxI;
        bellArmLimits (xFI, r, bArmMinI, bArmMaxI);
        const float bArmI = qToArmPx (bellQInactive, OutputFilter::MID_Q_MIN, OutputFilter::MID_Q_MAX, bArmMinI, bArmMaxI);
        paintQBracket (g, xFI, gainToY (bellGInactive, r), bArmI, +1, false);
        paintQBracket (g, xFI, gainToY (bellGInactive, r), bArmI, -1, false);
    }

    // --- Active channel: bell halo (shared between modes) ---
    {
        const float xF      = freqToX (bellF, r);
        const float qNorm   = juce::jlimit (0.0f, 1.0f,
                                            (std::log10 (bellQ) - std::log10 (OutputFilter::MID_Q_MIN))
                                          / (std::log10 (OutputFilter::MID_Q_MAX) - std::log10 (OutputFilter::MID_Q_MIN)));
        const float haloHalfW = (1.0f - qNorm) * 0.45f * innerF.getWidth() + 4.0f;

        juce::ColourGradient grad (
            juce::Colour (GC::FREQ_BRIGHT).withAlpha (0.0f),  xF - haloHalfW, cy,
            juce::Colour (GC::FREQ_BRIGHT).withAlpha (0.0f),  xF + haloHalfW, cy,
            false);
        grad.addColour (0.5, juce::Colour (GC::FREQ_BRIGHT).withAlpha (0.55f));
        g.setGradientFill (grad);
        g.fillRect (juce::Rectangle<float> (xF - haloHalfW, innerF.getY(),
                                            2.0f * haloHalfW, innerF.getHeight())
                    .getIntersection (innerF));

        g.setColour (juce::Colour (GC::FAFO_TEXT).withAlpha (0.65f));
        g.fillRect (xF - 0.5f, innerF.getY(), 1.0f, innerF.getHeight());
    }

    // --- Active channel: response curve, handles/dots ---
    if (modeX)
    {
        paintModeXResponseCurve (g, innerF, bsTF, bsTG, bellF, bellG, bellQ, trTF, trTG,
                                 activeColour, activeStroke);
        paintYinyangHandle (g, "yin",  freqToX (bsTF, r), gainToY (bsTG, r));   // bass = gold yin
        paintYinyangHandle (g, "yang", freqToX (trTF, r), gainToY (trTG, r));   // treble = cream yang
    }
    else
    {
        paintResponseCurve (g, innerF, hpHz, lpHz, bellF, bellG, bellQ,
                            hpQ, lpQ,
                            activeColour, activeStroke);
        const float hpDotY = hpLpQToY (hpQ, r);
        const float lpDotY = hpLpQToY (lpQ, r);
        paintCutoffHandle (g, freqToX (hpHz, r), innerF.getY(), innerF.getBottom(), true, hpDotY);
        paintCutoffHandle (g, freqToX (lpHz, r), innerF.getY(), innerF.getBottom(), true, lpDotY);
    }

    // Bell-peak dot and Q bracket (both modes)
    paintBellPeakDot (g, freqToX (bellF, r), gainToY (bellG, r));

    {
        const float xFActive = freqToX (bellF, r);
        float bellArmMin, bellArmMax;
        bellArmLimits (xFActive, r, bellArmMin, bellArmMax);
        const float bellArm = qToArmPx (bellQ, OutputFilter::MID_Q_MIN, OutputFilter::MID_Q_MAX, bellArmMin, bellArmMax);
        paintQBracket (g, xFActive, gainToY (bellG, r), bellArm, +1, true);
        paintQBracket (g, xFActive, gainToY (bellG, r), bellArm, -1, true);
    }

    // Decade markers along the bottom.
    {
        const float labelHeight = (float) GF::SUB_LABEL_SIZE * 0.55f;
        g.setColour (juce::Colour (GC::FRAC_TEXT).withAlpha (0.65f));
        g.setFont (getAveriaFont (labelHeight));
        for (float f : { 100.0f, 1000.0f, 10000.0f })
        {
            const float fx = freqToX (f, r);
            g.fillRect (fx, innerF.getBottom() - 5.0f, 1.0f, 5.0f);
            const juce::String s = (f >= 1000.0f) ? juce::String ((int)(f / 1000.0f)) + "k"
                                                  : juce::String ((int) f);
            g.drawText (s, juce::Rectangle<float> (fx + 2.0f, innerF.getBottom() - 16.0f, 32.0f, 12.0f).toNearestInt(),
                        juce::Justification::left, false);
        }
    }

    // Gain ticks (-12 / -6 / +6 / +12) — tucked against the inner strip's right edge.
    {
        const float labelHeight = (float) GF::SUB_LABEL_SIZE * 0.55f;
        g.setColour (juce::Colour (GC::LABEL).withAlpha (0.65f));
        g.setFont (getAveriaFont (labelHeight));
        for (int db : { -12, -6, 6, 12 })
        {
            const float py = gainToY ((float) db, r);
            const juce::String s = (db > 0 ? juce::String ("+") : juce::String()) + juce::String (db);
            g.drawText (s, juce::Rectangle<float> (innerF.getRight() - 24.0f, py - 5.0f, 22.0f, 11.0f).toNearestInt(),
                        juce::Justification::right, false);
        }
    }

    juce::ignoreUnused (rf);

    // (Q-gesture hint chip removed — tooltips on the dots now carry the
    // discoverability cost; persistent chip was visual noise.)
}

float OutFilterRow::applyTiltSoftKnee (float rawGain)
{
    constexpr float kLinearLimit = 9.0f;
    constexpr float kHardLimit   = 12.0f;
    constexpr float kKneeWidth   = kHardLimit - kLinearLimit;  // 3 dB
    const float sign = (rawGain >= 0.0f) ? 1.0f : -1.0f;
    const float abs  = std::abs (rawGain);
    if (abs <= kLinearLimit)
        return rawGain;
    const float over = abs - kLinearLimit;
    return sign * (kLinearLimit + kKneeWidth * (1.0f - std::exp (-over / kKneeWidth)));
}

void OutFilterRow::paintTiltDot (juce::Graphics& g, float x, float y, bool isActive)
{
    const float r    = isActive ? (float) kHandleDotR : (float) kHandleDotR * 0.75f;
    const float alpha = isActive ? 1.0f : 0.6f;
    g.setColour (juce::Colour (GC::FAFO_TEXT).withAlpha (alpha));
    g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
    // Horizontal line through centre to distinguish from bell dot
    g.setColour (juce::Colour (GC::FRAC_BG).withAlpha (alpha));
    g.fillRect (x - r + 1.5f, y - 1.0f, r * 2.0f - 3.0f, 2.0f);
}

void OutFilterRow::paintYinyangHandle (juce::Graphics& g, const char* childId, float x, float y)
{
    if (yinyangDrawable_ == nullptr)
        return;

    // Scale: SVG outer radius 95 → kYinyangDispR screen pixels. Mirrors
    // RhythmEcho's split (yin/yang children) and colour mapping; the yang
    // half's 180° rotation is baked into the SVG. Anchored on wayq's combined
    // freq+gain tilt dot rather than RhythmEcho's top-edge freq-only placement.
    const float scale = kYinyangDispR / 95.0f;

    // Maps the SVG centre (100,100) to the screen anchor (x, y).
    const auto transform = juce::AffineTransform::scale (scale)
                                                  .translated (x - 100.0f * scale,
                                                               y - 100.0f * scale);

    if (auto* half = dynamic_cast<juce::Drawable*> (yinyangDrawable_->findChildWithID (childId)))
        half->draw (g, 1.0f, transform);
}

void OutFilterRow::paintBellPeakDot (juce::Graphics& g, float x, float y)
{
    g.setColour (juce::Colour (GC::FAFO_TEXT));
    g.fillEllipse (x - kHandleDotR, y - kHandleDotR,
                   kHandleDotR * 2.0f, kHandleDotR * 2.0f);
    g.setColour (juce::Colour (GC::FRAC_BG));
    g.drawEllipse (x - (kHandleDotR - 1.5f), y - (kHandleDotR - 1.5f),
                   (kHandleDotR - 1.5f) * 2.0f, (kHandleDotR - 1.5f) * 2.0f, 1.5f);
}

void OutFilterRow::paintCutoffHandle (juce::Graphics& g, float x, float y0, float y1, bool isActive, float dotY)
{
    const float lineWidth  = isActive ? 2.0f : 1.0f;
    const float handleDotR = isActive ? kHandleDotR : (kHandleDotR * 0.75f);

    g.setColour (juce::Colour (GC::FAFO_TEXT).withAlpha (isActive ? 1.0f : 0.6f));
    g.fillRect (x - lineWidth * 0.5f, y0, lineWidth, y1 - y0);

    g.fillEllipse (x - handleDotR, dotY - handleDotR,
                   handleDotR * 2.0f, handleDotR * 2.0f);
    g.setColour (juce::Colour (GC::FRAC_BG));
    g.drawEllipse (x - (handleDotR - 1.5f), dotY - (handleDotR - 1.5f),
                   (handleDotR - 1.5f) * 2.0f, (handleDotR - 1.5f) * 2.0f, 1.5f);
}

void OutFilterRow::paintQBracket (juce::Graphics& g, float anchorX, float anchorY,
                                  float armPx, int direction, bool isActive)
{
    const float tipX  = anchorX + (float)direction * armPx;
    const float alpha = isActive ? 0.75f : 0.35f;
    const float lw    = isActive ? 3.0f : 2.0f;

    g.setColour (juce::Colour (GC::FAFO_TEXT).withAlpha (alpha));

    const float armX0 = (direction > 0) ? anchorX : tipX;
    const float armX1 = (direction > 0) ? tipX    : anchorX;
    g.fillRect (armX0, anchorY - lw * 0.5f, armX1 - armX0, lw);

    g.fillRect (tipX - lw * 0.5f, anchorY - kBracketCapH * 0.5f, lw, kBracketCapH);
}

// Total dB at frequency f, given current HP / LP / bell. Mirrors the audio
// chain enough for the GUI: 2-pole HP and LP rolloff (-12 dB/oct) and an
// RBJ peakingEQ for the bell. Sample rate fixed at the JUCE preferred 48 kHz
// for display purposes — visualization is independent of host SR.
// Evaluate a biquad's magnitude in dB at frequency f given coefficients.
static float biquadMagDb (float f, float fs,
                          float b0, float b1, float b2,
                          float a0, float a1, float a2)
{
    const float w  = juce::MathConstants<float>::twoPi * f / fs;
    const float cw = std::cos (w),    sw = std::sin (w);
    const float c2 = std::cos (2.0f * w), s2 = std::sin (2.0f * w);

    const float numRe = b0 + b1 * cw + b2 * c2;
    const float numIm = -(b1 * sw + b2 * s2);
    const float denRe = a0 + a1 * cw + a2 * c2;
    const float denIm = -(a1 * sw + a2 * s2);

    const float numMag2 = numRe * numRe + numIm * numIm;
    const float denMag2 = denRe * denRe + denIm * denIm;
    return 10.0f * std::log10 (std::max (numMag2 / std::max (denMag2, 1.0e-20f), 1.0e-20f));
}

static float totalDb (float f, float hpHz, float lpHz,
                      float bellF, float bellG, float bellQ,
                      float hpQ = 0.707f, float lpQ = 0.707f)
{
    constexpr float fs = 48000.0f;

    // Bell: RBJ peakingEQ magnitude in dB.
    const float A     = std::pow (10.0f, bellG / 40.0f);
    const float w0    = juce::MathConstants<float>::twoPi * bellF / fs;
    const float cosw0 = std::cos (w0);
    const float sinw0 = std::sin (w0);
    const float alpha = sinw0 / (2.0f * bellQ);

    const float bellB0 = 1.0f + alpha * A;
    const float bellB1 = -2.0f * cosw0;
    const float bellB2 = 1.0f - alpha * A;
    const float bellA0 = 1.0f + alpha / A;
    const float bellA1 = -2.0f * cosw0;
    const float bellA2 = 1.0f - alpha / A;

    float db = biquadMagDb (f, fs, bellB0, bellB1, bellB2, bellA0, bellA1, bellA2);

    // HP biquad magnitude using bilinear-transformed 2-pole HP with Q.
    {
        const float K    = std::tan (juce::MathConstants<float>::pi * hpHz / fs);
        const float Ksq  = K * K;
        const float qInv = 1.0f / hpQ;
        const float den  = 1.0f + qInv * K + Ksq;
        const float hb0  =  1.0f / den;
        const float hb1  = -2.0f / den;
        const float hb2  =  1.0f / den;
        const float ha1  =  2.0f * (Ksq - 1.0f) / den;
        const float ha2  =  (1.0f - qInv * K + Ksq) / den;
        db += biquadMagDb (f, fs, hb0, hb1, hb2, 1.0f, ha1, ha2);
    }

    // LP biquad magnitude using bilinear-transformed 2-pole LP with Q.
    {
        const float K    = std::tan (juce::MathConstants<float>::pi * lpHz / fs);
        const float Ksq  = K * K;
        const float qInv = 1.0f / lpQ;
        const float den  = 1.0f + qInv * K + Ksq;
        const float lb0  =  Ksq / den;
        const float lb1  =  2.0f * Ksq / den;
        const float lb2  =  Ksq / den;
        const float la1  =  2.0f * (Ksq - 1.0f) / den;
        const float la2  =  (1.0f - qInv * K + Ksq) / den;
        db += biquadMagDb (f, fs, lb0, lb1, lb2, 1.0f, la1, la2);
    }

    return db;
}

void OutFilterRow::paintResponseCurve (juce::Graphics& g, juce::Rectangle<float> rf,
                                       float hp, float lp, float bellF, float bellG, float bellQ,
                                       float hpQ, float lpQ,
                                       juce::Colour curveColour, float strokeWidth)
{
    juce::Path curve;
    bool started = false;

    const auto rInt = rf.toNearestInt();
    const int xL = rInt.getX();
    const int xR = rInt.getRight();
    for (int px = xL; px <= xR; ++px)
    {
        const float f  = xToFreq (px, rInt);
        const float db = totalDb (f, hp, lp, bellF, bellG, bellQ, hpQ, lpQ);
        const float py = juce::jlimit (rf.getY(), rf.getBottom(), gainToY (db, rInt));
        if (! started) { curve.startNewSubPath ((float) px, py); started = true; }
        else            curve.lineTo            ((float) px, py);
    }

    g.setColour (curveColour);
    g.strokePath (curve, juce::PathStrokeType (strokeWidth));
}

// Magnitude response (dB) for the tilt biquad — matches OutTiltBiquad::setCoefficients.
// RBJ low-shelf with A = 10^(G/20), b's divided by A → 0 dB at pivot, ±G at DC/Nyquist.
static float tiltMagDb (float f, float pivotHz, float gainDb, float fs = 48000.0f)
{
    if (std::abs (gainDb) < 0.01f) return 0.0f;
    const float A      = std::pow (10.0f, gainDb / 20.0f);   // /20 to match DSP
    const float sqrtA  = std::sqrt (A);
    const float w0     = juce::MathConstants<float>::twoPi * pivotHz / fs;
    const float cosw0  = std::cos (w0);
    const float sinw0  = std::sin (w0);
    const float alpha  = sinw0 * 0.70710678118f;  // S=1

    const float a0raw  = (A+1.0f) + (A-1.0f)*cosw0 + 2.0f*sqrtA*alpha;
    // Low-shelf b's divided by A
    const float hb0    = ((A+1.0f) - (A-1.0f)*cosw0 + 2.0f*sqrtA*alpha) / a0raw;
    const float hb1    = 2.0f * ((A-1.0f) - (A+1.0f)*cosw0) / a0raw;
    const float hb2    = ((A+1.0f) - (A-1.0f)*cosw0 - 2.0f*sqrtA*alpha) / a0raw;
    const float ha1    = -2.0f * ((A-1.0f) + (A+1.0f)*cosw0) / a0raw;
    const float ha2    = ((A+1.0f) + (A-1.0f)*cosw0 - 2.0f*sqrtA*alpha) / a0raw;
    return biquadMagDb (f, fs, hb0, hb1, hb2, 1.0f, ha1, ha2);
}

static float totalDbModeX (float f,
                            float bassTiltF, float bassTiltG,
                            float bellF, float bellG, float bellQ,
                            float trebTiltF, float trebTiltG)
{
    return tiltMagDb (f, bassTiltF, bassTiltG)
         + totalDb (f, 20.0f, 20000.0f, bellF, bellG, bellQ)  // HP/LP at extremes = transparent
         + tiltMagDb (f, trebTiltF, -trebTiltG);              // negate: treble tilt mirrors bass (matches DSP)
}

void OutFilterRow::paintModeXResponseCurve (juce::Graphics& g, juce::Rectangle<float> rf,
                                            float bassTiltF, float bassTiltG,
                                            float bellF, float bellG, float bellQ,
                                            float trebTiltF, float trebTiltG,
                                            juce::Colour curveColour, float strokeWidth)
{
    juce::Path curve;
    bool started = false;
    const auto rInt = rf.toNearestInt();
    for (int px = rInt.getX(); px <= rInt.getRight(); ++px)
    {
        const float f  = xToFreq (px, rInt);
        const float db = totalDbModeX (f, bassTiltF, bassTiltG, bellF, bellG, bellQ, trebTiltF, trebTiltG);
        const float py = juce::jlimit (rf.getY(), rf.getBottom(), gainToY (db, rInt));
        if (! started) { curve.startNewSubPath ((float) px, py); started = true; }
        else            curve.lineTo            ((float) px, py);
    }
    g.setColour (curveColour);
    g.strokePath (curve, juce::PathStrokeType (strokeWidth));
}

// ---------------------------------------------------------------------------
// Hit testing
// ---------------------------------------------------------------------------
OutFilterRow::DragMode OutFilterRow::hitZone (juce::Point<int> p, juce::Rectangle<int> cell,
                                              float hpHz, float lpHz,
                                              float bellF, float bellG, float bellQ,
                                              float hpQ, float lpQ) const
{
    if (! cell.contains (p)) return DragMode::None;

    const int   xHp     = (int) freqToX (hpHz, cell);
    const int   xLp     = (int) freqToX (lpHz, cell);
    const int   hpDotY  = (int) hpLpQToY (hpQ, cell);
    const int   lpDotY  = (int) hpLpQToY (lpQ, cell);
    const int   handleR = kHandleHitPx + kHandleDotR;
    const int   handleR2 = handleR * handleR;
    const int   dHpX = p.x - xHp, dHpY = p.y - hpDotY;
    const int   dLpX = p.x - xLp, dLpY = p.y - lpDotY;
    const int   distHp2 = dHpX * dHpX + dHpY * dHpY;
    const int   distLp2 = dLpX * dLpX + dLpY * dLpY;

    if (distHp2 <= handleR2 && distHp2 <= distLp2) return DragMode::HpHandle;
    if (distLp2 <= handleR2)                       return DragMode::LpHandle;

    {
        const int   xF_i    = (int) freqToX (bellF, cell);
        const int   yG_i    = (int) gainToY (bellG, cell);
        const int   dBellX  = p.x - xF_i, dBellY = p.y - yG_i;
        const int   bellDotR = kHandleDotR + 2;
        if (dBellX * dBellX + dBellY * dBellY <= bellDotR * bellDotR)
            return DragMode::BellPosition;

        const float xF      = (float) xF_i;
        const float yG      = (float) yG_i;
        float bArmMin, bArmMax;
        bellArmLimits (xF, cell, bArmMin, bArmMax);
        const float bellArm = qToArmPx (bellQ, OutputFilter::MID_Q_MIN, OutputFilter::MID_Q_MAX, bArmMin, bArmMax);
        if (std::abs ((float)p.y - yG) <= kBracketHitH)
        {
            if ((p.x >= (int)xF && p.x <= (int)(xF + bellArm)) ||
                (p.x >= (int)(xF - bellArm) && p.x < (int)xF))
                return DragMode::BellQBracket;
        }
    }

    return DragMode::None;
}

OutFilterRow::DragMode OutFilterRow::hitZoneModeX (juce::Point<int> p, juce::Rectangle<int> cell,
                                                   float bassTiltF, float bassTiltG,
                                                   float bellF, float bellG, float bellQ,
                                                   float trebTiltF, float trebTiltG) const
{
    if (! cell.contains (p)) return DragMode::None;

    const int handleR  = kHandleHitPx + kHandleDotR;
    const int handleR2 = handleR * handleR;

    // Bass tilt dot
    {
        const int xB = (int) freqToX (bassTiltF, cell);
        const int yB = (int) gainToY (bassTiltG, cell);
        const int dx = p.x - xB, dy = p.y - yB;
        if (dx * dx + dy * dy <= handleR2) return DragMode::BassTiltDot;
    }
    // Treble tilt dot
    {
        const int xT = (int) freqToX (trebTiltF, cell);
        const int yT = (int) gainToY (trebTiltG, cell);
        const int dx = p.x - xT, dy = p.y - yT;
        if (dx * dx + dy * dy <= handleR2) return DragMode::TrebTiltDot;
    }
    // Bell (always available in mode X)
    {
        const int xF = (int) freqToX (bellF, cell);
        const int yG = (int) gainToY (bellG, cell);
        const int dx = p.x - xF, dy = p.y - yG;
        const int bellDotR = kHandleDotR + 2;
        if (dx * dx + dy * dy <= bellDotR * bellDotR)
            return DragMode::BellPosition;

        float bArmMin, bArmMax;
        bellArmLimits ((float) xF, cell, bArmMin, bArmMax);
        const float bellArm = qToArmPx (bellQ, OutputFilter::MID_Q_MIN, OutputFilter::MID_Q_MAX, bArmMin, bArmMax);
        if (std::abs ((float) p.y - (float) yG) <= kBracketHitH)
        {
            if ((p.x >= xF && p.x <= (int)(xF + bellArm)) ||
                (p.x >= (int)(xF - bellArm) && p.x < xF))
                return DragMode::BellQBracket;
        }
    }
    return DragMode::None;
}

// ---------------------------------------------------------------------------
// writeKnob — push a value through the SliderAttachment to APVTS.
// ---------------------------------------------------------------------------
void OutFilterRow::writeKnob (Knob k, float rangeUnits)
{
    knobs_[k].setValue (rangeUnits, juce::sendNotificationSync);
}

// ---------------------------------------------------------------------------
// copyChannelParams — copy all per-channel params from one channel to the other.
// ---------------------------------------------------------------------------
void OutFilterRow::copyChannelParams (int from, int to)
{
    for (int k = 0; k < NumKnobs; ++k)
    {
        if (auto* src = apvts_.getParameter (paramIds_[k][from]))
            if (auto* dst = apvts_.getParameter (paramIds_[k][to]))
                dst->setValueNotifyingHost (src->getValue());
    }
    if (auto* src = apvts_.getParameter (deltaPid_[from]))
        if (auto* dst = apvts_.getParameter (deltaPid_[to]))
            dst->setValueNotifyingHost (src->getValue());
    if (auto* src = apvts_.getParameter (eqModeXPid_[from]))
        if (auto* dst = apvts_.getParameter (eqModeXPid_[to]))
            dst->setValueNotifyingHost (src->getValue());
}

// ---------------------------------------------------------------------------
// Mouse handlers — all gestures for the unified cell are caught here.
// ---------------------------------------------------------------------------
void OutFilterRow::mouseDown (const juce::MouseEvent& e)
{
    auto cell = cellBounds();
    if (! cell.contains (e.getPosition())) { dragMode_ = DragMode::None; return; }

    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu m;
        m.addItem (1, "Copy Ch A ==> Ch B");
        m.addItem (2, "Copy Ch B ==> Ch A");
        m.showMenuAsync (juce::PopupMenu::Options(),
            [this] (int result)
            {
                if (result == 1) copyChannelParams (0, 1);
                else if (result == 2) copyChannelParams (1, 0);
            });
        return;
    }

    const bool modeX = *apvts_.getRawParameterValue (eqModeXPid_[activeChan_]) > 0.5f;

    const float hpHz  = *apvts_.getRawParameterValue (paramIds_[KHP]        [activeChan_]);
    const float hpQ   = *apvts_.getRawParameterValue (paramIds_[KHpQ]       [activeChan_]);
    const float lpHz  = *apvts_.getRawParameterValue (paramIds_[KLP]        [activeChan_]);
    const float lpQ   = *apvts_.getRawParameterValue (paramIds_[KLpQ]       [activeChan_]);
    const float bellF = *apvts_.getRawParameterValue (paramIds_[KMidF]      [activeChan_]);
    const float bellG = *apvts_.getRawParameterValue (paramIds_[KMidG]      [activeChan_]);
    const float bellQ = *apvts_.getRawParameterValue (paramIds_[KMidQ]      [activeChan_]);
    const float bsTF  = *apvts_.getRawParameterValue (paramIds_[KBassTiltF] [activeChan_]);
    const float bsTG  = *apvts_.getRawParameterValue (paramIds_[KBassTiltG] [activeChan_]);
    const float trTF  = *apvts_.getRawParameterValue (paramIds_[KTrebTiltF] [activeChan_]);
    const float trTG  = *apvts_.getRawParameterValue (paramIds_[KTrebTiltG] [activeChan_]);

    if (modeX)
        dragMode_ = hitZoneModeX (e.getPosition(), cell, bsTF, bsTG, bellF, bellG, bellQ, trTF, trTG);
    else
        dragMode_ = hitZone (e.getPosition(), cell, hpHz, lpHz, bellF, bellG, bellQ, hpQ, lpQ);

    dragStart_           = e.getPosition();
    dragStartHp_         = hpHz;
    dragStartLp_         = lpHz;
    dragStartBellF_      = bellF;
    dragStartBellG_      = bellG;
    dragStartBellQ_      = bellQ;
    dragStartHpQ_        = hpQ;
    dragStartLpQ_        = lpQ;
    dragStartBassTiltF_  = bsTF;
    dragStartBassTiltG_  = bsTG;
    dragStartTrebTiltF_  = trTF;
    dragStartTrebTiltG_  = trTG;

    dragBellQSign_ = (dragStart_.x >= (int) freqToX (dragStartBellF_, cell)) ? +1 : -1;
    {
        const float xF = freqToX (dragStartBellF_, cell);
        bellArmLimits (xF, cell, dragBracketArmMin_, dragBracketArmMax_);
        dragStartArm_ = qToArmPx (dragStartBellQ_, OutputFilter::MID_Q_MIN, OutputFilter::MID_Q_MAX,
                                  dragBracketArmMin_, dragBracketArmMax_);
    }
}

void OutFilterRow::mouseDrag (const juce::MouseEvent& e)
{
    if (dragMode_ == DragMode::None) return;

    auto cell = cellBounds();
    const float dx       = (float)(e.x - dragStart_.x);
    const float dy       = (float)(e.y - dragStart_.y);
    const float dxFine   = e.mods.isShiftDown() ? dx * kFineSensitivity : dx;
    const float dyFine   = e.mods.isShiftDown() ? dy * kFineSensitivity : dy;

    switch (dragMode_)
    {
        case DragMode::HpHandle:
        {
            const float xStart = freqToX (dragStartHp_, cell);
            const float newHp  = clampInside (xToFreq ((int) std::round (xStart + dxFine), cell),
                                              OutputFilter::FREQ_MIN, dragStartLp_);
            writeKnob (KHP, newHp);
            // Y-drag → Q: up = more resonance, down = less
            {
                const float octaves = -dyFine / kBracketQPxPerOctave;
                const float newHpQ  = clampInside (dragStartHpQ_ * std::pow (2.0f, octaves),
                                                   OutputFilter::HP_LP_Q_MIN, OutputFilter::HP_LP_Q_MAX);
                writeKnob (KHpQ, newHpQ);
            }
            break;
        }
        case DragMode::LpHandle:
        {
            const float xStart = freqToX (dragStartLp_, cell);
            const float newLp  = clampInside (xToFreq ((int) std::round (xStart + dxFine), cell),
                                              dragStartHp_, OutputFilter::FREQ_MAX);
            writeKnob (KLP, newLp);
            // Y-drag → Q: up = more resonance, down = less
            {
                const float octaves = -dyFine / kBracketQPxPerOctave;
                const float newLpQ  = clampInside (dragStartLpQ_ * std::pow (2.0f, octaves),
                                                   OutputFilter::HP_LP_Q_MIN, OutputFilter::HP_LP_Q_MAX);
                writeKnob (KLpQ, newLpQ);
            }
            break;
        }
        case DragMode::BellPosition:
        {
            const float xStart  = freqToX (dragStartBellF_, cell);
            const float newF    = clampInside (xToFreq ((int) std::round (xStart + dxFine), cell),
                                               OutputFilter::FREQ_MIN, OutputFilter::FREQ_MAX);
            writeKnob (KMidF, newF);

            const float yStart  = gainToY (dragStartBellG_, cell);
            const float newG    = clampInside (yToGain ((int) std::round (yStart + dyFine), cell),
                                               OutputFilter::MID_GAIN_MIN, OutputFilter::MID_GAIN_MAX);
            writeKnob (KMidG, newG);
            break;
        }
        case DragMode::BellQBracket:
        {
            const float newArm = juce::jlimit (dragBracketArmMin_, dragBracketArmMax_,
                                               dragStartArm_ + dxFine * (float) dragBellQSign_);
            const float newQ   = armPxToQ (newArm, OutputFilter::MID_Q_MIN, OutputFilter::MID_Q_MAX,
                                           dragBracketArmMin_, dragBracketArmMax_);
            writeKnob (KMidQ, newQ);
            break;
        }
        case DragMode::BassTiltDot:
        {
            // X → bass tilt pivot frequency (clamped below treble tilt pivot)
            const float xStart   = freqToX (dragStartBassTiltF_, cell);
            const float curTrebF = *apvts_.getRawParameterValue (paramIds_[KTrebTiltF][activeChan_]);
            const float newF     = clampInside (xToFreq ((int) std::round (xStart + dxFine), cell),
                                                OutputFilter::FREQ_MIN, curTrebF);
            writeKnob (KBassTiltF, newF);
            // Y → tilt gain with soft knee
            const float yStart   = gainToY (dragStartBassTiltG_, cell);
            const float rawGain  = yToGain ((int) std::round (yStart + dyFine), cell);
            writeKnob (KBassTiltG, applyTiltSoftKnee (rawGain));
            break;
        }
        case DragMode::TrebTiltDot:
        {
            // X → treble tilt pivot frequency (clamped above bass tilt pivot)
            const float xStart   = freqToX (dragStartTrebTiltF_, cell);
            const float curBassF = *apvts_.getRawParameterValue (paramIds_[KBassTiltF][activeChan_]);
            const float newF     = clampInside (xToFreq ((int) std::round (xStart + dxFine), cell),
                                                curBassF, OutputFilter::FREQ_MAX);
            writeKnob (KTrebTiltF, newF);
            // Y → tilt gain with soft knee
            const float yStart   = gainToY (dragStartTrebTiltG_, cell);
            const float rawGain  = yToGain ((int) std::round (yStart + dyFine), cell);
            writeKnob (KTrebTiltG, applyTiltSoftKnee (rawGain));
            break;
        }
        case DragMode::None:
        default: break;
    }
}

void OutFilterRow::mouseUp (const juce::MouseEvent&)
{
    dragMode_ = DragMode::None;
}

void OutFilterRow::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto cell = cellBounds();
    if (! cell.contains (e.getPosition())) return;

    const bool modeX  = *apvts_.getRawParameterValue (eqModeXPid_[activeChan_]) > 0.5f;
    const float bellF = *apvts_.getRawParameterValue (paramIds_[KMidF]      [activeChan_]);
    const float bellG = *apvts_.getRawParameterValue (paramIds_[KMidG]      [activeChan_]);
    const float bellQ = *apvts_.getRawParameterValue (paramIds_[KMidQ]      [activeChan_]);

    if (modeX)
    {
        const float bsTF = *apvts_.getRawParameterValue (paramIds_[KBassTiltF][activeChan_]);
        const float bsTG = *apvts_.getRawParameterValue (paramIds_[KBassTiltG][activeChan_]);
        const float trTF = *apvts_.getRawParameterValue (paramIds_[KTrebTiltF][activeChan_]);
        const float trTG = *apvts_.getRawParameterValue (paramIds_[KTrebTiltG][activeChan_]);
        const auto where = hitZoneModeX (e.getPosition(), cell, bsTF, bsTG, bellF, bellG, bellQ, trTF, trTG);
        if (where == DragMode::BassTiltDot)
        {
            resetSingleParam (KBassTiltF, activeChan_);
            resetSingleParam (KBassTiltG, activeChan_);
        }
        else if (where == DragMode::TrebTiltDot)
        {
            resetSingleParam (KTrebTiltF, activeChan_);
            resetSingleParam (KTrebTiltG, activeChan_);
        }
        else
            resetBellTrio (activeChan_);
    }
    else
    {
        const float hpHz = *apvts_.getRawParameterValue (paramIds_[KHP]  [activeChan_]);
        const float hpQ  = *apvts_.getRawParameterValue (paramIds_[KHpQ] [activeChan_]);
        const float lpHz = *apvts_.getRawParameterValue (paramIds_[KLP]  [activeChan_]);
        const float lpQ  = *apvts_.getRawParameterValue (paramIds_[KLpQ] [activeChan_]);
        const auto where = hitZone (e.getPosition(), cell, hpHz, lpHz, bellF, bellG, bellQ, hpQ, lpQ);
        if (where == DragMode::HpHandle)
        {
            resetSingleParam (KHP, activeChan_);
            resetSingleParam (KHpQ, activeChan_);
        }
        else if (where == DragMode::LpHandle)
        {
            resetSingleParam (KLP, activeChan_);
            resetSingleParam (KLpQ, activeChan_);
        }
        else
            resetBellTrio (activeChan_);
    }
}

// ---------------------------------------------------------------------------
// Tooltips — position-aware. Hovering over HP/LP/bell-peak handles surfaces
// their current values; anywhere else returns empty so no tooltip appears.
// ---------------------------------------------------------------------------
namespace
{
    juce::String formatFreq (float hz)
    {
        if (hz < 1000.0f)
            return juce::String ((int) std::round (hz)) + " Hz";
        return juce::String::formatted ("%.2f kHz", hz / 1000.0f);
    }

    juce::String formatGain (float db)
    {
        return juce::String::formatted ("%+0.1f dB", db);
    }

    juce::String formatQ (float q)
    {
        return juce::String::formatted ("%.2f", q);
    }
}

juce::String OutFilterRow::getTooltip()
{
    auto cell = cellBounds();
    auto pos  = getMouseXYRelative();
    if (! cell.contains (pos)) return {};

    const bool  modeX = *apvts_.getRawParameterValue (eqModeXPid_[activeChan_]) > 0.5f;
    const float hpHz  = *apvts_.getRawParameterValue (paramIds_[KHP]        [activeChan_]);
    const float hpQ   = *apvts_.getRawParameterValue (paramIds_[KHpQ]       [activeChan_]);
    const float lpHz  = *apvts_.getRawParameterValue (paramIds_[KLP]        [activeChan_]);
    const float lpQ   = *apvts_.getRawParameterValue (paramIds_[KLpQ]       [activeChan_]);
    const float bellF = *apvts_.getRawParameterValue (paramIds_[KMidF]      [activeChan_]);
    const float bellG = *apvts_.getRawParameterValue (paramIds_[KMidG]      [activeChan_]);
    const float bellQ = *apvts_.getRawParameterValue (paramIds_[KMidQ]      [activeChan_]);
    const float bsTF  = *apvts_.getRawParameterValue (paramIds_[KBassTiltF] [activeChan_]);
    const float bsTG  = *apvts_.getRawParameterValue (paramIds_[KBassTiltG] [activeChan_]);
    const float trTF  = *apvts_.getRawParameterValue (paramIds_[KTrebTiltF] [activeChan_]);
    const float trTG  = *apvts_.getRawParameterValue (paramIds_[KTrebTiltG] [activeChan_]);

    const int handleR  = kHandleHitPx + kHandleDotR;
    const int handleR2 = handleR * handleR;

    if (modeX)
    {
        // Bass tilt dot
        {
            const int xB = (int) freqToX (bsTF, cell);
            const int yB = (int) gainToY (bsTG, cell);
            const int dx = pos.x - xB, dy = pos.y - yB;
            if (dx * dx + dy * dy <= handleR2)
                return "Bass Tilt: " + formatFreq (bsTF) + "    Gain: " + formatGain (bsTG)
                     + "    (drag \xe2\x86\x94 pivot, \xe2\x86\x95 tilt)";
        }
        // Treble tilt dot
        {
            const int xT = (int) freqToX (trTF, cell);
            const int yT = (int) gainToY (trTG, cell);
            const int dx = pos.x - xT, dy = pos.y - yT;
            if (dx * dx + dy * dy <= handleR2)
                return "Treble Tilt: " + formatFreq (trTF) + "    Gain: " + formatGain (trTG)
                     + "    (drag \xe2\x86\x94 pivot, \xe2\x86\x95 tilt)";
        }
    }
    else
    {
        const int xHp    = (int) freqToX (hpHz, cell);
        const int xLp    = (int) freqToX (lpHz, cell);
        const int hpDotY = (int) hpLpQToY (hpQ, cell);
        const int lpDotY = (int) hpLpQToY (lpQ, cell);
        {
            const int dx = pos.x - xHp, dy = pos.y - hpDotY;
            if (dx * dx + dy * dy <= handleR2)
                return "HP: " + formatFreq (hpHz) + "    Q: " + formatQ (hpQ)
                     + "    (drag \xe2\x86\x94 freq, \xe2\x86\x95 Q)";
        }
        {
            const int dx = pos.x - xLp, dy = pos.y - lpDotY;
            if (dx * dx + dy * dy <= handleR2)
                return "LP: " + formatFreq (lpHz) + "    Q: " + formatQ (lpQ)
                     + "    (drag \xe2\x86\x94 freq, \xe2\x86\x95 Q)";
        }
    }

    // Bell: available in both modes
    {
        const int xF = (int) freqToX (bellF, cell);
        const int yG = (int) gainToY (bellG, cell);
        const int dx = pos.x - xF, dy = pos.y - yG;
        if (dx * dx + dy * dy <= handleR2)
            return "Freq: " + formatFreq (bellF)
                 + "    Gain: " + formatGain (bellG)
                 + "    Q: " + formatQ (bellQ);
    }
    {
        const int xF = (int) freqToX (bellF, cell);
        const int yG = (int) gainToY (bellG, cell);
        float bArmMin, bArmMax;
        bellArmLimits ((float) xF, cell, bArmMin, bArmMax);
        const float bArm = qToArmPx (bellQ, OutputFilter::MID_Q_MIN, OutputFilter::MID_Q_MAX, bArmMin, bArmMax);
        if (std::abs (pos.y - yG) <= (int) kBracketHitH)
        {
            if ((pos.x >= xF && pos.x <= xF + (int) bArm) ||
                (pos.x >= xF - (int) bArm && pos.x < xF))
                return "Bell Q: " + formatQ (bellQ) + " — drag to adjust resonance";
        }
    }

    return {};
}

// ---------------------------------------------------------------------------
// Existing API (link, channel, reset) — unchanged in behavior.
// ---------------------------------------------------------------------------
void OutFilterRow::setLinked (bool linked)
{
    if (linked == linked_)
    {
        linkBtn_.setToggleState (linked, juce::dontSendNotification);
        return;
    }
    linked_ = linked;
    linkBtn_.setToggleState (linked, juce::dontSendNotification);

    if (linked)
    {
        stashB();
        mirrorAtoB();
        tabB_.setEnabled (false);
        tabA_.setVisible (false);
        tabB_.setVisible (false);
        if (activeChan_ == 1) selectChannel (0);  // snap to A
    }
    else
    {
        restoreB();
        tabB_.setEnabled (true);
        tabA_.setVisible (true);
        tabB_.setVisible (true);
    }
    deltaPolarityBtn_B_.setVisible (! linked_);
    eqModeXBtnB_.setVisible        (! linked_);

    if (onLinkChanged) onLinkChanged (linked);

    resized();
    repaint();
}

void OutFilterRow::selectChannel (int channel)
{
    activeChan_ = channel;
    tabA_.setToggleState (channel == 0, juce::dontSendNotification);
    tabB_.setToggleState (channel == 1, juce::dontSendNotification);
    rebindAttachments();
    syncDeltaButtonsFromParam();
    syncEqModeXButtonFromParam();
    repaint();
}

void OutFilterRow::rebindAttachments()
{
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    for (int k = 0; k < NumKnobs; ++k)
    {
        attach_[k].reset();   // detach old
        attach_[k] = std::make_unique<SA> (apvts_, paramIds_[k][activeChan_], knobs_[k]);
    }
}

void OutFilterRow::mirrorAtoB()
{
    for (int k = 0; k < NumKnobs; ++k)
    {
        auto* pa = apvts_.getParameter (paramIds_[k][0]);
        auto* pb = apvts_.getParameter (paramIds_[k][1]);
        if (pa && pb)
            pb->setValueNotifyingHost (pa->getValue());  // 0..1 normalized
    }
    // Delta and EQ mode X mirror with the same A→B rule.
    if (auto* pa = apvts_.getParameter (deltaPid_[0]))
        if (auto* pb = apvts_.getParameter (deltaPid_[1]))
            pb->setValueNotifyingHost (pa->getValue());
    if (auto* pa = apvts_.getParameter (eqModeXPid_[0]))
        if (auto* pb = apvts_.getParameter (eqModeXPid_[1]))
            pb->setValueNotifyingHost (pa->getValue());
}

void OutFilterRow::stashB()
{
    for (int k = 0; k < NumKnobs; ++k)
        if (auto* pb = apvts_.getParameter (paramIds_[k][1]))
            savedB_[k] = pb->getValue();   // store normalized
}

void OutFilterRow::restoreB()
{
    for (int k = 0; k < NumKnobs; ++k)
        if (auto* pb = apvts_.getParameter (paramIds_[k][1]))
            pb->setValueNotifyingHost (savedB_[k]);
}

void OutFilterRow::resetSingleParam (Knob k, int channel)
{
    if (auto* p = apvts_.getParameter (paramIds_[k][channel]))
        p->setValueNotifyingHost (p->getDefaultValue());
}

void OutFilterRow::resetBellTrio (int channel)
{
    resetSingleParam (KMidF, channel);
    resetSingleParam (KMidG, channel);
    resetSingleParam (KMidQ, channel);
}

void OutFilterRow::resetChannelDefaults (int channel)
{
    for (int k = 0; k < NumKnobs; ++k)
        resetSingleParam ((Knob) k, channel);
    if (auto* p = apvts_.getParameter (eqModeXPid_[channel]))
        p->setValueNotifyingHost (p->getDefaultValue());
}

void OutFilterRow::resetSectionToDefaults()
{
    resetChannelDefaults (0);
    resetChannelDefaults (1);
}

OutFilterRow::~OutFilterRow()
{
    apvts_.removeParameterListener (deltaPid_[0], this);
    apvts_.removeParameterListener (deltaPid_[1], this);
    apvts_.removeParameterListener (eqModeXPid_[0], this);
    apvts_.removeParameterListener (eqModeXPid_[1], this);

    for (int k = 0; k < NumKnobs; ++k)
    {
        apvts_.removeParameterListener (paramIds_[k][0], this);
        apvts_.removeParameterListener (paramIds_[k][1], this);
    }
}

void OutFilterRow::parameterChanged (const juce::String& parameterID, float /*newValue*/)
{
    if (parameterID == deltaPid_[0] || parameterID == deltaPid_[1])
    {
        juce::Component::SafePointer<OutFilterRow> safe (this);
        juce::MessageManager::callAsync ([safe]
        {
            if (auto* self = safe.getComponent())
                self->syncDeltaButtonsFromParam();
        });
    }
    else if (parameterID == eqModeXPid_[0] || parameterID == eqModeXPid_[1])
    {
        juce::Component::SafePointer<OutFilterRow> safe (this);
        juce::MessageManager::callAsync ([safe]
        {
            if (auto* self = safe.getComponent())
                self->syncEqModeXButtonFromParam();
        });
    }
    else
    {
        // Filter curve param change (preset load, host automation, copy operation).
        // The SliderAttachments only cover the active channel, so the inactive
        // channel's curve won't repaint unless we do it here.
        juce::Component::SafePointer<OutFilterRow> safe (this);
        juce::MessageManager::callAsync ([safe]
        {
            if (auto* self = safe.getComponent())
                self->repaint();
        });
    }
}


void OutFilterRow::syncDeltaButtonsFromParam()
{
    static const juce::String kTexts[3] = {
        juce::String::fromUTF8 ("\xce\x94"),   // Δ — off-state hint (first click moves to delta)
        juce::String::fromUTF8 ("\xce\x94"),   // Δ — delta active
        juce::String::fromUTF8 ("\xc3\x98")    // Ø — polarity active
    };
    const int modeA = juce::jlimit (0, 2, (int) *apvts_.getRawParameterValue (deltaPid_[0]));
    deltaPolarityBtn_.setToggleState (modeA != 0, juce::dontSendNotification);
    deltaPolarityBtn_.setButtonText (kTexts[modeA]);
    const int modeB = juce::jlimit (0, 2, (int) *apvts_.getRawParameterValue (deltaPid_[1]));
    deltaPolarityBtn_B_.setToggleState (modeB != 0, juce::dontSendNotification);
    deltaPolarityBtn_B_.setButtonText (kTexts[modeB]);
    deltaPolarityBtn_.repaint();
    deltaPolarityBtn_B_.repaint();
}

void OutFilterRow::syncEqModeXButtonFromParam()
{
    const bool modeXA = *apvts_.getRawParameterValue (eqModeXPid_[0]) > 0.5f;
    const bool modeXB = *apvts_.getRawParameterValue (eqModeXPid_[1]) > 0.5f;
    eqModeXBtnA_.setToggleState (modeXA, juce::dontSendNotification);
    eqModeXBtnB_.setToggleState (modeXB, juce::dontSendNotification);
    eqModeXBtnA_.setButtonText (modeXA ? "X" : "P");
    eqModeXBtnB_.setButtonText (modeXB ? "X" : "P");
    repaint();
}
