/*
  ==============================================================================
    SUPERSEDED BY HpLp.h AND Baxandall.h.  KEPT FOR REFERENCE ONLY.
  ==============================================================================
*/
/*
  ==============================================================================
    OutputFilter.h
    RhythmEcho VST3 Plugin

    Issue #29: Output filter — -12 dB/oct highpass + lowpass placed on the
    WET output of the delay, just BEFORE feedback.

    Same filter design as ScFilter.h (Issue #6): Direct Form II transposed
    2-pole Butterworth HP + LP.

    FAFO only — defaults to HP=20 Hz, LP=20 kHz (transparent at both ends).
    Persists settings when switching back to Sane (pass-through in Sane).

    Single-channel: each Delay owns its own OutputFilter, so stereo is
    achieved by PluginProcessor instantiating two Delays (delayL / delayR)
    and wiring A params to delayL's filter and B params to delayR's filter.

    HP and LP share the 20 Hz – 20 kHz log scale and block each other
    WITHIN this channel:
      LP cutoff >= HP cutoff at all times.
    Cross-channel constraint (A LP blocking B HP, etc.) is NOT enforced —
    the two filters are independent.

    Frequency readout (used by GUI):
      20–999 Hz  → integer Hz string, e.g. "400"
      >=1000 Hz  → integer kHz string (no decimals), e.g. "2k"

    Signal chain (within one Delay):
      shaped  (post-SigCon wet output, pre-feedback)
        → OutputFilter::processSample()
        → filtered
        → feedback = tanh(filtered * feedbackGain + xFbIn * xFeedback)
        → tanh limiter → output

    (c) 2026 Rich Rath / Way.Net
  ==============================================================================
*/

#pragma once
#include <cmath>
#include <algorithm>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ─────────────────────────────────────────────────────────────────────────────
// OutTwoPoleHP — Direct Form II transposed 2-pole Butterworth highpass
// (same design as ScFilter::TwoPoleHP)
// ─────────────────────────────────────────────────────────────────────────────
struct OutTwoPoleHP
{
    double b0 = 1.0, b1 = -2.0, b2 = 1.0;
    double a1 = 0.0, a2 = 0.0;
    double w1 = 0.0, w2 = 0.0;

    void reset() { w1 = w2 = 0.0; }

    void setFreq (float freqHz, float sampleRate)
    {
        const double f   = static_cast<double> (freqHz);
        const double sr  = static_cast<double> (sampleRate);
        const double wc  = 2.0 * M_PI * f / sr;
        const double K   = std::tan (wc * 0.5);
        const double Ksq = K * K;
        const double sq2 = 1.41421356237;
        const double den = 1.0 + sq2 * K + Ksq;

        b0 =  1.0 / den;
        b1 = -2.0 / den;
        b2 =  1.0 / den;
        a1 =  2.0 * (Ksq - 1.0) / den;
        a2 =  (1.0 - sq2 * K + Ksq) / den;
    }

    void setFreqQ (float freqHz, float Q, float sampleRate)
    {
        const double wc  = 2.0 * M_PI * static_cast<double> (freqHz) / static_cast<double> (sampleRate);
        const double K   = std::tan (wc * 0.5);
        const double Ksq = K * K;
        const double qInv = 1.0 / static_cast<double> (Q);
        const double den = 1.0 + qInv * K + Ksq;

        b0 =  1.0 / den;
        b1 = -2.0 / den;
        b2 =  1.0 / den;
        a1 =  2.0 * (Ksq - 1.0) / den;
        a2 =  (1.0 - qInv * K + Ksq) / den;
    }

