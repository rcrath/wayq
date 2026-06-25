/*
  ==============================================================================
    TopPanel.h
    WayQ — top panel of the editor — houses preset nav, slot dropdown, preset
    management, undo/redo, bypass, tooltips toggle, perf-mode, hamburger menu.

    Single-file component that renders one horizontal strip with:
      - Prev / Next slot stepper
      - scrollable dropdown of all 128 slots (selecting a slot loads it)
      - preset-management button → Save / Save As / Rename / Copy / Paste /
        Import / Export / Import bank / Export bank
      - undo / redo
      - bypass (power glyph) — lit = engaged, unlit = true bypass
      - tooltips on/off
      - performance mode (skip the modified-preset guard dialog)
      - hamburger button → About + external links

    Slot 0 is the locked factory "Init" preset (Save and Rename are disabled
    when slot 0 is current). Bank state lives on the processor; this component
    reads/writes it via the public processor API and refreshes on
    ChangeBroadcaster callbacks.

    Ported from RhythmEcho's TopPanel; demo countdown, get-it-free link, and the
    Preferences item have been removed.

    (c) 2026 Rich Rath / Way.Net
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "GUIConstants.h"
#include "WayQLookAndFeel.h"
#include "AppPrefs.h"

// TextButton that draws its text using the GNU Unifont typeface. Needed for
// glyphs outside the Averia range — e.g. U+23FB (power symbol) on the bypass
// button. Base-class paint handles the chrome/background/border so on/off
// colour states behave exactly like a normal TextButton.
class UnifontTextButton : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;

    void paintButton (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override
    {
        auto& lf = getLookAndFeel();
        lf.drawButtonBackground (g, *this,
                                 findColour (getToggleState() ? buttonOnColourId
                                                              : buttonColourId),
                                 shouldDrawButtonAsHighlighted,
                                 shouldDrawButtonAsDown);

        g.setColour (findColour (getToggleState() ? textColourOnId : textColourOffId));
        const float glyphH = (float) juce::jmin (getWidth(), getHeight()) * 0.75f;
        g.setFont (getUnifontFont (glyphH));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);
    }
};

class TopPanel : public juce::Component,
                  private juce::ChangeListener,
                  private juce::AudioProcessorValueTreeState::Listener,
                  private juce::Timer
{
public:
    explicit TopPanel (WayQAudioProcessor& proc)
        : proc_ (proc)
    {
        styleToggleButton (prevBtn_,      "<");
        styleToggleButton (nextBtn_,      ">");
        // Three-bar hamburger glyph (U+2630).
        styleToggleButton (hamburgerBtn_, juce::String::fromUTF8 ("\xe2\x98\xb0"));

        styleToggleButton (presetMgmtBtn_, juce::String::fromUTF8 ("\xf0\x9f\x93\x9a"));
        presetMgmtBtn_.setTooltip ("preset management");
        presetMgmtBtn_.onClick = [this] { showPresetMgmtMenu(); };

        styleToggleButton (undoBtn_, juce::String::fromUTF8 ("\xe2\x86\xa9"));
        undoBtn_.setTooltip ("Undo, CTRL-Z");
        undoBtn_.onClick = [this] { proc_.undo(); };

        styleToggleButton (redoBtn_, juce::String::fromUTF8 ("\xe2\x86\xaa"));
        redoBtn_.setTooltip ("Redo, CTRL-Y");
        redoBtn_.onClick = [this] { proc_.redo(); };

        // Power glyph (U+23FB). Semantics are inverted vs. PID_BYPASS:
        //   toggle lit (on)   → plugin engaged  → PID_BYPASS = 0
        //   toggle unlit (off) → true bypass    → PID_BYPASS = 1
        // PID_BYPASS keeps JUCE/host convention (true = bypassed) internally;
        // only the UI flips the mapping. Managed manually (no ButtonAttachment)
        // so we can invert cleanly on both the read and write side.
        styleToggleButton (bypassBtn_, juce::String::fromUTF8 ("\xe2\x8f\xbb"));
        bypassBtn_.setClickingTogglesState (true);
        bypassBtn_.setTooltip ("power — lit means the plugin is engaged; unlit is a true bypass.");
        if (auto* pBypass = proc_.apvts.getRawParameterValue (WayQAudioProcessor::PID_BYPASS))
            bypassBtn_.setToggleState (*pBypass < 0.5f, juce::dontSendNotification);
        bypassBtn_.onClick = [this]
        {
            const bool lit = bypassBtn_.getToggleState();   // lit = engaged
            if (auto* p = proc_.apvts.getParameter (WayQAudioProcessor::PID_BYPASS))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (lit ? 0.0f : 1.0f);
                p->endChangeGesture();
            }
        };
        proc_.apvts.addParameterListener (WayQAudioProcessor::PID_BYPASS, this);

        // UI-only state (not automated, not saved with the preset).
        styleToggleButton (tooltipsBtn_, "?");
        tooltipsBtn_.setClickingTogglesState (true);
        tooltipsBtn_.setToggleState (true, juce::dontSendNotification);
        tooltipsBtn_.setTooltip ("tooltips on/off.");
        tooltipsBtn_.onClick = [this]
        {
            if (onTooltipsToggled)
                onTooltipsToggled (tooltipsBtn_.getToggleState());
        };

        styleToggleButton (perfModeBtn_, "P");
        perfModeBtn_.setClickingTogglesState (true);
        perfModeBtn_.setTooltip ("performance mode -- skip the save/discard dialog when switching presets");
        {
            const bool perfOn = getAppPrefs().getBoolValue ("optPerfMode", false);
            perfModeBtn_.setToggleState (perfOn, juce::dontSendNotification);
            proc_.perfModeActive.store (perfOn);
        }
        perfModeBtn_.onClick = [this]
        {
            const bool on = perfModeBtn_.getToggleState();
            proc_.perfModeActive.store (on);
            getAppPrefs().setValue ("optPerfMode", on);
            getAppPrefs().saveIfNeeded();
        };
        addAndMakeVisible (perfModeBtn_);

        prevBtn_.onClick      = [this] { stepSlot (-1); };
        nextBtn_.onClick      = [this] { stepSlot (+1); };
        hamburgerBtn_.onClick = [this] { showHamburgerMenu(); };

        const auto controlBg   = juce::Colour (GUIConstants::Color::ZONE_BG);
        const auto faintBorder = controlBg.brighter (0.10f);
        const auto dimText     = juce::Colour (GUIConstants::Color::LABEL_DIM);
        slotsCombo_.setColour (juce::ComboBox::backgroundColourId, controlBg);
        slotsCombo_.setColour (juce::ComboBox::textColourId,       dimText);
        slotsCombo_.setColour (juce::ComboBox::outlineColourId,    faintBorder);
        slotsCombo_.setColour (juce::ComboBox::arrowColourId,      dimText);
        slotsCombo_.onChange = [this]
        {
            const int id = slotsCombo_.getSelectedId();
            if (id <= 0) return;
            const int pendingSlot = id - 1;
            const int curSlot = proc_.getCurrentSlot();

            if (! isModified_ || pendingSlot == curSlot || perfModeBtn_.getToggleState())
            {
                proc_.loadFromSlot (pendingSlot);
                if (onStateChanged) onStateChanged();
                return;
            }

            // Modified: reset combo visually then show guard dialog
            slotsCombo_.setSelectedId (curSlot + 1, juce::dontSendNotification);
            updateComboAppearance (true);
            showModifiedGuard (pendingSlot);
        };
        addAndMakeVisible (slotsCombo_);

        proc_.addChangeListener (this);

        {
            juce::String saved = getAppPrefs().getValue ("lastFileChooserDir", {});
            if (saved.isNotEmpty())
            {
                juce::File f (saved);
                if (f.isDirectory())
                    lastFileChooserDir_ = f;
            }
        }

        startTimerHz (4);
        refresh();
    }

    ~TopPanel() override
    {
        stopTimer();
        proc_.apvts.removeParameterListener (WayQAudioProcessor::PID_BYPASS, this);
        proc_.removeChangeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        // Match the editor's main background so the bar disappears as a strip
        // and only the discrete controls remain visible.
        g.fillAll (juce::Colour (GUIConstants::Color::BG));
    }

    void resized() override
    {
        auto full = getLocalBounds().reduced (2);

        const int rowH  = juce::jmin (22, full.getHeight());
        const int btnW  = GUIConstants::LINK_BTN_SIZE;   // 28, matches A/B link buttons
        const int gap   = 3;
        const int midY  = full.getCentreY();
        auto b = full.withSizeKeepingCentre (full.getWidth(), rowH);

        const int workAreaRightX = (int)(getWidth() * 0.45f);

        // Left cluster — prev and next at far left
        prevBtn_.setBounds (full.getX(),            midY - btnW / 2, btnW, btnW);
        nextBtn_.setBounds (full.getX() + btnW + 1, midY - btnW / 2, btnW, btnW);

        // presetMgmt right-anchored at workAreaRightX
        const int mgmtLeft = workAreaRightX - 3 * btnW - 2 * gap;
        presetMgmtBtn_.setBounds (mgmtLeft, midY - btnW / 2, btnW, btnW);
        perfModeBtn_.setBounds (mgmtLeft + btnW + gap, midY - btnW / 2, btnW, btnW);

        // undo/redo centered above the center panel
        const int centerMidX = (int)(getWidth() * 0.50f);
        const int undoLeft   = centerMidX - (btnW + gap + btnW) / 2;
        const int redoLeft   = undoLeft + btnW + gap;
        undoBtn_.setBounds (undoLeft, midY - btnW / 2, btnW, btnW);
        redoBtn_.setBounds (redoLeft, midY - btnW / 2, btnW, btnW);

        // slotsCombo_ fills the gap between next and presetMgmt
        {
            const int comboLeft  = full.getX() + btnW + 1 + btnW + gap;
            const int comboRight = mgmtLeft - gap;
            const int newComboW  = juce::jmax (120, comboRight - comboLeft);
            slotsCombo_.setBounds (comboLeft, b.getY(), newComboW, b.getHeight());
        }

        // Right cluster — tooltips, bypass, hamburger
        const int hamLeft = full.getRight() - btnW;
        const int bypLeft = hamLeft - gap - btnW;
        const int tipLeft = bypLeft - gap - btnW;

        hamburgerBtn_.setBounds (hamLeft, midY - btnW / 2, btnW, btnW);
        bypassBtn_   .setBounds (bypLeft, midY - btnW / 2, btnW, btnW);
        tooltipsBtn_ .setBounds (tipLeft, midY - btnW / 2, btnW, btnW);
    }

private:
    void changeListenerCallback (juce::ChangeBroadcaster* /*src*/) override
    {
        refresh();
        isModified_ = proc_.isPresetModified();
        updateComboAppearance (isModified_);
    }

    void timerCallback() override
    {
        const bool modified = proc_.isPresetModified();
        if (modified != isModified_)
        {
            isModified_ = modified;
            updateComboAppearance (modified);
        }
    }

    void updateComboAppearance (bool modified)
    {
        const int cur = proc_.getCurrentSlot();
        slotsCombo_.setSelectedId (cur + 1, juce::dontSendNotification);
        if (modified)
            slotsCombo_.setText ("* " + slotsCombo_.getText(), juce::dontSendNotification);
    }

