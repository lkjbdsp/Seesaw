#include "PluginProcessor.h"
#include "PluginEditor.h"


AnalyserButter::AnalyserButter(const juce::String& label)
    : juce::TextButton(label), font(juce::FontOptions())
{
    setClickingTogglesState(true);
}

void AnalyserButter::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto& lf = getLookAndFeel();

    lf.drawButtonBackground(g, *this, findColour(getToggleState() ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    g.setFont(font);
    g.setColour(findColour(getToggleState() ? TextButton::textColourOnId : TextButton::textColourOffId).withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f));

    Lkjb_seesawAudioProcessorEditor::drawVerticalText(g, getButtonText(), font, getLocalBounds());
}

void Lkjb_seesawAudioProcessorEditor::setupSlider(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);
    s.setLookAndFeel(&sliderLookAndFeel);
}

void Lkjb_seesawAudioProcessorEditor::setupButton(juce::TextButton& b, int value)
{
    if (value >= 0)
    {
        b.setClickingTogglesState(true);
        b.setRadioGroupId(1111, juce::sendNotificationSync);
        b.onClick = [this, value]() {
            auto mode = audioProcessor.mode;
            mode->setValueNotifyingHost(mode->getNormalisableRange().convertTo0to1(static_cast<float> (value)));
        };
    }

    auto col1 = b.findColour(juce::TextButton::buttonColourId);
    auto col = b.findColour(juce::TextButton::buttonOnColourId);
    b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4C6470));
}

Lkjb_seesawAudioProcessorEditor::Lkjb_seesawAudioProcessorEditor(Lkjb_seesawAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), resizer(this, &constrainer),
    freqPlot(p.bandAnalyser),
    stereo("Stereo"), mid("Mid"), side("Side"),
    anInput("ANALYSE IN"), anOutput("ANALYSE OUT")
{
    constrainer.setSizeLimits(320, 200, 3200, 2000);
    constrainer.setFixedAspectRatio(1.6);
    setupSlider(freq);
    setupSlider(gain);
    sliderAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters, "Freq", freq));
    sliderAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters, "Gain", gain));    

    addAndMakeVisible(freq);
    addAndMakeVisible(gain);
    addAndMakeVisible(resizer);
    addAndMakeVisible(freqPlot);
    addAndMakeVisible(stereo);
    addAndMakeVisible(mid);
    addAndMakeVisible(side);
    addAndMakeVisible(anInput);
    addAndMakeVisible(anOutput);

    stereo.setConnectedEdges(juce::Button::ConnectedOnBottom);
    mid.setConnectedEdges(juce::Button::ConnectedOnTop | juce::Button::ConnectedOnBottom);
    side.setConnectedEdges(juce::Button::ConnectedOnTop);

    setupButton(stereo, 0);
    setupButton(mid, 1);
    setupButton(side, 2);
    setupButton(anInput, -1);
    setupButton(anOutput, -1);
    anInput.onClick = [this]() {
        anOutput.setToggleState(false, juce::dontSendNotification);
        audioProcessor.analyserMode = anInput.getToggleState() ? Lkjb_seesawAudioProcessor::kAnalyserPre : Lkjb_seesawAudioProcessor::kAnalyserDisabled;
        audioProcessor.updateBandAnalyserEnabled();
        freqPlot.repaint();
    };

    anOutput.onClick = [this]() {
        anInput.setToggleState(false, juce::dontSendNotification);
        audioProcessor.analyserMode = anOutput.getToggleState() ? Lkjb_seesawAudioProcessor::kAnalyserPost : Lkjb_seesawAudioProcessor::kAnalyserDisabled;
        audioProcessor.updateBandAnalyserEnabled();
        freqPlot.repaint();
    };

    audioProcessor.parameters.addParameterListener("Freq", this);
    audioProcessor.parameters.addParameterListener("Gain", this);
    audioProcessor.parameters.addParameterListener("Mode", this);
    parameterChanged("", 0.f);
    syncProcMode(true);
    updateAnalyserButtons();
    updateMonoState();

    setSize (p.guiSize.getX(), p.guiSize.getY());
    startTimer(150);
}

