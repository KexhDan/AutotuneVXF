#include "PluginEditor.h"
#include "MusicalScale.h"

#include <cmath>

namespace
{
    const juce::Colour kBackground  (0xff0d0f13);
    const juce::Colour kPanel       (0xff171a21);
    const juce::Colour kPanelLight  (0xff20242d);
    const juce::Colour kLine        (0xff343a46);
    const juce::Colour kText        (0xffe8edf4);
    const juce::Colour kMuted       (0xff8b95a5);
    const juce::Colour kCyan        (0xff2ed3c6);
    const juce::Colour kOrange      (0xfff0a13c);
}

AutoVoxTuneAudioProcessorEditor::AutoVoxTuneAudioProcessorEditor (AutoVoxTuneAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (780, 610);

    titleLabel.setText ("AUTOVOX TUNE", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (25.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, kText);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Correccion de pitch para voz  -  VST3", juce::dontSendNotification);
    subtitleLabel.setFont (juce::FontOptions (12.0f));
    subtitleLabel.setColour (juce::Label::textColourId, kMuted);
    subtitleLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (subtitleLabel);

    auto setupCombo = [this] (juce::ComboBox& box, juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, kMuted);
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);

        box.setColour (juce::ComboBox::backgroundColourId, kPanelLight);
        box.setColour (juce::ComboBox::textColourId, kText);
        box.setColour (juce::ComboBox::outlineColourId, kLine);
        addAndMakeVisible (box);
    };

    rootBox.addItemList (Musical::getRootNames(), 1);
    scaleBox.addItemList (Musical::getScaleNames(), 1);
    setupCombo (rootBox,  rootLabel,  "RAIZ");
    setupCombo (scaleBox, scaleLabel, "ESCALA");

    auto setupKnob = [this] (juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 18);
        slider.setColour (juce::Slider::rotarySliderFillColourId, kCyan);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, kPanelLight);
        slider.setColour (juce::Slider::textBoxTextColourId, kText);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, kPanel);
        slider.setColour (juce::Slider::textBoxOutlineColourId, kLine);
        addAndMakeVisible (slider);

        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, kMuted);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    setupKnob (retuneSlider,   retuneLabel,   "VELOCIDAD DE SALTO");
    setupKnob (humanizeSlider, humanizeLabel, "HUMANIZACION");
    setupKnob (mixSlider,      mixLabel,      "MEZCLA");
    setupKnob (a4Slider,       a4Label,       "A4 (Hz)");
    setupKnob (formantSlider,  formantLabel,  "FORMANT");
    setupKnob (transposeSlider, transposeLabel, "TRANSPOSE");

    retuneHint.setText ("0 natural   >   100 robotico", juce::dontSendNotification);
    humanizeHint.setText ("0 exacto   >   100 vibrato", juce::dontSendNotification);
    formantHint.setText ("-12 grave   >   0   >   +12 agudo", juce::dontSendNotification);
    transposeHint.setText ("-12 st   >   0 = escala sin cambio   >   +12 st", juce::dontSendNotification);
    for (auto* hint : { &retuneHint, &humanizeHint, &formantHint, &transposeHint })
    {
        hint->setFont (juce::FontOptions (10.0f));
        hint->setColour (juce::Label::textColourId, kMuted);
        hint->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (hint);
    }

    detectedCaption.setText ("NOTA DETECTADA", juce::dontSendNotification);
    detectedCaption.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    detectedCaption.setColour (juce::Label::textColourId, kMuted);
    detectedCaption.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (detectedCaption);

    detectedLabel.setText ("--", juce::dontSendNotification);
    detectedLabel.setFont (juce::FontOptions (54.0f, juce::Font::bold));
    detectedLabel.setColour (juce::Label::textColourId, kCyan);
    detectedLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (detectedLabel);

    centsLabel.setText ("", juce::dontSendNotification);
    centsLabel.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    centsLabel.setColour (juce::Label::textColourId, kOrange);
    centsLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (centsLabel);

    targetLabel.setText ("OBJETIVO  --", juce::dontSendNotification);
    targetLabel.setFont (juce::FontOptions (14.0f));
    targetLabel.setColour (juce::Label::textColourId, kText);
    targetLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (targetLabel);

    correctionLabel.setText ("CORRECCION  0.0 ct", juce::dontSendNotification);
    correctionLabel.setFont (juce::FontOptions (12.0f));
    correctionLabel.setColour (juce::Label::textColourId, kMuted);
    correctionLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (correctionLabel);

    statusLabel.setText ("SILENCIO", juce::dontSendNotification);
    statusLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    statusLabel.setColour (juce::Label::textColourId, kMuted);
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    auto& state = processor.apvts;
    rootAtt     = std::make_unique<ComboAttachment>  (state, "root",  rootBox);
    scaleAtt    = std::make_unique<ComboAttachment>  (state, "scale", scaleBox);
    retuneAtt   = std::make_unique<SliderAttachment> (state, "retune",   retuneSlider);
    humanizeAtt = std::make_unique<SliderAttachment> (state, "humanize", humanizeSlider);
    mixAtt      = std::make_unique<SliderAttachment> (state, "mix",      mixSlider);
    a4Att       = std::make_unique<SliderAttachment> (state, "a4",       a4Slider);
    formantAtt  = std::make_unique<SliderAttachment> (state, "formant",  formantSlider);
    transposeAtt = std::make_unique<SliderAttachment> (state, "transpose", transposeSlider);

    startTimerHz (30);
}

