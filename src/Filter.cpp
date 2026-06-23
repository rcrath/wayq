#include "Filter.h"

void Filter::prepare (float sr)          { bp_.prepare (sr); }
void Filter::reset()                     { bp_.reset(); }
void Filter::setDelta (int d)            { delta_ = d; }

void Filter::setModeX        (bool b)    { bp_.setModeX (b); }
void Filter::setHpFreq       (float f)   { bp_.setHpFreq (f); }
void Filter::setHpQ          (float q)   { bp_.setHpQ (q); }
void Filter::setMidFreq      (float f)   { bp_.setMidFreq (f); }
void Filter::setMidGain      (float g)   { bp_.setMidGain (g); }
void Filter::setMidQ         (float q)   { bp_.setMidQ (q); }
void Filter::setLpFreq       (float f)   { bp_.setLpFreq (f); }
void Filter::setLpQ          (float q)   { bp_.setLpQ (q); }
void Filter::setBassTiltFreq (float f)   { bp_.setBassTiltFreq (f); }
void Filter::setBassTiltGain (float g)   { bp_.setBassTiltGain (g); }
void Filter::setTrebTiltFreq (float f)   { bp_.setTrebTiltFreq (f); }
void Filter::setTrebTiltGain (float g)   { bp_.setTrebTiltGain (g); }

void Filter::processSample (float in, bool fafoActive, float& out)
{
    float processed;
    bp_.processSample (in, fafoActive, processed);

    switch (delta_)
    {
        case 1:  out = in - processed; break;
        case 2:  out = -processed;     break;
        default: out = processed;      break;
    }
}