Lkjb_seesawAudioProcessorEditor::~Lkjb_seesawAudioProcessorEditor()
{
    audioProcessor.currentEditorPointer = nullptr;
    freq.setLookAndFeel(nullptr);
    gain.setLookAndFeel(nullptr);
    audioProcessor.parameters.removeParameterListener("Freq", this);
    audioProcessor.parameters.removeParameterListener("Gain", this);
    audioProcessor.parameters.removeParameterListener("Mode", this);
}

void Lkjb_seesawAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto h = getHeight();

    g.setGradientFill(juce::ColourGradient(juce::Colour(0xFF555555), 0.f, 0.f, juce::Colour(0xFF333333), 0.f, h - 1.f, false));
    g.fillAll();
    g.setColour(juce::Colour(0xFF888888));
    g.drawImageAt(noiseImg, 0, 0, true);

    const auto font = freqPlot.getFont();

    {
        g.saveState();
        g.setColour(juce::Colour(0xFFAAAFA7));
        const auto rw = juce::GlyphArrangement::getStringWidthInt(font, "     ");
        const auto rFreq = juce::Rectangle<int>(freq.getX() - rw, freq.getY(), rw, freq.getHeight() - freq.getTextBoxHeight());
        const auto rGain = juce::Rectangle<int>(gain.getRight(), gain.getY(), rw, gain.getHeight() - gain.getTextBoxHeight());
        const auto lFreq = juce::String("FREQ");
        const auto lGain = juce::String("GAIN");
        drawVerticalText(g, lFreq, font, rFreq);
        drawVerticalText(g, lGain, font, rGain);
        g.restoreState();
    }

    {
        const auto bold = juce::Font(juce::FontOptions(font.getHeight() * 2, juce::Font::bold));
        const auto h0 = freqPlot.getY() * 4/5;
        g.setFont(bold);
        g.setColour(juce::Colour(0xFFFFFFF));
        g.drawText("lkjb SeeSaw", freqPlot.getX(), 0, freqPlot.getWidth(), h0, juce::Justification::bottomLeft, false);
        g.setColour(juce::Colour(0xFFAABBAA));
        g.drawText("lkjb SeeSaw", freqPlot.getX(), 0, freqPlot.getWidth(), h0, juce::Justification::bottomLeft, false);
        const auto xVer = freqPlot.getX() + juce::GlyphArrangement::getStringWidthInt(bold, "lkjb SeeSaw ");

        g.setFont(font);
        g.setColour(juce::Colour(0xAACCCCCC));
        g.drawText(juce::String("V") + ProjectInfo::versionString, xVer, 0, freqPlot.getRight() - xVer, h0, juce::Justification::bottomLeft, false);
    }

    {
        g.setColour(juce::Colours::black);
        g.drawRect(freqPlot.getBounds().expanded(1));
    }
}

