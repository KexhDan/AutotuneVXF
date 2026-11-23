#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <vector>

// Realza y desplaza los formantes vocales (F1/F2/F3) con un banco de filtros
// peaking. Trabaja sobre la senal ya corregida, por lo que no altera el pitch.
// Rango: -12 .. +12 semitonos, 0 = sin cambio.
class FormantShifter
{
public:
    void prepare (double sampleRate, int numChannels);
    void reset() noexcept;
    void setAmount (float semitones) noexcept { targetAmount = semitones; }

    // Suaviza el parametro y actualiza los coeficientes una vez por bloque.
    void advance (int numSamples) noexcept;

    float processSample (float input, int channel) noexcept;

private:
    struct Bank
    {
        juce::SingleThreadedIIRFilter boost[3];
        juce::SingleThreadedIIRFilter cut[3];
        juce::SingleThreadedIIRFilter tilt;
    };

    void updateCoefficients() noexcept;

    std::vector<Bank> banks;
    double sampleRate = 44100.0;
    float targetAmount = 0.0f;
    float currentAmount = 0.0f;
};
