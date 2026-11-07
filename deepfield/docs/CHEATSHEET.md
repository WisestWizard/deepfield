# DEEP FIELD cheat sheet

Three rules: a page button always lands on the same page and SELECT steps between pages; a lit button is a mode;
the screen mirrors the panel (the keys strip is your keys, the four knob cells are your four knobs).

## Keys
| Do | Result |
| --- | --- |
| LATCH on (SEL lit, the default) | A key strikes a voice that stays. The key it sounds on (lit) lets it go as you let go, however slowly you pressed. |
| Hold a lit key and turn a knob | VOICE opens: KNOB 1–4 = its own LEVEL (−24..+6 dB, MUTE) · TONE (its TIMBRE ±50 %) · TUNE (±50 cents) · LOCK (OFF / ON: never moved by GEN, out of the arp, a pedal). Several keys: all of them. Let go: back to the page, the voice kept; saved with the preset; out as NRPN |
| LATCH off (SEL dark, the screen says KEYS PLAY) | The held chord stays; a key plays a lead over it: the preset's model struck short (LEAD page), held while the key is down, released after. A latched key plays a lead on top of its voice (an accent). |
| Aftertouch (press harder on a held key) | Brighter, more vibrato, while you press. |
| Hold SEL, touch keys (pill SEL TRANS) | Each key moves the latched chord there, the lowest note to it, gliding over GLIDE. Let go of SEL: the keys strike again, LATCH as it was. |
| Hold SEL a second, no key | ALL NOTES OFF: every voice let go; the keys strike again at once. |
| Keys light | One meaning a level: LIT = a voice sounds here (tap: let go; hold + knob: its values), a lead while held; MID = a voice fading, the held chord under LATCH off, a lead releasing; GLOW = the scale. The light moves with a moved voice. A lead's strike winks its key off. The screen's keys strip shows the same. |

## Buttons
| Button | Tap | Hold |
| --- | --- | --- |
| HOME | HOME page (the four macros) | 1 s: the preset back as saved (its sound, effect switches and GEN values, over MORPH); the chord, LATCH, GEN on/off and FREEZE stay; SAVE goes dark |
| EDIT | OSC page (SELECT for NOISE) | |
| ENV | ENVELOPE page (SELECT for LEAD) | |
| LFO | LFO page | |
| FX | The effects pages: ENSEMBLE, then SELECT for DELAY, DELAY 2 and REVERB | Keys F3 G3 A3 B3 = ensemble, delay, reverb, shimmer on / off on every preset (one at zero comes in at a usable amount), C4 = the noise layer, shown large (saved with the preset) |
| ARP | GEN page (SELECT for GEN 2) | |
| GLO | GLOBAL page (SELECT for MIDI, again for USB) | |
| SEL | LATCH on / off (lit while on) | A key touched: the chord moves there (while held); 1 s with no key: every voice let go |
| PLAY | GEN on / off (as it goes down); its colour is the MODE: green WALK (one voice moves), orange SHIFT, RANDOM and PROG (the chord moves: along the scale, scattered, or through a progression), red ARP (an arpeggio of leads over the held chord, on after the chord is let go), orange LOOP (your last notes replayed, fading) and STRUM (a roll of dips across the chord); the pill fills toward the next move, a line reaches from a beam toward where it is going; MIDI clock in times both | |
| SEQ | FREEZE on / off (lit): the hall holds forever | |
| SAVE | Save dialog: PRESET picks U1..U16, SAVE again saves, HOME cancels | |
| OCT− / OCT+ | Octave −2..+2; the button half-lights one octave out, lights two out | |
| REC | Scan for a Bluetooth MIDI device, ALGO picks, REC again connects | 1 s: Bluetooth on / off |

The page's button lights (mid while FX is held). A page stays until a button or SELECT moves it. SEL and REC held
blink from a third of a second until their hold fires. PLAY's colour while GEN runs: green WALK, orange SHIFT,
orange RANDOM winking at each move, red ARP winking at each step. SAVE lights for a sound or a setting changed (not
browsing, LATCH or the octave), and blinks in its dialog. An effect switched off reads OFF on its knobs; a turn brings it back on.

## Encoders
| Encoder | Does |
| --- | --- |
| SELECT | The page before or after (HOME, OSC, NOISE, ENVELOPE, LEAD, LFO, FX ENSEMBLE, FX DELAY, FX DELAY 2, FX REVERB, GEN, GEN 2, GLOBAL, MIDI, USB) |
| PRESET | Scroll the presets (the view lists them: neighbours, the 32 slots as marks, a line filling); 0.7 s after you stop, its sound morphs in over MORPH under your chord, each voice crossing into its model; LATCH, GEN (and its page while it runs), FREEZE stay. Nothing latched: + CHORD, its chord comes too |
| ALGO | Model (the algorithm): ADDITIVE, SUPERSAW, FM, BOWED, FORMANT, BELL, SHEPARD, WAVEFOLD, WAVETABLE, FM 4-OP, SYNC, TAPE, RESONATOR (each voice crosses over in 0.8 s) |
| KNOB 1–4 | The page's four values (below), shown in a row under the knobs. Slow: 3 a click (43 a sweep); fast: 8 (16 a sweep); TEMPO 1 BPM a click; few-value knobs one value a click |
| MASTER | Volume |

