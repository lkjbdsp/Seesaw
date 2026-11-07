#pragma once

#include "lkjbfilter.h"
#include <vector>

class SeeSaw
{
public:

    struct PlotData
    {
        void resize(size_t newSize);
        void setSampleRate(double newSampleRate);
        std::vector<float> freqs;
        std::vector<float> sin2w2;
        std::vector<float> mags;
        double sampleRate = 0.;
        bool valid = false;
    };

    void reset();
    bool setSeeSaw(float freq, float gain, double sampleRate);
    void processBlock(float* ch, int numSamples);
    void processBlock(float* chL, float* chR, int numSamples);
    void getResponse(PlotData& plotData) const;

private:

    lkjb::MZTIFilterParams<float> filters[3];
    lkjb::MZTIFilterParams<float>::State states[3];
    lkjb::MZTIFilterParams<float>::State statesRight[3];
    float tiltGain = 1.f;

    float curFreq = 0.f;
    float curGain = 0.f;
    double curSampleRate = 0.;
};