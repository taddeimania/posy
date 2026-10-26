# Posy

A chord generator for Ableton Live. Posy plays one chord at a time from a set
of twelve, one per key of the octave, and sends it out as MIDI: straight, strummed,
held, or arpeggiated. Put any instrument after it and play pads, keys or plucks
with one finger.

It ships with **158 chord sets**, each a palette of twelve chords that belong
together: pop, rock, jazz, neo soul, gospel, funk, house, techno, synthwave,
film, ambient, game music, flamenco, Celtic, the modes, baroque and more. Find
one in seconds with the search box, keep favourites with a star, and drag what
you played straight into a clip.

Posy makes no sound itself. It's MIDI only.

![Posy's panel](docs/posy.png)

New to Posy? The [Getting started guide](docs/Posy-Getting-Started.pdf) (PDF) wires
it to an instrument in Live in about five minutes and shows you round the panel.

## What's on the panel

**SETS** — every chord set, searchable.
- Type to filter. Words match genres (`house`, `bossa`), chord names (`Am7`,
  `F#m9`, `/G`, `sus4`) and set numbers (`42`). Every word must match, so
  `Dm7 G7 jazz` finds jazz sets containing both chords. Flats and sharps are
  the same thing (`Bb` finds `A#`), and `maj7` finds `M7`.
- Up / Down step through the results while you type; Esc clears.
- Genre buttons narrow the list; **STARRED** shows the sets you've starred
  (click the star on a row), **MY SETS** your own banks. Stars are remembered
  for every Posy on the machine.
- `<` and `>` in the header step through the list as filtered.

Sets 1–100 are the original palettes. Sets 101–158 were added in 0.2.0, and
in them every pad's lowest note is its own key (the root, or the bass of a
slash chord), so the C pad plays something over C and the F# pad something
over F#. Only Techno Dub breaks the rule: its stabs leave the bass to a
bassline. The table shows each set's white-key chords (C D E F G A):

