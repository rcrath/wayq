#include "PluginProcessor.h"
#include "PluginEditor.h"

WayQAudioProcessor::WayQAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "WayQState", createParameterLayout())
{
    // Snapshot the factory defaults first thing, before any host
    // setStateInformation can run, so slot 0 ("Init") always restores true
    // pristine flat-EQ defaults.
    defaultStateTree_ = apvts.copyState();

    // Bank starts empty; slot 0 ("Init") is virtual — always reported used.
    bankTree.setProperty ("currentSlot", 0, nullptr);

    // Push an undo snapshot whenever a parameter gesture ends (knob release).
    gestureListener_ = std::make_unique<GestureListener> (*this);
    for (auto* p : getParameters())
        p->addListener (gestureListener_.get());
}

WayQAudioProcessor::~WayQAudioProcessor()
{
    if (gestureListener_)
        for (auto* p : getParameters())
            p->removeListener (gestureListener_.get());
}

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

    // Full bypass — true = bypassed (dry passthrough). Excluded from preset
    // snapshots and sticky across preset loads (mirrors rhythmecho).
    params.push_back (std::make_unique<AudioParameterBool> (PID_BYPASS, "Bypass", false));

    return { params.begin(), params.end() };
}

void WayQAudioProcessor::prepareToPlay (double sampleRate, int)
{
    io_.prepare (sampleRate, apvts);
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

    // Full bypass — main input is already in the buffer; returning without
    // modifying leaves it intact so the host reads input as output (dry).
    if (auto* b = apvts.getRawParameterValue (PID_BYPASS))
        if (b->load() > 0.5f)
            return;

    io_.processBlock (buffer, apvts);
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
    auto state = apvts.copyState();
    // createCopy is required because a ValueTree node can only have one parent.
    state.removeChild (state.getChildWithName ("PresetBank"), nullptr);
    state.appendChild (bankTree.createCopy(), nullptr);
    state.setProperty ("schemaVersion", SCHEMA_VERSION, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WayQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;   // null / garbage / wrong root — leave defaults intact, no crash

    auto state = juce::ValueTree::fromXml (*xml);
    if (! state.isValid())
        return;

    // Pull the PresetBank child out first (and detach it from `state`) so the
    // bank survives the param replaceState below. Missing on first run or on an
    // older blob — in that case the constructor-default empty bank is kept.
    auto savedBank = state.getChildWithName ("PresetBank");
    if (savedBank.isValid())
    {
        state.removeChild (savedBank, nullptr);
        bankTree = savedBank.createCopy();
        if (! bankTree.hasProperty ("currentSlot"))
            bankTree.setProperty ("currentSlot", 0, nullptr);
    }

    {
        int blobSchema = (int) state.getProperty ("schemaVersion", 0);
        stateSchemaMismatch_.store (blobSchema < SCHEMA_VERSION, std::memory_order_relaxed);
    }

    apvts.replaceState (state);
    juce::MessageManager::callAsync ([this] { captureCleanSnapshot(); });
    sendChangeMessage();
}

juce::AudioProcessorParameter* WayQAudioProcessor::getBypassParameter() const
{
    return apvts.getParameter (PID_BYPASS);
}

//==============================================================================
// Preset bank helpers (file-local)
//==============================================================================
static void stripParamsFromState (juce::ValueTree state,
                                  std::initializer_list<juce::StringRef> ids)
{
    for (int i = state.getNumChildren() - 1; i >= 0; --i)
    {
        auto child = state.getChild (i);
        if (! child.hasType ("PARAM")) continue;
        const auto id = child.getProperty ("id").toString();
        for (auto wanted : ids)
            if (id == wanted)
            {
                state.removeChild (i, nullptr);
                break;
            }
    }
}

// Builds a self-contained "Slot" node for export/clipboard. Slot 0 emits an
// "Init" snapshot from the captured factory defaults; user slots copy storage.
static juce::ValueTree buildSlotSnapshot (int slot,
                                          const juce::ValueTree& defaultStateTree,
                                          const std::function<juce::ValueTree (int)>& findSlot)
{
    if (slot < 0 || slot >= WayQAudioProcessor::kNumPresetSlots)
        return {};

    if (slot == 0)
    {
        juce::ValueTree node ("Slot");
        node.setProperty ("index", 0, nullptr);
        node.setProperty ("name",  "Init", nullptr);
        auto defaults = defaultStateTree.createCopy();
        stripParamsFromState (defaults, { WayQAudioProcessor::PID_BYPASS });
        node.appendChild (defaults, nullptr);
        return node;
    }

    auto existing = findSlot (slot);
    if (! existing.isValid()) return {};
    return existing.createCopy();
}

static bool writeSlotFromValueTree (int slot,
                                    juce::ValueTree imported,
                                    juce::ValueTree& bankTree,
                                    const std::function<juce::ValueTree (int)>& findSlot)
{
    if (slot <= 0 || slot >= WayQAudioProcessor::kNumPresetSlots) return false;
    if (! imported.isValid() || ! imported.hasType ("Slot")) return false;

    auto stateChild = imported.getChild (0);
    if (! stateChild.isValid()) return false;

    auto existing = findSlot (slot);
    if (existing.isValid())
        bankTree.removeChild (existing, nullptr);

    juce::ValueTree slotNode ("Slot");
    slotNode.setProperty ("index", slot, nullptr);
    juce::String name = imported.getProperty ("name", juce::String ("Slot ") + juce::String (slot)).toString();
    slotNode.setProperty ("name", name, nullptr);
    auto sc = stateChild.createCopy();
    stripParamsFromState (sc, { WayQAudioProcessor::PID_BYPASS });
    slotNode.appendChild (sc, nullptr);
    bankTree.appendChild (slotNode, nullptr);
    return true;
}

//==============================================================================
// Undo / redo
//==============================================================================
void WayQAudioProcessor::pushUndoSnapshot()
{
    if (suppressUndoPush_.load (std::memory_order_relaxed)) return;
    undoStack_.push_back ({ apvts.copyState(), getCurrentSlot() });
    while ((int) undoStack_.size() > kUndoHistoryDepth)
        undoStack_.pop_front();
    redoStack_.clear();
    sendChangeMessage();
}

void WayQAudioProcessor::undo()
{
    if (undoStack_.empty()) return;
    redoStack_.push_back ({ apvts.copyState(), getCurrentSlot() });
    auto entry = std::move (undoStack_.back());
    undoStack_.pop_back();
    apvts.replaceState (entry.state);
    bankTree.setProperty ("currentSlot", entry.slot, nullptr);
    sendChangeMessage();
}

void WayQAudioProcessor::redo()
{
    if (redoStack_.empty()) return;
    undoStack_.push_back ({ apvts.copyState(), getCurrentSlot() });
    auto entry = std::move (redoStack_.back());
    redoStack_.pop_back();
    apvts.replaceState (entry.state);
    bankTree.setProperty ("currentSlot", entry.slot, nullptr);
    sendChangeMessage();
}

void WayQAudioProcessor::reannounceAllParams()
{
    suppressUndoPush_.store (true, std::memory_order_relaxed);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getValue());
    suppressUndoPush_.store (false, std::memory_order_relaxed);
    sendChangeMessage();
}

