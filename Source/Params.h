// Params.h — Posy's host parameters. Everything on the panel is automatable,
// including which chord set is loaded, so a set change can live in an
// arrangement.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine/Player.h"

namespace posy::ids
{
inline constexpr const char* chordSet  = "chordSet";
inline constexpr const char* key       = "key";
inline constexpr const char* octave    = "octave";
inline constexpr const char* invert    = "invert";
inline constexpr const char* spread    = "spread";
inline constexpr const char* bass      = "bass";
inline constexpr const char* strum     = "strum";
inline constexpr const char* strumDown = "strumDown";
inline constexpr const char* velocity  = "velocity";
inline constexpr const char* fixedVel  = "fixedVel";
inline constexpr const char* legato    = "legato";
inline constexpr const char* followOct = "followOct";
inline constexpr const char* hold      = "hold";
inline constexpr const char* arp       = "arp";
inline constexpr const char* arpMode   = "arpMode";
inline constexpr const char* arpRate   = "arpRate";
inline constexpr const char* arpOct    = "arpOct";
inline constexpr const char* arpGate   = "arpGate";
inline constexpr const char* channel   = "channel";

inline juce::String keyText (int v)
{
    if (v == 0)
        return "C (0)";
    return juce::String (posy::keyName (v)) + " (" + (v > 0 ? "+" : "") + juce::String (v) + ")";
}

inline juce::String signedText (int v) { return v > 0 ? "+" + juce::String (v) : juce::String (v); }

inline const juce::StringArray& spreadNames()
{
    static const juce::StringArray n { "Close", "Open", "Wide" };
    return n;
}

inline juce::StringArray arpModeNames()
{
    juce::StringArray n;
    for (auto* s : kArpModeNames) n.add (s);
    return n;
}

inline juce::StringArray arpRateNames()
{
    juce::StringArray n;
    for (auto* s : kArpRateNames) n.add (s);
    return n;
}
} // namespace posy::ids

namespace posy
{
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto text = [] (std::function<String (int)> f)
    {
        return AudioParameterIntAttributes().withStringFromValueFunction ([f] (int v, int) { return f (v); });
    };

    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::chordSet, 1 }, "Chord Set",
        1, numChordSets(), 1, text ([] (int v)
        {
            return String (v).paddedLeft ('0', 3) + " " + String (chordSet (v - 1).genre);
        })));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::key, 1 }, "Key",
        -12, 12, 0, text (ids::keyText)));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::octave, 1 }, "Octave",
        -3, 3, 0, text (ids::signedText)));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::invert, 1 }, "Invert",
        -4, 4, 0, text (ids::signedText)));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::spread, 1 }, "Spread",
        ids::spreadNames(), 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::bass, 1 }, "Bass", false));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::strum, 1 }, "Strum",
        0, 200, 0, text ([] (int v) { return v == 0 ? String ("Off") : String (v) + " ms"; })));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::strumDown, 1 }, "Strum Down", false));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::velocity, 1 }, "Velocity", 1, 127, 100));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::fixedVel, 1 }, "Fixed Velocity", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::legato, 1 }, "Legato", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::followOct, 1 }, "Keyboard Octave", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::hold, 1 }, "Hold", false));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::arp, 1 }, "Arp", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::arpMode, 1 }, "Arp Mode",
        ids::arpModeNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::arpRate, 1 }, "Arp Rate",
        ids::arpRateNames(), kArpRateDefault));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::arpOct, 1 }, "Arp Octaves", 1, 4, 1));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::arpGate, 1 }, "Arp Gate",
        5, 100, 60, text ([] (int v) { return String (v) + "%"; })));

    layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::channel, 1 }, "MIDI Channel", 1, 16, 1));
    return layout;
}
} // namespace posy
