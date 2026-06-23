#include "StereoSliderRow.h"
#include "WayQLookAndFeel.h"

StereoSliderRow::StereoSliderRow (const juce::String& labelText,
                                  juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& paramIdA,
                                  const juce::String& paramIdB,
                                  bool showLinkButton)
    : labelText_ (labelText),
      showLinkBtn_ (showLinkButton),
      paramIdA_ (paramIdA),
      paramIdB_ (paramIdB)
{
    apvts_ = &apvts;
    addAndMakeVisible (sliderA_);
    addAndMakeVisible (sliderB_);

    if (showLinkBtn_)
    {
        linkBtn_.setClickingTogglesState (true);
        linkBtn_.setToggleState (true, juce::dontSendNotification);
        applyLinkButtonStyle (linkBtn_);
        linkBtn_.setTooltip ("on: link channels. off: restore previous. double click reset just these to their default");
        linkBtn_.onClick = [this] { setLinked (linkBtn_.getToggleState()); };
        linkBtn_.onDoubleClick = [this] { resetSectionToDefaults(); };
        addAndMakeVisible (linkBtn_);
    }

    // Create attachments (only if parameters exist in APVTS)
    if (apvts.getParameter (paramIdA) != nullptr)
        attA_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramIdA, sliderA_);
    if (apvts.getParameter (paramIdB) != nullptr)
        attB_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramIdB, sliderB_);

    // Set default values from the parameter range (denormalize through the range's skew)
    if (auto* paramA = apvts.getParameter (paramIdA))
        sliderA_.setDefaultValue (paramA->convertFrom0to1 (paramA->getDefaultValue()));
    if (auto* paramB = apvts.getParameter (paramIdB))
        sliderB_.setDefaultValue (paramB->convertFrom0to1 (paramB->getDefaultValue()));

    // Capture B's actual value after attachment so setLinked(false) restores it correctly
    savedBValue_ = (float) sliderB_.getValue();

    // Bidirectional mirror while linked — gangs the two APVTS params so DSP
    // always sees identical values on both sides, no DSP-side link awareness
    // needed. Re-entry guard prevents an infinite loop through the callbacks.
    sliderA_.onValueChange = [this]
    {
        if (! linked_ || mirroring_) return;
        mirroring_ = true;
        sliderB_.setValue (sliderA_.getValue(), juce::sendNotificationSync);
        mirroring_ = false;
    };
    sliderB_.onValueChange = [this]
    {
        if (! linked_ || mirroring_) return;
        mirroring_ = true;
        sliderA_.setValue (sliderB_.getValue(), juce::sendNotificationSync);
        mirroring_ = false;
    };

    // Initial gang: start with B's APVTS equal to A's so both channels match.
    if (linked_)
    {
        mirroring_ = true;
        sliderB_.setValue (sliderA_.getValue(), juce::sendNotificationSync);
        mirroring_ = false;
        sliderB_.setVisible (false);
    }
}

