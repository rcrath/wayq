#include "HpLp.h"
#include <algorithm>

void HpLp::prepare (float sr)
{
    sr_ = sr;
    recalcHp();
    bell_.setCoefficients (midFreq_, midGain_, midQ_, sr_);
    recalcLp();
}

void HpLp::reset()
{
    hp_.reset();
    bell_.reset();
    lp_.reset();
}

void HpLp::setHpFreq (float f)
{
    hpFreq_ = std::max (OutputFilter::FREQ_MIN, std::min (f, lpFreq_));
    recalcHp();
}

void HpLp::setHpQ (float q)
{
    hpQ_ = std::max (OutputFilter::HP_LP_Q_MIN, std::min (q, OutputFilter::HP_LP_Q_MAX));
    recalcHp();
}

void HpLp::setLpFreq (float f)
{
    lpFreq_ = std::max (hpFreq_, std::min (f, OutputFilter::FREQ_MAX));
    recalcLp();
}

void HpLp::setLpQ (float q)
{
    lpQ_ = std::max (OutputFilter::HP_LP_Q_MIN, std::min (q, OutputFilter::HP_LP_Q_MAX));
    recalcLp();
}

void HpLp::setMidFreq (float f)
{
    midFreq_ = std::max (OutputFilter::FREQ_MIN, std::min (f, OutputFilter::FREQ_MAX));
    bell_.setCoefficients (midFreq_, midGain_, midQ_, sr_);
}

void HpLp::setMidGain (float gainDb)
{
    midGain_ = std::max (OutputFilter::MID_GAIN_MIN, std::min (gainDb, OutputFilter::MID_GAIN_MAX));
    bell_.setCoefficients (midFreq_, midGain_, midQ_, sr_);
}

void HpLp::setMidQ (float q)
{
    midQ_ = std::max (OutputFilter::MID_Q_MIN, std::min (q, OutputFilter::MID_Q_MAX));
    bell_.setCoefficients (midFreq_, midGain_, midQ_, sr_);
}

void HpLp::processSample (float in, float& out)
{
    // Fixed order: HP → LP with Bell in parallel (classic parallel-EQ
    // insertion). Matches RhythmEcho's permanent OUT-filter chain order.
    float hpOut  = hp_.process (in);
    float lpOut  = lp_.process (hpOut);
    float belOut = bell_.process (hpOut);
    out = lpOut + belOut - hpOut;
}

void HpLp::recalcHp() { hp_.setFreqQ (hpFreq_, hpQ_, sr_); }
void HpLp::recalcLp() { lp_.setFreqQ (lpFreq_, lpQ_, sr_); }
