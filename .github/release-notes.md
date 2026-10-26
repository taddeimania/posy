Posy is a chord generator for Ableton Live: twelve chords per set, one per key,
sent out as MIDI. Straight, strummed, held or arpeggiated.

## What's new in 0.3.0

**Your own chord banks.** Play your own voicings onto the pads and save them.
- Press **EDIT** (above the E and F pads), click a pad, play a chord on your
  MIDI keyboard and let go. The chord lands on the pad with a name (`CM9`,
  `A/Db`) and the next pad is selected, so you can play twelve in a row. You
  hear the notes through your instrument as you play them.
- Posy keeps the most notes you held at once, so a rolled chord or a
  corrected wrong note comes out right.
- Fix a voicing by clicking keys on the OUT keyboard. Double-click a pad to
  rename it; right-click to rename, re-name from its notes, or clear it.
- **EDIT** on a factory set starts a bank from a copy of it; **NEW** starts
  an empty one.
- **SAVE** / **SAVE AS** put banks under the new **MY SETS** button. Right-click
  a bank in the list to rename, show or delete it.
- The bank you're using is saved in the Live set too, so a project keeps
  playing even if the bank's file is renamed or deleted. Loading something
  else with unsaved edits asks first.

Banks are files in `~/Library/Application Support/Posy/Banks` (Windows
`%APPDATA%\Posy\Banks`, Linux `~/.config/Posy/Banks`). The getting started
guide has a new page on making one.

## Install

Unzip and copy `Posy.vst3` into your VST3 folder (macOS
`~/Library/Audio/Plug-Ins/VST3`, Windows `C:\Program Files\Common Files\VST3`,
Linux `~/.vst3`), then rescan plug-ins in Live. The guide and the README cover
routing Posy into an instrument.

**macOS:** the plugin isn't notarised. If macOS refuses to load it, run
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Posy.vst3`.
