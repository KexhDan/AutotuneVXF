#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <vector>

#include "PitchDetector.h"
#include "PitchShifter.h"
#include "FormantFilter.h"

class AutoVoxTuneAudioProcessor : public juce::AudioProcessor
{
public:
    AutoVoxTuneAudioProcessor();
    ~AutoVoxTuneAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Valores leidos por la UI (30 Hz) para el visualizador de pitch.
    float getDetectedMidi()  const noexcept { return detectedMidi.load(); }
    float getDeviationCents() const noexcept { return deviationCents.load(); }
    float getTargetMidi()    const noexcept { return targetMidi.load(); }
    float getCorrectionCents() const noexcept { return correctionCents.load(); }
    bool  isVoiced()         const noexcept { return voicedFlag.load() != 0; }

private:
    static constexpr int   kDetectionHop = 512;  // cada ~11.6 ms a 44.1 kHz
    static constexpr float kVibratoHz    = 5.0f;

    // DSP
    PitchDetector  detector;
    PitchShifter   shifter;
    FormantShifter formant;

    std::vector<float> analysisRing;
    std::vector<float> analysisFrame;
    int analysisWrite = 0;
    int samplesSinceDetection = 0;

    std::vector<std::vector<float>> dryDelay;
    int dryWrite = 0;

    double sampleRateHz = 44100.0;

    float desiredCents   = 0.0f;
    float smoothedCents  = 0.0f;
    float humanMix       = 0.0f;
    bool  voiceActive    = false;
    float vibratoPhase   = 0.0f;
    float drift          = 0.0f;
    float driftTarget    = 0.0f;
    unsigned int rngState = 0x9e3779b9u;
    int driftCounter = 0;

    float nextRandomSigned() noexcept;

    // Parametros (cacheados para el audio thread)
    std::atomic<float>* retuneParam    = nullptr;
    std::atomic<float>* humanizeParam  = nullptr;
    std::atomic<float>* mixParam       = nullptr;
    std::atomic<float>* a4Param        = nullptr;
    std::atomic<float>* transposeParam = nullptr;
    std::atomic<float>* formantParam   = nullptr;
    juce::AudioParameterChoice* rootParam  = nullptr;
    juce::AudioParameterChoice* scaleParam = nullptr;

    // Lecturas para la UI
    std::atomic<float> detectedMidi    { 69.0f };
    std::atomic<float> deviationCents  { 0.0f };
    std::atomic<float> targetMidi      { 69.0f };
    std::atomic<float> correctionCents { 0.0f };
    std::atomic<int>   voicedFlag      { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoVoxTuneAudioProcessor)
};