| Set | Genre | C D E F G A |
|---|---|---|
| 101 | Pop Royal Road | CM9 Dm9 Em7 FM7 G13 Am9 ... |
| 102 | Pop Anthem | C Dm Em F G Am ... |
| 103 | Pop Ballad | Cadd9 Dm7 C/E Fadd9 Gsus4 Am7 ... |
| 104 | Pop Minor | Cadd9 Dm7 E7 FM7 Gadd9 Am9 ... |
| 105 | Dream Pop | CM9 D9 Em9 FM7#11 G69 Am9 ... |
| 106 | Rock Mixolydian | C Dsus4 E F G Am ... |
| 107 | Rock Open Strings | Cadd9 Dsus2 Em7 FM7#11 G6 Asus4 ... |
| 108 | Rock Blues Shuffle | C7 D9 E7#9 F9 G7 A7 ... |
| 109 | Rock Metal | C5 Ddim Edim F5 G5 Adim ... |
| 110 | Jazz Rootless | CM9 Dm9 E7b9 FM9 G13 Am9 ... |
| 111 | Jazz Rootless Minor | Cm69 Dm7b5 Edim7 Fm9 G7alt Am7b5 ... |
| 112 | Jazz Quartal | C69 Dm11 Em11 FM7#11 G9sus4 Am11 ... |
| 113 | Jazz Dominants | C13 D7b9 E7#9 F9 G13b9 A7b13 ... |
| 114 | Jazz Minor ii-V | Cm69 Dm7b5 Em7b5 Fm9 G7b9 Am7b5 ... |
| 115 | Jazz Coltrane | CM9 D13 EM9 Fm9 GM9 Am9 ... |
| 116 | Jazz Bossa Jobim | CM9 Dm9 E7b9 Fm6 G7b13 Am9 ... |
| 117 | Jazz Blues | C9 D7b9 Edim7 F13 Gm9 Am7 ... |
| 118 | Gospel | CM9 D9 E7#9 F69 C/G A7#9 ... |
| 119 | Gospel Minor | Cm9 Dm7b5 Edim7 Fm11 G7#9 Am7b5 ... |
| 120 | Lofi Hip Hop | CM9 Dm9 E7#9 FM9 G13 Am11 ... |
| 121 | R&B 90s | CM9 Dm9 Em9 FM9 G13sus4 Am9 ... |
| 122 | Neo Soul 4-3-6-5 | C13 Dm9 E7#9 FM9 Gm9 Am9 ... |
| 123 | Soul Motown | C6 Dm7 E7 F6 G9 Am7 ... |
| 124 | Funk | C9 D9 E7#9 F9 G13 A13 ... |
| 125 | Afro Highlife | C6 Dm7 Em7 F6 G7 Am7 ... |
| 126 | Afro Dorian | Cm9 Dm7 Em7b5 F13 Gm9 Am7b5 ... |
| 127 | House Deep | Cm9 Dm11 Em11 Fm9 Gm9 Am11 ... |
| 128 | House UK Garage | Cm9 D9sus4 E7#9 Fm11 G7b9 Am7b5 ... |
| 129 | Trance Epic | Cadd9 Dmadd9 Esus4 Fadd9 Gadd9 Amadd9 ... |
| 130 | EDM Future Bass | CM9 D9sus4 Em9 FM9 G69 Am9 ... |
| 131 | Techno Chord Memory | Cm11 Dm11 Em11 Fm11 Gm11 Am11 ... |
| 132 | Techno Dub | Cm9 Dm9 Em9 Fm9 Gm9 Am9 ... |
| 133 | Synth Darkwave | Cmadd9 Dm7b5 Edim Fm9 Gm7b5 Adim ... |
| 134 | Synth Pop 80s | Csus2 Dm Em Fadd9 Gsus4 Am ... |
| 135 | Film Hope | Cadd9 Dadd9 E FM7#11 Gsus4 Am9 ... |
| 136 | Film Tension | CmM9 Dm7b5 Emadd9 Fm6 G7b13 Am6 ... |
| 137 | Cinematic Trailer | Cmadd9 Dm7b5 Eaug Fm Gsus4 Am7b5 ... |
| 138 | Film Horror | CmM7 Dm7b5 Edim FmM7 G7b9 Aaug ... |
| 139 | Ambient Lydian | CM9#11 D69 Em11 FM9#11 G69 Am11 ... |
| 140 | Shoegaze Dream | CM7#11 D69 Eadd9 FM7#11 G6 Aadd9 ... |
| 141 | Ambient Suspended | Csus2 D9sus4 Esus4 Fsus2 G9sus4 Asus4 ... |
| 142 | Game Overworld | Cadd9 Dadd9 Em7 Fadd9 Gsus4 Am7 ... |
| 143 | Game Town | CM7 Dm9 Em7 FM7 G9sus4 Am7 ... |
| 144 | Flamenco World | C Dm E7b9 FM7 G Am ... |
| 145 | Celtic World | Cadd9 Dsus2 Em7 F Gadd9 Am7 ... |
| 146 | Klezmer World | Cm D7b9 Em7b5 F Gm6 A7 ... |
| 147 | Arabic Hijaz World | C Dm7b5 Em FmM7 G7b9 Am ... |
| 148 | Samba MPB World | C69 Dm6 E7b13 F69 G7b13 A7b13 ... |
| 149 | Modal Dorian | Cm9 Dm7 Edim7 F13 Gm9 Am7b5 ... |
| 150 | Modal Lydian | CM9#11 D13 Em9 FM7 GM9 Am9 ... |
| 151 | Modal Phrygian Dominant | C7b9 Dm7b5 Edim7 FmM7 Gm7b5 A7b9 ... |
| 152 | Modal Harmonic Minor | CmM9 Dm7b5 Edim7 Fm9 G7b9 Am7b5 ... |
| 153 | Modal Melodic Minor | CmM9 Dm11 Edim7 F7#11 G7b13 Am7b5 ... |
| 154 | Modal Mixolydian | C13 Dm9 Em7b5 FM9 Gm9 Am7 ... |
| 155 | Baroque Trad | C Dm Em F G7 Am ... |
| 156 | Romantic Trad | Cm Dm7b5 Edim7 Fm6 G7 Am7b5 ... |
| 157 | Barbershop Trad | C6 D7 E7 F6 G7 A7 ... |
| 158 | Country Trad | C D7 C/E F G7 Am ... |

**CHORDS** — the twelve chords of the set, laid out like an octave of a
keyboard with the sharps above. Click to play, drag across to move between
chords. Each pad shows the chord name (transposed with KEY) and the notes it
will send. Higher on a pad plays louder, unless FIXED velocity is on.

A MIDI keyboard on Posy's track plays the same chords: C plays the C pad, C#
the C# pad, and so on in every octave. With **KBD OCT** on, the octave you play
in moves the chord (C3 is home); off, every octave plays the same chord.
Mod wheel, pitch bend, sustain and other controllers pass straight through.

Keys stack: the last one pressed plays, and letting it go returns to the one
you're still holding.

**MY SETS — your own chord banks.** Play your own voicings onto the pads and
save them as a bank.
1. Press **EDIT** (above the E and F pads). On a factory set, Posy starts a
   bank from a copy of it; **NEW** starts an empty one.
2. Click a pad, play a chord on your MIDI keyboard and let go. You hear it as
   you play (the notes go straight through to your instrument); when you let
   go it lands on the pad, gets a name (`CM9`, `A/Db`, or its notes if it isn't
   a standard chord) and the next pad is selected, so you can play twelve
   chords in a row. The most notes you held at once is what's kept, so a
   rolled chord or a corrected wrong note comes out right.
