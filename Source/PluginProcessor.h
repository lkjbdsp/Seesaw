#pragma once

#include <JuceHeader.h>
#include "SeeSaw.h"
#include "BandAnalyser/BandAnalyser.h"

class Lkjb_seesawAudioProcessorEditor;

class Lkjb_seesawAudioProcessor  : public juce::AudioProcessor
{
public:
    enum AnalyserMode
    {
        kAnalyserDisabled,
        kAnalyserPre,
        kAnalyserPost
    };
    Lkjb_seesawAudioProcessor();
    ~Lkjb_seesawAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    bool applyBusLayouts(const BusesLayout& layouts) override;

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

    void updateBandAnalyserEnabled();

    juce::AudioParameterFloat* freq = nullptr;
    juce::AudioParameterFloat* gain = nullptr;
    juce::AudioParameterInt* mode = nullptr;
    SeeSaw seeSaw;
    BandAnalyser bandAnalyser;
    bool isMono = false;
    AnalyserMode analyserMode = kAnalyserPre;
    juce::AudioProcessorValueTreeState parameters;
    juce::Point<int> guiSize = juce::Point<int>(480, 300);

    bool seeSawWasUpdated = false;
    Lkjb_seesawAudioProcessorEditor* currentEditorPointer = nullptr;

private:

    void updateParameters();
    juce::AudioBuffer<float> procBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Lkjb_seesawAudioProcessor)
};
