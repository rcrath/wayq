#include "DualHandleFilterRow.h"
#include "WayQLookAndFeel.h"

DualHandleFilterRow::DualHandleFilterRow (const juce::String& labelText,
                                          juce::AudioProcessorValueTreeState& apvts,
                                          const juce::String& hpIdA, const juce::String& lpIdA,
                                          const juce::String& hpIdB, const juce::String& lpIdB)
    : labelText_ (labelText)
{
    sliderA_.setRange (GR::FILTER_MIN, GR::FILTER_MAX, true);
    sliderB_.setRange (GR::FILTER_MIN, GR::FILTER_MAX, true);

    hpParamA_ = apvts.getParameter (hpIdA);
    lpParamA_ = apvts.getParameter (lpIdA);
    hpParamB_ = apvts.getParameter (hpIdB);
    lpParamB_ = apvts.getParameter (lpIdB);

    // Initialize slider values from parameters
    auto denormHP = [&] (juce::RangedAudioParameter* p) -> double {
        return p ? (double) p->convertFrom0to1 (p->getValue()) : GR::HP_DEFAULT;
    };
    auto denormLP = [&] (juce::RangedAudioParameter* p) -> double {
        return p ? (double) p->convertFrom0to1 (p->getValue()) : GR::LP_DEFAULT;
    };

    sliderA_.setValues (denormHP (hpParamA_), denormLP (lpParamA_), juce::dontSendNotification);
    sliderB_.setValues (denormHP (hpParamB_), denormLP (lpParamB_), juce::dontSendNotification);

    // Wire slider changes to parameter updates. Bidirectional mirror while
    // linked gangs both APVTS params; re-entry guard prevents infinite loop.
    sliderA_.onHPChanged = [this] {
        if (hpParamA_)
            hpParamA_->setValueNotifyingHost (hpParamA_->convertTo0to1 ((float) sliderA_.getHPValue()));
        if (linked_ && ! mirroring_) syncBtoA();
    };
    sliderA_.onLPChanged = [this] {
        if (lpParamA_)
            lpParamA_->setValueNotifyingHost (lpParamA_->convertTo0to1 ((float) sliderA_.getLPValue()));
        if (linked_ && ! mirroring_) syncBtoA();
    };
    sliderB_.onHPChanged = [this] {
        if (hpParamB_)
            hpParamB_->setValueNotifyingHost (hpParamB_->convertTo0to1 ((float) sliderB_.getHPValue()));
        if (linked_ && ! mirroring_) syncAtoB();
    };
    sliderB_.onLPChanged = [this] {
        if (lpParamB_)
            lpParamB_->setValueNotifyingHost (lpParamB_->convertTo0to1 ((float) sliderB_.getLPValue()));
        if (linked_ && ! mirroring_) syncAtoB();
    };

    addAndMakeVisible (sliderA_);
    addAndMakeVisible (sliderB_);

    // Capture B's values so setLinked(false) can restore them.
    savedHpB_ = (float) sliderB_.getHPValue();
    savedLpB_ = (float) sliderB_.getLPValue();

    // Initial gang: start with B's HP/LP params equal to A's, then hide B.
    if (linked_)
    {
        syncBtoA();
        sliderB_.setVisible (false);
    }

    // Link button
    linkBtn_.setClickingTogglesState (true);
    linkBtn_.setToggleState (true, juce::dontSendNotification);
    applyLinkButtonStyle (linkBtn_);
    linkBtn_.setTooltip ("on: link channels. off: restore previous. double click reset just these to their default");
    linkBtn_.onClick = [this] { setLinked (linkBtn_.getToggleState()); };
    linkBtn_.onDoubleClick = [this] { resetSectionToDefaults(); };
    addAndMakeVisible (linkBtn_);
}

void DualHandleFilterRow::resetSectionToDefaults()
{
    // Write param default and return the denormalised default for the widget.
    auto reset = [] (juce::RangedAudioParameter* p) -> float
    {
        if (p == nullptr) return 0.0f;
        p->setValueNotifyingHost (p->getDefaultValue());
        return p->convertFrom0to1 (p->getDefaultValue());
    };
    const float hpDefA = reset (hpParamA_);
    const float lpDefA = reset (lpParamA_);
    const float hpDefB = reset (hpParamB_);
    const float lpDefB = reset (lpParamB_);

    // DualHandleSlider has no param listener, so sync widgets manually.
    mirroring_ = true;
    if (hpParamA_ && lpParamA_) sliderA_.setValues (hpDefA, lpDefA, juce::dontSendNotification);
    if (hpParamB_ && lpParamB_) sliderB_.setValues (hpDefB, lpDefB, juce::dontSendNotification);
    mirroring_ = false;
}

void DualHandleFilterRow::resized()
{
    float kW      = getWidth() / 626.0f;
    int workareaX = (int)(GUIConstants::WORKAREA_X * kW);
    int workareaW = (int)(GUIConstants::WORKAREA_W * kW);
    int halfH     = getHeight() / 2;

    sliderA_.setBounds (workareaX, 0,     workareaW, halfH);
    sliderB_.setBounds (workareaX, halfH, workareaW, getHeight() - halfH);

    int lnkSz = GUIConstants::LINK_BTN_SIZE;
    int lx = (int)(GUIConstants::LINK_CX * kW) - lnkSz / 2;
    int ly = juce::jmax (0, getHeight() / 2 - lnkSz / 2);
    linkBtn_.setBounds (lx, ly, lnkSz, lnkSz);
}

void DualHandleFilterRow::paint (juce::Graphics& g)
{
    float kW = getWidth() / 626.0f;
    int halfH = getHeight() / 2;

    // Label aligns with top row (Ch A) so it stays anchored when Ch B is hidden.
    auto labelArea = juce::Rectangle<int> (0, 0, (int)(GUIConstants::WORKAREA_X * kW), halfH);
    g.setColour (juce::Colour (GC::LABEL));
    g.setFont (getAveriaFont (GF::ROW_LABEL_SIZE));
    g.drawText (labelText_, labelArea.reduced (4, 0), juce::Justification::centredRight);
}

void DualHandleFilterRow::setLinked (bool linked)
{
    linked_ = linked;
    linkBtn_.setToggleState (linked, juce::dontSendNotification);

    if (linked)
    {
        savedHpB_ = (float) sliderB_.getHPValue();
        savedLpB_ = (float) sliderB_.getLPValue();
        syncBtoA();
        sliderB_.setVisible (false);
    }
    else
    {
        sliderB_.setVisible (true);
        mirroring_ = true;
        sliderB_.setValues (savedHpB_, savedLpB_, juce::sendNotificationSync);
        mirroring_ = false;
    }

    if (onLinkChanged) onLinkChanged (linked);
}

void DualHandleFilterRow::applyFafoColors (bool isFafo)
{
    sliderA_.applyFafoColors (isFafo);
    sliderB_.applyFafoColors (isFafo);
    repaint();
}

void DualHandleFilterRow::syncBtoA()
{
    mirroring_ = true;
    sliderB_.setValues (sliderA_.getHPValue(), sliderA_.getLPValue(), juce::sendNotificationSync);
    mirroring_ = false;
}

void DualHandleFilterRow::syncAtoB()
{
    mirroring_ = true;
    sliderA_.setValues (sliderB_.getHPValue(), sliderB_.getLPValue(), juce::sendNotificationSync);
    mirroring_ = false;
}