void StereoSliderRow::resized()
{
    float kW       = getWidth() / 626.0f;
    int workareaX  = (int)(GUIConstants::WORKAREA_X   * kW);
    int workareaW  = (int)(GUIConstants::WORKAREA_W   * kW);
    int halfH      = getHeight() / 2;

    sliderA_.setBounds (workareaX, 0,     workareaW, halfH);
    sliderB_.setBounds (workareaX, halfH, workareaW, getHeight() - halfH);

    if (showLinkBtn_)
    {
        int lnkSz = GUIConstants::LINK_BTN_SIZE;
        int lx = (int)(GUIConstants::LINK_CX * kW) - lnkSz / 2;
        int ly = juce::jmax (0, getHeight() / 2 - lnkSz / 2);
        linkBtn_.setBounds (lx, ly, lnkSz, lnkSz);
    }

    // Position meter overlays to match the track area inside each slider.
    // Our drawLinearSlider uses thumbR = height * 0.4f, so meter inset must match.
    if (meterA_ || meterB_)
    {
        int thumbInset = (int)(halfH * 0.4f);
        if (meterA_) meterA_->setBounds (sliderA_.getBounds().reduced (thumbInset, 0));
        if (meterB_) meterB_->setBounds (sliderB_.getBounds().reduced (thumbInset, 0));
    }

    // Position LED overlays — same diameter as slider thumbs (thumbR = halfH * 0.4, dia = halfH * 0.8)
    if (ledA_)
    {
        int ledSz = juce::jmax (1, (int)(halfH * 0.8f));
        int ledY  = sliderA_.getY() + (halfH - ledSz) / 2;
        ledA_->setBounds (sliderA_.getX(), ledY, ledSz, ledSz);
    }
    if (ledB_)
    {
        int ledSz = juce::jmax (1, (int)(halfH * 0.8f));
        int ledY  = sliderB_.getY() + (halfH - ledSz) / 2;
        ledB_->setBounds (sliderB_.getX(), ledY, ledSz, ledSz);
    }

    // Position curve-circle overlays. The label and circle form a compact
    // group right-anchored at the slider edge. Attack: [O][label]. Decay:
    // [label][O]. Both rows use the same group width so the columns visually
    // line up. A and B circles expand outward from the row centre with a 2px
    // gap, reaching the top and bottom of the full 2-channel row.
    if (curveA_ || curveB_)
    {
        const int dia      = juce::jmax (8, halfH - 1);
        const int gap      = 4;
        const int rightPad = 4;
        auto labelFont = getAveriaFont (GF::ROW_LABEL_SIZE);
        const int textW    = labelFont.getStringWidth (labelText_);
        const int groupW   = textW + gap + dia;
        const int rightX   = workareaX - rightPad;
        const int leftX    = rightX - groupW;
        const int circleX  = (curveSide_ == CurveSide::Left) ? leftX : (rightX - dia);
        if (curveA_) curveA_->setBounds (circleX, 0,         dia, dia);
        if (curveB_) curveB_->setBounds (circleX, halfH + 1, dia, dia);
    }
}

void StereoSliderRow::paint (juce::Graphics& g)
{
    float kW = getWidth() / 626.0f;
    int halfH = getHeight() / 2;

    // When meters are present, paint track backgrounds here so they show at silence
    // (sliders are made transparent so the meter layer paints on top of this).
    // Use the same thumbIndent inset as JUCE uses for the track, so the background
    // matches the track area exactly (no bleed beyond left/right track edges).
    if (meterA_ != nullptr)
    {
        int thumbInset = (int)(halfH * 0.4f);
        g.setColour (juce::Colour (fafo_ ? GC::FAFO_TRACK_BG : GC::SLIDER_BG));
        // Track background uses pill ends to match drawLinearSlider (trackH = halfH * 0.6).
        const float trackR = (halfH * 0.6f) * 0.5f;
        if (sliderA_.getWidth() > 0)
            g.fillRoundedRectangle (sliderA_.getBounds().reduced (thumbInset, 0).toFloat(), trackR);
        if (sliderB_.isVisible() && sliderB_.getWidth() > 0)
            g.fillRoundedRectangle (sliderB_.getBounds().reduced (thumbInset, 0).toFloat(), trackR);
    }

    g.setColour (juce::Colour (fafo_ ? GC::FAFO_TEXT : GC::LABEL));
    g.setFont (getAveriaFont (GF::ROW_LABEL_SIZE));

    int workareaX = (int)(GUIConstants::WORKAREA_X * kW);
    if (curveA_ || curveB_ || reserveLabelSpace_)
    {
        // Compact group: label vertically aligned with Ch A (top half), circle
        // adjacent. Attack (Left): [O][label] right-anchored at slider edge so
        // the label aligns with the right-column convention. Decay (Right):
        // [label][O] same total width, circle at slider edge, label snug to it.
        // reserveLabelSpace_: same layout but no actual circle (space reserved
        // for an external control such as the Inertia mode button).
        const int dia      = juce::jmax (8, halfH - 1);
        const int gap      = 4;
        const int rightPad = 4;
        const int textW    = g.getCurrentFont().getStringWidth (labelText_);
        const int groupW   = textW + gap + dia;
        const int rightX   = workareaX - rightPad;
        const int leftX    = rightX - groupW;
        const int labelX   = (curveSide_ == CurveSide::Left) ? (leftX + dia + gap) : leftX;
        auto labelArea = juce::Rectangle<int> (labelX, 0, textW, halfH);
        g.drawText (labelText_, labelArea, juce::Justification::centredRight);
    }
    else
    {
        // Standard layout: label aligns with top row (Ch A) so it stays
        // anchored when Ch B is hidden.
        auto labelArea = juce::Rectangle<int> (0, 0, workareaX, halfH);
        g.drawText (labelText_, labelArea.reduced (4, 0), juce::Justification::centredRight);
    }
}

