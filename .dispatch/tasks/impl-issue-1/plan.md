# Implement issue #1 — EQ-only VST3 + CLAP plumbing

- [x] Read issue #1 and comments from rcrath/wayq for full file list and parameter IDs
- [x] Read all existing src/ files for context (OutputFilter.h, OutFilterRow.h, StereoSliderRow.cpp, WayQSlider.h, WayQLookAndFeel.h, GUIConstants.h, and others)
- [x] Create src/DragTooltipProvider.h — abstract drag-tooltip base class
- [x] Create src/HpLp.h/.cpp — HP+Bell+LP chain (absorb OutTwoPoleHP, OutTwoPolePeak, OutTwoPoleLP from OutputFilter.h; implement 4 chain-order variants)
- [x] Create src/Baxandall.h/.cpp — BassTilt+Bell+TrebTilt chain (absorb OutTiltBiquad from OutputFilter.h)
- [x] Create src/BP.h/.cpp — owns one HpLp and one Baxandall; routes processSample based on modeX; owns dcBlock
- [x] Create src/IO.h/.cpp — reads all APVTS params per block, iterates samples, drives two BP instances (L and R)
- [x] Create src/PluginProcessor.h/.cpp — WayQAudioProcessor: owns APVTS + IO; createParameterLayout() with all EQ params; prepareToPlay, processBlock, getStateInformation, setStateInformation
- [x] Create src/PluginEditor.h/.cpp — WayQAudioProcessorEditor: instantiates OutFilterRow with all param IDs; sets window size from GUIConstants
- [x] Create CMakeLists.txt — JUCE FetchContent, juce_add_plugin VST3+CLAP, juce_add_binary_data for fonts/SVGs, target_sources, target_link_libraries
- [x] Fix src/StereoSliderRow.cpp line 3 — remove stale #include "PluginProcessor.h"; also stubbed RhythmEchoProcessor-dependent function bodies (formatFraction, updateRelativeSliderValue, timerCallback, setRelativeModeForChannel) since that type has no equivalent in WayQ
- [x] Retire src/OutputFilter.h — add a comment at the top noting it is superseded by HpLp.h and Baxandall.h
- [x] Write summary to .dispatch/tasks/impl-issue-1/output.md
