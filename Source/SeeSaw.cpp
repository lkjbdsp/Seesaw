#include "SeeSaw.h"
#include "JuceHeader.h"

void SeeSaw::PlotData::resize(size_t newSize)
{
    if (newSize == freqs.size())
        return;

    freqs.resize(newSize);
    sin2w2.resize(newSize);
    mags.resize(newSize);

    for (size_t i = 0; i < newSize; ++i)
        freqs[i] = 20.f * std::pow(10.f, 3.f * i / (newSize - 1.f));

    if (sampleRate != 0.)
    {
        sampleRate = 0.;
        setSampleRate(sampleRate);
    }
    valid = false;
}

void SeeSaw::PlotData::setSampleRate(double newSampleRate)
{
    if (newSampleRate == sampleRate && newSampleRate != 0)
        return;

    sampleRate = newSampleRate;
    valid = false;
}

void SeeSaw::reset()
{
    for (int i = 0; i < 3; ++i)
    {
        states[i].clear();
        statesRight[i].clear();
    }
}

bool SeeSaw::setSeeSaw(float freq, float gain, double sampleRate)
{
    if (freq == curFreq && gain == curGain && sampleRate == curSampleRate)
        return false;

    curFreq = freq;
    curGain = gain;
    curSampleRate = sampleRate;

    const auto Q = 0.3650f;
    const auto spread = 7.9818f;
    const auto g3Mod = 0.205f;
    const auto q3Mod = 0.72f;

    const auto gain3 = gain * g3Mod;
    const auto Q3 = Q * std::pow(2.f, q3Mod);
    const auto midFreq = std::sqrt(20.f * 20e3f);
    const auto loFreq = midFreq / spread;
    const auto hiFreq = midFreq * spread;

    lkjb::MatchedZTransform::setMZTiFilter(filters[0], loFreq, Q, -gain, lkjb::filterTypeLoShelf, sampleRate);
    lkjb::MatchedZTransform::setMZTiFilter(filters[1], hiFreq, Q, gain, lkjb::filterTypeHiShelf, sampleRate);
    lkjb::MatchedZTransform::setMZTiFilter(filters[2], midFreq, Q3, gain3, lkjb::filterTypeHiShelf, sampleRate);

    auto gainComp = 1.f;
    for (int i = 0; i < 3; ++i)
        gainComp *= static_cast<float> (filters[i].response(freq / sampleRate));

    tiltGain = 1.f / gainComp;

    return true;
}

void SeeSaw::processBlock(float* ch, int numSamples)
{
    if (ch == nullptr)
        return;


    auto processFilter = [](float* inOut, const lkjb::MZTIFilterParams<float>& filter, lkjb::MZTIFilterParams<float>::State& state, int numSamples) {
        for (int i = 0; i < numSamples; ++i)
            inOut[i] = filter.process(state, inOut[i]);
    };

    for (int i = 0; i < 3; ++i)
        processFilter(ch, filters[i], states[i], numSamples);

    juce::FloatVectorOperations::multiply(ch, tiltGain, numSamples);
}

void SeeSaw::processBlock(float* chL, float* chR, int numSamples)
{
    if (chR == nullptr)
    {
        processBlock(chL, numSamples);
        return;
    }

    auto processFilter = [](float* ioL, float* ioR, const lkjb::MZTIFilterParams<float>& filter, 
        lkjb::MZTIFilterParams<float>::State& state, lkjb::MZTIFilterParams<float>::State& stateRight,
        int numSamples) {
        for (int i = 0; i < numSamples; ++i)
        {
            ioL[i] = filter.process(state, ioL[i]);
            ioR[i] = filter.process(stateRight, ioR[i]);
        }
            
    };

    for (int i = 0; i < 3; ++i)
        processFilter(chL, chR, filters[i], states[i], statesRight[i], numSamples);

    juce::FloatVectorOperations::multiply(chL, tiltGain, numSamples);
    juce::FloatVectorOperations::multiply(chR, tiltGain, numSamples);
}

void SeeSaw::getResponse(PlotData& plotData) const
{
    if (curSampleRate == 0)
    {
        plotData.valid = false;
        return;
    }

    plotData.setSampleRate(curSampleRate);

    for (auto& m : plotData.mags)
        m = 1.f;

    auto getResponse = [](PlotData& data, const lkjb::MZTIFilterParams<float>& filter) {
        for (size_t i = 0; i < data.freqs.size(); ++i)
            data.mags[i] *= static_cast<float> (filter.response(data.freqs[i] / data.sampleRate));
    };

    for (int i = 0; i < 3; ++i)
        getResponse(plotData, filters[i]);

    const auto tg2 = tiltGain * tiltGain;

    for (auto& m : plotData.mags)
        m = 10.f * std::log10(tg2 * m);

    plotData.valid = true;
}
