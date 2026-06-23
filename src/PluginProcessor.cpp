#include "PluginProcessor.h"
#include "PluginEditor.h"

WayQAudioProcessor::WayQAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_ (*this, nullptr, "WayQState", createParameterLayout())
{
}

WayQAudioProcessor::~WayQAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout WayQAudioProcessor::createParameterLayout()
{
    using namespace juce;

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    auto makeFreq = [](const char* id, const char* name, float defaultHz)
    {
        NormalisableRange<float> r (OutputFilter::FREQ_MIN, OutputFilter::FREQ_MAX);
        r.setSkewForCentre (OutputFilter::MID_FREQ_DEFAULT);
        return std::make_unique<AudioParameterFloat> (id, name, r, defaultHz);
    };

    auto makeQ = [](const char* id, const char* name, float qMin, float qMax, float def)
    {
        return std::make_unique<AudioParameterFloat> (
            id, name, NormalisableRange<float> (qMin, qMax), def);
    };

    auto makeGain = [](const char* id, const char* name)
    {
        return std::make_unique<AudioParameterFloat> (
            id, name,
            NormalisableRange<float> (OutputFilter::TILT_GAIN_MIN, OutputFilter::TILT_GAIN_MAX),
            0.0f);
    };

    auto makeMidGain = [](const char* id, const char* name)
    {
        return std::make_unique<AudioParameterFloat> (
            id, name,
            NormalisableRange<float> (OutputFilter::MID_GAIN_MIN, OutputFilter::MID_GAIN_MAX),
            0.0f);
    };

    // Channel A
    params.push_back (makeFreq ("outHpA",  "HP Freq A",      OutputFilter::HP_DEFAULT));
    params.push_back (makeQ    ("outHpQA", "HP Q A",
        OutputFilter::HP_LP_Q_MIN, OutputFilter::HP_LP_Q_MAX, OutputFilter::HP_LP_Q_DEFAULT));
    params.push_back (makeFreq ("outMidFA", "Mid Freq A",     OutputFilter::MID_FREQ_DEFAULT));
    params.push_back (makeMidGain ("outMidGA", "Mid Gain A"));
    params.push_back (makeQ    ("outMidQA", "Mid Q A",
        OutputFilter::MID_Q_MIN, OutputFilter::MID_Q_MAX, OutputFilter::MID_Q_DEFAULT));
    params.push_back (makeFreq ("outLpA",  "LP Freq A",       OutputFilter::LP_DEFAULT));
    params.push_back (makeQ    ("outLpQA", "LP Q A",
        OutputFilter::HP_LP_Q_MIN, OutputFilter::HP_LP_Q_MAX, OutputFilter::HP_LP_Q_DEFAULT));
    params.push_back (std::make_unique<AudioParameterInt>  ("outDeltaA",    "Delta A",    0, 2, 0));
    params.push_back (makeFreq ("outBassTiltFA", "Bass Tilt Freq A", OutputFilter::BASS_TILT_DEFAULT_FREQ));
    params.push_back (makeGain ("outBassTiltGA", "Bass Tilt Gain A"));
    params.push_back (makeFreq ("outTrebTiltFA", "Treb Tilt Freq A", OutputFilter::TREB_TILT_DEFAULT_FREQ));
    params.push_back (makeGain ("outTrebTiltGA", "Treb Tilt Gain A"));
    params.push_back (std::make_unique<AudioParameterBool> ("outEqModeXA",  "EQ Mode X A", false));

    // Channel B
    params.push_back (makeFreq ("outHpB",  "HP Freq B",      OutputFilter::HP_DEFAULT));
    params.push_back (makeQ    ("outHpQB", "HP Q B",
        OutputFilter::HP_LP_Q_MIN, OutputFilter::HP_LP_Q_MAX, OutputFilter::HP_LP_Q_DEFAULT));
    params.push_back (makeFreq ("outMidFB", "Mid Freq B",     OutputFilter::MID_FREQ_DEFAULT));
    params.push_back (makeMidGain ("outMidGB", "Mid Gain B"));
    params.push_back (makeQ    ("outMidQB", "Mid Q B",
        OutputFilter::MID_Q_MIN, OutputFilter::MID_Q_MAX, OutputFilter::MID_Q_DEFAULT));
    params.push_back (makeFreq ("outLpB",  "LP Freq B",       OutputFilter::LP_DEFAULT));
    params.push_back (makeQ    ("outLpQB", "LP Q B",
        OutputFilter::HP_LP_Q_MIN, OutputFilter::HP_LP_Q_MAX, OutputFilter::HP_LP_Q_DEFAULT));
    params.push_back (std::make_unique<AudioParameterInt>  ("outDeltaB",    "Delta B",    0, 2, 0));
    params.push_back (makeFreq ("outBassTiltFB", "Bass Tilt Freq B", OutputFilter::BASS_TILT_DEFAULT_FREQ));
    params.push_back (makeGain ("outBassTiltGB", "Bass Tilt Gain B"));
    params.push_back (makeFreq ("outTrebTiltFB", "Treb Tilt Freq B", OutputFilter::TREB_TILT_DEFAULT_FREQ));
    params.push_back (makeGain ("outTrebTiltGB", "Treb Tilt Gain B"));
    params.push_back (std::make_unique<AudioParameterBool> ("outEqModeXB",  "EQ Mode X B", false));

    return { params.begin(), params.end() };
}

void WayQAudioProcessor::prepareToPlay (double sampleRate, int)
{
    io_.prepare (sampleRate, apvts_);
}

void WayQAudioProcessor::releaseResources()
{
    io_.reset();
}

bool WayQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;
    return true;
}

void WayQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    io_.processBlock (buffer, apvts_);
}

juce::AudioProcessorEditor* WayQAudioProcessor::createEditor()
{
    return new WayQAudioProcessorEditor (*this);
}

bool WayQAudioProcessor::hasEditor() const  { return true; }

const juce::String WayQAudioProcessor::getName() const { return JucePlugin_Name; }

bool WayQAudioProcessor::acceptsMidi() const  { return false; }
bool WayQAudioProcessor::producesMidi() const { return false; }
bool WayQAudioProcessor::isMidiEffect() const { return false; }
double WayQAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int WayQAudioProcessor::getNumPrograms()                        { return 1; }
int WayQAudioProcessor::getCurrentProgram()                     { return 0; }
void WayQAudioProcessor::setCurrentProgram (int)                {}
const juce::String WayQAudioProcessor::getProgramName (int)     { return {}; }
void WayQAudioProcessor::changeProgramName (int, const juce::String&) {}

void WayQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WayQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts_.state.getType()))
        apvts_.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WayQAudioProcessor();
}
