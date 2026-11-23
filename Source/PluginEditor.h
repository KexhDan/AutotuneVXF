#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class AutoVoxTuneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit AutoVoxTuneAudioProcessorEditor (AutoVoxTuneAudioProcessor&);
    ~AutoVoxTuneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawPitchMeter (juce::Graphics& g, juce::Rectangle<int> bounds);

    AutoVoxTuneAudioProcessor& processor;

    juce::ComboBox rootBox, scaleBox;
    juce::Slider retuneSlider, humanizeSlider, mixSlider, a4Slider;

    juce::Label titleLabel, subtitleLabel;
    juce::Label rootLabel, scaleLabel;
    juce::Label retuneLabel, humanizeLabel, mixLabel, a4Label;
    juce::Label retuneHint, humanizeHint;
    juce::Label detectedCaption, detectedLabel, centsLabel;
    juce::Label targetLabel, correctionLabel, statusLabel;

    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<ComboAttachment> rootAtt, scaleAtt;
    std::unique_ptr<SliderAttachment> retuneAtt, humanizeAtt, mixAtt, a4Att;

    juce::Rectangle<int> meterBounds;
    float displayedCents = 0.0f;
    bool  displayedVoiced = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoVoxTuneAudioProcessorEditor)
};
