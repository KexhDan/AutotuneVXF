#include "PitchDetector.h"

#include <algorithm>
#include <cmath>

namespace
{
    inline float clampValue (float v, float lo, float hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    inline float clamp01 (float v) noexcept { return clampValue (v, 0.0f, 1.0f); }
}

void PitchDetector::prepare (double sampleRate) noexcept
{
    decimatedRate = sampleRate / (double) decimation;

    x.assign (frameSize, 0.0f);
    difference.assign (frameSize + 1, 0.0f);
    cmnd.assign (frameSize + 1, 1.0f);
}

PitchDetector::Result PitchDetector::detect (const float* samples, int numSamples) noexcept
{
    if (samples == nullptr || numSamples < analysisLength || x.empty())
        return {};

    // Diezmado con filtro caja de 4 tapas: primer cero en sr/4 = nueva frecuencia de Nyquist.
    for (int i = 0; i < frameSize; ++i)
    {
        const int base = i * decimation;
        float sum = 0.0f;
        int count = 0;

        for (int k = -1; k <= 2; ++k)
        {
            const int idx = base + k;
            if (idx >= 0 && idx < numSamples)
            {
                sum += samples[idx];
                ++count;
            }
        }

        x[(size_t) i] = count > 0 ? sum / (float) count : 0.0f;
    }

    const int tauMin = std::max (2, (int) (decimatedRate / 1200.0));  // hasta ~1200 Hz
    const int tauMax = std::min (frameSize / 2, (int) (decimatedRate / 70.0)); // desde ~70 Hz
    const int half   = frameSize / 2;

    if (tauMax <= tauMin + 2)
        return {};

    // 1) Funcion diferencia de YIN.
    std::fill (difference.begin(), difference.end(), 0.0f);

    for (int tau = 1; tau <= tauMax; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < half; ++j)
        {
            const float d = x[(size_t) j] - x[(size_t) (j + tau)];
            sum += d * d;
        }
        difference[(size_t) tau] = sum;
    }

    // 2) Cumulative mean normalized difference.
    double running = 0.0;
    cmnd[0] = 1.0f;

    for (int tau = 1; tau <= tauMax; ++tau)
    {
        running += difference[(size_t) tau];
        cmnd[(size_t) tau] = running > 1.0e-12
                           ? (float) (difference[(size_t) tau] * (double) tau / running)
                           : 1.0f;
    }

    // 3) Primer minimo por debajo del umbral; si no, minimo global.
    int bestTau = -1;
    constexpr float kThreshold = 0.13f;

    for (int tau = tauMin; tau <= tauMax; ++tau)
    {
        if (cmnd[(size_t) tau] < kThreshold)
        {
            int t = tau;
            while (t < tauMax && cmnd[(size_t) (t + 1)] < cmnd[(size_t) t])
                ++t;
            bestTau = t;
            break;
        }
    }

    if (bestTau < 0)
    {
        float minimum = 1.0f;
        for (int tau = tauMin; tau <= tauMax; ++tau)
        {
            if (cmnd[(size_t) tau] < minimum)
            {
                minimum = cmnd[(size_t) tau];
                bestTau = tau;
            }
        }
    }

    if (bestTau < tauMin || bestTau > tauMax)
        return {};

    const float minimum = cmnd[(size_t) bestTau];
    const float clarity = clamp01 (1.0f - minimum);

    // Silencio / ruido: la difference function es ~0 y la CMND queda en 1.
    // 0.80 equivale a cmnd < 0.20, cerca del umbral clasico de YIN (0.10-0.15).
    if (clarity < 0.80f)
        return { 0.0f, clarity, false };

    // 4) Interpolacion parabolica para precision sub-muestra.
    float period = (float) bestTau;

    if (bestTau > tauMin && bestTau < tauMax)
    {
        const float a = cmnd[(size_t) (bestTau - 1)];
        const float b = cmnd[(size_t) bestTau];
        const float c = cmnd[(size_t) (bestTau + 1)];
        const float denom = a - 2.0f * b + c;

        if (std::abs (denom) > 1.0e-8f)
        {
            float delta = 0.5f * (a - c) / denom;
            delta = clampValue (delta, -0.5f, 0.5f);
            period += delta;
        }
    }

    if (period <= 0.0f)
        return {};

    const float frequency = (float) (decimatedRate / (double) period);

    if (frequency < 60.0f || frequency > 1500.0f)
        return { frequency, clarity, false };

    return { frequency, clarity, true };
}
