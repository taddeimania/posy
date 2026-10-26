#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "Settings.h"
#include "UI/Panels.h"
#include "UI/Widgets.h"

// The panel is laid out in a fixed design space and scaled to fit the window.
class PosyPanel final : public juce::Component, private juce::Timer
{
public:
    static constexpr int designW = 1400;
    static constexpr int designH = 900;

    explicit PosyPanel (PosyProcessor&);
    ~PosyPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override {}

    std::function<void (bool)> onTooltips;   // the editor shows or hides its tooltip window
    void syncTooltips();

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void timerCallback() override;

    void addKnob (posy::ui::Knob&, const char* id, const juce::String& caption,
                  juce::Rectangle<int>, const juce::String& tip);
    void addToggle (posy::ui::PanelButton&, const char* id, juce::Rectangle<int>, const juce::String& tip);
    void addButton (posy::ui::PanelButton&, juce::Rectangle<int>, const juce::String& tip,
                    std::function<void (const juce::ModifierKeys&)> click);
    void addChoice (posy::ui::ChoiceDisplay&, const char* id, const juce::String& caption,
                    juce::Rectangle<int>, const juce::String& tip);
    void chooseParam (juce::Component& target, const char* id);

    PosyProcessor& proc;
    posy::ui::PanelLookAndFeel lnf;
    posy::Settings settings;

    // Header
    posy::ui::SetDisplay setDisplay { proc };
    posy::ui::PanelButton prevBtn { "<" }, nextBtn { ">" }, tipsBtn { "TIPS" }, panicBtn { "ALL OFF" };
    posy::ui::ChoiceDisplay channelChoice;

    // Browser, pads, notes, take
    posy::ui::Browser   browser { proc, settings };
    posy::ui::ChordPads pads { proc };
    posy::ui::NoteStrip strip { proc };
    posy::ui::TakeStrip takeStrip { proc };
    posy::ui::PanelButton recBtn { "REC" }, clearTakeBtn { "CLEAR" };

    // Custom banks
    posy::ui::PanelButton editBtn { "EDIT" }, saveBtn { "SAVE" }, saveAsBtn { "SAVE AS" }, newBtn { "NEW" };
    posy::ui::ChoiceDisplay takeLenChoice;

    // Voice
    posy::ui::Knob keyKnob, octaveKnob, invertKnob, spreadKnob, strumKnob, velocityKnob;
    posy::ui::PanelButton bassBtn { "BASS" }, legatoBtn { "LEGATO" }, followBtn { "KBD OCT" },
                          strumDownBtn { "DOWN" }, fixedVelBtn { "FIXED" };

    // Arp & hold
    posy::ui::PanelButton arpBtn { "ARP" }, holdBtn { "HOLD" };
    posy::ui::ChoiceDisplay arpModeChoice, arpRateChoice;
    posy::ui::Knob arpOctKnob, arpGateKnob;

    std::vector<std::pair<posy::ui::PanelButton*, const char*>> toggles;
    std::vector<posy::ui::ChoiceDisplay*> choices;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    int tooltipsApplied = -1;
    juce::String arpStatus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PosyPanel)
};

class PosyEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PosyEditor (PosyProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PosyPanel panel;
    std::unique_ptr<juce::TooltipWindow> tooltips;   // null while TIPS is off

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PosyEditor)
};
