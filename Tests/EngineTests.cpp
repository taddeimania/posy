// Engine tests: the chord table, the voicing tools and the player. Plain C++,
// no JUCE. Run with ctest, or the PosyEngineTests binary directly.
#include "Engine/Chords.h"
#include "Engine/Player.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace posy;

namespace
{
int failures = 0;

#define CHECK(cond) \
    do { if (! (cond)) { std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

KeyEvent pad (int key, bool on, int offset = 0, int velocity = 100)
{
    KeyEvent e;
    e.offset = offset; e.on = on; e.source = kPadSource + key; e.key = key; e.velocity = velocity;
    return e;
}

BlockInfo block (int n, bool playing = false, double ppq = 0.0)
{
    BlockInfo b;
    b.numSamples = n; b.sampleRate = 48000.0; b.bpm = 120.0; b.playing = playing; b.ppq = ppq;
    return b;
}

std::vector<NoteOut> run (Player& p, const PlayerSettings& s, std::vector<KeyEvent> ev, int n = 512,
                          bool playing = false, double ppq = 0.0)
{
    std::vector<NoteOut> out;
    p.process (block (n, playing, ppq), s, ev.data(), (int) ev.size(), out);
    return out;
}

int count (const std::vector<NoteOut>& v, bool on)
{
    int c = 0;
    for (auto& e : v) c += e.on == on;
    return c;
}

// Tracks note state across blocks to catch stuck or doubled notes.
struct Ledger
{
    std::array<int, 128> on {};
    bool bad = false;
    void add (const std::vector<NoteOut>& v)
    {
        for (auto& e : v)
        {
            if (e.on) { if (on[(size_t) e.note]) bad = true; on[(size_t) e.note] = 1; }
            else      { on[(size_t) e.note] = 0; }
        }
    }
    int sounding() const { int c = 0; for (int x : on) c += x; return c; }
};

void testTable()
{
    CHECK (numChordSets() == 158);
    for (int i = 0; i < numChordSets(); ++i)
    {
        const auto& s = chordSet (i);
        CHECK (s.number == i + 1);
        CHECK (s.genre != nullptr && s.genre[0] != '\0');
        for (auto& c : s.chords)
        {
            CHECK (c.name != nullptr && c.name[0] != '\0');
            CHECK (c.count >= 2 && c.count <= kMaxStored);
            for (int k = 1; k < c.count; ++k)
                CHECK (c.notes[k] >= c.notes[k - 1]);   // one factory voicing doubles a note
        }
    }
    // Spot checks against the table.
    CHECK (std::string (chordSet (0).chords[0].name) == "Cadd9");
    CHECK (chordSet (0).chords[0].notes[0] == 48);
    CHECK (std::string (chordSet (54).genre) == "Jazz House");
    CHECK (std::string (chordSet (54).chords[11].name) == "CM7#5/G#");

    // The second bank: every chord's lowest note is its pad's key, apart from
    // Techno Dub, whose stabs leave the bass to a bassline.
    CHECK (std::string (chordSet (100).genre) == "Pop Royal Road");
    CHECK (std::string (chordSet (100).chords[0].name) == "CM9");
    for (int i = 100; i < numChordSets(); ++i)
    {
        const auto& s = chordSet (i);
        if (std::string (s.genre) == "Techno Dub")
            continue;
        for (int k = 0; k < kKeys; ++k)
            CHECK (s.chords[k].notes[0] % 12 == k);
    }

    const auto tags = genreTags (chordSet (64));   // "Gospel/R&B"
    CHECK (tags.size() == 2 && tags[0] == "Gospel" && tags[1] == "R&B");
}

void testVoicing()
{
    const ChordDef cadd9 { "Cadd9", 4, { 48, 55, 62, 64 } };
    VoiceSettings vs;
    auto v = voice (cadd9, vs);
    CHECK (v.count == 4 && v.notes[0] == 48 && v.notes[3] == 64);

    vs.invert = 1;
    v = voice (cadd9, vs);
    CHECK (v.count == 4 && v.notes[0] == 55 && v.notes[3] == 64 && v.contains (60));

    vs.invert = -1;
    v = voice (cadd9, vs);
    CHECK (v.notes[0] == 48 && v.contains (52) && v.highest() == 62);

    vs = {};
    vs.transpose = 2; vs.octave = 1;
    v = voice (cadd9, vs);
    CHECK (v.notes[0] == 62);

    vs = {};
    vs.bass = true;
    v = voice (cadd9, vs);
    CHECK (v.count == 5 && v.notes[0] == 36);

    vs = {};
    vs.spread = Spread::open;
    v = voice (cadd9, vs);
    CHECK (v.count == 4 && v.notes[0] == 48 && v.contains (67));

    // Out-of-range notes fold back in.
    vs = {};
    vs.octave = 7;
    v = voice (cadd9, vs);
    for (int i = 0; i < v.count; ++i)
        CHECK (v.notes[(size_t) i] >= 0 && v.notes[(size_t) i] <= 127);

    CHECK (noteName (60) == "C3");
}

void testPressRelease()
{
    Player p;
    PlayerSettings s;
    auto out = run (p, s, { pad (0, true) });
    CHECK (count (out, true) == 4);
    CHECK (p.currentKey() == 0);
    out = run (p, s, { pad (0, false, 10) });
    CHECK (count (out, false) == 4);
    CHECK (out[0].offset == 10);
    CHECK (p.currentKey() == -1);
}

void testHold()
{
    Player p;
    PlayerSettings s;
    s.hold = true;
    Ledger l;
    l.add (run (p, s, { pad (0, true) }));
    l.add (run (p, s, { pad (0, false) }));
    CHECK (l.sounding() == 4);                 // still ringing
    l.add (run (p, s, { pad (2, true), pad (2, false, 5) }));
    CHECK (p.currentKey() == 2);
    CHECK (l.sounding() == 4);
    s.hold = false;
    l.add (run (p, s, {}));
    CHECK (l.sounding() == 0);
    CHECK (! l.bad);
}

void testFallback()
{
    Player p;
    PlayerSettings s;
    run (p, s, { pad (0, true), pad (5, true, 4) });
    CHECK (p.currentKey() == 5);
    run (p, s, { pad (5, false) });
    CHECK (p.currentKey() == 0);               // back to the key still held
    run (p, s, { pad (0, false) });
    CHECK (p.currentKey() == -1);
}

void testLegato()
{
    Player p;
    PlayerSettings s;
    s.legato = true;
    s.setIndex = 13;                           // octave stacks: C4+C5, C#4+C#5 ...
    run (p, s, { pad (0, true) });
    // Same set, a key a whole octave of stacks away shares nothing; use the
    // re-voice path instead: transpose by 12 keeps the upper note.
    s.voice.transpose = 12;
    auto out = run (p, s, {});
    CHECK (count (out, false) == 1 && count (out, true) == 1);
}

void testStrum()
{
    Player p;
    PlayerSettings s;
    s.strumMs = 10;
    auto out = run (p, s, { pad (0, true) }, 4096);
    CHECK (count (out, true) == 4);
    CHECK (out[0].offset == 0 && out[1].offset == 480 && out[3].offset == 1440);
    CHECK (out[0].note < out[3].note);
    // Released mid-strum: the notes not yet played never start.
    Player q;
    out = run (q, s, { pad (0, true), pad (0, false, 600) }, 4096);
    CHECK (count (out, true) == 2 && count (out, false) == 2);
}

void testArpFree()
{
    Player p;
    PlayerSettings s;
    s.arp = true;                              // 1/16 at 120 bpm, 48 kHz: 6000 samples
    Ledger l;
    auto out = run (p, s, { pad (0, true) }, 48000);
    l.add (out);
    CHECK (count (out, true) == 8);
    std::vector<int> ons;
    for (auto& e : out) if (e.on) ons.push_back (e.note);
    CHECK (ons[0] == 48 && ons[1] == 55 && ons[2] == 62 && ons[3] == 64 && ons[4] == 48);
    l.add (run (p, s, { pad (0, false) }, 48000));
    CHECK (l.sounding() == 0);
    CHECK (! l.bad);
}

void testArpSynced()
{
    Player p;
    PlayerSettings s;
    s.arp = true;
    // One beat at 120 bpm is 24000 samples: four sixteenths.
    auto out = run (p, s, { pad (0, true) }, 24000, true, 0.0);
    CHECK (count (out, true) == 4);
    int i = 0;
    for (auto& e : out)
        if (e.on) { CHECK (e.offset == i * 6000); ++i; }
    // Next beat carries on from the grid.
    out = run (p, s, {}, 24000, true, 1.0);
    CHECK (count (out, true) == 4);
    CHECK (out.front().offset == 0);
}

void testArpModes()
{
    for (int m = 0; m < kNumArpModes; ++m)
    {
        Player p;
        PlayerSettings s;
        s.arp = true;
        s.arpMode = (ArpMode) m;
        s.arpOctaves = 2;
        Ledger l;
        l.add (run (p, s, { pad (3, true) }, 96000));
        l.add (run (p, s, { pad (3, false) }, 96000));
        CHECK (l.sounding() == 0);
    }
}

void testFuzz()
{
    std::mt19937 rng (1234);
    Player p;
    Ledger l;
    PlayerSettings s;
    for (int b = 0; b < 4000; ++b)
    {
        if (rng() % 50 == 0) s.arp = ! s.arp;
        if (rng() % 70 == 0) s.hold = ! s.hold;
        if (rng() % 40 == 0) s.legato = ! s.legato;
        if (rng() % 30 == 0) s.voice.invert = (int) (rng() % 9) - 4;
        if (rng() % 30 == 0) s.setIndex = (int) (rng() % 100);
        if (rng() % 30 == 0) s.strumMs = (int) (rng() % 60);
        if (rng() % 30 == 0) s.arpMode = (ArpMode) (rng() % kNumArpModes);
        if (rng() % 30 == 0) s.arpRate = (int) (rng() % kNumArpRates);
        std::vector<KeyEvent> ev;
        const int n = (int) (rng() % 3);
        for (int i = 0; i < n; ++i)
        {
            KeyEvent e;
            e.offset = (int) (rng() % 256);
            e.on = rng() % 2;
            e.key = (int) (rng() % 12);
            e.source = (int) (rng() % 6);
            e.octave = (int) (rng() % 3) - 1;
            ev.push_back (e);
        }
        std::sort (ev.begin(), ev.end(), [] (auto& a, auto& c) { return a.offset < c.offset; });
        const bool playing = (b / 500) % 2 == 1;
        l.add (run (p, s, ev, 256, playing, b * 256 / 24000.0));
    }
    p.panic();
    l.add (run (p, s, {}));
    CHECK (l.sounding() == 0);
    CHECK (! l.bad);
}
} // namespace

int main()
{
    testTable();
    testVoicing();
    testPressRelease();
    testHold();
    testFallback();
    testLegato();
    testStrum();
    testArpFree();
    testArpSynced();
    testArpModes();
    testFuzz();
    if (failures == 0)
        std::printf ("all engine tests passed\n");
    return failures == 0 ? 0 : 1;
}
