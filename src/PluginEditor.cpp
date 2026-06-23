#include "PluginEditor.h"
#include "GUIConstants.h"
#include "WayQLookAndFeel.h"

WayQAudioProcessorEditor::WayQAudioProcessorEditor (WayQAudioProcessor& p)
    : juce::AudioProcessorEditor (p),
      processor_ (p),
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

    addAndMakeVisible (outFilterRow_);
    // Stub layout: size the window to a single graph row (half the eventual
    // full-layout height) so the graph fills it with no top/bottom dead space.
    setSize (GUIConstants::DEFAULT_W, GUIConstants::DEFAULT_H / 2);
}

WayQAudioProcessorEditor::~WayQAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void WayQAudioProcessorEditor::resized()
{
    // Window is sized to the graph row (see ctor), so the graph fills it
    // edge-to-edge — no top/bottom padding, matching the sides.
    outFilterRow_.setBounds (getLocalBounds());
}
