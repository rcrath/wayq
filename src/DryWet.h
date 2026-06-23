/*
  ==============================================================================
    DryWet.h
    RhythmEcho VST3 Plugin

    MONO dry/wet mixer. At mix=0 output is fully dry; at mix=1 fully wet.

    (c) 2026 Rich Rath / Way.Net
  ==============================================================================
*/

#pragma once

class DryWet
{
public:
    DryWet();
    ~DryWet();

    void prepare (double sampleRate);
    void reset();

    //==========================================================================
    // Process a single sample.
    //
    //   dry  = DryL inlet~  (dry audio sample)
    //   wet  = Wet  inlet~  (wet/delayed audio sample)
    //   mix  = mix  inlet   (control float, 0.0 = fully dry, 1.0 = fully wet)
    //
    // Returns: DryL * (1 - mix) + Wet * mix   (outlet~ Mix)
    //==========================================================================
    float process (float dry, float wet, float mix);

private:
    double sampleRate = 44100.0;   // stored; unused in computation
};
