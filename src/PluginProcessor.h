#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <deque>
#include "IO.h"

static constexpr const char* PLUGIN_VERSION = "v0.1.1-alpha";

class WayQAudioProcessor : public juce::AudioProcessor,
                           public juce::ChangeBroadcaster
{
public:
    WayQAudioProcessor();
    ~WayQAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

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

    // Hands the host the bypass parameter so its native bypass control drives
    // PID_BYPASS (and automation of PID_BYPASS also flips the host's indicator).
    juce::AudioProcessorParameter* getBypassParameter() const override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Public APVTS — matches rhythmecho so the Stage-2 TopPanel port reads
    // proc_.apvts directly. getAPVTS() is kept as the existing accessor.
    juce::AudioProcessorValueTreeState apvts;

    // Flags read/written by the top panel (message thread). Not audio-thread
    // critical; atomic for safety. perfModeActive skips the modified-preset
    // guard dialog; stateSchemaMismatch_ flags a stale state blob on load.
    std::atomic<bool> perfModeActive       { false };
    std::atomic<bool> suppressUndoPush_    { false };
    std::atomic<bool> stateSchemaMismatch_ { false };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Bump to trigger a stale-state warning path on older saved blobs.
    static constexpr int SCHEMA_VERSION = 1;

    //==========================================================================
    // Preset bank — 128 sparse slots, message-thread only.
    // Slot 0 is permanently locked as the factory "Init" preset (every param at
    // its registered APVTS default = flat EQ). Slots 1-127 are user-saved and
    // start empty. Storage: a juce::ValueTree child named "PresetBank" folded
    // into getStateInformation/setStateInformation alongside the APVTS state.
    // Empty slots are omitted (sparse). Editor subscribes via ChangeBroadcaster.
    //==========================================================================
    static constexpr int kNumPresetSlots = 128;

    void         saveToSlot         (int slot);
    void         loadFromSlot       (int slot);
    void         clearSlot          (int slot);
    void         renameSlot         (int slot, juce::String name);
    juce::String getSlotName        (int slot) const;
    bool         isSlotUsed         (int slot) const;
    int          getCurrentSlot     () const;
    void         setCurrentSlot     (int slot);
    bool         isPresetModified   ();
    bool         exportSlotToFile   (int slot, juce::File dest);
    bool         importSlotFromFile (int slot, juce::File src);

    // System-clipboard round-trip. slotToXmlString builds the same XML that
    // exportSlotToFile writes (works for slot 0 too — emits an Init snapshot).
    // importSlotFromXmlString parses clipboard text and writes to the given
    // slot; returns false if text is not a valid single-preset Slot XML.
    juce::String slotToXmlString         (int slot) const;
    bool         importSlotFromXmlString (int slot, const juce::String& xml);

    // Bank-level file I/O. Export writes a PresetBank XML containing every used
    // user slot (slot 0 is always factory Init and is never stored). Import is
    // replace-all: wipes every existing user slot first, then repopulates.
    // Returns false if the file is not a valid PresetBank XML.
    bool         exportBankToFile    (juce::File dest) const;
    bool         importBankFromFile  (juce::File src);

    // Re-broadcasts every parameter at its current value so attached GUI
    // controls resync (host "refresh controls").
    void reannounceAllParams();

    //==========================================================================
    // Undo/redo history — snapshot stacks of full APVTS state. A snapshot is
    // pushed on every parameter gesture-end (knob release) and on preset loads.
    //==========================================================================
    void pushUndoSnapshot();
    void undo();
    void redo();
    bool canUndo() const { return ! undoStack_.empty(); }
    bool canRedo() const { return ! redoStack_.empty(); }

    //==========================================================================
    // Parameter ID string constants
    //==========================================================================
    // Full bypass — main input passes straight to main output, DSP skipped.
    // Sticky across preset loads: captured before any state change in
    // loadFromSlot() and restored after; never written into slot XML.
    static constexpr const char* PID_BYPASS = "bypass";

private:
    IO io_;

    //==========================================================================
    // Preset-bank internal state
    //==========================================================================
    juce::ValueTree bankTree { "PresetBank" };

    // Pristine factory-default parameter state, captured once at construction
    // before any preset or host state load. Slot 0 ("Init") restores from a
    // copy of this via replaceState so the load fires all parameter listeners.
    juce::ValueTree defaultStateTree_;

    // Snapshot of the loaded preset (bypass excluded) used by isPresetModified.
    juce::ValueTree cleanSnapshot_;

    void captureCleanSnapshot();
    juce::ValueTree findSlotChild (int slot) const;

    //==========================================================================
    // Undo/redo history
    //==========================================================================
    static constexpr int kUndoHistoryDepth = 50;
    struct UndoEntry { juce::ValueTree state; int slot; };
    std::deque<UndoEntry> undoStack_;
    std::deque<UndoEntry> redoStack_;

    struct GestureListener : public juce::AudioProcessorParameter::Listener
    {
        WayQAudioProcessor& proc_;
        explicit GestureListener (WayQAudioProcessor& p) : proc_ (p) {}
        void parameterValueChanged   (int, float) override {}
        void parameterGestureChanged (int, bool gestureIsStarting) override
        {
            if (! gestureIsStarting)
                proc_.pushUndoSnapshot();
        }
    };
    std::unique_ptr<GestureListener> gestureListener_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WayQAudioProcessor)
};
