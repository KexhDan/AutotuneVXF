#include "PitchShifter.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kPi = 3.14159265358979323846f;

    inline float clampValue (float v, float lo, float hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }
}

void PitchShifter::prepare (int numChannels)
{
    const int channels = std::max (1, numChannels);
    buffer.assign ((size_t) channels, std::vector<float> ((size_t) length, 0.0f));
    reset();
}

void PitchShifter::reset() noexcept
{
    writePos = 0;
    readPos  = (float) window; // retardo nominal = window muestras

    for (auto& channelBuffer : buffer)
        std::fill (channelBuffer.begin(), channelBuffer.end(), 0.0f);
}

void PitchShifter::processSample (const float* input, float* output,
                                  int numChannels, float ratio) noexcept
{
    if (input == nullptr || output == nullptr || buffer.empty())
        return;

    ratio = clampValue (ratio, 0.5f, 2.0f);

    const int channels = std::min (numChannels, (int) buffer.size());

    // Escribimos la muestra actual antes de leer.
    for (int ch = 0; ch < channels; ++ch)
        buffer[(size_t) ch][(size_t) writePos] = input[ch];

    // Fase de la cabeza 0 dentro del ciclo completo (0 .. 2*window).
    float phase = readPos - (float) writePos;
    phase = std::fmod (phase, (float) length);
    if (phase < 0.0f)
        phase += (float) length;

    const float gain0 = windowGain (phase);
    const float gain1 = 1.0f - gain0; // las dos ventanas se complementan exactamente

    float secondHead = readPos + (float) window;
    if (secondHead >= (float) length)
        secondHead -= (float) length;

    for (int ch = 0; ch < channels; ++ch)
    {
        const float s0 = readInterpolated (ch, readPos);
        const float s1 = readInterpolated (ch, secondHead);
        output[ch] = gain0 * s0 + gain1 * s1;
    }

    // Avanzamos los cursores: la lectura se mueve 'ratio' muestras por cada
    // muestra escrita, de modo que la frecuencia de salida = ratio * frecuencia de entrada.
    if (++writePos >= length)
        writePos = 0;

    readPos += ratio;
    while (readPos >= (float) length)
        readPos -= (float) length;
}

float PitchShifter::readInterpolated (int channel, float position) const noexcept
{
    const auto& data = buffer[(size_t) channel];

    int i0 = (int) position;
    if (i0 >= length)
        i0 = length - 1;
    if (i0 < 0)
        i0 = 0;

    const float fraction = clampValue (position - (float) i0, 0.0f, 1.0f);
    int i1 = i0 + 1;
    if (i1 >= length)
        i1 = 0;

    return data[(size_t) i0] * (1.0f - fraction) + data[(size_t) i1] * fraction;
}

float PitchShifter::windowGain (float phase) const noexcept
{
    // Hann a lo largo de todo el buffer: 0 -> 1 en la primera mitad, 1 -> 0 en la segunda.
    if (phase < (float) window)
        return 0.5f - 0.5f * std::cos (kPi * phase / (float) window);

    return 0.5f + 0.5f * std::cos (kPi * (phase - (float) window) / (float) window);
}
