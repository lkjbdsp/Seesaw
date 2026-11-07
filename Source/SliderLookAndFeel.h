#pragma once
#include "JuceHeader.h"

class SliderLookAndFeel : public juce::LookAndFeel_V4
{
public:

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle,
        float rotaryEndAngle, juce::Slider&) override
    {
        const auto rect = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(width*0.05f);
        const auto colin = juce::Colour(0xFF333333);
        const auto colOut = juce::Colour(0xFF777777);

        juce::ColourGradient grad(colin.withAlpha(0.5f), rect.getCentre(), colOut, juce::Point<float> (rect.getCentreX(), rect.getY()), true);
        grad.addColour(0.7f, colin);
        g.setGradientFill(grad);
        g.fillEllipse(rect);

        g.setColour(juce::Colour(0xFFCCCCCC));
        juce::Path p;
        p.startNewSubPath(rect.getCentreX(), rect.getY());
        p.lineTo(rect.getCentreX(), rect.getY() + rect.getHeight() * 0.2f);
        const auto angle = (rotaryEndAngle - rotaryStartAngle) * sliderPosProportional - 0.5f * (rotaryEndAngle - rotaryStartAngle);
        const auto thickness = 0.05f * width;
        g.strokePath(p, juce::PathStrokeType(thickness), juce::AffineTransform::rotation(angle, rect.getCentreX(), rect.getCentreY()));
        g.drawEllipse(rect, 0.5f*thickness);
    }

};