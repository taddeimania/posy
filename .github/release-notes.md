Posy is a chord generator for Ableton Live: twelve chords per set, one per key,
sent out as MIDI. Straight, strummed, held or arpeggiated.

## What's new in 0.3.1

**Drag on a pad for velocity.** Hold a pad and drag up for louder, down for
softer; a number and a bar on the pad show it. Sliding sideways to another pad
in the same row still changes chord.
- Notes the pad starts from then on take the new velocity, so with ARP on each
  step follows your drag (unless FIXED velocity is on).
- The drag also goes to your instrument so a sounding chord can swell. Choose
  what with the new **DRAG** setting in VOICE: **PRESS** (aftertouch; map
  Pressure to volume or filter in Wavetable, Drift or Meld), **MOD** (mod
  wheel, CC1), **EXPR** (expression, CC11) or **OFF**. Letting go of the pad
  puts it back.

**Fixed: the search box held on to the keyboard.** Once you'd clicked in the
search box, Live's computer MIDI keyboard stopped playing until you closed
Posy. The box now hands the keyboard back when you press Return or Esc, click
anywhere else in Posy, or stop typing for a few seconds, and clicking the sets
list no longer takes it.

## Install

Unzip and copy `Posy.vst3` into your VST3 folder (macOS
`~/Library/Audio/Plug-Ins/VST3`, Windows `C:\Program Files\Common Files\VST3`,
Linux `~/.vst3`), then rescan plug-ins in Live. The guide and the README cover
routing Posy into an instrument.

**macOS:** the plugin isn't notarised. If macOS refuses to load it, run
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Posy.vst3`.
