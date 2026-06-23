#include "Baxandall.h"
#include <algorithm>

void Baxandall::prepare (float sr)
{
    sr_ = sr;
    recalcBassTilt();
    bell_.setCoefficients (midFreq_, midGain_, midQ_, sr_);
    recalcTrebTilt();
}

void Baxandall::reset()
{
    bassTilt_.reset();
    trebTilt_.reset();
    bell_.reset();
}

void Baxandall::setBassTiltFreq (float f)
{
    bassTiltFreq_ = std::max (OutputFilter::FREQ_MIN, std::min (f, OutputFilter::FREQ_MAX));
    recalcBassTilt();
}

void Baxandall::setBassTiltGain (float gainDb)
{
    bassTiltGain_ = std::max (OutputFilter::TILT_GAIN_MIN, std::min (gainDb, OutputFilter::TILT_GAIN_MAX));
    recalcBassTilt();
}

void Baxandall::setTrebTiltFreq (float f)
{
    trebTiltFreq_ = std::max (OutputFilter::FREQ_MIN, std::min (f, OutputFilter::FREQ_MAX));
    recalcTrebTilt();
}

void Baxandall::setTrebTiltGain (float gainDb)
{
    trebTiltGain_ = std::max (OutputFilter::TILT_GAIN_MIN, std::min (gainDb, OutputFilter::TILT_GAIN_MAX));
    recalcTrebTilt();
}

void Baxandall::setMidFreq (float f)
{
    midFreq_ = std::max (OutputFilter::FREQ_MIN, std::min (f, OutputFilter::FREQ_MAX));
    bell_.setCoefficients (midFreq_, midGain_, midQ_, sr_);
}

void Baxandall::setMidGain (float gainDb)
{
    midGain_ = std::max (OutputFilter::MID_GAIN_MIN, std::min (gainDb, OutputFilter::MID_GAIN_MAX));
    bell_.setCoefficients (midFreq_, midGain_, midQ_, sr_);
}

void Baxandall::setMidQ (float q)
{
    midQ_ = std::max (OutputFilter::MID_Q_MIN, std::min (q, OutputFilter::MID_Q_MAX));
    bell_.setCoefficients (midFreq_, midGain_, midQ_, sr_);
}

void Baxandall::processSample (float in, float& out)
{
    float b   = bassTilt_.process (in);
    float bel = bell_.process (b);
    out = trebTilt_.process (bel);
}

void Baxandall::recalcBassTilt() { bassTilt_.setCoefficients (bassTiltFreq_, bassTiltGain_, sr_); }
// Treble tilt is the mirror of bass: OutTiltBiquad treats +gain as a bass-side
// tilt (boost lows / cut highs), so negate here to make a positive treble gain
// boost the high side — handle up => high/right edge up.
void Baxandall::recalcTrebTilt() { trebTilt_.setCoefficients (trebTiltFreq_, -trebTiltGain_, sr_); }
