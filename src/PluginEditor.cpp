#include "PluginEditor.h"
#include "GUIConstants.h"
#include "WayQLookAndFeel.h"

WayQAudioProcessorEditor::WayQAudioProcessorEditor (WayQAudioProcessor& p)
    : juce::AudioProcessorEditor (p),
      processor_ (p),
      topPanel_ (p),
      outFilterRow_ ("EQ", p.getAPVTS(),
                     "outHpA",        "outHpB",
                     "outHpQA",       "outHpQB",
                     "outMidFA",      "outMidFB",
                     "outMidGA",      "outMidGB",
                     "outMidQA",      "outMidQB",
                     "outLpA",        "outLpB",
                     "outLpQA",       "outLpQB",
                     "outDeltaA",     "outDeltaB",
                     "outBassTiltFA", "outBassTiltFB",
                     "outBassTiltGA", "outBassTiltGB",
                     "outTrebTiltFA", "outTrebTiltFB",
                     "outTrebTiltGA", "outTrebTiltGB",
                     "outEqModeXA",   "outEqModeXB")
{
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf_);
    setLookAndFeel (&lnf_);

    addAndMakeVisible (topPanel_);
    addAndMakeVisible (outFilterRow_);

    // Top panel toggles tooltips on/off by driving the shared tooltip window's
    // appear delay (700 ms on; effectively never off).
    topPanel_.onTooltipsToggled = [this] (bool on)
    {
        tooltipWindow_.setMillisecondsBeforeTipAppears (on ? 700 : 0x7fffffff);
    };
    // A preset load / refresh-controls / undo-redo only needs the graph redrawn;
    // the OutFilterRow's own APVTS listeners already resync the curves.
    topPanel_.onStateChanged    = [this] { outFilterRow_.repaint(); };
    topPanel_.onRefreshControls = [this] { outFilterRow_.repaint(); };

    // Vertical stack: [top margin] -> [top panel] -> [graph] -> [bottom margin].
    // The top/bottom margins are empty and about half the width of the right "L"
    // graph margin. Size the window to fit graph ≈ the old single-row height.
    const float kW       = GUIConstants::DEFAULT_W / 626.0f;
    const int   lMargin  = GUIConstants::DEFAULT_W - (int)(GUIConstants::WORKAREA_END * kW);
    const int   marginV  = lMargin / 2;
    const int   barH     = 38;
    const int   graphH   = GUIConstants::DEFAULT_H / 2;
    setSize (GUIConstants::DEFAULT_W, marginV * 2 + barH + graphH);
}

WayQAudioProcessorEditor::~WayQAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void WayQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill the whole window — including the empty top/bottom margins — with
    // wayq's background. Without this, the unpainted area shows the host/
    // wrapper's default clear colour: black/grey under VST3 but a garish
    // orange under CLAP. Painting it explicitly makes both formats identical.
    g.fillAll (juce::Colour (GUIConstants::Color::BG));
}

void WayQAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    const float kW      = getWidth() / 626.0f;
    const float sfY     = getHeight() / (float) GUIConstants::DEFAULT_H;
    // Right "L" graph margin width — the empty top/bottom margins are half of it.
    const int   lMargin = getWidth() - (int)(GUIConstants::WORKAREA_END * kW);
    const int   marginV = lMargin / 2;
    const int   barH    = juce::jmax (32, (int)(38.0f * sfY));   // rhythmecho row height

    area.removeFromTop (marginV);                       // empty top margin
    topPanel_.setBounds (area.removeFromTop (barH));
    area.removeFromBottom (marginV);                    // empty bottom margin
    outFilterRow_.setBounds (area);                     // graph fills the rest
}
