#pragma once

#include <juce_core/juce_core.h>

#include <array>

namespace Musical
{
    // Nombres para los combos de la interfaz / automatizacion.
    const juce::StringArray& getRootNames();
    const juce::StringArray& getScaleNames();

    // Mascara de 12 pitch classes (0 = C, 1 = C#, ... 11 = B).
    using ScaleMask = std::array<bool, 12>;

    ScaleMask makeScaleMask (int rootIndex, int scaleIndex) noexcept;

    // Devuelve la nota MIDI entera permitida mas cercana a 'midiNote' (puede tener decimales).
    int snapToScale (double midiNote, const ScaleMask& mask) noexcept;

    double midiToFrequency (double midiNote, double referenceA4 = 440.0) noexcept;
    double frequencyToMidi (double frequency, double referenceA4 = 440.0) noexcept;

    // p. ej. 69 -> "A4", 61 -> "C#4"
    juce::String noteName (int midiNote);
}