void StereoSliderRow::addMeterOverlay (std::atomic<float>* srcA, std::atomic<float>* srcB)
{
    meterA_ = std::make_unique<MeterBar>();
    meterB_ = std::make_unique<MeterBar>();
    meterA_->setSource (srcA);
    meterB_->setSource (srcB);
    meterA_->setInterceptsMouseClicks (false, false);
    meterB_->setInterceptsMouseClicks (false, false);
    addAndMakeVisible (*meterA_);
    addAndMakeVisible (*meterB_);

    // Meters sit behind the sliders so the thumb paints on top
    meterA_->toBehind (&sliderA_);
    meterB_->toBehind (&sliderB_);

    // Respect current linked state — hide B meter if B slider is hidden
    if (linked_)
        meterB_->setVisible (false);

    // Sliders become transparent — track bg drawn by parent, meter fill visible through track
    for (auto* s : { &sliderA_, &sliderB_ })
    {
        s->setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
        s->setColour (juce::Slider::trackColourId,      juce::Colours::transparentBlack);
    }

    resized();
}

void StereoSliderRow::addLEDOverlay (std::atomic<uint32_t>* trigA, std::atomic<uint32_t>* trigB)
{
    ledA_ = std::make_unique<LEDComponent>();
    ledB_ = std::make_unique<LEDComponent>();
    ledA_->setTrigger (trigA);
    ledB_->setTrigger (trigB);
    ledA_->setInterceptsMouseClicks (false, false);
    ledB_->setInterceptsMouseClicks (false, false);
    addAndMakeVisible (*ledA_);
    addAndMakeVisible (*ledB_);

    // LEDs must paint on top of everything (meters, sliders, thumbs)
    ledA_->toFront (false);
    ledB_->toFront (false);

    // Respect current linked state — hide B LED if B slider is hidden
    if (linked_)
        ledB_->setVisible (false);

    resized();
}

void StereoSliderRow::setLinked (bool linked)
{
    linked_ = linked;
    linkBtn_.setToggleState (linked, juce::dontSendNotification);

    if (linked)
    {
        // Stash B, then write A's value into B's APVTS param so both channels
        // gang together. DSP reads both params unconditionally.
        savedBValue_ = (float) sliderB_.getValue();
        mirroring_ = true;
        sliderB_.setValue (sliderA_.getValue(), juce::sendNotificationSync);
        mirroring_ = false;
        sliderB_.setVisible (false);
        if (meterB_) meterB_->setVisible (false);
        if (ledB_)   ledB_->setVisible (false);

        // Curve circle: stash B, gang B's param to A's, hide B
        if (curveA_ && curveB_ && curveAttB_)
        {
            savedBCurveValue_ = curveB_->getValue();
            curveMirroring_ = true;
            curveAttB_->setValueAsCompleteGesture (curveA_->getValue());
            curveB_->setValue (curveA_->getValue(), juce::dontSendNotification);
            curveMirroring_ = false;
        }
        if (curveB_) curveB_->setVisible (false);
    }
    else
    {
        sliderB_.setVisible (true);
        if (meterB_) meterB_->setVisible (true);
        if (ledB_)   ledB_->setVisible (true);
        mirroring_ = true;
        sliderB_.setValue (savedBValue_, juce::sendNotificationSync);
        mirroring_ = false;

        if (curveB_)
        {
            if (curveAttB_)
            {
                curveMirroring_ = true;
                curveAttB_->setValueAsCompleteGesture (savedBCurveValue_);
                curveB_->setValue (savedBCurveValue_, juce::dontSendNotification);
                curveMirroring_ = false;
            }
            curveB_->setVisible (true);
        }
    }

    if (onLinkChanged) onLinkChanged (linked);
}

void StereoSliderRow::applyMeterOnlyStyle()
{
    for (auto* s : { &sliderA_, &sliderB_ })
    {
        s->setColour (juce::Slider::backgroundColourId, juce::Colour (GC::SLIDER_BG));
        s->setColour (juce::Slider::trackColourId,      juce::Colour (GC::SLIDER_BG));
        s->setColour (juce::Slider::thumbColourId,      juce::Colour (GC::SLIDER_BG));
    }
    repaint();
}

