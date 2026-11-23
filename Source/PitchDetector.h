#pragma once

#include <vector>

// Detector de pitch monofonico basado en YIN (normalized difference function).
// Se ejecuta sobre la senal diezmada por 2 para abaratar el coste en el thread de audio.
class PitchDetector
{
public:
    struct Result
    {
        float frequency = 0.0f; // Hz, 0 si no hay voz
        float clarity   = 0.0f; // 0..1
        bool  voiced    = false;
    };

    static constexpr int decimation     = 2;
    static constexpr int frameSize      = 1024;                    // muestras tras diezmar
    static constexpr int analysisLength = frameSize * decimation;  // muestras a sample rate completo

    void prepare (double sampleRate) noexcept;
    Result detect (const float* samples, int numSamples) noexcept;

private:
    double decimatedRate = 22050.0;
    std::vector<float> x;
    std::vector<float> difference;
    std::vector<float> cmnd;
};
