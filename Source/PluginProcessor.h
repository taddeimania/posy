#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

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
    void  loadSet (int index) { setParam (posy::ids::chordSet, (float) (juce::jlimit (0, posy::numChordSets() - 1, index) + 1)); }

    // The voicing a key of the current set plays with the panel's settings.
    posy::Voicing previewVoicing (int key) const;

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
    void timerCallback() override;      // collects captured chords
    posy::PlayerSettings readSettings() const;

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

    juce::Array<TakeChord> takeChords;
    uint32_t takeVersion = 0;
    bool recordTake = true;
    int beatsPerTakeChord = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PosyProcessor)
};
