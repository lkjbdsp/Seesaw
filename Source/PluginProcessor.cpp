#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
auto normToFreq = [](float start, float, float norm) { return start * std::pow(10.f, 3.f * juce::jlimit(0.f, 1.f, norm)); };
auto freqToNorm = [](float start, float end, float freq) { return std::log(juce::jlimit(start, end, freq) / 20.f) / (3.f * std::log(10.f)); };
auto snapFreq = [](float, float, float freq) { return 0.1f * std::floor(freq * 10.f); };

auto freqRange = juce::NormalisableRange<float>(20.f, 20e3f, normToFreq, freqToNorm, snapFreq);
auto gainRange = juce::NormalisableRange<float>(-24.f, 24.f);
auto freqToText = [](float freq, int) {
    return freq < 1000.f ? juce::String(freq, 1) : juce::String(freq * 0.001, 2) + "k";
};
auto textToFreq = [](const juce::String& text) {
    return text.getFloatValue();
};

auto valToMode = [](int val, int) { return juce::String(val == 1 ? "Mid" : val == 2 ? "Side" : "Stereo"); };
auto modeToVal = [](const juce::String& mode) { return mode.toLowerCase().contains("m") ? 1 : mode.toLowerCase().contains("si") ? 2 : 0; };
}

Lkjb_seesawAudioProcessor::Lkjb_seesawAudioProcessor()
     : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
     parameters(*this, nullptr, juce::Identifier("SEESAW"),
        {
            std::make_unique<juce::AudioParameterFloat>("Freq", "Freq", freqRange, 632.f, "Hz", juce::AudioProcessorParameter::genericParameter, freqToText, textToFreq), 
            std::make_unique<juce::AudioParameterFloat>("Gain", "Gain", gainRange, 0.f, "dB", juce::AudioProcessorParameter::genericParameter, freqToText, textToFreq), 
            std::make_unique<juce::AudioParameterInt>("Mode", "Mode", 0, 2, 0, "", valToMode, modeToVal)
        }
     )
{
    freq = dynamic_cast<juce::AudioParameterFloat*> (parameters.getParameter("Freq"));
    gain = dynamic_cast<juce::AudioParameterFloat*> (parameters.getParameter("Gain"));
    mode = dynamic_cast<juce::AudioParameterInt*> (parameters.getParameter("Mode"));
}

Lkjb_seesawAudioProcessor::~Lkjb_seesawAudioProcessor()
{
}

const juce::String Lkjb_seesawAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Lkjb_seesawAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Lkjb_seesawAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Lkjb_seesawAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Lkjb_seesawAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Lkjb_seesawAudioProcessor::getNumPrograms()
{
    return 1;   
}

int Lkjb_seesawAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Lkjb_seesawAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String Lkjb_seesawAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void Lkjb_seesawAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

void Lkjb_seesawAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    updateParameters();
    const auto numFilters = 32;
    const auto minFreq = 20.;
    const auto maxFreq = 20e3;
    const auto order = 4;

    procBuffer.setSize(2, samplesPerBlock);

    bandAnalyser.setup(numFilters, minFreq, maxFreq, sampleRate, order);
    bandAnalyser.setBlockSize(samplesPerBlock);
}

void Lkjb_seesawAudioProcessor::releaseResources()
{
}

bool Lkjb_seesawAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

bool Lkjb_seesawAudioProcessor::applyBusLayouts(const BusesLayout& layouts)
{
    isMono = layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
    return juce::AudioProcessor::applyBusLayouts(layouts);
}

