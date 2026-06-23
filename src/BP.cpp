#include "BP.h"
#include <algorithm>

void BP::prepare (float sr)
{
    sr_ = sr;
    hpLp_.prepare (sr);
    baxandall_.prepare (sr);
    dcBlock_.setFreq (OutputFilter::DC_BLOCK_FREQ, sr);
}

void BP::reset()
{
    hpLp_.reset();
    baxandall_.reset();
    dcBlock_.reset();
}

void BP::setModeX (bool modeX)
{
    if (modeX == modeX_) return;
    modeX_ = modeX;
    if (modeX_)
        hpLp_.reset();
    else
        baxandall_.reset();
}

void BP::setHpFreq      (float f)      { hpLp_.setHpFreq (f); }
void BP::setHpQ         (float q)      { hpLp_.setHpQ (q); }
void BP::setLpFreq      (float f)      { hpLp_.setLpFreq (f); }
void BP::setLpQ         (float q)      { hpLp_.setLpQ (q); }
void BP::setMidFreq     (float f)      { hpLp_.setMidFreq (f);  baxandall_.setMidFreq (f); }
void BP::setMidGain     (float gainDb) { hpLp_.setMidGain (gainDb); baxandall_.setMidGain (gainDb); }
void BP::setMidQ        (float q)      { hpLp_.setMidQ (q);     baxandall_.setMidQ (q); }
void BP::setBassTiltFreq(float f)      { baxandall_.setBassTiltFreq (f); }
void BP::setBassTiltGain(float gainDb) { baxandall_.setBassTiltGain (gainDb); }
void BP::setTrebTiltFreq(float f)      { baxandall_.setTrebTiltFreq (f); }
void BP::setTrebTiltGain(float gainDb) { baxandall_.setTrebTiltGain (gainDb); }

void BP::processSample (float in, bool fafoActive, float& out)
{
    if (! fafoActive)
    {
        out = dcBlock_.process (in);
        return;
    }

    float filtered;
    if (modeX_)
        baxandall_.processSample (in, filtered);
    else
        hpLp_.processSample (in, filtered);

    out = dcBlock_.process (filtered);
}