AutoVoxTuneAudioProcessorEditor::~AutoVoxTuneAudioProcessorEditor()
{
    stopTimer();
}

void AutoVoxTuneAudioProcessorEditor::timerCallback()
{
    displayedVoiced = processor.isVoiced();
    displayedCents  = processor.getDeviationCents();

    if (displayedVoiced)
    {
        const int midi = (int) std::floor (processor.getDetectedMidi() + 0.5f);
        const int target = (int) std::floor (processor.getTargetMidi() + 0.5f);

        detectedLabel.setText (Musical::noteName (midi), juce::dontSendNotification);

        const juce::String sign = displayedCents >= 0.0f ? "+" : "-";
        centsLabel.setText (sign + juce::String (std::abs (displayedCents), 0) + " ct",
                            juce::dontSendNotification);

        targetLabel.setText (juce::String ("OBJETIVO  ") + Musical::noteName (target),
                             juce::dontSendNotification);
        statusLabel.setText ("VOZ DETECTADA", juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, kCyan);
    }
    else
    {
        detectedLabel.setText ("--", juce::dontSendNotification);
        centsLabel.setText ("", juce::dontSendNotification);
        targetLabel.setText ("OBJETIVO  --", juce::dontSendNotification);
        statusLabel.setText ("SILENCIO", juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, kMuted);
    }

    correctionLabel.setText (juce::String ("CORRECCION  ")
                                 + juce::String (processor.getCorrectionCents(), 1) + " ct",
                             juce::dontSendNotification);

    repaint();
}

void AutoVoxTuneAudioProcessorEditor::drawPitchMeter (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (bounds.isEmpty())
        return;

    const auto r = bounds.toFloat().reduced (3.0f);

    g.setColour (kPanelLight);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (kLine);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    auto xForCents = [&r] (float cents)
    {
        const float clamped = juce::jlimit (-50.0f, 50.0f, cents);
        return r.getX() + (clamped + 50.0f) / 100.0f * r.getWidth();
    };

    const float centreX = xForCents (0.0f);
    g.setColour (kCyan.withAlpha (0.55f));
    g.drawLine (centreX, r.getY() + 4.0f, centreX, r.getBottom() - 4.0f, 1.5f);

    g.setFont (juce::FontOptions (9.0f));
    for (int cents = -50; cents <= 50; cents += 25)
    {
        const float x = xForCents ((float) cents);
        const bool isCentre = (cents == 0);

        g.setColour (isCentre ? kCyan : kMuted);
        g.drawLine (x, r.getBottom() - 9.0f, x, r.getBottom() - 4.0f, isCentre ? 2.0f : 1.0f);

        g.drawFittedText (juce::String (cents),
                          juce::Rectangle<int> ((int) x - 14, (int) r.getY() + 2, 28, 14),
                          juce::Justification::centred, 1);
    }

    const float needleX = xForCents (displayedCents);
    const float needleY = r.getCentreY();

    g.setColour (displayedVoiced ? kOrange : kMuted);
    g.fillEllipse (needleX - 5.0f, needleY - 5.0f, 10.0f, 10.0f);
    g.setColour (kBackground);
    g.drawEllipse (needleX - 5.0f, needleY - 5.0f, 10.0f, 10.0f, 1.5f);
}

void AutoVoxTuneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBackground);

    const auto face = getLocalBounds().reduced (6).toFloat();
    g.setColour (kPanel);
    g.fillRoundedRectangle (face, 12.0f);
    g.setColour (kLine);
    g.drawRoundedRectangle (face, 12.0f, 1.5f);

    drawPitchMeter (g, meterBounds);

    g.setColour (kMuted);
    g.drawFittedText ("Detector YIN  -  crossfade DSP  -  escala + transpose + formant",
                      getLocalBounds().removeFromBottom (18),
                      juce::Justification::centred, 1);
}

void AutoVoxTuneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);

    auto header = area.removeFromTop (48);
    titleLabel.setBounds (header.removeFromLeft (280));
    subtitleLabel.setBounds (header.removeFromRight (320));
    area.removeFromTop (8);

    auto left = area.removeFromLeft (280);
    area.removeFromLeft (16);
    auto right = area;

    // Panel izquierdo: nota detectada y desviacion en cents.
    auto display = left.reduced (6);
    detectedCaption.setBounds (display.removeFromTop (22));
    detectedLabel.setBounds  (display.removeFromTop (66));
    centsLabel.setBounds     (display.removeFromTop (28));
    display.removeFromTop (8);
    meterBounds = display.removeFromTop (52);
    display.removeFromTop (10);
    targetLabel.setBounds    (display.removeFromTop (26));
    correctionLabel.setBounds(display.removeFromTop (24));
    statusLabel.setBounds    (display.removeFromTop (24));

    // Panel derecho: parametros.
    const int gap = 12;
    const int colWidth = (right.getWidth() - gap) / 2;

    auto comboRow = right.removeFromTop (42);
    auto rootCell = comboRow.removeFromLeft (colWidth);
    rootLabel.setBounds (rootCell.removeFromLeft (66));
    rootBox.setBounds (rootCell.reduced (3));
    comboRow.removeFromLeft (gap);
    auto scaleCell = comboRow;
    scaleLabel.setBounds (scaleCell.removeFromLeft (66));
    scaleBox.setBounds (scaleCell.reduced (3));

    right.removeFromTop (10);

    auto placeKnob = [] (juce::Rectangle<int> cell, juce::Label& label,
                         juce::Slider& slider, juce::Label* hint)
    {
        label.setBounds (cell.removeFromTop (22));
        slider.setBounds (cell.removeFromTop (100));
        if (hint != nullptr)
            hint->setBounds (cell.removeFromTop (18));
    };

    auto knobRow1 = right.removeFromTop (150);
    placeKnob (knobRow1.removeFromLeft (colWidth), retuneLabel, retuneSlider, &retuneHint);
    knobRow1.removeFromLeft (gap);
    placeKnob (knobRow1, humanizeLabel, humanizeSlider, &humanizeHint);

    right.removeFromTop (6);

    auto knobRow2 = right.removeFromTop (128);
    placeKnob (knobRow2.removeFromLeft (colWidth), mixLabel, mixSlider, nullptr);
    knobRow2.removeFromLeft (gap);
    placeKnob (knobRow2, a4Label, a4Slider, nullptr);

    right.removeFromTop (6);

    auto knobRow3 = right.removeFromTop (150);
    placeKnob (knobRow3.removeFromLeft (colWidth), formantLabel, formantSlider, &formantHint);
    knobRow3.removeFromLeft (gap);
    placeKnob (knobRow3, transposeLabel, transposeSlider, &transposeHint);
}
