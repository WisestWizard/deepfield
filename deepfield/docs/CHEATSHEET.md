# DEEP FIELD cheat sheet

Three rules: a page button always lands on the same page and SELECT steps between pages; a lit button is a mode;
the screen mirrors the panel (the keys strip is your keys, the four knob cells are your four knobs).

## Keys
| Do | Result |
| --- | --- |
| LATCH on (SEL lit, the default) | A key strikes a body that stays. The key it sounds on (lit) lets it go as you let go, however slowly you pressed. |
| Hold a lit key and turn a knob | BODY opens: KNOB 1–4 = its own LEVEL (−24..+6 dB, MUTE) · TONE (its MASS ±50 %) · TUNE (±50 cents) · ORBIT (MOVES / PINNED: never moved, out of the arp, a pedal). Several keys: all of them. Let go: back to the page, the body kept; saved with the scene; out as NRPN |
| LATCH off (SEL dark, the screen says KEYS PLAY) | The bed stays; a key plays a lead over it: the scene's model struck short (LEAD page), held while the key is down, released after. A latched key plays a lead on top of its body (an accent). |
| Lean on a key | Brighter, more wobble, while you lean. |
| Hold SEL, touch keys (pill SEL TRANS) | Each key moves the latched chord there, the lowest note to it, gliding over GLIDE. Let go of SEL: the keys strike again, LATCH as it was. |
| Hold SEL a second, no key | Every body let go; the keys strike again at once. |
| Keys light | One meaning a level: LIT = a body sounds here (tap: let go; hold + knob: its values), a lead while held; MID = a body fading, the bed under LATCH off, a lead releasing; GLOW = the scale. The light moves with a moved body. A lead's strike winks its key off. The screen's keys strip shows the same. |

## Buttons
| Button | Tap | Hold |
| --- | --- | --- |
| HOME | HOME page (the four forces) | 1 s: the scene back as saved (its sound, effect switches and ORBIT values, over MORPH); the chord, LATCH, ORBIT on/off and FREEZE stay; SAVE goes dark |
| EDIT | BODY page (SELECT for NOISE) | |
| ENV | ENVELOPE page (SELECT for LEAD) | |
| LFO | MOTION page | |
| FX | The effects pages: ENSEMBLE, then SELECT for DELAY, DELAY 2 and REVERB | Keys F3 G3 A3 B3 = ensemble, delay, reverb, shimmer on / off on every scene (one at zero comes in at a usable amount), C4 = the noise layer, shown large (saved with the scene) |
| ARP | ORBIT page (SELECT for ORBIT 2) | |
| GLO | GLOBAL page (SELECT for MIDI, again for USB) | |
| SEL | LATCH on / off (lit while on) | A key touched: the chord moves there (while held); 1 s with no key: every body let go |
| PLAY | ORBIT on / off (as it goes down); its colour is the MODE: green WANDER (one body moves), orange CHORD, SCATTER and CYCLE (the chord moves: along the scale, scattered, or through a progression), red ARP (a runner of leads over the bed, on after the bed is let go), orange ECHO (your last notes replayed, fading) and PULSE (a roll of dips across the chord); the pill fills toward the next move, a line reaches from a beam toward where it is going; MIDI clock in times both | |
| SEQ | FREEZE on / off (lit): the hall holds forever | |
| SAVE | Save dialog: PRESET picks U1..U16, SAVE again saves, HOME cancels | |
| OCT− / OCT+ | Octave −2..+2; the button half-lights one octave out, lights two out | |
| REC | Scan for a Bluetooth MIDI device, ALGO picks, REC again connects | 1 s: Bluetooth on / off |

The page's button lights (mid while FX is held). A page stays until a button or SELECT moves it. SEL and REC held
blink from a third of a second until their hold fires. PLAY's colour while ORBIT runs: green WANDER, orange CHORD,
orange SCATTER winking at each move, red ARP winking at each step. SAVE lights for a sound or a setting changed (not
browsing, LATCH or the octave), and blinks in its dialog. An effect switched off reads OFF on its knobs; a turn brings it back on.

## Encoders
| Encoder | Does |
| --- | --- |
| SELECT | The page before or after (HOME, BODY, NOISE, ENVELOPE, LEAD, MOTION, FX ENSEMBLE, FX DELAY, FX DELAY 2, FX REVERB, ORBIT, ORBIT 2, GLOBAL, MIDI, USB) |
| PRESET | Scroll the scenes (the view lists them: neighbours, the 32 slots as marks, a line filling); 0.7 s after you stop, its sound morphs in over MORPH under your chord, each body crossing into its model; LATCH, ORBIT (and its page while it runs), FREEZE stay. Nothing latched: + CHORD, its chord comes too |
| ALGO | Model (the algorithm): SOLAR, VOID, GLASS, STRING, CHOIR, BELL, SHEPARD, FOLD, SCAN, DX, TEAR, TAPE, RESONATOR (each body crosses over in 0.8 s) |
| KNOB 1–4 | The page's four values (below), shown in a row under the knobs. Slow: 3 a click (43 a sweep); fast: 8 (16 a sweep); RATE 1 BPM a click; few-value knobs one value a click |
| MASTER | Volume |