## Pages (KNOB 1 · 2 · 3 · 4)
| Page | 1 | 2 | 3 | 4 |
| --- | --- | --- | --- | --- |
| HOME | TIMBRE (bright) | DRIFT (wander) | REVERB (the hall) | TREMOLO (ebb and swell) |
| OSC | MODEL | SHAPE (named for the model) | SUB | NOISE (named for the model) |
| NOISE | LEVEL (0 = off) | COLOR (brown → white) | RATE (how fast the band wanders; how often a gust) | BANDPASS (wide → narrow on the newest note → past half, breath on every voice) |
| ENVELOPE | ATTACK (ms below 1 s) | RELEASE | AFTERTOUCH | GLIDE |
| LEAD | ATTACK | RELEASE | TRANSPOSE (semitones up) | LEVEL |
| LFO | TREM RATE | VIBRATO | RANDOM | WIDTH |
| FX ENSEMBLE | ENSEMBLE (mode) | ENS MIX | – | – |
| FX DELAY | TYPE: PING PONG (left, right), TAPE (centre, worn, wavering), MULTI TAP (three a time; a held chord pulses in triplets), REVERSE (swells; a held chord breathes in), OCT UP (octaves up), OCT DOWN (octaves down), GRANULAR (pieces scattered, at the note, an octave either way or a fifth up) | DLY TIME (50 to 800 ms; from 100 the beat's division 1/16 1/8 1/4T 1/8D 1/4 1/4D 1/2, from ARP's TEMPO or the MIDI clock, halved when too long; GRANULAR: how often a piece) | DLY FDBK | DLY MIX (full: the first echo about as loud as the note) |
| FX DELAY 2 | TONE (repeats dark and full to bright and thin) | DIFFUSION (repeats smeared into a cloud) | WOW (the tape's wander, clean to seasick) | GATE (only a slice of each DLY TIME goes in: a held chord pulses) |
| FX REVERB | DECAY (0.6–8 s) | DAMPING | SHIMMER | REVERB (the send, as on HOME) |
| GEN | MODE | WALK, SHIFT, RANDOM, PROG, LOOP, STRUM: RATE (0.1–60 s; under MIDI clock in beats, shown; LOOP from 1 s; no destination shown under 0.5 s); ARP: TEMPO (BPM, EXT on MIDI clock) | CHANCE; RANDOM: VOICES; ARP: GATE; LOOP: DECAY | RANGE; SHIFT: RANGE; ARP: PATTERN; PROG: CHORDS; LOOP: OCT JUMP; STRUM: SPEED |
| GEN 2 | RETURN | OCTAVES (ARP) | EUCLID (ARP: 8/8 5/8 3/8 7/12 5/12 4/9 7/16 5/16, hits spread evenly, rests between) | DIV (ARP: 1/16 1/16T 1/8 1/8T 1/8D 1/4 1/4T 1/2 1 BAR of TEMPO's beat) |
| GLOBAL | SCALE | ROOT | MORPH | – |
| MIDI | IN CH | OUT CH | LOCAL | VELOCITY |
| USB | SPEAKER (ON; AUTO: silent while a computer records; OFF) | USB LEVEL (the recording, 0 to -18 dB) | CLOCK OUT (24 a beat while ARP runs from TEMPO) | – |

With no page, set by each preset: the noise's TYPE, GUST, DUCK and WIDTH; the voices' SPREAD, CROSS MOD, TIMBRE DRIFT and
PAN DRIFT. A preset you save keeps those of the preset it came from.

## Models
TIMBRE, SHAPE and NOISE are named on the screen for what they do on the model (shown here in caps); an empty cell is
a knob the model doesn't use.

| Model | What | TIMBRE (HOME 1) | SHAPE (OSC 2) | NOISE (OSC 4) |
| --- | --- | --- | --- | --- |
| ADDITIVE | 8 breathing harmonics | HARMONICS: more, brighter | ODD/EVEN: odd only → all → even | – |
| SUPERSAW | 3 detuned saws + sub, dark filter | CUTOFF (Hz) | DETUNE | – |
| FM | 2-op FM, breathing index, detuned 2nd carrier | FM AMOUNT | RATIO 1, 2, 3, 3.5, 4, 5, 7, 1.41 | – |
| BOWED | bowed string, noise + sine bow | BRIGHTNESS | – | BOW NOISE |
| FORMANT | a buzz through three vowel formants | FORMANT: up, upper ones louder | VOWEL: A → E → I → O → U | BREATH |
| BELL | 4 breathing partials | BRIGHTNESS: upper partials | INHARMONIC: harmonic → bell ratios | STRIKE (hiss) |
| SHEPARD | 6 octaves under a moving window: the endless rise | CENTER: where the window leans | RISE/FALL: falling ← still → rising, and how fast | NOISE |
| WAVEFOLD | a sine through a wavefolder | CUTOFF after the fold | FOLD: how hard | NOISE into the fold |
| WAVETABLE | Pocket Poly's wavetables, one sweep of 48 frames | CUTOFF following the note (level kept) | POSITION: ANALOG 1 → VOX → GLASS 16 | UNISON: a second copy elsewhere in the sweep |
| FM 4-OP | 4-op FM, its envelopes at ATTACK's pace | FM AMOUNT | PATCH: GLASS PAD, BRASS, ORGAN, BELL PAD, E.PIANO, WOOD, STRINGS, CLUSTER | FEEDBACK |
| SYNC | a saw hard-synced to the note, DRIFT breathing the ratio | CUTOFF following the note, a little drive | RATIO X1.0 → X8.0 | RING MOD with the note's sine |
| TAPE | a worn tape loop: wow, flutter, dropouts (all DRIFT), saturation | BANDWIDTH | SOURCE: soft organ → saw | HISS |
| RESONATOR | noise into five resonances: a bowl kept singing | BRIGHTNESS: upper resonances | MATERIAL: PURE → GLASS → BOWL → BRONZE | NOISE: scrape, wider resonances |

## The view
Every voice a beam of light rising from its key in the key's colour, a white-hot head at its
level, breathing; a sun spark over it when latched, a sun bar at its foot when locked, white frames while open, dim
while it fades; an ARP step flashes its head; GEN slides a beam to the neighbouring key and marks where it was
struck, after a line and a dotted ghost show where it is going. The noise is a mound of mist. FREEZE: ice brackets,
and the view holds still. A voice
below or above the keys stands at the edge. The noise layer is a soft wide beam where its band sits, brown to white.

## Screen
Colours, one meaning each: a voice's colour = that voice; sun = latched; white = your hand (a lead, an open voice);
cyan = where you are; teal = an effect on; orange = look (unsaved, LOAD).
Header: the model's glyph, preset slot and name (a small orange square = unsaved); octave, BLE, voices/8 and CPU at
the right; the three states as pills at fixed places, named for their buttons (SEL LATCH, PLAY GEN with GEN's
countdown along its foot, SEQ FRZ); E D R S N lit for the effects heard and the noise; a morph line along the bottom.
View: the voices as beams; every knob or button large over it for a moment; FX held names the switches
over their keys.
Keys strip: the 27 keys as dark glass lit by their voices (a sun cap when latched, white for a lead, framed white
when open; the scale slightly lighter).
Footer: the page band (the button's icon and name, the page, the SELECT arrows; model and scale at the right), then
the four knobs in a row: name, the number big with its unit small (or a word), and a bar by kind (segments for an
amount, a cell per choice, two cells for on/off, hits and rests for EUCLID). TIMBRE, SHAPE and NOISE are named for the
model (see Models). A voice open: VOICE and its note, its four values.

## Factory presets
F1 FIRST LIGHT · F2 VOID TIDE · F3 GLASS ORBIT (made for GEN) · F4 BOWED SKY · F5 PULSAR · F6 NEBULA ·
F7 DEEP VOID · F8 STARLIGHT · F9 CATHEDRAL · F10 LOW CHOIR · F11 GAMELAN (made for GEN) · F12 ICE BELLS ·
F13 ASCENT · F14 DESCENT · F15 FOLDED SUN · F16 TIDE POOL. U1..U16 are yours. Every factory chord stands on C and
plays in ROOT. Switching on: the name falls into a vortex (3.4 s; any key, button or knob skips it, and does what it
does), then the last preset's sound in silence, no chord.

## MIDI
In: notes hold while on (CC 64 keeps them), any channel by default. Out: key down = note on, let go = note off (later if latched), GEN moves too.
CC 16–19 TIMBRE DRIFT REVERB TREMOLO · 20 NOISE LEVEL · 74 TIMBRE · 71 SHAPE · 72 RELEASE · 73 ATTACK · 91 REVERB · 93 ENS MIX · 94 DLY MIX · 102 DLY TYPE · 103 WOW · 104 GATE · 1 vibrato · 7 volume · 123 all off.
NRPN (99 · 98 = the voice's note · 6), in and out: 1 LEVEL (0 MUTE, 1–31 = −24..+6 dB) · 2 TONE (64 ± 50) · 3 TUNE (64 ± 50 cents) · 4 LOCK (64 up ON).
Program change 0–15 = F1–F16, 16–31 = U1–U16: the sound, as PRESET.
