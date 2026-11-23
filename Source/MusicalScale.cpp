#include "MusicalScale.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
    const juce::StringArray kRootNames { "C", "C#", "D", "D#", "E", "F",
                                         "F#", "G", "G#", "A", "A#", "B" };

    // El orden debe coincidir con el ComboBox de la UI y con el parametro 'scale'.
    const juce::StringArray kScaleNames {
        "Chromatic",
        "Major",
        "Natural Minor",
        "Harmonic Minor",
        "Melodic Minor",
        "Dorian",
        "Phrygian",
        "Lydian",
        "Mixolydian",
        "Locrian",
        "Major Pentatonic",
        "Minor Pentatonic",
        "Blues"
    };

    const std::vector<std::vector<int>> kIntervals {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, // Cromatica
        { 0, 2, 4, 5, 7, 9, 11 },                  // Mayor (Jonio)
        { 0, 2, 3, 5, 7, 8, 10 },                  // Menor natural (Eolio)
        { 0, 2, 3, 5, 7, 8, 11 },                  // Menor armonica
        { 0, 2, 3, 5, 7, 9, 11 },                  // Menor melodica (ascendente)
        { 0, 2, 3, 5, 7, 9, 10 },                  // Dorio
        { 0, 1, 3, 5, 7, 8, 10 },                  // Frigio
        { 0, 2, 4, 6, 7, 9, 11 },                  // Lidio
        { 0, 2, 4, 5, 7, 9, 10 },                  // Mixolidio
        { 0, 1, 3, 5, 6, 8, 10 },                  // Locrio
        { 0, 2, 4, 7, 9 },                         // Pentatonica mayor
        { 0, 3, 5, 7, 10 },                        // Pentatonica menor
        { 0, 3, 5, 6, 7, 10 }                      // Blues
    };
}

namespace Musical
{
    const juce::StringArray& getRootNames()  { return kRootNames; }
    const juce::StringArray& getScaleNames() { return kScaleNames; }

    ScaleMask makeScaleMask (int rootIndex, int scaleIndex) noexcept
    {
        ScaleMask mask {};
        mask.fill (false);

        const int root = juce::jlimit (0, 11, rootIndex);
        const int scale = juce::jlimit (0, (int) kIntervals.size() - 1, scaleIndex);

        for (int interval : kIntervals[(size_t) scale])
            mask[(size_t) ((root + interval) % 12)] = true;

        return mask;
    }

    int snapToScale (double midiNote, const ScaleMask& mask) noexcept
    {
        const int centre = (int) std::floor (midiNote + 0.5);
        const int baseOctave = (int) std::floor (centre / 12.0);

        int bestNote = centre;
        double bestDistance = std::numeric_limits<double>::max();

        // Buscamos en la octave central y una a cada lado; suficiente para any note.
        for (int octave = baseOctave - 1; octave <= baseOctave + 1; ++octave)
        {
            for (int pc = 0; pc < 12; ++pc)
            {
                if (! mask[(size_t) pc])
                    continue;

                const int candidate = octave * 12 + pc;
                const double distance = std::abs ((double) candidate - midiNote);

                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestNote = candidate;
                }
            }
        }

        return bestNote;
    }

    double midiToFrequency (double midiNote, double referenceA4) noexcept
    {
        return referenceA4 * std::pow (2.0, (midiNote - 69.0) / 12.0);
    }

    double frequencyToMidi (double frequency, double referenceA4) noexcept
    {
        if (frequency <= 0.0 || referenceA4 <= 0.0)
            return 0.0;

        return 69.0 + 12.0 * std::log2 (frequency / referenceA4);
    }

    juce::String noteName (int midiNote)
    {
        const int pc = ((midiNote % 12) + 12) % 12;
        const int octave = (int) std::floor (midiNote / 12.0) - 1;
        return kRootNames[(size_t) pc] + juce::String (octave);
    }
}