void Lkjb_seesawAudioProcessorEditor::resized()
{
    const auto w = getWidth();
    const auto h = getHeight();
    audioProcessor.guiSize = juce::Point<int>(w, h);

    {
        noiseImg = juce::Image(juce::Image::ARGB, w, h, true);
        const auto w0 = 320;
        const auto h0 = 200;
        juce::Image img(juce::Image::ARGB, w0, h0, true);
        juce::Random rnd(0x6374246);
        const auto maxVal = 20;

        for (int x = 0; x < w0; ++x)
            for (int y = 0; y < h0; ++y)
                img.setPixelAt(x, y, juce::Colour(rnd.nextInt(maxVal) << 24));

        juce::Graphics g(noiseImg);
        g.drawImage(img, noiseImg.getBounds().toFloat(), juce::RectanglePlacement::stretchToFit, false);
    }

    freqPlot.setBounds(w / 10, h / 10, w - 2*w / 10, h - h/10 - h / 3);
    resizer.setBounds(w - 15, h - 15, 15, 15);

    const auto dist = h / 20;
    const auto sw = h/5;
    const auto sh = sw + 20;
    const auto ySlider = freqPlot.getBottom() + dist;
    freq.setBounds(w / 3 - sw, ySlider, sw, sh);
    gain.setBounds(w - w / 3, ySlider, sw, sh);

    const auto xBut = (w - sw) / 2;
    const auto hBut = sw / 3;
    stereo.setBounds(xBut, ySlider, sw, hBut);
    mid.setBounds(xBut, ySlider + hBut, sw, hBut);
    side.setBounds(xBut, ySlider + 2 * hBut, sw, hBut);

    {
        const auto font = freqPlot.getFont().boldened();
        anInput.font = font;
        anOutput.font = font;
        const auto wAn = juce::GlyphArrangement::getStringWidthInt(font, "        ");
        const auto yAn = freqPlot.getY();
        const auto hAn = freqPlot.getHeight();
        const auto xIn = (freqPlot.getX() - wAn) / 2;
        const auto xOut = (w + freqPlot.getRight() - wAn) / 2;
        anInput.setBounds(xIn, yAn, wAn, hAn);
        anOutput.setBounds(xOut, yAn, wAn, hAn);
    }

    parameterChanged("", 0.f);
}

void Lkjb_seesawAudioProcessorEditor::handleAsyncUpdate()
{
    parameterChanged("", 0.f);
    updateAnalyserButtons();
    setSize(audioProcessor.guiSize.getX(), audioProcessor.guiSize.getY());
    updateMonoState();
}

void Lkjb_seesawAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.seeSawWasUpdated)
    {
        audioProcessor.seeSawWasUpdated = false;
        updateFreqRes();
    }
}

void Lkjb_seesawAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float /*newValue*/)
{
    if (parameterID == "Mode")
    {
        syncProcMode(false);
    }
    else
    {
        updateFreqRes();
    }
}

void Lkjb_seesawAudioProcessorEditor::drawVerticalText(const juce::Graphics& g, const juce::String& text, const juce::Font& font, const juce::Rectangle<int> rect)
{
    const auto fh = static_cast<int> (1.3f * font.getHeight() + 1.f);
    const auto textLen = text.length();
    const auto y0 = rect.getY() + (rect.getHeight() - textLen * fh) / 2;

    for (int i = 0; i < textLen; ++i)
        g.drawText(text.substring(i, i + 1), rect.getX(), y0 + i * fh, rect.getWidth(), fh, juce::Justification::centred, false);
}

void Lkjb_seesawAudioProcessorEditor::updateAnalyserButtons()
{
    anInput.setToggleState(audioProcessor.analyserMode == Lkjb_seesawAudioProcessor::kAnalyserPre, juce::dontSendNotification);
    anOutput.setToggleState(audioProcessor.analyserMode == Lkjb_seesawAudioProcessor::kAnalyserPost, juce::dontSendNotification);
}

void Lkjb_seesawAudioProcessorEditor::updateMonoState()
{
    const auto mono = audioProcessor.isMono;

    stereo.setVisible(! mono);
    mid.setVisible(! mono);
    side.setVisible(! mono);
}

void Lkjb_seesawAudioProcessorEditor::syncProcMode(bool notifyChange)
{
    const auto notify = notifyChange ? juce::sendNotificationSync : juce::dontSendNotification;
    switch (audioProcessor.mode->get())
    {
    default:
        jassertfalse;
    case 0:
        stereo.setToggleState(true, notify);
        break;
    case 1:
        mid.setToggleState(true, notify);
        break;
    case 2:
        side.setToggleState(true, notify);
        break;
    }
}

void Lkjb_seesawAudioProcessorEditor::updateFreqRes()
{
    audioProcessor.seeSaw.getResponse(freqPlot.getPlotData());
    freqPlot.updateFreqRes();
    freqPlot.repaint();
}