// Chords.h — Posy's chord data and voicing tools. No JUCE, so the tests can
// build it on its own.
//
// A chord set is twelve chords, one per key of the octave (C, C#, ... B). Each
// chord is stored as a ready-made voicing: absolute MIDI notes, lowest first.
// The voice controls (key, octave, invert, spread, bass) are applied on top.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace posy
{
inline constexpr int kKeys = 12;
inline constexpr int kMaxStored = 10;   // notes in a stored voicing (factory sets use up to 6)
inline constexpr int kMaxVoice  = 16;   // notes after voicing tools (bass, spread)

struct ChordDef
{
    const char* name;
    int count;
    int notes[kMaxStored];
};

struct ChordSet
{
    int number;            // 1-based, as printed on the panel
    const char* genre;
    ChordDef chords[kKeys];
};

// The factory sets, numbered 1..kNumSets in order.
const std::vector<ChordSet>& chordSets();
int numChordSets();
const ChordSet& chordSet (int index);   // 0-based, clamped

// Genre tags for the browser: the genre split on '/', e.g. "Gospel/R&B" ->
// {"Gospel", "R&B"}. Words like "Min" or "2" stay part of their tag.
std::vector<std::string> genreTags (const ChordSet&);

// ---------------------------------------------------------------------------
// Voicing
// ---------------------------------------------------------------------------
struct Voicing
{
    int count = 0;
    std::array<int, kMaxVoice> notes {};   // ascending, unique, 0..127

    bool operator== (const Voicing& o) const noexcept
    {
        if (count != o.count) return false;
        for (int i = 0; i < count; ++i)
            if (notes[(size_t) i] != o.notes[(size_t) i]) return false;
        return true;
    }
    bool operator!= (const Voicing& o) const noexcept { return ! (*this == o); }
    bool contains (int n) const noexcept
    {
        for (int i = 0; i < count; ++i)
            if (notes[(size_t) i] == n) return true;
        return false;
    }
    int lowest() const noexcept  { return count > 0 ? notes[0] : -1; }
    int highest() const noexcept { return count > 0 ? notes[(size_t) count - 1] : -1; }
};

enum class Spread { close = 0, open, wide };

struct VoiceSettings
{
    int transpose = 0;     // semitones, -12..12 (KEY)
    int octave    = 0;     // octaves, -3..3, plus the played key's own octave
    int invert    = 0;     // -4..4: + lifts the lowest note an octave, - drops the highest
    Spread spread = Spread::close;
    bool bass     = false; // double the lowest note an octave below
};

// The stored voicing with the voice controls applied. `extraOctaves` is the
// octave the key was played in (from a MIDI keyboard), added to `octave`.
Voicing voice (const ChordDef&, const VoiceSettings&, int extraOctaves = 0);

// A chord symbol for a voicing you played: "Cm9", "F/A", "G13sus4". The
// lowest note is the bass; if it isn't the chord's root the name gets a slash.
// Notes it can't name come back as their note names joined by spaces.
std::string nameChord (const int* notes, int count);

// Note names the way Ableton prints them: MIDI 60 = C3.
std::string noteName (int midiNote);
const char* keyName (int key);   // "C", "C#", ...
} // namespace posy
