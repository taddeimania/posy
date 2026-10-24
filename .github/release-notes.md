Posy is a chord generator for Ableton Live: twelve chords per set, one per key,
sent out as MIDI. Straight, strummed, held or arpeggiated.

## What's new in 0.2.0

**58 new chord sets (101–158), for 158 in all.**
- Pop and rock: Royal Road, Anthem, Ballad, Pop Minor, Dream Pop, Rock
  Mixolydian, Open Strings, Blues Shuffle, Metal.
- Jazz: rootless piano voicings (major and minor), quartal "So What" stacks,
  twelve dominant colours, minor ii–V, Coltrane changes, Jobim bossa, jazz blues.
- Soul and R&B: Gospel, Gospel Minor, Lofi Hip Hop, 90s R&B, Neo Soul 4-3-6-5,
  Motown, Funk, Afro Highlife, Afro Dorian.
- House and techno: Deep House, UK Garage, Trance Epic, Future Bass, Detroit
  chord memory, dub stabs.
- Synth, film and ambient: Darkwave, 80s synth-pop, Film Hope, Film Tension,
  Trailer, Horror, Ambient Lydian, Shoegaze, Suspended.
- Game, world, modal and traditional: Overworld, Town, Flamenco, Celtic,
  Klezmer, Arabic Hijaz, Samba/MPB, the six modes, Baroque, Romantic,
  Barbershop, Country.

In the new sets every pad's lowest note is its own key, so the C pad plays
something over C. Only Techno Dub leaves the bass out, for a bassline to fill.

**New genre buttons:** ROCK, AMBIENT, GAME, WORLD and MODAL.

**Getting started guide:** a PDF that wires Posy to an instrument in Live and
shows you round the panel. It's in every download.

**If you automated the chord set in 0.1.0:** the Chord Set parameter now spans
158 sets instead of 100, so automation drawn in 0.1.0 lands on different sets.
Redraw it after upgrading.

## Install

Unzip and copy `Posy.vst3` into your VST3 folder (macOS
`~/Library/Audio/Plug-Ins/VST3`, Windows `C:\Program Files\Common Files\VST3`,
Linux `~/.vst3`), then rescan plug-ins in Live. The guide and the README cover
routing Posy into an instrument.

**macOS:** the plugin isn't notarised. If macOS refuses to load it, run
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Posy.vst3`.
