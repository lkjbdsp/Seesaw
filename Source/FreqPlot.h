#pragma once

#include "JuceHeader.h"
#include "SeeSaw.h"
#include "BandAnalyser/BandAnalyser.h"

class FreqPlot : public juce::Component, public juce::Timer
{
public:

    FreqPlot(BandAnalyser& analyser);
    void resized() override;
    void paint(juce::Graphics& g) override;

    void timerCallback() override;
    void updateFreqRes();
    SeeSaw::PlotData& getPlotData();
    juce::Font getFont() const;

private:

    void drawGrid(juce::Graphics& g);
    
    float getXForFreq(float freq) const;
    float getYForMag(float mag) const;

    BandAnalyser& bandAnalyser;
    SeeSaw::PlotData plotData;
    juce::Path freqRes;
    juce::Path magPath;
    std::vector<float> anFreqs;
    std::vector<float> anMags;
};