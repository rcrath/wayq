#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "OutFilterRow.h"
#include "WayQLookAndFeel.h"
#include "DragAwareTooltipWindow.h"

class WayQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit WayQAudioProcessorEditor (WayQAudioProcessor& p);
    ~WayQAudioProcessorEditor() override;

    void resized() override;

private:
    WayQAudioProcessor& processor_;
    WayQLookAndFeel     lnf_;            // declared first: must outlive the children that use it
    OutFilterRow        outFilterRow_;
    DragAwareTooltipWindow tooltipWindow_ { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WayQAudioProcessorEditor)
};
