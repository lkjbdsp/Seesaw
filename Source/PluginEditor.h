#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "FreqPlot.h"
#include "SliderLookAndFeel.h"

class AnalyserButter : public juce::TextButton
{
public:

    AnalyserButter(const juce::String& label);
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font font;
private:

    
};

class Lkjb_seesawAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::AudioProcessorValueTreeState::Listener, public juce::AsyncUpdater, public juce::Timer
{
public:
    Lkjb_seesawAudioProcessorEditor (Lkjb_seesawAudioProcessor&);
    ~Lkjb_seesawAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void handleAsyncUpdate() override;
    void timerCallback() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    static void drawVerticalText(const juce::Graphics& g, const juce::String& text, const juce::Font& font, const juce::Rectangle<int> rect);
    void updateAnalyserButtons();
    void updateMonoState();

private:
    
    void setupSlider(juce::Slider& s);
    void setupButton(juce::TextButton& b, int value);
    void syncProcMode(bool notifyChange);
    void updateFreqRes();

    Lkjb_seesawAudioProcessor& audioProcessor;

    juce::ComponentBoundsConstrainer constrainer;
    juce::ResizableCornerComponent resizer;
    juce::Image noiseImg;
    
    FreqPlot freqPlot;

    juce::Slider freq;
    juce::Slider gain;
    juce::TextButton stereo;
    juce::TextButton mid;
    juce::TextButton side;
    AnalyserButter anInput;
    AnalyserButter anOutput;


    SliderLookAndFeel sliderLookAndFeel;

    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Lkjb_seesawAudioProcessorEditor)
};
