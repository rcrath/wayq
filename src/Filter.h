#pragma once
#include "BP.h"

// Single-channel filter layer. Owns one BP; applies delta/polarity post-EQ.
// delta 0 = off (pass processed), 1 = delta (in - processed), 2 = polarity (-processed).
class Filter
{
public:
    void prepare (float sr);
    void reset();
    void setDelta (int d);

    void setModeX        (bool modeX);
    void setHpFreq       (float f);
    void setHpQ          (float q);
    void setMidFreq      (float f);
    void setMidGain      (float gainDb);
    void setMidQ         (float q);
    void setLpFreq       (float f);
    void setLpQ          (float q);
    void setBassTiltFreq (float f);
    void setBassTiltGain (float gainDb);
    void setTrebTiltFreq (float f);
    void setTrebTiltGain (float gainDb);

    void processSample (float in, bool fafoActive, float& out);

private:
    BP  bp_;
    int delta_ = 0;
};
