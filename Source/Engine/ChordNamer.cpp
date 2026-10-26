// ChordNamer.cpp — names a played voicing.
//
// Every pitch class in the chord is tried as the root. A root fits a template
// when the chord has all the template's required intervals and nothing outside
// its required + optional ones (a plain fifth and, in 11th and 13th chords, the
// ninth may be left out). The lowest note is preferred as the root, then the
// simplest template; a root other than the bass becomes a slash chord.
#include "Chords.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace posy
{
namespace
{
struct Template
{
    const char* suffix;
    uint16_t required;   // bit n: interval of n semitones above the root
    uint16_t optional;
};

constexpr uint16_t bits (std::initializer_list<int> ivs)
{
    uint16_t b = 0;
    for (int i : ivs) b = (uint16_t) (b | (1u << i));
    return b;
}

// Simplest first: when two templates fit, the earlier one names the chord.
const Template kTemplates[] = {
    { "",       bits ({ 0, 4, 7 }),        0 },
    { "m",      bits ({ 0, 3, 7 }),        0 },
    { "5",      bits ({ 0, 7 }),           0 },
    { "dim",    bits ({ 0, 3, 6 }),        0 },
    { "aug",    bits ({ 0, 4, 8 }),        0 },
    { "sus4",   bits ({ 0, 5, 7 }),        0 },
    { "sus2",   bits ({ 0, 2, 7 }),        0 },
    { "7",      bits ({ 0, 4, 10 }),       bits ({ 7 }) },
    { "M7",     bits ({ 0, 4, 11 }),       bits ({ 7 }) },
    { "m7",     bits ({ 0, 3, 10 }),       bits ({ 7 }) },
    { "m7b5",   bits ({ 0, 3, 6, 10 }),    0 },
    { "dim7",   bits ({ 0, 3, 6, 9 }),     0 },
    { "mM7",    bits ({ 0, 3, 11 }),       bits ({ 7 }) },
    { "6",      bits ({ 0, 4, 9 }),        bits ({ 7 }) },
    { "m6",     bits ({ 0, 3, 9 }),        bits ({ 7 }) },
    { "7sus4",  bits ({ 0, 5, 10 }),       bits ({ 7 }) },
    { "add9",   bits ({ 0, 2, 4 }),        bits ({ 7 }) },
    { "madd9",  bits ({ 0, 2, 3 }),        bits ({ 7 }) },
    { "M7#5",   bits ({ 0, 4, 8, 11 }),    0 },
    { "7#5",    bits ({ 0, 4, 8, 10 }),    0 },
    { "9",      bits ({ 0, 2, 4, 10 }),    bits ({ 7 }) },
    { "M9",     bits ({ 0, 2, 4, 11 }),    bits ({ 7 }) },
    { "m9",     bits ({ 0, 2, 3, 10 }),    bits ({ 7 }) },
    { "69",     bits ({ 0, 2, 4, 9 }),     bits ({ 7 }) },
    { "m69",    bits ({ 0, 2, 3, 9 }),     bits ({ 7 }) },
    { "7b9",    bits ({ 0, 1, 4, 10 }),    bits ({ 7 }) },
    { "7#9",    bits ({ 0, 3, 4, 10 }),    bits ({ 7 }) },
    { "9sus4",  bits ({ 0, 2, 5, 10 }),    bits ({ 7 }) },
    { "mM9",    bits ({ 0, 2, 3, 11 }),    bits ({ 7 }) },
    { "m11",    bits ({ 0, 3, 5, 10 }),    bits ({ 2, 7 }) },
    { "M7#11",  bits ({ 0, 4, 6, 11 }),    bits ({ 7 }) },
    { "7#11",   bits ({ 0, 4, 6, 10 }),    bits ({ 7 }) },
    { "M9#11",  bits ({ 0, 2, 4, 6, 11 }), bits ({ 7 }) },
    { "9#11",   bits ({ 0, 2, 4, 6, 10 }), bits ({ 7 }) },
    { "13",     bits ({ 0, 4, 9, 10 }),    bits ({ 2, 7 }) },
    { "M13",    bits ({ 0, 4, 9, 11 }),    bits ({ 2, 7 }) },
    { "13sus4", bits ({ 0, 5, 9, 10 }),    bits ({ 2, 7 }) },
    { "m13",    bits ({ 0, 3, 9, 10 }),    bits ({ 2, 5, 7 }) },
    { "7b13",   bits ({ 0, 4, 8, 10 }),    bits ({ 7 }) },
    { "13b9",   bits ({ 0, 1, 4, 9, 10 }), bits ({ 7 }) },
    { "7b9b13", bits ({ 0, 1, 4, 8, 10 }), 0 },
    { "7alt",   bits ({ 0, 1, 3, 4, 8, 10 }), bits ({ 6 }) },
    { "add11",  bits ({ 0, 4, 5 }),        bits ({ 7 }) },
    { "madd11", bits ({ 0, 3, 5 }),        bits ({ 7 }) },
    { "M7sus4", bits ({ 0, 5, 11 }),       bits ({ 7 }) },
    { "M7b5",   bits ({ 0, 4, 6, 11 }),    0 },
    { "7b5",    bits ({ 0, 4, 6, 10 }),    0 },
    { "M9(no3)",  bits ({ 0, 2, 11 }),     bits ({ 7 }) },
    { "M13(no3)", bits ({ 0, 2, 9, 11 }),  bits ({ 7 }) },
    { "(no5)",    bits ({ 0, 4 }),         0 },
    { "m(no5)",   bits ({ 0, 3 }),         0 },
};

const char* const kRootNames[12] { "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };

uint16_t rotate (uint16_t pcs, int root)
{
    uint16_t out = 0;
    for (int p = 0; p < 12; ++p)
        if (pcs & (1u << p))
            out = (uint16_t) (out | (1u << ((p - root + 12) % 12)));
    return out;
}
} // namespace

std::string nameChord (const int* notes, int count)
{
    if (notes == nullptr || count <= 0)
        return {};

    int bassNote = 127;
    uint16_t pcs = 0;
    for (int i = 0; i < count; ++i)
    {
        const int n = std::clamp (notes[i], 0, 127);
        bassNote = std::min (bassNote, n);
        pcs = (uint16_t) (pcs | (1u << (n % 12)));
    }
    const int bass = bassNote % 12;

    // One pitch class: a single note, or the same note in octaves.
    if ((pcs & (pcs - 1)) == 0)
        return std::string (kRootNames[bass]) + (count > 1 ? " oct" : " note");

    int bestRoot = -1, bestTemplate = -1, bestScore = 1 << 30;
    const int numTemplates = (int) (sizeof (kTemplates) / sizeof (kTemplates[0]));
    for (int root = 0; root < 12; ++root)
    {
        if (! (pcs & (1u << root)))
            continue;
        const uint16_t rel = rotate (pcs, root);
        for (int t = 0; t < numTemplates; ++t)
        {
            const auto& tp = kTemplates[t];
            const bool hasRequired = (rel & tp.required) == tp.required;
            const bool nothingElse = (rel & ~(tp.required | tp.optional)) == 0;
            if (! hasRequired || ! nothingElse)
                continue;
            const int score = (root == bass ? 0 : 1000) + t;
            if (score < bestScore)
            {
                bestScore = score;
                bestRoot = root;
                bestTemplate = t;
            }
            break;   // the first template that fits this root is its simplest name
        }
    }

    if (bestRoot < 0)
    {
        // Nothing fits: list the notes, bass first.
        std::string out = kRootNames[bass];
        for (int p = 1; p < 12; ++p)
        {
            const int pc = (bass + p) % 12;
            if (pcs & (1u << pc))
                out += std::string (" ") + kRootNames[pc];
        }
        return out;
    }

    std::string name = std::string (kRootNames[bestRoot]) + kTemplates[bestTemplate].suffix;
    if (bestRoot != bass)
        name += std::string ("/") + kRootNames[bass];
    return name;
}
} // namespace posy
