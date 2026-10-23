#include "Chords.h"

#include <algorithm>

namespace posy
{
namespace
{
const ChordSet kSets[] = {
#include "ChordSets.inc"
};

const char* const kKeyNames[kKeys] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

void sortUnique (Voicing& v)
{
    std::sort (v.notes.begin(), v.notes.begin() + v.count);
    v.count = (int) (std::unique (v.notes.begin(), v.notes.begin() + v.count) - v.notes.begin());
}

void push (Voicing& v, int n)
{
    if (v.count < kMaxVoice)
        v.notes[(size_t) v.count++] = n;
}
} // namespace

const std::vector<ChordSet>& chordSets()
{
    static const std::vector<ChordSet> sets (std::begin (kSets), std::end (kSets));
    return sets;
}

int numChordSets() { return (int) chordSets().size(); }

const ChordSet& chordSet (int index)
{
    const auto& s = chordSets();
    return s[(size_t) std::clamp (index, 0, (int) s.size() - 1)];
}

std::vector<std::string> genreTags (const ChordSet& set)
{
    std::vector<std::string> out;
    std::string cur;
    for (const char* p = set.genre; ; ++p)
    {
        if (*p == '/' || *p == '\0')
        {
            while (! cur.empty() && cur.back() == ' ') cur.pop_back();
            size_t a = 0;
            while (a < cur.size() && cur[a] == ' ') ++a;
            if (a < cur.size())
                out.push_back (cur.substr (a));
            cur.clear();
            if (*p == '\0')
                break;
        }
        else
        {
            cur += *p;
        }
    }
    return out;
}

Voicing voice (const ChordDef& c, const VoiceSettings& s, int extraOctaves)
{
    Voicing v;
    const int shift = s.transpose + 12 * (s.octave + extraOctaves);
    for (int i = 0; i < c.count && i < kMaxStored; ++i)
        push (v, c.notes[i] + shift);
    sortUnique (v);
    if (v.count == 0)
        return v;

    // Inversions: rotate the voicing an octave at a time.
    for (int i = 0; i < s.invert; ++i)
    {
        v.notes[0] += 12;
        sortUnique (v);
    }
    for (int i = 0; i < -s.invert; ++i)
    {
        v.notes[(size_t) v.count - 1] -= 12;
        sortUnique (v);
    }

    // Spread: open lifts the second note from the bottom an octave; wide lifts
    // every other note above the bass.
    if (s.spread != Spread::close && v.count >= 3)
    {
        Voicing w = v;
        for (int i = 1; i < v.count; ++i)
        {
            const bool lift = s.spread == Spread::open ? (i == 1) : (i % 2 == 1);
            if (lift)
                w.notes[(size_t) i] += 12;
        }
        sortUnique (w);
        v = w;
    }

    if (s.bass)
    {
        push (v, v.notes[0] - 12);
        sortUnique (v);
    }

    // Keep everything playable: fold out-of-range notes back by octaves.
    for (int i = 0; i < v.count; ++i)
    {
        int& n = v.notes[(size_t) i];
        while (n < 0)   n += 12;
        while (n > 127) n -= 12;
    }
    sortUnique (v);
    return v;
}

std::string noteName (int n)
{
    n = std::clamp (n, 0, 127);
    return std::string (kKeyNames[n % 12]) + std::to_string (n / 12 - 2);
}

const char* keyName (int key)
{
    return kKeyNames[((key % kKeys) + kKeys) % kKeys];
}
} // namespace posy
