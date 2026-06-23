#include "IO.h"

static void applyParamsToChannel (Filter& filter,
                                   juce::AudioProcessorValueTreeState& apvts,
                                   const char* hpId,  const char* hpQId,
                                   const char* midFId, const char* midGId, const char* midQId,
                                   const char* lpId,  const char* lpQId,
                                   const char* bassTiltFId, const char* bassTiltGId,
                                   const char* trebTiltFId, const char* trebTiltGId,
                                   const char* eqModeXId)
{
    filter.setHpFreq       (*apvts.getRawParameterValue (hpId));
    filter.setHpQ          (*apvts.getRawParameterValue (hpQId));
    filter.setMidFreq      (*apvts.getRawParameterValue (midFId));
    filter.setMidGain      (*apvts.getRawParameterValue (midGId));
    filter.setMidQ         (*apvts.getRawParameterValue (midQId));
    filter.setLpFreq       (*apvts.getRawParameterValue (lpId));
    filter.setLpQ          (*apvts.getRawParameterValue (lpQId));
    filter.setBassTiltFreq (*apvts.getRawParameterValue (bassTiltFId));
    filter.setBassTiltGain (*apvts.getRawParameterValue (bassTiltGId));
    filter.setTrebTiltFreq (*apvts.getRawParameterValue (trebTiltFId));
    filter.setTrebTiltGain (*apvts.getRawParameterValue (trebTiltGId));
    filter.setModeX        (*apvts.getRawParameterValue (eqModeXId) > 0.5f);
}

void IO::prepare (double sr, juce::AudioProcessorValueTreeState&)
{
    sr_ = static_cast<float> (sr);
    filterL_.prepare (sr_);
    filterR_.prepare (sr_);
}

void IO::reset()
{
    filterL_.reset();
    filterR_.reset();
}

void IO::processBlock (juce::AudioBuffer<float>& buffer,
                       juce::AudioProcessorValueTreeState& apvts)
{
    applyParamsToChannel (filterL_, apvts,
        "outHpA", "outHpQA", "outMidFA", "outMidGA", "outMidQA",
        "outLpA", "outLpQA",
        "outBassTiltFA", "outBassTiltGA", "outTrebTiltFA", "outTrebTiltGA",
        "outEqModeXA");
    filterL_.setDelta (static_cast<int> (*apvts.getRawParameterValue ("outDeltaA")));

    applyParamsToChannel (filterR_, apvts,
        "outHpB", "outHpQB", "outMidFB", "outMidGB", "outMidQB",
        "outLpB", "outLpQB",
        "outBassTiltFB", "outBassTiltGB", "outTrebTiltFB", "outTrebTiltGB",
        "outEqModeXB");
    filterR_.setDelta (static_cast<int> (*apvts.getRawParameterValue ("outDeltaB")));

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    auto* L = (numChannels > 0) ? buffer.getWritePointer (0) : nullptr;
    auto* R = (numChannels > 1) ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        float out = 0.0f;
        if (L) { filterL_.processSample (L[i], true, out); L[i] = out; }
        if (R) { filterR_.processSample (R[i], true, out); R[i] = out; }
    }
}
