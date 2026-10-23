// Player.h — the real-time half of Posy: turns key presses (from the panel's
// pads or a MIDI keyboard) into chords, strums, holds and arpeggios. No JUCE,
// so the tests can drive it directly.
//
// One chord sounds at a time. Keys stack: the last one pressed plays, and
// letting it go falls back to the one held before it. With HOLD on, the chord
// stays after the keys are released until another key replaces it or HOLD is
// switched off.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "Chords.h"

namespace posy
{
// ---------------------------------------------------------------------------
// Arpeggiator choices
// ---------------------------------------------------------------------------
enum class ArpMode { up = 0, down, upDown, downUp, upDownInc, converge, random, chord };
inline constexpr int kNumArpModes = 8;
inline constexpr const char* kArpModeNames[kNumArpModes] {
    "Up", "Down", "Up & Down", "Down & Up", "Up & Down +", "Converge", "Random", "Chord"
};

inline constexpr int kNumArpRates = 9;
inline constexpr const char* kArpRateNames[kNumArpRates] {
    "1/1", "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T"
};
inline constexpr double kArpRateQuarters[kNumArpRates] {
    4.0, 2.0, 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125, 1.0 / 12.0
};
inline constexpr int kArpRateDefault = 5;   // 1/16

// ---------------------------------------------------------------------------
struct PlayerSettings
{
    int setIndex = 0;              // 0-based chord set
    VoiceSettings voice;
    bool hold   = false;
    bool legato = false;           // keep notes two chords share instead of restriking them
    int  strumMs   = 0;            // 0..200: delay between the notes of a chord
    bool strumDown = false;        // strum from the top note down
    int  velocity  = 100;          // 1..127
    bool fixedVelocity = false;    // ignore the velocity the key was played with
    bool followOctave  = true;     // a MIDI key's octave moves the chord with it

    bool    arp = false;
    ArpMode arpMode = ArpMode::up;
    int     arpRate = kArpRateDefault;
    int     arpOctaves = 1;        // 1..4
    int     arpGate = 60;          // 5..100 % of a step
};

struct BlockInfo
{
    int    numSamples = 0;
    double sampleRate = 44100.0;
    bool   playing = false;        // host transport
    double bpm = 120.0;
    double ppq = 0.0;              // host position at the start of the block, in quarter notes
};

// A key going down or up. `source` identifies the physical key so its release
// can be matched: a MIDI note number, or kPadSource + key for the panel's pads.
struct KeyEvent
{
    int  offset = 0;               // samples into the block
    bool on = true;
    int  source = 0;
    int  key = 0;                  // 0..11: which chord of the set
    int  octave = 0;               // octaves above/below the home octave
    int  velocity = 100;
};
inline constexpr int kPadSource = 1000;

struct NoteOut
{
    int  offset;
    int  note;
    int  velocity;
    bool on;
};

// The chord most recently started by a key, for the panel and the take.
struct Started
{
    uint32_t serial = 0;           // moves on every new chord
    int setIndex = 0, key = -1, octave = 0;
    Voicing voicing;
};

// ---------------------------------------------------------------------------
class Player
{
public:
    Player();

    // Appends this block's notes to `out` (offsets within the block, in order).
    void process (const BlockInfo&, const PlayerSettings&, const KeyEvent* events, int numEvents,
                  std::vector<NoteOut>& out);

    // Silence everything and forget the held keys (next block).
    void panic() noexcept { panicRequested = true; }

    int  currentKey() const noexcept    { return curKey; }       // -1: nothing playing
    int  currentOctave() const noexcept { return curOct; }
    bool keyIsDown (int key) const noexcept;
    const Started& started() const noexcept { return lastStarted; }

private:
    struct Held { int source, key, octave, velocity; };
    struct Pending
    {
        int64_t time;
        int  note, velocity;
        bool on;
        bool chordOn;              // a chord note-on that a new chord may cancel
    };

    // Scheduling
    void schedule (int64_t time, int note, int velocity, bool on, bool chordOn = false);
    void cancelChordOns();
    void flushUntil (int64_t end, std::vector<NoteOut>& out);
    void allOff (int64_t time);

    // Chords
    Voicing voicingFor (int key, int octave) const;
    void engage (int64_t time, int key, int octave, int velocity, bool fromKey);
    void release (int64_t time);
    void chordTo (const Voicing& v, int64_t time, bool strum);

    // Arpeggiator
    void arpRun (int64_t from, int64_t to);
    void arpFire (int64_t time);
    double stepSamples() const;

    // --- state -----------------------------------------------------------------
    PlayerSettings s;
    BlockInfo info;
    int64_t now = 0;                     // samples since the plugin started

    std::array<Held, 32> held {};
    int numHeld = 0;

    int curKey = -1, curOct = 0, curVel = 100;
    Voicing chord;                       // what the current chord should be
    std::array<bool, 128> chordNotes {}; // chord-path notes on or about to be (not arp)

    std::vector<Pending> pending;
    std::array<bool, 128> outOn {};      // what we've actually sent

    // Arp
    bool    arpWasOn = false;
    bool    wasPlaying = false;
    uint32_t arpStep = 0;
    double  nextStepTime = 0.0;          // free-running clock (transport stopped)
    int64_t lastFiredK = -1;             // synced clock: last grid step fired
    int64_t skipBefore = 0;              // don't fire a grid step before this (just played one)
    uint32_t rng = 0x2545F491u;
    int     lastRandom = -1;

    int lastSet = -1;
    bool panicRequested = false;
    Started lastStarted;
};
} // namespace posy
