/*
  ==============================================================================
    DryWet.cpp
    RhythmEcho VST3 Plugin

    Dry/wet mixer: dryGain = 1 - mix, output = dryOut + wetOut

    (c) 2026 Rich Rath / Way.Net
  ==============================================================================
*/

#include "DryWet.h"
#include <algorithm>   // std::clamp

DryWet::DryWet()  {}
DryWet::~DryWet() {}

void DryWet::prepare (double sr)
{
    sampleRate = sr;
}

// Module is stateless -- nothing to reset.
void DryWet::reset()
{
}

float DryWet::process (float dry, float wet, float mix)
{
    // PD-DIVERGENCE: clamp mix to [0,1] for safety; PD relies on upstream UI.
    const float m = std::clamp (mix, 0.0f, 1.0f);

    // [expr 1 - $f1]       -> dryGain
    // [*~] DryL * dryGain  -> dryOut
    // [*~] Wet  * m        -> wetOut
    // [+~] dryOut + wetOut -> outlet~ Mix
    return dry * (1.0f - m) + wet * m;
}
