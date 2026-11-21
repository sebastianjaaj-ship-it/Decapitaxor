// Decapitaxor - analog saturation DSP building blocks (header-only, no JUCE dependency)
#pragma once

#include <cmath>
#include <algorithm>

namespace dcx
{
constexpr double kPi = 3.14159265358979323846;

//==============================================================================
// RBJ biquad (Transposed Direct Form II), double precision state.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    void reset() noexcept { z1 = z2 = 0; }

    inline float process (float xIn) noexcept
    {
        const double x = xIn;
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }

    void setIdentity() noexcept { b0 = 1; b1 = b2 = a1 = a2 = 0; }

    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2) noexcept
    {
        b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0;
        a1 = na1 / na0; a2 = na2 / na0;
    }

    static double clampFreq (double f, double fs) { return std::clamp (f, 5.0, fs * 0.49); }

    void lowPass (double fs, double f, double q)
    {
        const double w = 2 * kPi * clampFreq (f, fs) / fs, c = std::cos (w), a = std::sin (w) / (2 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + a, -2 * c, 1 - a);
    }

    void highPass (double fs, double f, double q)
    {
        const double w = 2 * kPi * clampFreq (f, fs) / fs, c = std::cos (w), a = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + a, -2 * c, 1 - a);
    }

    void peak (double fs, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2 * kPi * clampFreq (f, fs) / fs, c = std::cos (w), a = std::sin (w) / (2 * q);
        set (1 + a * A, -2 * c, 1 - a * A, 1 + a / A, -2 * c, 1 - a / A);
    }

    void lowShelf (double fs, double f, double gainDb, double slope = 1.0)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2 * kPi * clampFreq (f, fs) / fs, c = std::cos (w), s = std::sin (w);
        const double a = s / 2 * std::sqrt ((A + 1 / A) * (1 / slope - 1) + 2);
        const double sq = 2 * std::sqrt (A) * a;
        set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
             (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }

    void highShelf (double fs, double f, double gainDb, double slope = 1.0)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2 * kPi * clampFreq (f, fs) / fs, c = std::cos (w), s = std::sin (w);
        const double a = s / 2 * std::sqrt ((A + 1 / A) * (1 / slope - 1) + 2);
        const double sq = 2 * std::sqrt (A) * a;
        set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
             (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
};

//==============================================================================
// Saturation styles. Every curve has unity small-signal gain (slope 1 at 0) so
// the drive knob alone decides how hard the signal hits the "circuit".
enum Style { Tape = 0, Console, Transformer, Triode, Pentode, NumStyles };

inline float softClipPow (float x, float n) noexcept
{
    return x / std::pow (1.0f + std::pow (std::abs (x), n), 1.0f / n);
}

inline float shape (int style, float x) noexcept
{
    switch (style)
    {
        case Tape:
        {
            // Smooth, compressed, mild even harmonics from a small bias.
            constexpr float b = 0.12f;
            const float tb = std::tanh (b);
            return (std::tanh (x + b) - tb) / (1.0f - tb * tb);
        }
        case Console:
        {
            // Brighter, firmer knee: a "driven desk" edge.
            const float y = softClipPow (x * 1.05f, 2.5f) / 1.05f;
            return y + 0.04f * y * y;
        }
        case Transformer:
        {
            // Thick and round: soft curve plus strong 2nd harmonic.
            const float t = std::tanh (0.85f * x) / 0.85f;
            return t + 0.18f * t * t;
        }
        case Triode:
        {
            // Asymmetric tube stage: negative half saturates earlier.
            return x >= 0.0f ? std::tanh (x) : std::tanh (1.6f * x) / 1.6f;
        }
        case Pentode:
        default:
        {
            // Aggressive, lots of odd harmonics, almost square at high drive.
            const float y = softClipPow (x, 5.0f);
            return y - 0.08f * y * y * y + 0.08f * x / (1.0f + x * x);
        }
    }
}

// Per-style voicing: pre-emphasis before the curve and de-emphasis after it.
struct StyleVoicing
{
    double preGainDb;       // extra drive the style adds
    double preLowDb, preLowHz;
    double preHighDb, preHighHz;
    double postLowDb, postLowHz;
    double postHighDb, postHighHz;
    double postPeakDb, postPeakHz, postPeakQ;
    double outTrimDb;
};

inline const StyleVoicing& voicing (int style)
{
    static const StyleVoicing v[NumStyles] = {
        // Tape: highs hit harder (saturate first) then get rolled back, head bump.
        {  0.0,  0.0, 120,  4.0, 5000,  1.5,  90, -4.5, 5000,  0.0, 1000, 0.7,  0.0 },
        // Console: bright and forward.
        {  2.0,  0.0, 120,  3.0, 3500,  0.0, 100, -1.0, 9000,  1.0, 2500, 0.8, -1.0 },
        // Transformer: fat low end drives the core.
        {  1.0,  4.0, 140,  0.0, 5000, -2.0, 140,  0.0, 5000,  0.0, 1000, 0.7, -1.0 },
        // Triode: warm, slightly dark.
        {  3.0,  1.0, 200, -1.0, 7000,  0.0, 100, -1.5, 8000,  1.0,  800, 0.7, -2.0 },
        // Pentode: hot input, mid bark.
        {  8.0,  0.0, 120,  1.5, 2500,  0.0, 100, -2.0, 7000,  2.5, 1400, 0.9, -5.0 },
    };
    return v[std::clamp (style, 0, NumStyles - 1)];
}

//==============================================================================
// One-pole envelope follower for RMS (used for Auto gain and the meter).
struct RmsFollower
{
    double coeff = 0.999, state = 0;
    void setTime (double fs, double seconds) { coeff = std::exp (-1.0 / (seconds * fs)); }
    void reset() { state = 0; }
    inline void push (float x) noexcept { state = coeff * state + (1.0 - coeff) * (double) x * x; }
    double rms() const noexcept { return std::sqrt (state); }
};

inline double dbToGain (double db) { return std::pow (10.0, db / 20.0); }
inline double gainToDb (double g)  { return 20.0 * std::log10 (std::max (g, 1.0e-9)); }

} // namespace dcx