void StereoSliderRow::setLEDColour (juce::Colour c)
{
    if (ledA_) ledA_->setLitColour (c);
    if (ledB_) ledB_->setLitColour (c);
}

void StereoSliderRow::setPanMode (bool enable)
{
    panMode_ = enable;

    if (enable)
    {
        // Make the filled (active) track portion the same color as the background
        // so there is no left-of-thumb fill — just a flat uniform track.
        juce::Colour trackBg (GC::FAFO_TRACK_BG);
        sliderA_.setColour (juce::Slider::trackColourId, trackBg);
        sliderB_.setColour (juce::Slider::trackColourId, trackBg);
        sliderA_.setShowPanTooltip (true);
        sliderB_.setShowPanTooltip (true);
    }

    repaint();
}

void StereoSliderRow::setTickAtValue (float value)
{
    tickValue_ = value;
    sliderA_.setTickValue (value);
    sliderB_.setTickValue (value);
    sliderA_.setShowDbTooltip (true);
    sliderB_.setShowDbTooltip (true);
}

void StereoSliderRow::paintOverChildren (juce::Graphics& g)
{
    const float halfH   = getHeight() / 2.0f;
    const float triBase = juce::jmax (6.0f, halfH * 0.45f);
    const float triH    = juce::jmax (4.0f, halfH * 0.32f);

    // Helper: draw a triangle at a given x, above or below slider bounds
    auto drawTriangle = [&] (juce::Rectangle<int> sliderBounds, float cx, bool pointDown)
    {
        juce::Path tri;
        if (pointDown)
        {
            float tipY  = (float)sliderBounds.getY() + triH;
            float baseY = (float)sliderBounds.getY() - 1.0f;
            tri.startNewSubPath (cx - triBase, baseY);
            tri.lineTo          (cx + triBase, baseY);
            tri.lineTo          (cx,           tipY);
            tri.closeSubPath();
        }
        else
        {
            float tipY  = (float)sliderBounds.getBottom() - triH;
            float baseY = (float)sliderBounds.getBottom() + 1.0f;
            tri.startNewSubPath (cx - triBase, baseY);
            tri.lineTo          (cx + triBase, baseY);
            tri.lineTo          (cx,           tipY);
            tri.closeSubPath();
        }
        g.fillPath (tri);
    };

    // Pan mode: center-tick triangles at midpoint of each slider (FAFO color)
    if (panMode_)
    {
        g.setColour (juce::Colour (GC::FAFO_THUMB_FILL));
        float cxA = sliderA_.getBounds().getX() + sliderA_.getBounds().getWidth() * 0.5f;
        drawTriangle (sliderA_.getBounds(), cxA, true);

        if (sliderB_.isVisible())
        {
            float cxB = sliderB_.getBounds().getX() + sliderB_.getBounds().getWidth() * 0.5f;
            drawTriangle (sliderB_.getBounds(), cxB, false);
        }
    }

}

