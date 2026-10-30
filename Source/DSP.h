#pragma once
#include <JuceHeader.h>
#include <complex>
#include <vector>
#include <cmath>

namespace DSP
{
inline float fromDb (float db) { return std::exp (db * 0.11512925f); }

// RBJ biquad. Setting coefficients never allocates (safe for the audio thread).
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }

    inline float process (float x)
    {
        const double in = x;
        const double y = b0 * in + z1;
        z1 = b1 * in - a1 * y + z2;
        z2 = b2 * in - a2 * y;
        return (float) y;
    }

    void copyCoefs (const Biquad& o) { b0 = o.b0; b1 = o.b1; b2 = o.b2; a1 = o.a1; a2 = o.a2; }

    void setRaw (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    static double cf (double sr, double f) { return juce::jlimit (10.0, sr * 0.45, f); }

    void lowShelf (double sr, double f, double dB)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2.0 * juce::MathConstants<double>::pi * cf (sr, f) / sr;
        const double cs = std::cos (w), sn = std::sin (w), al = sn / 2.0 * std::sqrt (2.0), t = 2.0 * std::sqrt (A) * al;
        setRaw (A * ((A + 1) - (A - 1) * cs + t), 2 * A * ((A - 1) - (A + 1) * cs), A * ((A + 1) - (A - 1) * cs - t),
                (A + 1) + (A - 1) * cs + t, -2 * ((A - 1) + (A + 1) * cs), (A + 1) + (A - 1) * cs - t);
    }

    void highShelf (double sr, double f, double dB)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2.0 * juce::MathConstants<double>::pi * cf (sr, f) / sr;
        const double cs = std::cos (w), sn = std::sin (w), al = sn / 2.0 * std::sqrt (2.0), t = 2.0 * std::sqrt (A) * al;
        setRaw (A * ((A + 1) + (A - 1) * cs + t), -2 * A * ((A - 1) + (A + 1) * cs), A * ((A + 1) + (A - 1) * cs - t),
                (A + 1) - (A - 1) * cs + t, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - t);
    }

    void peak (double sr, double f, double Q, double dB)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2.0 * juce::MathConstants<double>::pi * cf (sr, f) / sr;
        const double cs = std::cos (w), al = std::sin (w) / (2.0 * Q);
        setRaw (1 + al * A, -2 * cs, 1 - al * A, 1 + al / A, -2 * cs, 1 - al / A);
    }

    void highpass (double sr, double f, double Q)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * cf (sr, f) / sr;
        const double cs = std::cos (w), al = std::sin (w) / (2.0 * Q);
        setRaw ((1 + cs) / 2, -(1 + cs), (1 + cs) / 2, 1 + al, -2 * cs, 1 - al);
    }

    void bandpass (double sr, double f, double Q)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * cf (sr, f) / sr;
        const double cs = std::cos (w), al = std::sin (w) / (2.0 * Q);
        setRaw (al, 0, -al, 1 + al, -2 * cs, 1 - al);
    }

    // used by the EQ curve display in the editor
    double magnitudeDb (double f, double sr) const
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * f / sr;
        const std::complex<double> e1 = std::polar (1.0, -w), e2 = std::polar (1.0, -2.0 * w);
        const auto num = b0 + b1 * e1 + b2 * e2;
        const auto den = 1.0 + a1 * e1 + a2 * e2;
        return 20.0 * std::log10 (std::abs (num / den) + 1e-12);
    }
};

// Circular delay line with linear interpolation. Allocation happens only in prepare().
struct DelayLine
{
    std::vector<float> buf;
    int w = 0, size = 0;

    void prepare (int n) { size = juce::jmax (8, n); buf.assign ((size_t) size, 0.f); w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.f); w = 0; }
    inline void push (float x) { buf[(size_t) w] = x; if (++w >= size) w = 0; }

    // read BEFORE push: returns the input from d samples ago
    inline float read (float d) const
    {
        d = juce::jlimit (1.0f, (float) (size - 2), d);
        float rp = (float) w - d;
        if (rp < 0.f) rp += (float) size;
        const int i0 = (int) rp;
        const float fr = rp - (float) i0;
        const int i1 = (i0 + 1 >= size) ? 0 : i0 + 1;
        return buf[(size_t) i0] * (1.f - fr) + buf[(size_t) i1] * fr;
    }
};
} // namespace DSP
