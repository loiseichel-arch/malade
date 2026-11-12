// VibratoEngine.h
// Pure C++ (no JUCE) vibrato engine, so it can be unit-tested on its own.
//
// How the vibrato works
// ---------------------
// The signal goes through a short delay line whose length is modulated.
// Changing the delay length changes the pitch: pitch ratio = 1 - dDelay/dt.
// So the LFO shape you choose is the shape of the PITCH movement (like on a
// synth), and the delay curve is its integral:
//   - Sine     -> smooth up/down pitch
//   - Triangle -> linear pitch sweeps
//   - Square   -> pitch jumps between two notes (a trill, like a Jupiter-4)
//   - Saw up / Saw down -> pitch ramps, then jumps back
//   - Random   -> smooth random pitch wander
// Because the delay curve is always continuous, the square and saw shapes
// jump in pitch without clicks.
//
// Depth is a pitch deviation: 100 % = +/-100 cents (one semitone).
//
// Envelope moves
// --------------
// An onset detector watches the input. Each new attack above the threshold
// (re)starts the moves:
//   Rate move : rate starts at Rate 1 and glides to Rate 2 over "Rate time".
//   Depth move: no vibrato for "Delay", a very short fade in to Depth 1,
//               then a glide to Depth 2 over "Depth time".
// Before the first attack, and once a glide is over, the end values hold.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace vib
{

constexpr double kPi = 3.14159265358979323846;

enum class Shape : int { Sine = 0, Triangle, Square, SawUp, SawDown, Random };

struct Params
{
    // Main window
    float rateHz     = 5.0f;   // used when rateMove is off (already converted from tempo sync)
    float depthPct   = 35.0f;  // 0..100, 100 % = +/-100 cents
    Shape shape      = Shape::Sine;
    float widthPct   = 0.0f;   // 0..100, right LFO offset up to 180 degrees
    float flutterPct = 0.0f;   // 0..100
    float vintagePct = 0.0f;   // 0..100
    float mixPct     = 100.0f; // 0..100
    float outputDb   = 0.0f;   // -24..+12
    bool  active     = true;   // the "on" switch

    // Envelope panel
    float thresholdDb  = -30.0f;
    bool  rateMove     = false;
    float rate1Hz      = 8.0f;
    float rate2Hz      = 2.0f;
    float rateTimeSec  = 1.2f;
    bool  depthMove    = false;
    float delayMs      = 250.0f;
    float depth1Pct    = 45.0f;
    float depth2Pct    = 15.0f;
    float depthTimeSec = 0.8f;
};

class Engine
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int numChannels)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        channels = std::clamp (numChannels, 1, maxChannels);

        // Longest delay: min delay + 2 * max excursion + flutter + margin.
        const int needed = (int) std::ceil (sr * 0.1) + 8;
        int size = 1;
        while (size < needed) size <<= 1;
        mask = size - 1;
        for (auto& b : buffers) b.assign ((size_t) size, 0.0f);

        maxExcursion  = (float) (sr * 0.020);   // 20 ms each way
        minDelay      = (float) (sr * 0.0005) + 2.0f;
        flutterMax    = (float) (sr * 0.00025); // 0.25 ms at 100 %
        fadeInSamples = (float) (sr * 0.025);   // 25 ms fade after the delay
        holdoffSamples = (int64_t) (sr * 0.08); // min time between two attacks
        xfadeLen      = std::max (1, (int) (sr * 0.03)); // 30 ms shape cross-fade

        kFastAtt  = coef (1.0);   kFastRel = coef (30.0);
        kSlowAtt  = coef (50.0);  kSlowRel = coef (300.0);
        kRate     = 1.0f - coef (15.0);
        kDepth    = 1.0f - coef (8.0);
        kParam    = 1.0f - coef (20.0);
        kDelay    = 1.0f - coef (1.0);

        // Flutter noise: one-pole low-pass at 12 Hz then 20 Hz, normalised.
        nA1 = (float) std::exp (-2.0 * kPi * 12.0 / sr);
        nA2 = (float) std::exp (-2.0 * kPi * 20.0 / sr);
        const double b = 1.0 - nA1;
        noiseNorm = (float) (1.0 / std::sqrt ((1.0 / 3.0) * b / (2.0 - b)));

        reset();
    }

    void reset()
    {
        for (auto& b : buffers) std::fill (b.begin(), b.end(), 0.0f);
        writeIndex = 0;
        phase = 0.0; cycle = 0;
        fastEnv = slowEnv = 0.0f;
        armed = true; hasTriggered = false;
        sinceTrigger = 0; sinceLastTrigger = holdoffSamples;
        triggerCount = 0;
        first = true;
        for (int c = 0; c < maxChannels; ++c) { delaySm[c] = minDelay; lpState[c] = 0.0f; }
        n1 = n2 = 0.0f;
    }

    void process (float* const* io, int numChannels, int numSamples, const Params& p)
    {
        const int nch = std::min ({ numChannels, channels, maxChannels });
        if (nch <= 0 || numSamples <= 0 || buffers[0].empty()) return;

        // Targets for smoothed controls.
        const float mixT    = p.active ? std::clamp (p.mixPct, 0.0f, 100.0f) * 0.01f : 0.0f;
        const float gainT   = p.active ? dbToGain (p.outputDb) : 1.0f;
        const float vint    = std::clamp (p.vintagePct, 0.0f, 100.0f) * 0.01f;
        const float drive   = 1.0f + 1.5f * vint;   // gentle: small signals stay at unity
        const float lpFc    = 20000.0f * std::pow (0.3f, vint);
        const float lpA     = (float) std::exp (-2.0 * kPi * std::min ((double) lpFc, sr * 0.45) / sr);
        const bool  useLp   = vint > 0.001f;
        const float widthOff = std::clamp (p.widthPct, 0.0f, 100.0f) * 0.005f; // 0..0.5 cycle
        const float flAmt   = std::clamp (p.flutterPct, 0.0f, 100.0f) * 0.01f * flutterMax;
        const float thrLin  = dbToGain (p.thresholdDb);

        const float logMain = std::log (std::max (0.01f, p.rateHz));
        const float logR1   = std::log (std::max (0.01f, p.rate1Hz));
        const float logR2   = std::log (std::max (0.01f, p.rate2Hz));
        const double rateTimeS  = std::max (0.001, (double) p.rateTimeSec * sr);
        const double depthTimeS = std::max (0.001, (double) p.depthTimeSec * sr);
        const int64_t delayS    = (int64_t) (std::max (0.0f, p.delayMs) * 0.001 * sr);
        if (first)
        {
            curShape = oldShape = p.shape;
            xfadeLeft = 0;
            mixSm = mixT; gainSm = gainT;
            logRateSm = p.rateMove ? logR2 : logMain;
            depthSm   = p.depthMove ? p.depth2Pct : p.depthPct;
            first = false;
        }

        if (p.shape != curShape)
        {
            oldShape = curShape;
            curShape = p.shape;
            for (int c = 0; c < maxChannels; ++c) delaySmOld[c] = delaySm[c];
            xfadeLeft = xfadeLen;
            snapNew = true;
        }
        const float slope    = shapeSlope (curShape);
        const float slopeOld = shapeSlope (oldShape);

        for (int i = 0; i < numSamples; ++i)
        {
            // ---- Onset detection (on the input) ----
            float peak = 0.0f;
            for (int c = 0; c < nch; ++c) peak = std::max (peak, std::abs (io[c][i]));
            fastEnv = peak > fastEnv ? kFastAtt * fastEnv + (1.0f - kFastAtt) * peak
                                     : kFastRel * fastEnv + (1.0f - kFastRel) * peak;
            slowEnv = peak > slowEnv ? kSlowAtt * slowEnv + (1.0f - kSlowAtt) * peak
                                     : kSlowRel * slowEnv + (1.0f - kSlowRel) * peak;

            if (fastEnv < slowEnv * 1.41f) armed = true;            // settled: ready for the next attack
            if (armed && fastEnv > thrLin && fastEnv > slowEnv * 2.0f // +6 dB jump above the average
                && sinceLastTrigger >= holdoffSamples)
            {
                armed = false;
                hasTriggered = true;
                sinceTrigger = 0;
                sinceLastTrigger = 0;
                ++triggerCount;
            }

            // ---- Rate (log domain, so the glide sounds even) ----
            float logT;
            if (p.rateMove)
            {
                if (hasTriggered)
                {
                    const float t = (float) std::min (1.0, (double) sinceTrigger / rateTimeS);
                    logT = logR1 + (logR2 - logR1) * t;
                }
                else logT = logR2;
            }
            else logT = logMain;
            logRateSm += kRate * (logT - logRateSm);
            const float rate = std::exp (logRateSm);

            // ---- Depth ----
            float depthT;
            if (p.depthMove)
            {
                if (hasTriggered)
                {
                    if (sinceTrigger < delayS) depthT = 0.0f;  // straight note during the delay
                    else
                    {
                        const double u = (double) (sinceTrigger - delayS);
                        const float fade = (float) std::min (1.0, u / fadeInSamples);
                        const float t    = (float) std::min (1.0, u / depthTimeS);
                        depthT = (p.depth1Pct + (p.depth2Pct - p.depth1Pct) * t) * fade;
                    }
                }
                else depthT = p.depth2Pct;
            }
            else depthT = p.depthPct;
            depthSm += kDepth * (std::max (0.0f, depthT) - depthSm);

            // ---- Excursion (samples) for the wanted pitch deviation ----
            const float c = std::exp2 (depthSm / 1200.0f) - 1.0f;  // depth % == cents
            const float excursion = std::min (maxExcursion, (float) (c * sr / (rate * slope)));
            const bool  fading = xfadeLeft > 0;
            const float excursionOld = fading ? std::min (maxExcursion, (float) (c * sr / (rate * slopeOld))) : 0.0f;
            const float xf = fading ? 1.0f - (float) xfadeLeft / (float) xfadeLen : 1.0f;

            // ---- Flutter (shared by both channels) ----
            float fl = 0.0f;
            if (flAmt > 0.0f)
            {
                const float w = nextNoise();
                n1 = nA1 * n1 + (1.0f - nA1) * w;
                n2 = nA2 * n2 + (1.0f - nA2) * (n1 * noiseNorm);
                fl = std::tanh (0.5f * n2);
            }

            mixSm  += kParam * (mixT - mixSm);
            gainSm += kParam * (gainT - gainSm);

            // ---- Per channel: write, read modulated, colour, mix ----
            for (int ch = 0; ch < nch; ++ch)
            {
                auto& buf = buffers[(size_t) ch];
                const float dry = io[ch][i];
                buf[(size_t) writeIndex] = dry;

                double pch = phase + (ch == 1 ? widthOff : 0.0);
                uint64_t k = cycle;
                if (pch >= 1.0) { pch -= 1.0; ++k; }

                const float flOff = flAmt * (1.0f + fl);
                const float g = shapeValue (curShape, (float) pch, k);   // -1..1
                const float target = minDelay + excursion * (1.0f + g) + flOff;
                if (snapNew) delaySm[ch] = target;
                else         delaySm[ch] += kDelay * (target - delaySm[ch]);

                float wet = readHermite (buf, delaySm[ch]);

                if (fading)
                {
                    // Shape change: fade from a read head on the old shape
                    // to one on the new shape, so the switch never zips.
                    const float gOld = shapeValue (oldShape, (float) pch, k);
                    const float targetOld = minDelay + excursionOld * (1.0f + gOld) + flOff;
                    delaySmOld[ch] += kDelay * (targetOld - delaySmOld[ch]);
                    const float wetOld = readHermite (buf, delaySmOld[ch]);
                    wet = wetOld + xf * (wet - wetOld);
                }

                if (vint > 0.0f)
                {
                    const float sat = std::tanh (drive * wet) / drive;
                    wet = wet + vint * (sat - wet);
                }
                if (useLp)
                {
                    lpState[ch] = lpA * lpState[ch] + (1.0f - lpA) * wet;
                    wet = lpState[ch];
                }
                else lpState[ch] = wet;

                io[ch][i] = (dry + mixSm * (wet - dry)) * gainSm;
            }

            snapNew = false;
            if (xfadeLeft > 0) --xfadeLeft;

            writeIndex = (writeIndex + 1) & mask;

            phase += rate / sr;
            while (phase >= 1.0) { phase -= 1.0; ++cycle; }

            if (sinceTrigger < (int64_t) 1 << 40) ++sinceTrigger;
            if (sinceLastTrigger < (int64_t) 1 << 40) ++sinceLastTrigger;
        }

        displayRate  = std::exp (logRateSm);
        displayDepth = depthSm;
        displayLevelDb = 20.0f * std::log10 (fastEnv + 1.0e-9f);
    }

    // Values for the interface (read after process()).
    float    getCurrentRateHz()   const { return displayRate; }
    float    getCurrentDepthPct() const { return displayDepth; }
    float    getInputLevelDb()    const { return displayLevelDb; }
    uint32_t getTriggerCount()    const { return triggerCount; }
    float    getPhase()           const { return (float) phase; }
    uint64_t getCycle()           const { return cycle; }

    // Delay curves: normalised integral of the pitch shape, in -1..1,
    // continuous at the cycle wrap. k = cycle index (used by Random).
    static float shapeValue (Shape s, float p, uint64_t k)
    {
        switch (s)
        {
            case Shape::Sine:     return -std::cos (2.0f * (float) kPi * p);
            case Shape::Triangle: return p < 0.5f ? 8.0f * (2.0f * p * p - p)
                                                  : 8.0f * (3.0f * p - 2.0f * p * p - 1.0f);
            case Shape::Square:   return p < 0.5f ? 4.0f * p - 1.0f : 3.0f - 4.0f * p;
            case Shape::SawUp:    return 8.0f * (p * p - p) + 1.0f;
            case Shape::SawDown:  return -(8.0f * (p * p - p) + 1.0f);
            case Shape::Random:
            {
                const float a = hashToUnit (k - 1), b = hashToUnit (k);
                return a + (b - a) * 0.5f * (1.0f - std::cos ((float) kPi * p));
            }
        }
        return 0.0f;
    }

    // Peak slope of the delay curve (per cycle), so depth means the same
    // pitch deviation whatever the shape.
    static float shapeSlope (Shape s)
    {
        switch (s)
        {
            case Shape::Sine:     return 2.0f * (float) kPi;
            case Shape::Triangle: return 8.0f;
            case Shape::Square:   return 4.0f;
            case Shape::SawUp:
            case Shape::SawDown:  return 8.0f;
            case Shape::Random:   return 0.7f * (float) kPi;
        }
        return 2.0f * (float) kPi;
    }