## Pages (KNOB 1 · 2 · 3 · 4)
| Page | 1 | 2 | 3 | 4 |
| --- | --- | --- | --- | --- |
| HOME | MASS (bright) | DRIFT (wander) | SPACE (the hall) | TIDE (ebb and swell) |
| BODY | MODEL | SHAPE | SUB | AIR |
| NOISE | LEVEL (0 = off) | COLOUR (brown → white) | WIND (how fast the band wanders; how often a gust) | BAND (wide bed → narrow on the newest note → past half, breath on every body) |
| ENVELOPE | SWELL (ms below 1 s) | FADE | PRESS | GLIDE |
| LEAD | ATTACK | RELEASE | LIFT (semitones up) | LEVEL |
| MOTION | TIDE RATE | WOBBLE | WALK | WIDTH |
| FX ENSEMBLE | ENSEMBLE (mode) | ENS MIX | – | – |
| FX DELAY | TYPE: PING (left, right), TAPE (centre, worn, wavering), HEADS (three a time; a held chord pulses in triplets), REVERSE (swells; a held chord breathes in), RISE (octaves up), FALL (octaves down), GRAINS (pieces scattered, at the note, an octave either way or a fifth up) | DLY TIME (50 to 800 ms; from 100 the beat's division 1/16 1/8 1/4T 1/8D 1/4 1/4D 1/2, from the runner's RATE or the MIDI clock, halved when too long; GRAINS: how often a piece) | DLY FDBK | DLY MIX (full: the first echo about as loud as the note) |
| FX DELAY 2 | COLOUR (repeats dark and full to bright and thin) | DIFFUSE (repeats smeared into a cloud) | WOW (the tape's wander, clean to seasick) | CHOP (only a slice of each DLY TIME goes in: a held chord pulses) |
| FX REVERB | REV SIZE (decay 0.6–8 s) | REV DAMP | SHIMMER | SPACE (the send, as on HOME) |
| ORBIT | MODE | WANDER, CHORD, SCATTER, CYCLE, ECHO, PULSE: EVERY (0.1–60 s; under MIDI clock in beats, shown; ECHO from 1 s; no destination shown under 0.5 s); ARP: RATE (BPM, EXT on MIDI clock) | CHANCE; SCATTER: BODIES; ARP: GATE; ECHO: DECAY | STEPS UP TO; CHORD: DISTANCE; ARP: ORDER; CYCLE: CHORDS; ECHO: JUMPS; PULSE: ROLL |
| ORBIT 2 | HOME PULL | OCTAVES (ARP) | RHYTHM (ARP: 8/8 5/8 3/8 7/12 5/12 4/9 7/16 5/16, hits spread evenly, rests between) | DIV (ARP: 1/16 1/16T 1/8 1/8T 1/8D 1/4 1/4T 1/2 1 BAR of RATE's beat) |
| GLOBAL | SCALE | ROOT | MORPH | – |
| MIDI | IN CH | OUT CH | LOCAL | VELOCITY |
| USB | SPEAKER (ON; AUTO: silent while a computer records; OFF) | USB LEVEL (the recording, 0 to -18 dB) | CLOCK OUT (24 a beat while the runner runs from RATE) | – |

With no page, set by each scene: the noise's TEXTURE, GUST, DUCK and WIDTH; the bodies' SPREAD, CROSS, TIMBRE and
SWAY. A scene you save keeps those of the scene it came from.

## Models
| Model | What | MASS | SHAPE |
| --- | --- | --- | --- |
| SOLAR | 8 breathing harmonics | more, brighter harmonics | odd only → all → even |
| VOID | 3 detuned saws + sub, dark filter | cutoff (Hz) | detune |
| GLASS | 2-op FM, breathing index, detuned 2nd carrier | FM index | ratio 1, 2, 3, 3.5, 4, 5, 7, 1.41 |
| STRING | bowed string, noise + sine bow | brightness | (AIR = breath) |
| CHOIR | a buzz through three vowel formants | formants up, upper ones louder | the vowel: A → E → I → O → U (AIR = breath) |
| BELL | 4 breathing partials | upper partials brighter | harmonic → bell ratios (AIR = strike hiss) |
| SHEPARD | 6 octaves under a moving window: the endless rise | where the window leans | falling ← still → rising, and how fast |
| FOLD | a sine through a wavefolder | the filter after the fold | how hard it folds (AIR = noise into the fold) |
| SCAN | Pocket Poly's wavetables, one sweep of 48 frames | a filter that follows the note (level kept) | ANALOG 1 → VOX → GLASS 16 (AIR = a second copy elsewhere in the sweep) |
| DX | 4-op FM, its envelopes at SWELL's pace | FM depth | the voice: GLASS PAD, BRASS, ORGAN, BELL PAD, E.PIANO, WOOD, STRINGS, CLUSTER (AIR = feedback) |
| TEAR | a saw hard-synced to the note, DRIFT breathing the ratio | a filter that follows the note, a little drive | RATIO X1.0 → X8.0 (AIR = ring with the note's sine) |
| TAPE | a worn tape loop: wow, flutter, dropouts (all DRIFT), saturation | the tape's bandwidth | SOURCE: soft organ → saw (AIR = hiss) |
| RESONATOR | noise into five resonances: a bowl kept singing | the upper resonances brighter | MATERIAL: PURE → GLASS → BOWL → BRONZE (AIR = scrape, wider resonances) |

## The view
Every body a beam of light rising from its key in the key's colour, a white-hot head at its
level, breathing; a sun spark over it when latched, a sun bar at its foot when pinned, white frames while open, dim
while it fades; an ARP step flashes its head; ORBIT slides a beam to the neighbouring key and marks where it was
struck, after a line and a dotted ghost show where it is going. The noise is a mound of mist. FREEZE: ice brackets,
and the view holds still. A body
below or above the keys stands at the edge. The noise layer is a soft wide beam where its band sits, brown to white.

## Screen
Colours, one meaning each: a body's colour = that body; sun = latched; white = your hand (a lead, an open body);
cyan = where you are; teal = an effect on; orange = look (unsaved, LOAD).
Header: the model's glyph, scene slot and name (a small orange square = unsaved); octave, BLE, bodies/8 and CPU at
the right; the three states as pills at fixed places, named for their buttons (SEL LATCH, PLAY ORBIT with ORBIT's
countdown along its foot, SEQ FRZ); E D R S N lit for the effects heard and the noise; a morph line along the bottom.
View: the bodies, in the view chosen; every knob or button large over it for a moment; FX held names the switches
over their keys.
Keys strip: the 27 keys as dark glass lit by their bodies (a sun cap when latched, white for a lead, framed white
when open; the scale slightly lighter).
Footer: the page band (the button's icon and name, the page, the SELECT arrows; model and scale at the right), then
the four knobs in a row: name, the number big with its unit small (or a word), and a bar by kind (segments for an
amount, a cell per choice, two cells for on/off, hits and rests for RHYTHM). On DX, SHAPE reads VOICE; on SCAN,
TABLE. A body open: BODY and its note, its four values.

## Factory scenes
F1 FIRST LIGHT · F2 VOID TIDE · F3 GLASS ORBIT (made for ORBIT) · F4 BOWED SKY · F5 PULSAR · F6 NEBULA ·
F7 DEEP VOID · F8 STARLIGHT · F9 CATHEDRAL · F10 LOW CHOIR · F11 GAMELAN (made for ORBIT) · F12 ICE BELLS ·
F13 ASCENT · F14 DESCENT · F15 FOLDED SUN · F16 TIDE POOL. U1..U16 are yours. Every factory chord stands on C and
plays in ROOT. Switching on: the name falls into a vortex (3.4 s; any key, button or knob skips it, and does what it
does), then the last scene's sound in silence, no chord.

## MIDI
In: notes hold while on (CC 64 keeps them), any channel by default. Out: key down = note on, let go = note off (later if latched), ORBIT moves too.
CC 16–19 MASS DRIFT SPACE TIDE · 20 NOISE LEVEL · 74 MASS · 71 SHAPE · 72 FADE · 73 SWELL · 91 SPACE · 93 ENS MIX · 94 DLY MIX · 102 DLY TYPE · 103 WOW · 104 CHOP · 1 wobble · 7 volume · 123 all off.
NRPN (99 · 98 = the body's note · 6), in and out: 1 LEVEL (0 MUTE, 1–31 = −24..+6 dB) · 2 TONE (64 ± 50) · 3 TUNE (64 ± 50 cents) · 4 ORBIT (64 up PINNED).
Program change 0–15 = F1–F16, 16–31 = U1–U16: the sound, as PRESET.
