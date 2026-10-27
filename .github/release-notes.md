Posy is a chord generator for Ableton Live: twelve chords per set, one per key,
sent out as MIDI. Straight, strummed, held or arpeggiated.

## What's new in 0.3.2

**Fixed: dragging from a bottom-row pad up into the sharps.** In 0.3.1 a
vertical drag only changed velocity, so you couldn't glide from a white-key
pad up to a black-key pad any more. Now:
- While you stay on the held pad, its height sets the velocity: the top of
  the pad is loudest, the bottom softest, the same as where you click. The
  number and bar on the pad follow you, and the change goes out through
  **DRAG** (pressure, mod wheel or expression) as before.
- Drag past the pad's edge onto any other pad, in either row, and you move to
  that chord.

The softest point at the bottom of a pad is now velocity 20 (it was 40).

## Install

Unzip and copy `Posy.vst3` into your VST3 folder (macOS
`~/Library/Audio/Plug-Ins/VST3`, Windows `C:\Program Files\Common Files\VST3`,
Linux `~/.vst3`), then rescan plug-ins in Live. The guide and the README cover
routing Posy into an instrument.

**macOS:** the plugin isn't notarised. If macOS refuses to load it, run
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Posy.vst3`.
