#pragma once
#include "OutputFilter.h"

// BassTilt + Bell + TrebTilt chain.  Uses struct OutTiltBiquad from OutputFilter.h.
class Baxandall
{
public:
    void prepare (float sr);
    void reset();

    void setBassTiltFreq (float f);
    void setBassTiltGain (float gainDb);
    void setTrebTiltFreq (float f);
    void setTrebTiltGain (float gainDb);
    void setMidFreq      (float f);
    void setMidGain      (float gainDb);
    void setMidQ         (float q);

    void processSample (float in, float& out);

private:
    void recalcBassTilt();
    void recalcTrebTilt();

    OutTiltBiquad  bassTilt_;
    OutTiltBiquad  trebTilt_;
    OutTwoPolePeak bell_;

    float sr_          = 44100.0f;
    float bassTiltFreq_ = OutputFilter::BASS_TILT_DEFAULT_FREQ;
    float bassTiltGain_ = 0.0f;
    float trebTiltFreq_ = OutputFilter::TREB_TILT_DEFAULT_FREQ;
    float trebTiltGain_ = 0.0f;
    float midFreq_  = OutputFilter::MID_FREQ_DEFAULT;
    float midGain_  = OutputFilter::MID_GAIN_DEFAULT;
    float midQ_     = OutputFilter::MID_Q_DEFAULT;
};
