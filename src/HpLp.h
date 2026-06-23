#pragma once
#include "OutputFilter.h"

// HP + Bell + LP chain.  Uses the biquad structs from OutputFilter.h.
// Fixed stage order: HP→LP with Bell in parallel (classic parallel-EQ
// insertion), matching RhythmEcho's permanent OUT-filter order.
class HpLp
{
public:
    void prepare (float sr);
    void reset();

    void setHpFreq  (float f);
    void setHpQ     (float q);
    void setLpFreq  (float f);
    void setLpQ     (float q);
    void setMidFreq (float f);
    void setMidGain (float gainDb);
    void setMidQ    (float q);

    void processSample (float in, float& out);

private:
    void recalcHp();
    void recalcLp();

    OutTwoPoleHP   hp_;
    OutTwoPolePeak bell_;
    OutTwoPoleLP   lp_;

    float sr_      = 44100.0f;
    float hpFreq_  = OutputFilter::HP_DEFAULT;
    float hpQ_     = OutputFilter::HP_LP_Q_DEFAULT;
    float lpFreq_  = OutputFilter::LP_DEFAULT;
    float lpQ_     = OutputFilter::HP_LP_Q_DEFAULT;
    float midFreq_ = OutputFilter::MID_FREQ_DEFAULT;
    float midGain_ = OutputFilter::MID_GAIN_DEFAULT;
    float midQ_    = OutputFilter::MID_Q_DEFAULT;
};