//==============================================================================
// Preset bank
//==============================================================================
juce::ValueTree WayQAudioProcessor::findSlotChild (int slot) const
{
    for (int i = 0; i < bankTree.getNumChildren(); ++i)
    {
        auto c = bankTree.getChild (i);
        if (c.hasType ("Slot") && (int) c.getProperty ("index", -1) == slot)
            return c;
    }
    return {};
}

void WayQAudioProcessor::captureCleanSnapshot()
{
    cleanSnapshot_ = apvts.copyState();
    stripParamsFromState (cleanSnapshot_, { PID_BYPASS });
}

bool WayQAudioProcessor::isPresetModified()
{
    if (! cleanSnapshot_.isValid())
        return false;
    auto current = apvts.copyState();
    stripParamsFromState (current, { PID_BYPASS });
    return ! current.isEquivalentTo (cleanSnapshot_);
}

void WayQAudioProcessor::saveToSlot (int slot)
{
    if (slot <= 0 || slot >= kNumPresetSlots) return;   // slot 0 locked

    auto existing = findSlotChild (slot);
    juce::String name = existing.isValid() ? existing.getProperty ("name").toString()
                                           : juce::String ("Slot ") + juce::String (slot);
    if (existing.isValid())
        bankTree.removeChild (existing, nullptr);

    juce::ValueTree slotNode ("Slot");
    slotNode.setProperty ("index", slot, nullptr);
    slotNode.setProperty ("name", name, nullptr);
    slotNode.appendChild (apvts.copyState(), nullptr);
    if (auto stateChild = slotNode.getChild (0); stateChild.isValid())
        stripParamsFromState (stateChild, { PID_BYPASS });
    bankTree.appendChild (slotNode, nullptr);

    bankTree.setProperty ("currentSlot", slot, nullptr);
    juce::MessageManager::callAsync ([this] { captureCleanSnapshot(); });
    sendChangeMessage();
}

