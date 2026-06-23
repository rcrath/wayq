#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Filter.h"

// Stereo I/O layer.  Owns one Filter per channel and drives them from APVTS.
class IO
{
public:
    void prepare (double sr, juce::AudioProcessorValueTreeState& apvts);
    void reset();
    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::AudioProcessorValueTreeState& apvts);

private:
    Filter filterL_;
    Filter filterR_;
    float  sr_ = 44100.0f;
};
