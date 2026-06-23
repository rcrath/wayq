# impl-issue-1 output

## Files created

- src/DragTooltipProvider.h — abstract base (isDraggingForTooltip, getDragTooltip); required by WayQSlider.h
- src/HpLp.h / src/HpLp.cpp — HP + Bell + LP chain; uses OutTwoPoleHP/Peak/LP from OutputFilter.h; 4 chain orders
- src/Baxandall.h / src/Baxandall.cpp — BassTilt + Bell + TrebTilt chain; uses OutTiltBiquad from OutputFilter.h
- src/BP.h / src/BP.cpp — owns HpLp + Baxandall + dcBlock; routes by modeX; processSample with fafoActive
- src/IO.h / src/IO.cpp — owns bpL_ and bpR_; reads all 26 APVTS params per block; fafoActive=true always
- src/PluginProcessor.h / src/PluginProcessor.cpp — WayQAudioProcessor; APVTS with 28 params (14 A + 14 B); full JUCE boilerplate
- src/PluginEditor.h / src/PluginEditor.cpp — WayQAudioProcessorEditor; OutFilterRow wired with all 28 param IDs; size from GUIConstants
- CMakeLists.txt — JUCE 8.0.7 via FetchContent; VST3 + CLAP; juce_add_binary_data for 3 TTF + 1 OTF + 5 SVG

## Files modified

- src/StereoSliderRow.cpp — removed #include "PluginProcessor.h" (line 3); stubbed out four functions that referenced RhythmEchoProcessor::FRACTION_COUNT (formatFraction, updateRelativeSliderValue, timerCallback, setRelativeModeForChannel)
- src/OutputFilter.h — added obsolete header comment at top

## Notes

- The "outTrebTiltGB/B" typo in the issue was interpreted as outTrebTiltGA / outTrebTiltGB following the bass tilt pattern.
- StereoSliderRow's relative-mode functions (delay fraction selection) are stubs — they were RhythmEcho-specific and have no WayQ equivalent. The public API signature in StereoSliderRow.h is unchanged.
- unifont.otf included in juce_add_binary_data alongside the 3 TTFs; WayQLookAndFeel.h loads it by BinaryData name.
- outDeltaA/B params are registered in APVTS (range 0–2, default 0) but not wired into the DSP path this milestone; delta/polarity processing is a future concern.
