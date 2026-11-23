#pragma once

#include <vector>

// Desplazador de pitch en el dominio del tiempo: dos cabezas de lectura separadas
// medio buffer con crossfade en ventana Hann. Funciona bien para correcciones
// pequenas (las tipicas de una voz, +/- un semitono) con latencia baja.
class PitchShifter
{
public:
    void prepare (int numChannels);
    void reset() noexcept;

    // 'input' y 'output' son arrays de numChannels muestras en el instante actual.
    void processSample (const float* input, float* output, int numChannels, float ratio) noexcept;

    int getLatencySamples() const noexcept { return window; }

private:
    static constexpr int kWindow = 1024; // ~23 ms a 44.1 kHz

    int window = kWindow;
    int length = kWindow * 2;
    int writePos = 0;
    float readPos = 0.0f;

    std::vector<std::vector<float>> buffer;

    float readInterpolated (int channel, float position) const noexcept;
    float windowGain (float phase) const noexcept;
};
