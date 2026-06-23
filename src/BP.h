#pragma once
#include "HpLp.h"
#include "Baxandall.h"

// Single-channel band processor.  Owns HpLp + Baxandall + DC-block.
// modeX false → HpLp path, true → Baxandall (tilt) path.
class BP
{
public:
    void prepare (float sr);
    void reset();

    void setModeX      (bool modeX);

    void setHpFreq       (float f);
    void setHpQ          (float q);
    void setLpFreq       (float f);
    void setLpQ          (float q);
    void setMidFreq      (float f);
    void setMidGain      (float gainDb);
    void setMidQ         (float q);
    void setBassTiltFreq (float f);
    void setBassTiltGain (float gainDb);
    void setTrebTiltFreq (float f);
    void setTrebTiltGain (float gainDb);

    void processSample (float in, bool fafoActive, float& out);

private:
    HpLp         hpLp_;
    Baxandall    baxandall_;
    OutTwoPoleHP dcBlock_;

    bool  modeX_     = false;
    float sr_         = 44100.0f;
};
