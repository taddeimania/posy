// Banks.h — your own chord banks: twelve chords you played in, one per pad,
// saved as files and listed in the browser under MY SETS.
//
//   macOS:   ~/Library/Application Support/Posy/Banks/*.posybank
//   Windows: %APPDATA%\Posy\Banks
//   Linux:   ~/.config/Posy/Banks
//
// A bank file is small, readable XML:
//   <PosyBank name="Late Night" version="1">
//     <Chord key="0" name="Cm9" notes="48,51,58,62"/>
//     <Chord key="1" name="Gospel lift" notes="49,56,60,65" named="1"/>
//     ...
//   </PosyBank>
// The bank the plugin is using is also saved inside the Live set, so a set
// still plays even if the file is renamed or moved.
//
// Message thread only.
#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <array>

#include "Engine/Chords.h"

namespace posy
{
struct CustomChord
{
    juce::String name;
    juce::Array<int> notes;     // ascending MIDI notes, at most kMaxStored
    bool named = false;         // the name was typed, so editing the notes keeps it
};

struct CustomBank
{
    juce::String name;
    std::array<CustomChord, kKeys> chords;
    juce::File file;            // where it was loaded from / saved to; empty if never saved

    bool isEmpty() const;
    juce::ValueTree toTree() const;
    static CustomBank fromTree (const juce::ValueTree&);
};

namespace banks
{
    inline const juce::Identifier tag { "PosyBank" };
    inline constexpr const char* extension = ".posybank";

    juce::File folder();
    juce::Array<juce::File> list();                       // sorted by name
    bool load (const juce::File&, CustomBank& out);
    bool save (const CustomBank&, const juce::File&);
    juce::File fileFor (const juce::String& name);        // folder/name.posybank
    juce::String sanitise (const juce::String& name);
}
} // namespace posy