void StereoSliderRow::addCurveCircleOverlay (juce::AudioProcessorValueTreeState& apvts,
                                             const juce::String&                  paramIdA,
                                             const juce::String&                  paramIdB,
                                             CurveCircle::Direction              direction,
                                             CurveSide                            side)
{
    auto* paramA = apvts.getParameter (paramIdA);
    auto* paramB = apvts.getParameter (paramIdB);
    if (paramA == nullptr || paramB == nullptr) return;

    curveParamIdA_ = paramIdA;
    curveParamIdB_ = paramIdB;
    curveSide_ = side;
    curveA_ = std::make_unique<CurveCircle> (direction);
    curveB_ = std::make_unique<CurveCircle> (direction);
    addAndMakeVisible (*curveA_);
    addAndMakeVisible (*curveB_);
    curveA_->toFront (false);
    curveB_->toFront (false);

    // Param -> widget: external automation / preset load updates the visual
    curveAttA_ = std::make_unique<juce::ParameterAttachment> (
        *paramA,
        [this] (float v)
        {
            if (! curveA_) return;
            curveA_->setValue (v, juce::dontSendNotification);
        });
    curveAttB_ = std::make_unique<juce::ParameterAttachment> (
        *paramB,
        [this] (float v)
        {
            if (! curveB_) return;
            curveB_->setValue (v, juce::dontSendNotification);
        });

    // Widget -> param: user drag pushes value to host
    curveA_->onValueChange = [this] (float v)
    {
        if (curveAttA_) curveAttA_->setValueAsCompleteGesture (v);

        if (linked_ && ! curveMirroring_ && curveB_ && curveAttB_)
        {
            curveMirroring_ = true;
            curveAttB_->setValueAsCompleteGesture (v);
            curveB_->setValue (v, juce::dontSendNotification);
            curveMirroring_ = false;
        }
    };
    curveB_->onValueChange = [this] (float v)
    {
        if (curveAttB_) curveAttB_->setValueAsCompleteGesture (v);

        if (linked_ && ! curveMirroring_ && curveA_ && curveAttA_)
        {
            curveMirroring_ = true;
            curveAttA_->setValueAsCompleteGesture (v);
            curveA_->setValue (v, juce::dontSendNotification);
            curveMirroring_ = false;
        }
    };

    curveAttA_->sendInitialUpdate();
    curveAttB_->sendInitialUpdate();

    if (linked_)
    {
        // Stash B before ganging, so an unlink restores it (matches slider).
        savedBCurveValue_ = curveB_->getValue();
        curveMirroring_ = true;
        curveAttB_->setValueAsCompleteGesture (curveA_->getValue());
        curveB_->setValue (curveA_->getValue(), juce::dontSendNotification);
        curveMirroring_ = false;
        curveB_->setVisible (false);
    }

    resized();
}

float StereoSliderRow::fractionMultiplier (int idx)
{
    static constexpr float table[7] = {
        0.0f, 0.25f, 1.0f / 3.0f, 0.5f, 2.0f / 3.0f, 0.75f, 1.0f
    };
    if (idx < 0) idx = 0;
    if (idx > 6) idx = 6;
    return table[idx];
}

juce::String StereoSliderRow::formatFraction (int) { return {}; }

void StereoSliderRow::updateRelativeSliderValue (int) {}

void StereoSliderRow::timerCallback() {}

void StereoSliderRow::setRelativeModeForChannel (int, bool,
                                                 juce::AudioProcessorValueTreeState*,
                                                 const juce::String&,
                                                 std::atomic<float>*)
{
}

void StereoSliderRow::applyFafoColors (bool isFafo)
{
    auto setSliderColors = [] (WayQSlider& s, bool fafo)
    {
        if (fafo)
        {
            s.setColour (juce::Slider::backgroundColourId,    juce::Colour (GC::FAFO_TRACK_BG));
            s.setColour (juce::Slider::trackColourId,         juce::Colour (GC::FAFO_TRACK_BORDER));
            s.setColour (juce::Slider::thumbColourId,         juce::Colour (GC::FAFO_THUMB_FILL));
        }
        else
        {
            s.setColour (juce::Slider::backgroundColourId,    juce::Colour (GC::SLIDER_BG));
            s.setColour (juce::Slider::trackColourId,         juce::Colour (GC::SLIDER_FILL));
            s.setColour (juce::Slider::thumbColourId,         juce::Colour (GC::SLIDER_THUMB));
        }
    };

    setSliderColors (sliderA_, isFafo);
    setSliderColors (sliderB_, isFafo);
    fafo_ = isFafo;
    repaint();
}

void StereoSliderRow::resetSectionToDefaults()
{
    if (apvts_ == nullptr) return;

    auto resetParam = [this] (const juce::String& pid)
    {
        if (pid.isEmpty()) return;
        if (auto* p = apvts_->getParameter (pid))
            p->setValueNotifyingHost (p->getDefaultValue());
    };

    resetParam (paramIdA_);
    resetParam (paramIdB_);
    resetParam (curveParamIdA_);
    resetParam (curveParamIdB_);

    // Relative-mode (Mode B) fraction choice — reset only when active.
    for (int ch = 0; ch < 2; ++ch)
    {
        if (relMode_[ch] && relApvts_[ch] != nullptr && ! relFracId_[ch].isEmpty())
            if (auto* p = relApvts_[ch]->getParameter (relFracId_[ch]))
                p->setValueNotifyingHost (p->getDefaultValue());
    }
}
