#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <bitset>

#include "Banks.h"
#include "Engine/Chords.h"
#include "Engine/Player.h"
#include "Params.h"

// A chord captured into the TAKE strip: what you played, as it sounded.
struct TakeChord
{
    int setNumber = 1;
    int key = 0;
    juce::String name;
    juce::Array<int> notes;
};

class PosyProcessor final : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::Timer
{
public:
    PosyProcessor();
    ~PosyProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- panel (message thread) ---------------------------------------------
    void pressPad (int key, int velocity);
    void releasePad (int key);
    void panic() { panicRequested.store (true); }

    void setParam (const juce::String& id, float plainValue);
    float getParam (const juce::String& id) const { return params.at (id)->load(); }
    int   setIndex() const { return juce::jlimit (0, posy::numChordSets() - 1, juce::roundToInt (getParam (posy::ids::chordSet)) - 1); }
    void  loadSet (int index);   // a factory set (leaves any custom bank)

    // The voicing a key of the current set plays with the panel's settings.
    posy::Voicing previewVoicing (int key) const;
    juce::String chordName (int key) const;   // of the factory set or bank in use
    juce::String setTitle() const;            // "Neo Soul", or the bank's name

    // --- custom banks (message thread) ------------------------------------------
    // A custom bank replaces the factory set while it's loaded. EDIT mode turns
    // the MIDI keyboard into a recorder: notes pass straight through to the
    // instrument so you hear what you play, and when you let go of a chord it
    // lands on the selected pad, named, and the next pad is selected.
    bool isCustom() const noexcept           { return customActive.load(); }
    const posy::CustomBank& bank() const     { return custom; }
    bool bankDirty() const noexcept          { return customDirty; }
    uint32_t bankEdits() const noexcept      { return bankVersion; }      // moves on every change
    uint32_t bankFilesEdits() const noexcept { return bankFilesVersion; } // moves when files change
    void bankFilesChanged()                  { ++bankFilesVersion; }

    void startEditing();                      // copy the factory set into an editable bank
    void newBank();                           // twelve empty pads
    bool loadBank (const juce::File&);
    bool saveBank();                          // to its own file; false if it has none yet
    bool saveBankAs (const juce::String& name);
    void bankFileRenamed (const juce::File& from, const juce::File& to, const juce::String& name);

    void setPadNotes (int key, juce::Array<int> notes);   // and names the chord
    void setPadName (int key, const juce::String& name);  // empty: back to the automatic name
    void clearPad (int key);
    void togglePadNote (int key, int note);

    bool editing() const noexcept            { return editMode.load(); }
    void setEditing (bool on);
    int  editPad() const noexcept            { return selectedPad; }
    void selectPad (int key)                 { selectedPad = juce::jlimit (0, posy::kKeys - 1, key); ++bankVersion; }
    juce::String lastCaptured() const        { return captureText; }

    // --- read by the panel ---------------------------------------------------
    int  playingKey() const noexcept      { return uiKey.load (std::memory_order_relaxed); }
    bool hostPlaying() const noexcept     { return uiPlaying.load (std::memory_order_relaxed); }
    bool keyDown (int key) const noexcept { return (uiKeysDown.load (std::memory_order_relaxed) >> key) & 1u; }
    bool noteSounding (int note) const noexcept
    {
        return juce::isPositiveAndBelow (note, 128)
            && ((uiNotes[(size_t) (note >> 5)].load (std::memory_order_relaxed) >> (note & 31)) & 1u);
    }

    // --- take ------------------------------------------------------------------
    const juce::Array<TakeChord>& take() const noexcept { return takeChords; }
    void clearTake()                { takeChords.clear(); ++takeVersion; }
    void removeTake (int index)     { takeChords.remove (index); ++takeVersion; }
    uint32_t takeEdits() const      { return takeVersion; }
    bool takeRecording() const      { return recordTake; }
    void setTakeRecording (bool on) { recordTake = on; }
    int  takeBeats() const          { return beatsPerTakeChord; }   // 2, 4 or 8
    void setTakeBeats (int b)       { beatsPerTakeChord = b; }
    juce::File writeTakeMidi() const;   // a .mid file of the take, for dragging into a clip

    juce::AudioProcessorValueTreeState apvts;

private:
    void timerCallback() override;      // collects started and captured chords
    void parameterChanged (const juce::String& id, float) override;
    posy::PlayerSettings readSettings() const;
    void bankEdited();                  // mark dirty, publish to the audio thread
    void publishBank();
    const posy::ChordDef* beginReadBank() noexcept
    {
        const int p = bankPublished.load (std::memory_order_acquire);
        bankReading.store (p, std::memory_order_release);
        return bankTables[(size_t) p].data();
    }

    std::map<juce::String, std::atomic<float>*> params;

    posy::Player player;
    std::vector<posy::KeyEvent> events;
    std::vector<posy::NoteOut>  notesOut;
    juce::MidiBuffer passthrough;

    // Pad presses from the panel to the audio thread.
    juce::AbstractFifo padFifo { 256 };
    std::array<posy::KeyEvent, 256> padEvents {};
    juce::SpinLock padWriteLock;          // two editors could press at once

    // Chords started, from the audio thread to the take.
    juce::AbstractFifo startedFifo { 64 };
    std::array<posy::Started, 64> startedEvents {};
    uint32_t lastSerial = 0;

    std::atomic<bool> panicRequested { false };
    std::atomic<int> uiKey { -1 };
    std::atomic<bool> uiPlaying { false };
    std::atomic<uint32_t> uiKeysDown { 0 };
    std::array<std::atomic<uint32_t>, 4> uiNotes {};
    std::array<bool, 128> sounding {};

    // Custom bank: edited here, copied for the audio thread into one of three
    // tables (never the one it's reading or the one last handed over).
    posy::CustomBank custom;
    std::atomic<bool> customActive { false };
    bool customDirty = false;
    uint32_t bankVersion = 0, bankFilesVersion = 0;
    std::array<std::array<posy::ChordDef, posy::kKeys>, 3> bankTables {};
    std::atomic<int> bankPublished { 0 }, bankReading { 0 };

    // EDIT mode: chords played in on the MIDI keyboard.
    std::atomic<bool> editMode { false };
    int selectedPad = 0;
    juce::String captureText;
    std::bitset<128> heldIn, peakIn;     // audio thread: keys down now, and the most held at once
    juce::AbstractFifo captureFifo { 16 };
    std::array<std::bitset<128>, 16> captures {};

    juce::Array<TakeChord> takeChords;
    uint32_t takeVersion = 0;
    bool recordTake = true;
    int beatsPerTakeChord = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PosyProcessor)
};
