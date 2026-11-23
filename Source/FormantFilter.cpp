#include "FormantFilter.h"

#include <algorithm>
#include <cmath>

void FormantShifter::prepare (double newSampleRate, int numChannels)
{
    sampleRate = juce::jmax (8000.0, newSampleRate);

    banks.clear();
    banks.resize ((size_t) juce::jmax (1, numChannels));

    currentAmount = targetAmount;
    updateCoefficients();
    reset();
}

void FormantShifter::reset() noexcept
{
    for (auto& bank : banks)
    {
        for (int i = 0; i < 3; ++i)
        {
            bank.boost[i].reset();
            bank.cut[i].reset();
        }
        bank.tilt.reset();
    }
}

void FormantShifter::advance (int numSamples) noexcept
{
    if (banks.empty() || numSamples <= 0)
        return;

    const float smooth =
        1.0f - std::exp (- (float) numSamples / (0.02f * (float) sampleRate));
    const float next = currentAmount + smooth * (targetAmount - currentAmount);

    // Solo reconstruye los coeficientes cuando el cambio es audible.
    if (std::abs (next - currentAmount) < 0.01f)
        return;

    currentAmount = next;
    updateCoefficients();
}

void FormantShifter::updateCoefficients() noexcept
{
    const double ratio   = std::pow (2.0, (double) currentAmount / 12.0);
    const double nyquist = sampleRate * 0.45;
    const double amount  = juce::jlimit (0.0, 1.0, std::abs ((double) currentAmount) / 12.0);

    const auto limitFreq = [nyquist] (double f)
    {
        return juce::jlimit (60.0, nyquist, f);
    };

    // Formantes aproximados de una vocal neutra y sus anchos de banda.
    const double baseFreq[3] = { 500.0, 1500.0, 2800.0 };
    const double boostDb[3]  = { 6.0,   5.0,    4.0    };
    const double cutDb[3]    = { 4.0,   3.0,    2.0    };
    const double qValues[3]  = { 1.6,   2.0,    2.4    };

    for (auto& bank : banks)
    {
        for (int i = 0; i < 3; ++i)
        {
            bank.boost[i].setCoefficients (
                juce::IIRCoefficients::makePeakFilter (
                    sampleRate, limitFreq (baseFreq[i] * ratio), qValues[i],
                    juce::Decibels::decibelsToGain ((float) (boostDb[i] * amount))));

            bank.cut[i].setCoefficients (
                juce::IIRCoefficients::makePeakFilter (
                    sampleRate, limitFreq (baseFreq[i]), qValues[i],
                    juce::Decibels::decibelsToGain ((float) (-cutDb[i] * amount))));
        }

        // Inclinacion suave para reforzar el cambio de "tamano" de la vocal.
        bank.tilt.setCoefficients (
            juce::IIRCoefficients::makeHighShelf (
                sampleRate, 3200.0, 0.7,
                juce::Decibels::decibelsToGain (currentAmount * 0.2f)));
    }
}

float FormantShifter::processSample (float input, int channel) noexcept
{
    if (banks.empty())
        return input;

    const int index = juce::jlimit (0, (int) banks.size() - 1, channel);
    auto& bank = banks[(size_t) index];

    float y = input;
    for (int i = 0; i < 3; ++i)
    {
        y = bank.boost[i].processSingleSampleRaw (y);
        y = bank.cut[i].processSingleSampleRaw (y);
    }

    return bank.tilt.processSingleSampleRaw (y);
}
