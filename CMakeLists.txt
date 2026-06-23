cmake_minimum_required (VERSION 3.22)
project (WayQ VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

include (FetchContent)
FetchContent_Declare (
    JUCE
    URL      https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.12.tar.gz
    URL_HASH SHA256=dec1a8baee5aaec4502b717a421b850e257ab7e9c2efd03766185ec1957cf102
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
FetchContent_MakeAvailable (JUCE)

FetchContent_Declare (
    clap_juce_extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG        51a9359315298de632cf44e9d7524940868441e6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable (clap_juce_extensions)

juce_add_plugin (WayQ
    COMPANY_NAME              "Way.Net"
    PLUGIN_MANUFACTURER_CODE  RRth
    PLUGIN_CODE               WayQ
    FORMATS                   VST3
    PRODUCT_NAME              "WayQ"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

juce_add_binary_data (WayQData
    SOURCES
        src/AveriaGruesaLibre-Regular.ttf
        src/BarlowCondensed-Regular.ttf
        src/OpenSansCondensed-Light.ttf
        src/unifont.otf
        src/yinyang.svg
        src/feedback.png
        src/postdelay.png
)

target_sources (WayQ PRIVATE
    src/HpLp.cpp
    src/Baxandall.cpp
    src/BP.cpp
    src/Filter.cpp
    src/IO.cpp
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/OutFilterRow.cpp
    src/StereoSliderRow.cpp
    src/MonoSliderRow.cpp
    src/DualHandleFilterRow.cpp
    src/DualHandleSlider.cpp
    src/DryWet.cpp
)

target_link_libraries (WayQ
    PRIVATE
        WayQData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions (WayQ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_WIN_PER_MONITOR_DPI_AWARE=0
)

clap_juce_extensions_plugin (
    TARGET        WayQ
    CLAP_ID       "net.way.wayq"
    CLAP_FEATURES audio-effect equalizer stereo
)