void Lkjb_seesawAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    updateParameters();

    const auto numChannels = buffer.getNumChannels();
    isMono = numChannels == 1;
    const auto numSamples = buffer.getNumSamples();
    auto chL = buffer.getWritePointer(0);
    auto chR = numChannels> 1 ? buffer.getWritePointer(1) : nullptr;
    const auto modeIdx = mode->get();
    updateBandAnalyserEnabled();
    const auto analyserPre = analyserMode == kAnalyserPre;

    if (isMono)
    {
        if (analyserPre)
            bandAnalyser.processBlock(chL, numSamples);

        seeSaw.processBlock(chL, numSamples);

        if (! analyserPre)
            bandAnalyser.processBlock(chL, numSamples);
    }
    else if (modeIdx == 0) 
    {
        auto mid = procBuffer.getWritePointer(0);

        if (analyserPre)
        {
            juce::FloatVectorOperations::copy(mid, chL, numSamples);
            juce::FloatVectorOperations::add(mid, chR, numSamples);
            juce::FloatVectorOperations::multiply(mid, 0.5f, numSamples); 
            bandAnalyser.processBlock(mid, numSamples);
        }

        seeSaw.processBlock(chL, chR, numSamples);

        if (! analyserPre)
        {
            juce::FloatVectorOperations::copy(mid, chL, numSamples);
            juce::FloatVectorOperations::add(mid, chR, numSamples);
            juce::FloatVectorOperations::multiply(mid, 0.5f, numSamples);
            bandAnalyser.processBlock(mid, numSamples);
        }
    }
    else
    {
        auto mid = procBuffer.getWritePointer(0);
        auto side = procBuffer.getWritePointer(1);

        juce::FloatVectorOperations::copy(mid, chL, numSamples);
        juce::FloatVectorOperations::copy(side, chL, numSamples);
        juce::FloatVectorOperations::add(mid, chR, numSamples);
        juce::FloatVectorOperations::subtract(side, chR, numSamples);
        juce::FloatVectorOperations::multiply(mid, 0.5f, numSamples);
        juce::FloatVectorOperations::multiply(side, 0.5f, numSamples);

        if (modeIdx == 1) 
        {
            if (analyserPre)
                bandAnalyser.processBlock(mid, numSamples);

            seeSaw.processBlock(mid, numSamples);

            if (! analyserPre)
                bandAnalyser.processBlock(mid, numSamples);
        }
        else if (modeIdx == 2)
        {
            if (analyserPre)
                bandAnalyser.processBlock(side, numSamples);

            seeSaw.processBlock(side, numSamples);

            if (! analyserPre)
                bandAnalyser.processBlock(side, numSamples);
        }

        juce::FloatVectorOperations::copy(chL, mid, numSamples);
        juce::FloatVectorOperations::copy(chR, mid, numSamples);
        juce::FloatVectorOperations::add(chL, side, numSamples);
        juce::FloatVectorOperations::subtract(chR, side, numSamples);
    }
}

bool Lkjb_seesawAudioProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* Lkjb_seesawAudioProcessor::createEditor()
{
    currentEditorPointer = new Lkjb_seesawAudioProcessorEditor(*this);
    return currentEditorPointer;
}

void Lkjb_seesawAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml("STATE");

    xml.setAttribute("freq", freq->get());
    xml.setAttribute("gain", gain->get());
    xml.setAttribute("mode", mode->get());
    xml.setAttribute("analyser", static_cast<int> (analyserMode));
    xml.setAttribute("guiwidth", guiSize.getX());
    xml.setAttribute("guiheight", guiSize.getY());

    copyXmlToBinary(xml, destData);
}

void Lkjb_seesawAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml == nullptr || ! xml->hasTagName("STATE"))
        return;

    const auto freqVal = xml->getDoubleAttribute("freq", freq->get());
    const auto gainVal = xml->getDoubleAttribute("gain", gain->get());
    const auto modeVal = xml->getIntAttribute("mode", mode->get());
    const auto guiWidth = xml->getIntAttribute("guiwidth", -1);
    const auto guiHeight = xml->getIntAttribute("guiheight", -1);

    analyserMode = static_cast<AnalyserMode> (xml->getIntAttribute("analyser", static_cast<int> (analyserMode)));

    freq->setValueNotifyingHost(freq->getNormalisableRange().convertTo0to1(static_cast<float> (freqVal)));
    gain->setValueNotifyingHost(gain->getNormalisableRange().convertTo0to1(static_cast<float> (gainVal)));
    mode->setValueNotifyingHost(mode->getNormalisableRange().convertTo0to1(static_cast<float> (modeVal)));

    if (guiWidth > 0 && guiHeight > 0)
        guiSize = juce::Point<int>(guiWidth, guiHeight);

    updateParameters();

    if (auto ed = currentEditorPointer)
        ed->triggerAsyncUpdate();
}

void Lkjb_seesawAudioProcessor::updateBandAnalyserEnabled()
{
    bandAnalyser.setEnabled(analyserMode != kAnalyserDisabled && currentEditorPointer != nullptr);
}

void Lkjb_seesawAudioProcessor::updateParameters()
{
    const auto freqVal = freq->get();
    const auto gainVal = gain->get();
    seeSawWasUpdated |= seeSaw.setSeeSaw(freqVal, gainVal, getSampleRate());
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Lkjb_seesawAudioProcessor();
}