3. Fix a voicing by clicking keys on the OUT keyboard: the selected pad's
   notes are marked, and a click adds or removes one. Double-click a pad to
   rename it; right-click to rename it, name it from its notes again, or
   clear it.
4. **SAVE** or **SAVE AS** puts the bank in MY SETS. Right-click a bank in the
   list to rename it, show its file or delete it.

A bank plays like any set: KEY, the voicing controls, the arp, HOLD and TAKE
all work on it. The bank you're using is saved in the Live set too, so a
project keeps playing even if you rename or delete the bank's file. Loading
something else while a bank has unsaved edits asks first.

**OUT** — every note going out right now. In EDIT mode, click a key to add it
to the selected pad or take it away.

**TAKE** — the chords you've played, in order (playing the same chord twice in
a row counts once). **Drag the strip onto a clip slot in Live** to drop them in
as a MIDI clip, one chord per LENGTH (½ bar, 1 bar or 2 bars). Right-click a
chord to remove it; CLEAR empties the take; REC stops collecting. The take is
saved with the Live set.

**VOICE** — shape every chord:
- **KEY** transposes by semitones, **OCTAVE** by octaves.
- **INVERT** rotates the voicing: right lifts the lowest note an octave, left
  drops the highest.
- **SPREAD**: Close (as stored), Open (second note up an octave), Wide (every
  other note up an octave).
- **BASS** doubles the lowest note an octave below.
- **LEGATO** keeps notes two chords share ringing instead of restriking them.
- **STRUM** spreads a chord's notes out in time (up to 200 ms); **DOWN** strums
  from the top.
- **VELOCITY** with **FIXED**: every chord at one velocity, or the keyboard's
  own velocity.

Turning these while a chord is playing re-voices it on the spot.

**ARPEGGIO** — **ARP** plays the chord one note at a time.
- **MODE**: Up, Down, Up & Down, Down & Up, Up & Down + (repeats the top and
  bottom), Converge (outside in), Random, and Chord (restrikes the whole chord
  every step, for rhythmic stabs).
- **RATE**: 1/1 to 1/32, with triplets.
- **OCTAVES**: climb 1–4 octaves. **GATE**: how long each note lasts.
- While Live is playing the arp locks to its grid (SYNCED). When Live is
  stopped it free-runs at Live's tempo (FREE), so you can still jam.

**HOLD** — latch the chord: it keeps playing after you let go, until you play
another or turn HOLD off. Works with the arp too.

**ALL OFF** stops every note and lets go of a held chord.

Every knob and button is a host parameter, including the chord set, so you can
automate set changes in an arrangement.

## Using it in Live

1. Put **Posy** on a MIDI track (it's under Plug-Ins as an instrument).
2. On a second MIDI track with your instrument, set **MIDI From** to the Posy
   track, choose **Posy** in the second menu, and set Monitor to **In**.
3. Play the pads or arm Posy's track and play your keyboard.

To capture chords as a clip, either drag the TAKE strip onto a clip slot, or
arm the instrument track and record.

## Building

You need CMake 3.22+ and a C++20 compiler (Xcode on macOS, Visual Studio 2022
on Windows). JUCE 8.0.9 is downloaded on first configure.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release
```

On macOS and Linux the plugin is copied into your VST3 folder after each build
(`~/Library/Audio/Plug-Ins/VST3`, `~/.vst3`); turn that off with
`-DPOSY_COPY_PLUGIN=OFF`. Use a local JUCE checkout with
`-DPOSY_JUCE_DIR=/path/to/JUCE`. For a universal Mac binary add
`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

`-DPOSY_BUILD_TOOLS=ON` also builds `PosySnapshot`, which renders the panel to
a PNG without a host. The screenshot above is
`PosySnapshot docs/posy.png 72 0` (set 72, C pad pressed).

### Releases

`.github/workflows/build.yml` builds and tests macOS, Windows and Linux when you
run it from the Actions tab. Run it on a `v*` tag matching
`project(Posy VERSION ...)` in `CMakeLists.txt` and it creates a draft release
with the three archives attached.

## Where things are saved

Settings (tooltips, starred sets):
- macOS: `~/Library/Application Support/Posy/Posy.settings`
- Windows: `%APPDATA%\Posy\Posy.settings`
- Linux: `~/.config/Posy/Posy.settings`

Your banks, one file each (`.posybank`, readable XML):
- macOS: `~/Library/Application Support/Posy/Banks`
- Windows: `%APPDATA%\Posy\Banks`
- Linux: `~/.config/Posy/Banks`

Everything else (the set or bank in use, the knobs, the take) is saved with the
Live set.

## Licence

GNU Affero General Public License v3.0. See [LICENSE](LICENSE).