void WayQAudioProcessor::loadFromSlot (int slot)
{
    if (slot < 0 || slot >= kNumPresetSlots) return;

    cleanSnapshot_ = {};

    // Bypass is global, not per-preset: capture before the load and restore
    // after so switching presets never engages or disengages bypass.
    float stickyBypass = 0.0f;
    if (auto* r = apvts.getRawParameterValue (PID_BYPASS)) stickyBypass = r->load();
    auto restoreBypass = [this, stickyBypass]
    {
        if (auto* bp = apvts.getParameter (PID_BYPASS))
            bp->setValueNotifyingHost (stickyBypass);
    };

    if (slot == 0)
    {
        // replaceState (same mechanism as slots 1+) fires the parameter
        // listeners, so raw atomics and GUI attachments pick up the defaults.
        pushUndoSnapshot();
        apvts.replaceState (defaultStateTree_.createCopy());
        restoreBypass();
        juce::MessageManager::callAsync ([this] { captureCleanSnapshot(); });
        bankTree.setProperty ("currentSlot", 0, nullptr);
        sendChangeMessage();
        return;
    }

    auto slotNode = findSlotChild (slot);

    // Empty slot: still track currentSlot so Save / Import target it. Leave
    // live params untouched — user keeps hearing what was loaded.
    if (! slotNode.isValid())
    {
        bankTree.setProperty ("currentSlot", slot, nullptr);
        sendChangeMessage();
        return;
    }

    auto stateChild = slotNode.getChild (0);
    if (! stateChild.isValid() || ! stateChild.hasType (apvts.state.getType()))
    {
        bankTree.setProperty ("currentSlot", slot, nullptr);
        sendChangeMessage();
        return;
    }

    pushUndoSnapshot();
    apvts.replaceState (stateChild.createCopy());
    restoreBypass();
    juce::MessageManager::callAsync ([this] { captureCleanSnapshot(); });
    bankTree.setProperty ("currentSlot", slot, nullptr);
    sendChangeMessage();
}

void WayQAudioProcessor::clearSlot (int slot)
{
    if (slot <= 0 || slot >= kNumPresetSlots) return;   // slot 0 locked
    auto existing = findSlotChild (slot);
    if (existing.isValid())
    {
        bankTree.removeChild (existing, nullptr);
        sendChangeMessage();
    }
}

void WayQAudioProcessor::renameSlot (int slot, juce::String name)
{
    if (slot <= 0 || slot >= kNumPresetSlots) return;   // slot 0 locked
    auto existing = findSlotChild (slot);
    if (! existing.isValid()) return;
    existing.setProperty ("name", name, nullptr);
    sendChangeMessage();
}

