#include "FreqPlot.h"


FreqPlot::FreqPlot(BandAnalyser& analyser)
    : bandAnalyser(analyser), anFreqs(bandAnalyser.getNumBands()), anMags(bandAnalyser.getNumBands())
{
    bandAnalyser.getFreqs(anFreqs);
    setOpaque(true);
    startTimer(100);
}

void FreqPlot::resized()
{
    plotData.resize(getWidth() / 10 - 2);
}

void FreqPlot::paint(juce::Graphics& g)
{
    const auto h = getHeight();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xFF777777), 0.f, 0.f, juce::Colour(0xFF555555), 0.f, h - 1.f, false));
    g.fillAll();
    drawGrid(g);

    if (bandAnalyser.getEnabled())
    {
        const auto colTop = juce::Colour(0xAA775533);
        const auto colBot = juce::Colour(0xAA558833);
        juce::ColourGradient grad(colTop, 0.f, 1.f, colBot, 0.f, h - 1.f, false);
        grad.addColour(0.48, colTop);
        grad.addColour(0.52, colBot);
        g.setGradientFill(grad);
        g.fillPath(magPath);
    }

    g.setColour(juce::Colours::white);
    g.strokePath(freqRes, juce::PathStrokeType(1.f));
}

void FreqPlot::timerCallback()
{
    if (! bandAnalyser.getEnabled())
        return;

    bandAnalyser.getData(anMags);

    auto meanVal = 0.f;
    for (auto& m : anMags)
        meanVal += m;
    meanVal = juce::Decibels::gainToDecibels(meanVal / anMags.size());

    for (auto& m : anMags)
        m = juce::Decibels::gainToDecibels(m) - meanVal;

    magPath.clear();

    if (meanVal > -80.f)
    {
        magPath.startNewSubPath(getXForFreq(anFreqs.front()), getYForMag(0.f));

        for (size_t i=0; i<anFreqs.size(); ++i)
            magPath.lineTo(getXForFreq(anFreqs[i]), getYForMag(anMags[i]));
        
        magPath.lineTo(getXForFreq(anFreqs.back()), getYForMag(0.f));

        magPath.closeSubPath();
    }

    repaint();
}

void FreqPlot::updateFreqRes()
{
    if (! plotData.valid || plotData.freqs.size() == 0)
        return;

    const auto numPoints = plotData.freqs.size();
    freqRes.clear();
    freqRes.startNewSubPath(getXForFreq(plotData.freqs[0]), getYForMag(plotData.mags[0]));

    for (size_t i = 1; i < numPoints; ++i)
        freqRes.lineTo(getXForFreq(plotData.freqs[i]), getYForMag(plotData.mags[i]));
}

SeeSaw::PlotData& FreqPlot::getPlotData()
{
    return plotData;
}

juce::Font FreqPlot::getFont() const
{
    return juce::Font(juce::FontOptions(0.05f * getHeight()));
}

void FreqPlot::drawGrid(juce::Graphics& g)
{
    auto freqToString = [](float freq) {
        return freq < 1000 ? juce::String(static_cast<int> (freq)) : juce::String(static_cast<int> (freq * 0.001f)) + "k";
    };
    const auto w = getWidth();
    const auto h = getHeight();
    g.setColour(juce::Colours::lightgrey);
    g.drawRect(getLocalBounds());

    std::array<float, 8> freqs{ 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f };
    std::array<float, 7> mags{ -18.f, -12.f, -6.f, 0.f, 6.f, 12.f, 18.f };
    
    auto font = getFont();
    const auto fh = static_cast<int>(font.getHeight() * 1.2f);
    g.setFont(font);
    g.setColour(juce::Colours::lightgrey.withAlpha(0.5f));

    for (auto f : freqs)
    {
        const auto x = static_cast<int> (getXForFreq(f) + 0.5f);
        g.drawVerticalLine(x, 1.f, h - 1.f);
        const auto fs = " " + freqToString(f);
        const auto fw = juce::GlyphArrangement::getStringWidthInt(font, fs);
        g.drawText(fs, x, 1, fw, fh, juce::Justification::centred, false);
        g.drawText(fs, x, h-1-fh, fw, fh, juce::Justification::centred, false);
    }

    for (auto m : mags)
    {
        const auto y = static_cast<int> (getYForMag(m) + 0.5f);
        g.drawHorizontalLine(y, 1.f, w - 1.f);
        const auto fs = " " + freqToString(m);
        const auto fw = juce::GlyphArrangement::getStringWidthInt(font, fs);
        const auto yt = m < 0 ? y + 1 : y - fh - 1;
        g.drawText(fs, 1, yt, fw, fh, juce::Justification::centred, false);
        g.drawText(fs, w-fw-1, yt, fw, fh, juce::Justification::centred, false);
    }
}

float FreqPlot::getXForFreq(float freq) const
{
    return 1.f + (getWidth() - 2.f) * std::log(freq / 20.f) / (std::log(1000.f));
}

float FreqPlot::getYForMag(float mag) const
{
    const auto maxMag = 24.f;
    return 1.f + (getHeight() - 2.f) * (1.f - (mag + maxMag) / (2.f * maxMag));
}

