# DEEP FIELD

DEEP FIELD turns the M-VAVE FM-1 into an ambient drone machine. Each key you press starts a *voice*: a note that
swells in, keeps sounding, slowly breathes and drifts, and fades out when you stop it. Up to eight play at once.
Four knobs shape every voice at the same time, a held key sets one voice apart, GEN moves them to nearby notes or
runs an arpeggio over them, thirteen models range from breathing harmonics to wavetables and FM, and the reverb can
freeze and shimmer. MIDI comes in over USB, Bluetooth and the TRS jack, and goes out over USB and Bluetooth; every
knob has a CC, and the presets travel as SysEx.

## Play it in your browser

No FM-1 needed: [open DEEP FIELD in your browser](emu/). Headphones help. Chrome, Edge, Firefox and Safari all
work; on a phone, turn it sideways.

1. Press Switch on. The preset FIRST LIGHT loads in silence: nothing plays until you play.
2. Click a few keys, one at a time, or type them: Q to I and A to K are the white keys, 1 to 0 and the minus key
   the black ones. Each note swells in and keeps sounding.
3. Click a lit key again: that note fades out.
4. Drag KNOB 1 to KNOB 4 up or down. They set the brightness, the wander, the reverb and the slow rise and fall of
   every note at once.
5. Press PLAY (or the P key): the notes start moving to nearby notes on their own.
6. Turn PRESETS to change the preset: its sound blends in over a few seconds under the notes you are playing.
   Shift-click SEL to hold it while you turn, and the preset brings its own chord in place of yours.
7. Hold SEL (shift-click it) and press HOME to stop every note.

The [cheat sheet](cheatsheet/) has every control on one page, and the [manual](manual/) has the rest.

## On the FM-1

Version {{VERSION}}. Installing is at your own risk; the installer can put M-VAVE's own firmware back.

- [Install](install/): from Chrome or Edge, with the FM-1 connected by USB.
- [Cheat sheet](cheatsheet/): every control on one page, to print or keep open.
- [Manual](manual/): the screen, the controls, the presets, MIDI.
- [Download {{PKG}}](firmware/{{PKG}}): for the command-line installer. In a copy of DEEP FIELD's source code, run
  `python3 base/tools/fm1_install.py {{PKG}}`.

M-VAVE and FM-1 are trademarks of their owners. DEEP FIELD is not affiliated with M-VAVE.