juce::String WayQAudioProcessor::getSlotName (int slot) const
{
    if (slot == 0) return "Init";
    if (slot < 0 || slot >= kNumPresetSlots) return {};
    auto existing = findSlotChild (slot);
    if (! existing.isValid()) return {};
    return existing.getProperty ("name").toString();
}

bool WayQAudioProcessor::isSlotUsed (int slot) const
{
    if (slot == 0) return true;
    if (slot < 0 || slot >= kNumPresetSlots) return false;
    return findSlotChild (slot).isValid();
}

int WayQAudioProcessor::getCurrentSlot() const
{
    return (int) bankTree.getProperty ("currentSlot", 0);
}

void WayQAudioProcessor::setCurrentSlot (int slot)
{
    if (slot < 0 || slot >= kNumPresetSlots) return;
    bankTree.setProperty ("currentSlot", slot, nullptr);
    sendChangeMessage();
}

bool WayQAudioProcessor::exportSlotToFile (int slot, juce::File dest)
{
    auto node = buildSlotSnapshot (slot, defaultStateTree_,
                                   [this] (int s) { return findSlotChild (s); });
    if (! node.isValid()) return false;

    std::unique_ptr<juce::XmlElement> xml (node.createXml());
    if (xml == nullptr) return false;
    return xml->writeToFile (dest, {});
}

bool WayQAudioProcessor::importSlotFromFile (int slot, juce::File src)
{
    if (! src.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse (src);
    if (xml == nullptr) return false;
    auto imported = juce::ValueTree::fromXml (*xml);

    if (! writeSlotFromValueTree (slot, imported, bankTree,
                                  [this] (int s) { return findSlotChild (s); }))
        return false;
    sendChangeMessage();
    return true;
}

juce::String WayQAudioProcessor::slotToXmlString (int slot) const
{
    auto node = buildSlotSnapshot (slot, defaultStateTree_,
                                   [this] (int s) { return findSlotChild (s); });
    if (! node.isValid()) return {};
    std::unique_ptr<juce::XmlElement> xml (node.createXml());
    return xml != nullptr ? xml->toString() : juce::String();
}

bool WayQAudioProcessor::importSlotFromXmlString (int slot, const juce::String& xmlText)
{
    auto xml = juce::XmlDocument::parse (xmlText);
    if (xml == nullptr) return false;
    auto imported = juce::ValueTree::fromXml (*xml);
    if (! writeSlotFromValueTree (slot, imported, bankTree,
                                  [this] (int s) { return findSlotChild (s); }))
        return false;
    sendChangeMessage();
    return true;
}

bool WayQAudioProcessor::exportBankToFile (juce::File dest) const
{
    juce::ValueTree root ("PresetBank");
    for (int i = 1; i < kNumPresetSlots; ++i)
    {
        auto existing = findSlotChild (i);
        if (existing.isValid())
            root.appendChild (existing.createCopy(), nullptr);
    }
    std::unique_ptr<juce::XmlElement> xml (root.createXml());
    if (xml == nullptr) return false;
    return xml->writeToFile (dest, {});
}

bool WayQAudioProcessor::importBankFromFile (juce::File src)
{
    if (! src.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse (src);
    if (xml == nullptr) return false;
    auto imported = juce::ValueTree::fromXml (*xml);
    if (! imported.isValid() || ! imported.hasType ("PresetBank")) return false;

    // currentSlot resets to 0 because the prior selection may not exist now.
    while (bankTree.getNumChildren() > 0)
        bankTree.removeChild (0, nullptr);

    for (int i = 0; i < imported.getNumChildren(); ++i)
    {
        auto child = imported.getChild (i);
        if (child.hasType ("Slot"))
            bankTree.appendChild (child.createCopy(), nullptr);
    }

    bankTree.setProperty ("currentSlot", 0, nullptr);
    sendChangeMessage();
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WayQAudioProcessor();
}