private:
    float coef (double ms) const { return (float) std::exp (-1.0 / (ms * 0.001 * sr)); }
    static float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }

    static float hashToUnit (uint64_t k)
    {
        uint64_t z = k + 0x9E3779B97F4A7C15ull;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        z ^= z >> 31;
        return (float) ((double) (z >> 11) / (double) (1ull << 53)) * 2.0f - 1.0f;
    }

    float nextNoise()
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float) rng / 2147483648.0f - 1.0f;
    }

    float readHermite (const std::vector<float>& buf, float delay) const
    {
        const float pos = (float) writeIndex - delay;
        const float fl  = std::floor (pos);
        const float t   = pos - fl;
        const int i0 = (int) fl;
        const float xm1 = buf[(size_t) ((i0 - 1) & mask)];
        const float x0  = buf[(size_t) ( i0      & mask)];
        const float x1  = buf[(size_t) ((i0 + 1) & mask)];
        const float x2  = buf[(size_t) ((i0 + 2) & mask)];
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + x0;
    }

    double sr = 44100.0;
    int channels = 2;

    std::vector<float> buffers[maxChannels];
    int mask = 0, writeIndex = 0;

    float maxExcursion = 0, minDelay = 2, flutterMax = 0, fadeInSamples = 1;
    int64_t holdoffSamples = 0;

    float kFastAtt = 0, kFastRel = 0, kSlowAtt = 0, kSlowRel = 0;
    float kRate = 1, kDepth = 1, kParam = 1, kDelay = 1;

    double   phase = 0.0;
    uint64_t cycle = 0;

    float fastEnv = 0, slowEnv = 0;
    bool  armed = true, hasTriggered = false;
    int64_t sinceTrigger = 0, sinceLastTrigger = 0;
    uint32_t triggerCount = 0;

    Shape curShape = Shape::Sine, oldShape = Shape::Sine;
    int   xfadeLeft = 0, xfadeLen = 1;
    bool  snapNew = false;
    float delaySmOld[maxChannels] {};

    bool  first = true;
    float logRateSm = 0, depthSm = 0, mixSm = 1, gainSm = 1;
    float delaySm[maxChannels] {};
    float lpState[maxChannels] {};

    uint32_t rng = 0x12345678u;
    float n1 = 0, n2 = 0, nA1 = 0, nA2 = 0, noiseNorm = 1;

    float displayRate = 5, displayDepth = 0, displayLevelDb = -120;
};

} // namespace vib