public:
    // Keeps the bypass button in sync with PID_BYPASS. May fire on the audio
    // thread; marshals UI work to the message thread.
    void parameterChanged (const juce::String& paramID, float newValue) override
    {
        if (paramID == WayQAudioProcessor::PID_BYPASS)
        {
            const bool lit = newValue < 0.5f;
            juce::MessageManager::callAsync (
                [this, lit] { bypassBtn_.setToggleState (lit, juce::dontSendNotification); });
            return;
        }
    }

    void styleButton (juce::TextButton& b, const juce::String& text)
    {
        const auto controlBg = juce::Colour (GUIConstants::Color::BTN_BG);
        b.setButtonText (text);
        b.setColour (juce::TextButton::buttonColourId,    controlBg);
        b.setColour (juce::TextButton::buttonOnColourId,  controlBg.brighter (0.10f));
        b.setColour (juce::TextButton::textColourOffId,   juce::Colour (GUIConstants::Color::BTN_TEXT));
        b.setColour (juce::TextButton::textColourOnId,    juce::Colour (GUIConstants::Color::BTN_ACTIVE));
        addAndMakeVisible (b);
    }

    void styleToggleButton (juce::TextButton& b, const juce::String& text)
    {
        // Inversion: on-state = BTN_ACTIVE background + dark text (ZONE_BG).
        styleButton (b, text);
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (GUIConstants::Color::BTN_ACTIVE));
        b.setColour (juce::TextButton::textColourOnId,   juce::Colour (GUIConstants::Color::ZONE_BG));
    }

    void refresh()
    {
        const int cur = proc_.getCurrentSlot();

        slotsCombo_.clear (juce::dontSendNotification);
        for (int slot = 0; slot < WayQAudioProcessor::kNumPresetSlots; ++slot)
        {
            juce::String label = juce::String::formatted ("%03d  ", slot);
            if (proc_.isSlotUsed (slot))
            {
                auto sn = proc_.getSlotName (slot);
                label += sn.isNotEmpty() ? sn : juce::String ("(unnamed)");
            }
            else
            {
                label += "(empty)";
            }
            slotsCombo_.addItem (label, slot + 1);
        }
        slotsCombo_.setSelectedId (cur + 1, juce::dontSendNotification);
        undoBtn_.setEnabled (proc_.canUndo());
        undoBtn_.setToggleState (proc_.canUndo(), juce::dontSendNotification);
        redoBtn_.setEnabled (proc_.canRedo());
        redoBtn_.setToggleState (proc_.canRedo(), juce::dontSendNotification);
    }

    void stepSlot (int delta)
    {
        const int next = juce::jlimit (0, WayQAudioProcessor::kNumPresetSlots - 1,
                                       proc_.getCurrentSlot() + delta);
        if (next == proc_.getCurrentSlot()) return;

        if (! isModified_ || perfModeBtn_.getToggleState())
        {
            proc_.loadFromSlot (next);
            if (onStateChanged) onStateChanged();
            return;
        }

        showModifiedGuard (next);
    }

    void showModifiedGuard (int pendingSlot)
    {
        const int curSlot = proc_.getCurrentSlot();
        auto* aw = new juce::AlertWindow ("Preset modified",
                                          "This preset has been modified.",
                                          juce::AlertWindow::QuestionIcon);
        aw->addButton ("Save as new preset", 1);
        aw->addButton ("Overwrite current",  2);
        aw->addButton ("Discard",            3);
        if (curSlot == 0)
            if (auto* btn = aw->getButton ("Overwrite current"))
                btn->setEnabled (false);
        aw->enterModalState (true,
            juce::ModalCallbackFunction::create (
                [this, pendingSlot, curSlot] (int result)
                {
                    if (result == 1)
                    {
                        onSaveAsClicked();
                    }
                    else if (result == 2)
                    {
                        proc_.saveToSlot (curSlot);
                        proc_.loadFromSlot (pendingSlot);
                        if (onStateChanged) onStateChanged();
                    }
                    else if (result == 3)
                    {
                        proc_.loadFromSlot (pendingSlot);
                        if (onStateChanged) onStateChanged();
                    }
                }),
            true);
    }

    void showPresetMgmtMenu()
    {
        const int cur = proc_.getCurrentSlot();
        const bool slot0 = (cur == 0);
        const bool used  = proc_.isSlotUsed (cur);

        juce::PopupMenu m;
        m.addItem (1, "Save",     ! slot0);
        m.addItem (2, "Save As...");
        m.addItem (3, "Rename/Edit...", ! slot0 && used);
        m.addSeparator();
        // Copy works on any used slot (incl. slot 0 "Init"); Paste is only
        // allowed into user slots.
        m.addItem (6, "Copy",   used);
        m.addItem (7, "Paste",  ! slot0);
        m.addSeparator();
        m.addItem (4, "Import preset...", ! slot0);
        m.addItem (5, "Export preset...", used);
        m.addSeparator();
        m.addItem (8, "Import bank...");
        m.addItem (9, "Export bank...");

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetMgmtBtn_),
            [this] (int result)
            {
                switch (result)
                {
                    case 1: onSaveClicked();       break;
                    case 2: onSaveAsClicked();     break;
                    case 3: onRenameClicked();     break;
                    case 4: onImportClicked();     break;
                    case 5: onExportClicked();     break;
                    case 6: onCopyClicked();       break;
                    case 7: onPasteClicked();      break;
                    case 8: onImportBankClicked(); break;
                    case 9: onExportBankClicked(); break;
                    default: break;
                }
            });
    }

    void showHamburgerMenu()
    {
        juce::PopupMenu m;
        m.addItem (1, "Way Music");
        m.addItem (2, "WayQ");
        m.addItem (3, "Help");
        m.addItem (4, "WayQ on YouTube (TBA)");
        m.addItem (5, "Community");
        m.addItem (6, "Report a bug or suggest a feature");
        m.addItem (7, "Check for updates");
        m.addSeparator();
        m.addItem (9, "Refresh controls");
        m.addSeparator();
        m.addItem (8, "About");

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&hamburgerBtn_),
            [this] (int result)
            {
                switch (result)
                {
                    case 1: juce::URL ("https://ghost.way.net").launchInDefaultBrowser();                       break;
                    case 2: juce::URL ("https://way.net/wayq").launchInDefaultBrowser();                       break;
                    case 3: juce::URL ("https://github.com/rcrath/wayq/wiki").launchInDefaultBrowser();        break;
                    case 4: break;   // YouTube link TBA — no URL yet
                    case 5: juce::URL ("https://github.com/rcrath/wayq/discussions").launchInDefaultBrowser(); break;
                    case 6: juce::URL ("https://github.com/rcrath/wayq/issues").launchInDefaultBrowser();      break;
                    case 7: juce::URL ("https://way.net/wayq/installers").launchInDefaultBrowser();            break;
                    case 8: showAboutBox(); break;
                    case 9:
                        proc_.reannounceAllParams();
                        if (onRefreshControls)
                            onRefreshControls();
                        break;
                    default: break;
                }
            });
    }

    void showAboutBox()
    {
        juce::String msg =
            juce::String ("WayQ ") + PLUGIN_VERSION + "\n"
            "A project of Rich Rath and Way Music.\n"
            "(c) 2026, all rights reserved.\n\n"
            "Contact: TBA";

        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::InfoIcon,
            "About WayQ",
            msg,
            "OK",
            this);
    }

    void onSaveClicked()
    {
        const int cur = proc_.getCurrentSlot();
        if (cur == 0) return;   // locked

        if (proc_.isSlotUsed (cur))
        {
            const auto existingName = proc_.getSlotName (cur);
            juce::AlertWindow::showOkCancelBox (
                juce::AlertWindow::QuestionIcon,
                "Overwrite preset slot?",
                juce::String::formatted ("Overwrite slot %d (\"", cur)
                    + existingName + "\")?",
                "Overwrite", "Cancel", this,
                juce::ModalCallbackFunction::create (
                    [this, cur] (int result)
                    {
                        if (result == 1)
                            proc_.saveToSlot (cur);
                    }));
        }
        else
        {
            proc_.saveToSlot (cur);
        }
    }

    void onSaveAsClicked()
    {
        auto* aw = new juce::AlertWindow ("Save preset as...",
                                          "Pick a slot (1-127) and a name.",
                                          juce::AlertWindow::QuestionIcon);
        int suggest = 1;
        for (int i = 1; i < WayQAudioProcessor::kNumPresetSlots; ++i)
            if (! proc_.isSlotUsed (i)) { suggest = i; break; }

        aw->addTextEditor ("slot", juce::String (suggest), "Slot (1-127):");
        aw->addTextEditor ("name", juce::String ("Slot ") + juce::String (suggest), "Name:");
        aw->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
        aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        aw->enterModalState (true,
            juce::ModalCallbackFunction::create (
                [this, aw] (int result)
                {
                    if (result != 1) return;
                    const int slot = aw->getTextEditorContents ("slot").getIntValue();
                    const auto name = aw->getTextEditorContents ("name").trim();
                    if (slot <= 0 || slot >= WayQAudioProcessor::kNumPresetSlots)
                        return;   // slot 0 not selectable; out-of-range ignored
                    proc_.saveToSlot (slot);
                    if (name.isNotEmpty())
                        proc_.renameSlot (slot, name);
                    proc_.setCurrentSlot (slot);
                }),
            true);

        // The host's focus dance after enterModalState can land focus outside
        // the dialog; 150 ms lets it settle before we grab it back to the editor.
        juce::Timer::callAfterDelay (150,
            [awSafe = juce::Component::SafePointer<juce::AlertWindow> (aw)]
            {
                if (awSafe != nullptr)
                    if (auto* ed = awSafe->getTextEditor ("name"))
                        ed->grabKeyboardFocus();
            });
    }

    void onRenameClicked()
    {
        const int cur = proc_.getCurrentSlot();
        if (cur == 0) return;                 // locked
        if (! proc_.isSlotUsed (cur)) return;

        auto* aw = new juce::AlertWindow ("Rename/Edit preset",
                                          juce::String::formatted ("Rename/Edit slot %d", cur),
                                          juce::AlertWindow::QuestionIcon);
        aw->addTextEditor ("name", proc_.getSlotName (cur), "Name:");
        aw->addButton ("OK",     1, juce::KeyPress (juce::KeyPress::returnKey));
        aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        aw->enterModalState (true,
            juce::ModalCallbackFunction::create (
                [this, aw, cur] (int result)
                {
                    if (result != 1) return;
                    const auto name = aw->getTextEditorContents ("name").trim();
                    if (name.isNotEmpty())
                        proc_.renameSlot (cur, name);
                }),
            true);

        // Deferred focus grab — same host-focus-dance reason as Save As.
        juce::Timer::callAfterDelay (150,
            [awSafe = juce::Component::SafePointer<juce::AlertWindow> (aw)]
            {
                if (awSafe != nullptr)
                    if (auto* ed = awSafe->getTextEditor ("name"))
                        ed->grabKeyboardFocus();
            });
    }

    void onExportClicked()
    {
        const int cur = proc_.getCurrentSlot();
        if (! proc_.isSlotUsed (cur)) return;

        auto suggested = lastFileChooserDir_
                            .getChildFile (proc_.getSlotName (cur).replaceCharacter (' ', '_') + ".wayq.xml");

        fileChooser_ = std::make_unique<juce::FileChooser> (
            "Export preset to XML",
            suggested,
            "*.xml");

        fileChooser_->launchAsync (
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                | juce::FileBrowserComponent::warnAboutOverwriting,
            [this, cur] (const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == juce::File{}) return;
                lastFileChooserDir_ = file.getParentDirectory();
                getAppPrefs().setValue ("lastFileChooserDir", lastFileChooserDir_.getFullPathName());
                getAppPrefs().saveIfNeeded();
                if (! proc_.exportSlotToFile (cur, file))
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::WarningIcon,
                        "Export preset",
                        "Export failed. The file could not be written — check disk space and permissions.",
                        "OK", this);
            });
    }

    void onImportClicked()
    {
        const int cur = proc_.getCurrentSlot();
        if (cur == 0) return;   // can't overwrite Init

        fileChooser_ = std::make_unique<juce::FileChooser> (
            "Import preset XML",
            lastFileChooserDir_,
            "*.xml");

        fileChooser_->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == juce::File{}) return;
                lastFileChooserDir_ = file.getParentDirectory();
                getAppPrefs().setValue ("lastFileChooserDir", lastFileChooserDir_.getFullPathName());
                getAppPrefs().saveIfNeeded();
                promptDestinationSlot ("Import preset into slot...",
                    [this, file] (int slot)
                    {
                        if (proc_.importSlotFromFile (slot, file))
                            proc_.loadFromSlot (slot);
                        else
                            juce::AlertWindow::showMessageBoxAsync (
                                juce::AlertWindow::WarningIcon,
                                "Import preset",
                                "Could not import preset. The file may be missing, malformed, or from an incompatible version.",
                                "OK", this);
                    });
            });
    }

    void promptDestinationSlot (const juce::String& title,
                                std::function<void (int)> onAccept)
    {
        const int cur = proc_.getCurrentSlot();
        int suggest = cur > 0 ? cur : 1;
        if (suggest == 0 || proc_.isSlotUsed (suggest))
        {
            for (int i = 1; i < WayQAudioProcessor::kNumPresetSlots; ++i)
                if (! proc_.isSlotUsed (i)) { suggest = i; break; }
        }

        auto* aw = new juce::AlertWindow (title,
                                          "Destination slot (1-127):",
                                          juce::AlertWindow::QuestionIcon);
        aw->addTextEditor ("slot", juce::String (suggest), {});
        aw->addButton ("OK",     1, juce::KeyPress (juce::KeyPress::returnKey));
        aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        aw->enterModalState (true,
            juce::ModalCallbackFunction::create (
                [this, aw, onAccept = std::move (onAccept)] (int result)
                {
                    if (result != 1) return;
                    const int slot = aw->getTextEditorContents ("slot").getIntValue();
                    if (slot <= 0 || slot >= WayQAudioProcessor::kNumPresetSlots)
                        return;

                    if (proc_.isSlotUsed (slot))
                    {
                        juce::AlertWindow::showOkCancelBox (
                            juce::AlertWindow::QuestionIcon,
                            "Overwrite preset slot?",
                            juce::String::formatted ("Overwrite slot %d (\"", slot)
                                + proc_.getSlotName (slot) + "\")?",
                            "Overwrite", "Cancel", this,
                            juce::ModalCallbackFunction::create (
                                [slot, onAccept] (int ok)
                                {
                                    if (ok == 1) onAccept (slot);
                                }));
                    }
                    else
                    {
                        onAccept (slot);
                    }
                }),
            true);

        // Deferred focus grab — same host-focus-dance reason as Save As.
        juce::Timer::callAfterDelay (150,
            [awSafe = juce::Component::SafePointer<juce::AlertWindow> (aw)]
            {
                if (awSafe != nullptr)
                    if (auto* ed = awSafe->getTextEditor ("slot"))
                        ed->grabKeyboardFocus();
            });
    }

    void onCopyClicked()
    {
        const int cur = proc_.getCurrentSlot();
        if (! proc_.isSlotUsed (cur)) return;
        auto xml = proc_.slotToXmlString (cur);
        if (xml.isNotEmpty())
            juce::SystemClipboard::copyTextToClipboard (xml);
    }

    void onPasteClicked()
    {
        const auto text = juce::SystemClipboard::getTextFromClipboard();
        if (text.isEmpty()) return;

        if (! text.contains ("<Slot"))
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "Paste",
                "Clipboard does not contain a WayQ preset.",
                "OK", this);
            return;
        }

        promptDestinationSlot ("Paste preset into slot...",
            [this, text] (int slot)
            {
                if (proc_.importSlotFromXmlString (slot, text))
                    proc_.loadFromSlot (slot);
                else
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::WarningIcon,
                        "Paste",
                        "Could not import the preset. The clipboard content may be from an incompatible version.",
                        "OK", this);
            });
    }

    void onExportBankClicked()
    {
        auto suggested = lastFileChooserDir_
                            .getChildFile ("default_bank.xml");

        fileChooser_ = std::make_unique<juce::FileChooser> (
            "Export preset bank to XML",
            suggested,
            "*.xml");

        fileChooser_->launchAsync (
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                | juce::FileBrowserComponent::warnAboutOverwriting,
            [this] (const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == juce::File{}) return;
                lastFileChooserDir_ = file.getParentDirectory();
                getAppPrefs().setValue ("lastFileChooserDir", lastFileChooserDir_.getFullPathName());
                getAppPrefs().saveIfNeeded();
                if (! proc_.exportBankToFile (file))
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::WarningIcon,
                        "Export bank",
                        "Export failed. The file could not be written — check disk space and permissions.",
                        "OK", this);
            });
    }

    void onImportBankClicked()
    {
        fileChooser_ = std::make_unique<juce::FileChooser> (
            "Import preset bank (replaces all slots)",
            lastFileChooserDir_.getChildFile ("default_bank.xml"),
            "*.xml");

        fileChooser_->launchAsync (
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == juce::File{}) return;
                lastFileChooserDir_ = file.getParentDirectory();
                getAppPrefs().setValue ("lastFileChooserDir", lastFileChooserDir_.getFullPathName());
                getAppPrefs().saveIfNeeded();

                int usedCount = 0;
                for (int i = 1; i < WayQAudioProcessor::kNumPresetSlots; ++i)
                    if (proc_.isSlotUsed (i)) ++usedCount;

                juce::AlertWindow::showOkCancelBox (
                    juce::AlertWindow::WarningIcon,
                    "Replace entire preset bank?",
                    juce::String::formatted (
                        "This will erase all %d used slot(s) and replace them with the file's contents. Continue?",
                        usedCount),
                    "Replace", "Cancel", this,
                    juce::ModalCallbackFunction::create (
                        [this, file] (int ok)
                        {
                            if (ok != 1) return;
                            if (! proc_.importBankFromFile (file))
                                juce::AlertWindow::showMessageBoxAsync (
                                    juce::AlertWindow::WarningIcon,
                                    "Import bank",
                                    "File is not a valid WayQ preset bank.",
                                    "OK", this);
                        }));
            });
    }

    WayQAudioProcessor& proc_;

    juce::TextButton  prevBtn_, nextBtn_, hamburgerBtn_, presetMgmtBtn_, tooltipsBtn_,
                      undoBtn_, redoBtn_, perfModeBtn_;
    UnifontTextButton bypassBtn_;    // power glyph U+23FB needs Unifont
    juce::ComboBox    slotsCombo_;

public:
    std::function<void(bool)> onTooltipsToggled;
    std::function<void()>     onRefreshControls;
    std::function<void()>     onStateChanged;

private:

    bool isModified_ = false;
    std::unique_ptr<juce::FileChooser> fileChooser_;
    juce::File lastFileChooserDir_ {
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopPanel)
};
