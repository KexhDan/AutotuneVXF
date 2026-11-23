#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MusicalScale.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kPi = 3.14159265358979323846f;

    // retune 0   -> 200 ms (deslizamiento natural)
    // retune 100 ->   2 ms (afinacion "hard", el efecto classico de Auto-Tune)
    float retuneTimeConstant (float retune) noexcept
    {
        const float t = juce::jlimit (0.0f, 100.0f, retune) / 100.0f;
        return 0.20f * std::pow (0.002f / 0.20f, t);
    }
}

AutoVoxTuneAudioProcessor::AutoVoxTuneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    retuneParam    = apvts.getRawParameterValue ("retune");
    humanizeParam  = apvts.getRawParameterValue ("humanize");
    mixParam       = apvts.getRawParameterValue ("mix");
    a4Param        = apvts.getRawParameterValue ("a4");
    transposeParam = apvts.getRawParameterValue ("transpose");
    formantParam   = apvts.getRawParameterValue ("formant");
    rootParam     = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("root"));
    scaleParam    = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("scale"));
}

AutoVoxTuneAudioProcessor::~AutoVoxTuneAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout AutoVoxTuneAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "root", 1 }, "Root", Musical::getRootNames(), 0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "scale", 1 }, "Scale", Musical::getScaleNames(), 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "retune", 1 }, "Retune Speed",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 35.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "humanize", 1 }, "Humanize",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 20.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "a4", 1 }, "A4 Reference",
        juce::NormalisableRange<float> (430.0f, 450.0f, 0.1f), 440.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    // Transpose: -12 .. +12 cents, 0 en el centro = la escala no se desplaza.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "transpose", 1 }, "Transpose",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ct")));

    // Formant: -12 .. +12 semitonos de desplazamiento de formantes, 0 = neutro.
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "formant", 1 }, "Formant",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")));

    return { params.begin(), params.end() };
}

const juce::String AutoVoxTuneAudioProcessor::getName() const { return JucePlugin_Name; }
bool AutoVoxTuneAudioProcessor::acceptsMidi() const { return false; }
bool AutoVoxTuneAudioProcessor::producesMidi() const { return false; }
bool AutoVoxTuneAudioProcessor::isMidiEffect() const { return false; }
double AutoVoxTuneAudioProcessor::getTailLengthSeconds() const
{
    return (double) shifter.getLatencySamples() / sampleRateHz;
}
int AutoVoxTuneAudioProcessor::getNumPrograms() { return 1; }
int AutoVoxTuneAudioProcessor::getCurrentProgram() { return 0; }
void AutoVoxTuneAudioProcessor::setCurrentProgram (int) {}
const juce::String AutoVoxTuneAudioProcessor::getProgramName (int) { return {}; }
void AutoVoxTuneAudioProcessor::changeProgramName (int, const juce::String&) {}

void AutoVoxTuneAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz = sampleRate;

    const int channels = juce::jlimit (1, 2, getMainBusNumInputChannels());

    detector.prepare (sampleRate);
    shifter.prepare (channels);
    formant.prepare (sampleRate, channels);

    analysisRing.assign  ((size_t) PitchDetector::analysisLength, 0.0f);
    analysisFrame.assign ((size_t) PitchDetector::analysisLength, 0.0f);
    analysisWrite = 0;
    samplesSinceDetection = 0;

    dryDelay.assign ((size_t) channels,
                     std::vector<float> ((size_t) shifter.getLatencySamples(), 0.0f));
    dryWrite = 0;

    desiredCents  = 0.0f;
    smoothedCents = 0.0f;
    humanMix      = 0.0f;
    voiceActive   = false;
    vibratoPhase  = 0.0f;
    drift         = 0.0f;
    driftTarget   = 0.0f;
    rngState      = 0x9e3779b9u;
    driftCounter  = 0;

    detectedMidi.store (69.0f);
    deviationCents.store (0.0f);
    targetMidi.store (69.0f);
    correctionCents.store (0.0f);
    voicedFlag.store (0);

    setLatencySamples (shifter.getLatencySamples());
}

void AutoVoxTuneAudioProcessor::releaseResources() {}

bool AutoVoxTuneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut.isDisabled())
        return false;
    if (mainIn != mainOut)
        return false;

    return mainIn == juce::AudioChannelSet::mono()
        || mainIn == juce::AudioChannelSet::stereo();
}

float AutoVoxTuneAudioProcessor::nextRandomSigned() noexcept
{
    // xorshift32: deterministic y sin asignaciones en el audio thread.
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;

    return (float) (rngState & 0xFFFFu) / 32768.0f - 1.0f;
}

void AutoVoxTuneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midi);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jlimit (1, 2, buffer.getNumChannels());

    if (numSamples <= 0 || numChannels <= 0 || dryDelay.empty())
        return;

    const float retune   = retuneParam   ? retuneParam->load()   : 35.0f;
    const float humanAmt = juce::jlimit (0.0f, 1.0f,
                                         (humanizeParam ? humanizeParam->load() : 20.0f) / 100.0f);
    const float wetMix   = juce::jlimit (0.0f, 1.0f,
                                         (mixParam ? mixParam->load() : 100.0f) / 100.0f);
    const float a4          = a4Param ? a4Param->load() : 440.0f;
    const float transposeCt = transposeParam ? transposeParam->load() : 0.0f;
    const float formantSt   = formantParam ? formantParam->load() : 0.0f;

    const int rootIdx  = rootParam  ? rootParam->getIndex()  : 0;
    const int scaleIdx = scaleParam ? scaleParam->getIndex() : 0;
    const auto scaleMask = Musical::makeScaleMask (rootIdx, scaleIdx);

    const float tauSeconds = retuneTimeConstant (retune);
    const float onePole = 1.0f - std::exp (-1.0f / (tauSeconds * (float) sampleRateHz));
    const float humanSmooth = 1.0f - std::exp (-1.0f / (0.005 * (float) sampleRateHz));

    // Los coeficientes de formantes se actualizan una vez por bloque.
    formant.setAmount (formantSt);
    formant.advance (numSamples);

    float inputSamples[2]  = { 0.0f, 0.0f };
    float outputSamples[2] = { 0.0f, 0.0f };

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            inputSamples[ch] = buffer.getSample (ch, i);
            mono += inputSamples[ch];
        }
        mono /= (float) numChannels;

        // 1) Alimentamos el analizador de pitch con una ventana solapada.
        analysisRing[(size_t) analysisWrite] = mono;
        analysisWrite = (analysisWrite + 1) % PitchDetector::analysisLength;

        if (++samplesSinceDetection >= kDetectionHop)
        {
            samplesSinceDetection = 0;

            double sumSquares = 0.0;
            for (int j = 0; j < PitchDetector::analysisLength; ++j)
            {
                const float sample =
                    analysisRing[(size_t) ((analysisWrite + j) % PitchDetector::analysisLength)];
                analysisFrame[(size_t) j] = sample;
                sumSquares += (double) sample * (double) sample;
            }

            const auto result = detector.detect (analysisFrame.data(), PitchDetector::analysisLength);
            const float rms = (float) std::sqrt (sumSquares / (double) PitchDetector::analysisLength);

            // Puerta de nivel: evita que el ruido de fondo sea tratado como voz.
            if (result.voiced && rms > 0.0005f)
            {
                const double midi = Musical::frequencyToMidi ((double) result.frequency, a4);
                const int nearestSemitone = (int) std::floor (midi + 0.5);
                const int snappedNote = Musical::snapToScale (midi, scaleMask);

                // Transpose se suma DESPUES de la escala: con 0 ct la nota
                // objetivo es exactamente la que marca la escala elegida.
                const double targetMidiValue =
                    (double) snappedNote + (double) transposeCt / 100.0;
                const double targetFreq = Musical::midiToFrequency (targetMidiValue, a4);

                // Cuantos centavos hay que desplazar la senal (positivo = subir).
                desiredCents = (float) (1200.0 * std::log2 (targetFreq / (double) result.frequency));

                detectedMidi.store ((float) midi);
                deviationCents.store ((float) ((midi - (double) nearestSemitone) * 100.0));
                targetMidi.store ((float) targetMidiValue);
                voiceActive = true;
                voicedFlag.store (1);
            }
            else
            {
                desiredCents = 0.0f;
                voiceActive = false;
                voicedFlag.store (0);
            }
        }

        // 2) Vibrato y deriva se actualizan por muestra para que suenen continuos.
        vibratoPhase += 2.0f * kPi * kVibratoHz / (float) sampleRateHz;
        if (vibratoPhase >= 2.0f * kPi)
            vibratoPhase -= 2.0f * kPi;

        if (++driftCounter >= (int) (sampleRateHz * 0.2))
        {
            driftCounter = 0;
            driftTarget = nextRandomSigned();
        }
        drift += (driftTarget - drift) * 0.00008f;

        // 3) La "velocidad de salto" es la constante de este suavizado.
        smoothedCents += onePole * (desiredCents - smoothedCents);

        // La humanizacion se aplica DESPUES del suavizado: asi el vibrato sigue
        // siendo audible aunque la velocidad de salto sea muy lenta. Se funde
        // rapidamente para evitar clics al entrar/salir de la voz.
        humanMix += humanSmooth * ((voiceActive ? humanAmt : 0.0f) - humanMix);
        const float humanCents = humanMix * (14.0f * std::sin (vibratoPhase) + 8.0f * drift);
        const float totalCents = smoothedCents + humanCents;
        const float ratio = std::pow (2.0f, totalCents / 1200.0f);

        // 4) Desplazamiento de pitch + formantes, solo sobre la senal humeda.
        shifter.processSample (inputSamples, outputSamples, numChannels, ratio);
        for (int ch = 0; ch < numChannels; ++ch)
            outputSamples[ch] = formant.processSample (outputSamples[ch], ch);
        correctionCents.store (totalCents);

        const int delayLength = (int) dryDelay[0].size();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float dry = dryDelay[(size_t) ch][(size_t) dryWrite];
            dryDelay[(size_t) ch][(size_t) dryWrite] = inputSamples[ch];
            buffer.setSample (ch, i, dry * (1.0f - wetMix) + outputSamples[ch] * wetMix);
        }

        dryWrite = (dryWrite + 1) % delayLength;
    }
}

bool AutoVoxTuneAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AutoVoxTuneAudioProcessor::createEditor()
{
    return new AutoVoxTuneAudioProcessorEditor (*this);
}

void AutoVoxTuneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AutoVoxTuneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoVoxTuneAudioProcessor();
}