    float process (float x)
    {
        const double xd = static_cast<double> (x);
        const double y  = b0 * xd + w1;
        w1 = b1 * xd - a1 * y + w2;
        w2 = b2 * xd - a2 * y;
        return static_cast<float> (y);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// OutTwoPoleLP — Direct Form II transposed 2-pole Butterworth lowpass
// (same design as ScFilter::TwoPoleLP)
// ─────────────────────────────────────────────────────────────────────────────
struct OutTwoPoleLP
{
    double b0 = 1.0, b1 = 2.0, b2 = 1.0;
    double a1 = 0.0, a2 = 0.0;
    double w1 = 0.0, w2 = 0.0;

    void reset() { w1 = w2 = 0.0; }

    void setFreq (float freqHz, float sampleRate)
    {
        const double f   = static_cast<double> (freqHz);
        const double sr  = static_cast<double> (sampleRate);
        const double wc  = 2.0 * M_PI * f / sr;
        const double K   = std::tan (wc * 0.5);
        const double Ksq = K * K;
        const double sq2 = 1.41421356237;
        const double den = 1.0 + sq2 * K + Ksq;

        b0 =  Ksq / den;
        b1 =  2.0 * Ksq / den;
        b2 =  Ksq / den;
        a1 =  2.0 * (Ksq - 1.0) / den;
        a2 =  (1.0 - sq2 * K + Ksq) / den;
    }

    void setFreqQ (float freqHz, float Q, float sampleRate)
    {
        const double wc  = 2.0 * M_PI * static_cast<double> (freqHz) / static_cast<double> (sampleRate);
        const double K   = std::tan (wc * 0.5);
        const double Ksq = K * K;
        const double qInv = 1.0 / static_cast<double> (Q);
        const double den = 1.0 + qInv * K + Ksq;

        b0 =  Ksq / den;
        b1 =  2.0 * Ksq / den;
        b2 =  Ksq / den;
        a1 =  2.0 * (Ksq - 1.0) / den;
        a2 =  (1.0 - qInv * K + Ksq) / den;
    }

    float process (float x)
    {
        const double xd = static_cast<double> (x);
        const double y  = b0 * xd + w1;
        w1 = b1 * xd - a1 * y + w2;
        w2 = b2 * xd - a2 * y;
        return static_cast<float> (y);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// OutTwoPolePeak — Direct Form II transposed RBJ peaking-EQ biquad
// (Audio EQ Cookbook formula). gainDb in [-12, +12], Q in [0.3, 10].
// At gainDb = 0 the filter is mathematically a unity passthrough.
// ─────────────────────────────────────────────────────────────────────────────
struct OutTwoPolePeak
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;
    double w1 = 0.0, w2 = 0.0;

    void reset() { w1 = w2 = 0.0; }

    void setCoefficients (float freqHz, float gainDb, float Q, float sampleRate)
    {
        // RBJ peakingEQ
        const double A     = std::pow (10.0, static_cast<double> (gainDb) / 40.0);
        const double w0    = 2.0 * M_PI * static_cast<double> (freqHz) / static_cast<double> (sampleRate);
        const double cosw0 = std::cos (w0);
        const double sinw0 = std::sin (w0);
        const double alpha = sinw0 / (2.0 * static_cast<double> (Q));

        const double a0 = 1.0 + alpha / A;
        const double a1raw =     -2.0 * cosw0;
        const double a2raw = 1.0 - alpha / A;
        const double b0raw = 1.0 + alpha * A;
        const double b1raw =     -2.0 * cosw0;
        const double b2raw = 1.0 - alpha * A;

        b0 = b0raw / a0;
        b1 = b1raw / a0;
        b2 = b2raw / a0;
        a1 = a1raw / a0;
        a2 = a2raw / a0;
    }

    float process (float x)
    {
        const double xd = static_cast<double> (x);
        const double y  = b0 * xd + w1;
        w1 = b1 * xd - a1 * y + w2;
        w2 = b2 * xd - a2 * y;
        return static_cast<float> (y);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// OutTiltBiquad — true tilt (see-saw) EQ biquad.
// Positive gainDb boosts below pivotFreq and cuts above (bass-side tilt).
// Negative gainDb cuts below and boosts above (treble-side tilt).
// Response is +gainDb dB at DC, 0 dB at pivotFreq, -gainDb dB at Nyquist.
//
// Implementation: RBJ 2nd-order low-shelf with A = 10^(G/20) [twice the usual
// RBJ A exponent], then divide b0/b1/b2 by A so the pivot becomes 0 dB.
// The RBJ low-shelf's midpoint gain is A (=G dB) before the division; after
// division it is 1 (=0 dB), giving the balanced see-saw shape.
// ─────────────────────────────────────────────────────────────────────────────
struct OutTiltBiquad
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;
    double w1 = 0.0, w2 = 0.0;

    void reset() { w1 = w2 = 0.0; }

    void setFlat() noexcept
    {
        b0 = 1.0; b1 = 0.0; b2 = 0.0;
        a1 = 0.0; a2 = 0.0;
    }

    void setCoefficients (float freqHz, float gainDb, float sampleRate)
    {
        if (std::abs (gainDb) < 0.01f) { setFlat(); return; }

        // A = 10^(G/20): linear gain at DC (= +G dB).  Using /20 (not the
        // standard RBJ /40) so the shelf midpoint sits exactly at G dB before
        // normalisation, giving 0 dB at the pivot after dividing b's by A.
        const double A     = std::pow (10.0, static_cast<double> (gainDb) / 20.0);
        const double sqrtA = std::sqrt (A);
        const double w0    = 2.0 * M_PI * static_cast<double> (freqHz)
                             / static_cast<double> (sampleRate);
        const double cosw0 = std::cos (w0);
        const double sinw0 = std::sin (w0);
        const double alpha = sinw0 * 0.70710678118;  // sinw0 / sqrt(2), S=1

        // RBJ low-shelf coefficients (before normalisation).
        const double a0raw = (A+1.0) + (A-1.0)*cosw0 + 2.0*sqrtA*alpha;
        const double b0raw = A * ((A+1.0) - (A-1.0)*cosw0 + 2.0*sqrtA*alpha);
        const double b1raw = 2.0 * A * ((A-1.0) - (A+1.0)*cosw0);
        const double b2raw = A * ((A+1.0) - (A-1.0)*cosw0 - 2.0*sqrtA*alpha);
        const double a1raw = -2.0 * ((A-1.0) + (A+1.0)*cosw0);
        const double a2raw = (A+1.0) + (A-1.0)*cosw0 - 2.0*sqrtA*alpha;

        // Divide b's by A to shift the pivot to 0 dB.
        b0 = b0raw / (A * a0raw);
        b1 = b1raw / (A * a0raw);
        b2 = b2raw / (A * a0raw);
        a1 = a1raw / a0raw;
        a2 = a2raw / a0raw;
    }

    float process (float x)
    {
        const double xd = static_cast<double> (x);
        const double y  = b0 * xd + w1;
        w1 = b1 * xd - a1 * y + w2;
        w2 = b2 * xd - a2 * y;
        return static_cast<float> (y);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// OutputFilter — single-channel wet-output HP + LP
// ─────────────────────────────────────────────────────────────────────────────
class OutputFilter
{
public:
    static constexpr float FREQ_MIN    =    20.0f;
    static constexpr float FREQ_MAX    = 20000.0f;
    static constexpr float HP_DEFAULT  = FREQ_MIN;
    static constexpr float LP_DEFAULT  = FREQ_MAX;
    static constexpr float MID_FREQ_DEFAULT = 632.456f;
    static constexpr float MID_GAIN_DEFAULT = 0.0f;
    static constexpr float MID_Q_DEFAULT    = 0.7f;
    static constexpr float MID_GAIN_MIN     = -12.0f;
    static constexpr float MID_GAIN_MAX     =  12.0f;
    static constexpr float MID_Q_MIN        = 0.3f;
    static constexpr float MID_Q_MAX        = 17.31f;

    static constexpr float DC_BLOCK_FREQ    = 10.0f;

    static constexpr float HP_LP_Q_DEFAULT = 0.707f;
    static constexpr float HP_LP_Q_MIN     = 0.3f;
    static constexpr float HP_LP_Q_MAX     = 10.0f;

    static constexpr float BASS_TILT_DEFAULT_FREQ = 250.0f;
    static constexpr float TREB_TILT_DEFAULT_FREQ = 4000.0f;
    static constexpr float TILT_GAIN_MIN          = -12.0f;
    static constexpr float TILT_GAIN_MAX          =  12.0f;

    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);
        recalcHp();
        bell.setCoefficients (midFreq, midGain, midQ, sr);
        recalcLp();
        recalcBassTilt();
        recalcTrebTilt();
        dcBlock.setFreq (DC_BLOCK_FREQ, sr);
    }

    void reset()
    {
        hp.reset();
        bell.reset();
        lp.reset();
        bassTilt_.reset();
        trebTilt_.reset();
        dcBlock.reset();
    }

    void setHpFreq (float f)
    {
        hpFreq = std::max (FREQ_MIN, std::min (f, lpFreq));
        recalcHp();
    }

    void setHpQ (float q)
    {
        hpQ = std::max (HP_LP_Q_MIN, std::min (q, HP_LP_Q_MAX));
        recalcHp();
    }

    void setMidFreq (float f)
    {
        midFreq = std::max (FREQ_MIN, std::min (f, FREQ_MAX));
        bell.setCoefficients (midFreq, midGain, midQ, sr);
    }

    void setLpFreq (float f)
    {
        lpFreq = std::max (hpFreq, std::min (f, FREQ_MAX));
        recalcLp();
    }

    void setLpQ (float q)
    {
        lpQ = std::max (HP_LP_Q_MIN, std::min (q, HP_LP_Q_MAX));
        recalcLp();
    }

    void setMidGain (float gainDb)
    {
        midGain = std::max (MID_GAIN_MIN, std::min (gainDb, MID_GAIN_MAX));
        bell.setCoefficients (midFreq, midGain, midQ, sr);
    }

    void setMidQ (float q)
    {
        midQ = std::max (MID_Q_MIN, std::min (q, MID_Q_MAX));
        bell.setCoefficients (midFreq, midGain, midQ, sr);
    }

    float getHpFreq()        const { return hpFreq; }
    float getHpQ()           const { return hpQ; }
    float getMidFreq()       const { return midFreq; }
    float getMidGain()       const { return midGain; }
    float getMidQ()          const { return midQ; }
    float getLpFreq()        const { return lpFreq; }
    float getLpQ()           const { return lpQ; }
    float getBassTiltFreq()  const { return bassTiltFreq_; }
    float getBassTiltGain()  const { return bassTiltGain_; }
    float getTrebTiltFreq()  const { return trebTiltFreq_; }
    float getTrebTiltGain()  const { return trebTiltGain_; }
    bool  getEqModeX()       const { return modeX_; }

    void setEqModeX (bool modeX)
    {
        if (modeX == modeX_) return;
        modeX_ = modeX;
        if (modeX_)
        {
            hp.reset();
            lp.reset();
        }
        else
        {
            bassTilt_.reset();
            trebTilt_.reset();
        }
    }

    void setBassTiltFreq (float f)
    {
        bassTiltFreq_ = std::max (FREQ_MIN, std::min (f, FREQ_MAX));
        recalcBassTilt();
    }

    void setBassTiltGain (float gainDb)
    {
        bassTiltGain_ = std::max (TILT_GAIN_MIN, std::min (gainDb, TILT_GAIN_MAX));
        recalcBassTilt();
    }

    void setTrebTiltFreq (float f)
    {
        trebTiltFreq_ = std::max (FREQ_MIN, std::min (f, FREQ_MAX));
        recalcTrebTilt();
    }

    void setTrebTiltGain (float gainDb)
    {
        trebTiltGain_ = std::max (TILT_GAIN_MIN, std::min (gainDb, TILT_GAIN_MAX));
        recalcTrebTilt();
    }

    void setChainOrder (int order)
    {
        chainOrder_ = std::max (0, std::min (order, 3));
    }

    void processSample (float in, bool fafoActive, float& out)
    {
        if (! fafoActive)
        {
            out = dcBlock.process (in);
            return;
        }

        if (modeX_)
        {
            const float b   = bassTilt_.process (in);
            const float bel = bell.process (b);
            const float t   = trebTilt_.process (bel);
            out = dcBlock.process (t);
        }
        else
        {
            switch (chainOrder_)
            {
                case 0:   // Bell → HP → LP
                {
                    const float belOut = bell.process (in);
                    const float hpOut  = hp.process (belOut);
                    const float lpOut  = lp.process (hpOut);
                    out = dcBlock.process (lpOut);
                    break;
                }
                default:
                case 1:   // HP → Bell → LP  (original)
                {
                    const float hpOut  = hp.process (in);
                    const float belOut = bell.process (hpOut);
                    const float lpOut  = lp.process (belOut);
                    out = dcBlock.process (lpOut);
                    break;
                }
                case 2:   // HP → LP → Bell
                {
                    const float hpOut  = hp.process (in);
                    const float lpOut  = lp.process (hpOut);
                    const float belOut = bell.process (lpOut);
                    out = dcBlock.process (belOut);
                    break;
                }
                case 3:   // HP → LP with Bell in parallel (classic parallel-EQ insertion)
                {
                    const float hpOut  = hp.process (in);
                    const float lpOut  = lp.process (hpOut);
                    const float belOut = bell.process (hpOut);
                    out = dcBlock.process (lpOut + belOut - hpOut);
                    break;
                }
            }
        }
    }

private:
    void recalcHp()       { hp.setFreqQ (hpFreq, hpQ, sr); }
    void recalcLp()       { lp.setFreqQ (lpFreq, lpQ, sr); }
    void recalcBassTilt() { bassTilt_.setCoefficients (bassTiltFreq_, bassTiltGain_, sr); }
    void recalcTrebTilt() { trebTilt_.setCoefficients (trebTiltFreq_, trebTiltGain_, sr); }

    OutTwoPoleHP   hp;
    OutTwoPolePeak bell;
    OutTwoPoleLP   lp;
    OutTiltBiquad  bassTilt_;
    OutTiltBiquad  trebTilt_;
    OutTwoPoleHP   dcBlock;

    float hpFreq       = HP_DEFAULT;
    float hpQ          = HP_LP_Q_DEFAULT;
    float midFreq      = MID_FREQ_DEFAULT;
    float midGain      = MID_GAIN_DEFAULT;
    float midQ         = MID_Q_DEFAULT;
    float lpFreq       = LP_DEFAULT;
    float lpQ          = HP_LP_Q_DEFAULT;
    float bassTiltFreq_ = BASS_TILT_DEFAULT_FREQ;
    float bassTiltGain_ = 0.0f;
    float trebTiltFreq_ = TREB_TILT_DEFAULT_FREQ;
    float trebTiltGain_ = 0.0f;
    bool  modeX_        = false;
    int   chainOrder_   = 1;
    float sr            = 44100.0f;

public:

    //--- GUI readout helper ---------------------------------------------------
    // Format a frequency value for display.
    // 20–999 Hz  → integer Hz,  e.g. "400"
    // >= 1000 Hz → integer kHz, e.g. "2k"
    static std::string formatFreq (float hz)
    {
        if (hz < 1000.0f)
            return std::to_string (static_cast<int> (hz + 0.5f));
        return std::to_string (static_cast<int> (hz / 1000.0f + 0.5f)) + "k";
    }
};
