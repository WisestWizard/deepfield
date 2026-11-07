# DEEP FIELD cheat sheet

Three rules: a page button always lands on the same page and SELECT steps between pages; a lit button is a mode;
the screen mirrors the panel (the keys strip is your keys, the four knob cells are your four knobs).

## Keys
| Do | Result |
| --- | --- |
| LATCH on (SEL lit, the default) | A key strikes a body that stays. The same key again lets it go. |
| LATCH off (SEL dark, the screen says KEYS PLAY) | The bed stays; a key plays a lead over it: the scene's model struck short (LEAD page), held while the key is down, released after. A latched key plays a lead on top of its body (an accent). |
| Lean on a key | Brighter, more wobble, while you lean. |
| Hold SEL for a second | Every body let go (and TRANSPOSE ends). |
| Hold SEL, touch a key (TRANSPOSE, pill SEL TRANS) | Every key then moves the latched bodies together, the lowest to that key, gliding over GLIDE; letting go leaves them there. Tap SEL to latch again. |
| Keys light | Three levels: LIT = what the keys do now (a latched body under LATCH on, a lead while held); MID = the machine's (a body fading, a moved body's home key, the bed under LATCH off, a lead releasing, a body off the keys at the end key, ORBIT's destination, blinking as the move nears); GLOW = the scale. A lead's strike winks its key off. On screen the same, a yellow frame on a latched key, white for a lead, the destination in the body's dim colour. |

## Buttons
| Button | Tap | Hold |
| --- | --- | --- |
| HOME | HOME page (the four forces) | |
| EDIT | BODY page (SELECT for NOISE, again for NOISE 2) | |
| ENV | ENVELOPE page (SELECT for LEAD) | |
| LFO | MOTION page (SELECT for MOTION 2) | |
| FX | The effects pages: ENSEMBLE, then SELECT for DELAY, DELAY 2 and REVERB | Keys F3 G3 A3 B3 = ensemble, delay, reverb, shimmer on / off on every scene (one at zero comes in at a usable amount), C4 = the noise layer, shown large (saved with the scene) |
| ARP | ORBIT page (SELECT for ORBIT 2) | |
| GLO | GLOBAL page (SELECT for MIDI, again for USB) | Lean on a key: its pressure crossfades toward the next scene (push after the key settles; D5 up read no lean: held, the blend climbs to full in 2 s); let go and the blend stays |
| SEL | LATCH on / off (lit while on); from TRANSPOSE, LATCH on | 1 s: every body let go; with a key touched: TRANSPOSE |
| PLAY | ORBIT on / off; its colour is the MODE: green WANDER (one body moves), orange CHORD, SCATTER and CYCLE (the chord moves: along the scale, scattered, or through a progression), red ARP (a runner of leads over the bed, on after the bed is let go), orange ECHO (your last notes replayed, fading) and PULSE (a roll of dips across the chord); the pill fills toward the next move, a line reaches from a beam toward where it is going; MIDI clock in times both | ORBIT page |
| SEQ | FREEZE on / off (lit): the hall holds forever | |
| SAVE | Save dialog: PRESET picks U1..U16, SAVE again saves, HOME cancels | |
| OCT− / OCT+ | Octave −2..+2; the button half-lights one octave out, lights two out | |
| REC | Scan for a Bluetooth MIDI device, ALGO picks, REC again connects | 1 s: Bluetooth on / off |

The page's button lights (mid while FX is held). A page stays until a button or SELECT moves it. A held button
blinks from a third of a second until its hold fires. PLAY's colour while ORBIT runs: green WANDER, orange CHORD,
orange SCATTER winking at each move, red ARP winking at each step. SAVE blinks in its dialog.

## Encoders
| Encoder | Does |
| --- | --- |
| SELECT | The page before or after (HOME, BODY, NOISE, NOISE 2, ENVELOPE, LEAD, MOTION, MOTION 2, FX ENSEMBLE, FX DELAY, FX DELAY 2, FX REVERB, ORBIT, ORBIT 2, GLOBAL, MIDI, USB) |
| PRESET | Scroll the scenes (the presets); the one you stop on morphs in over MORPH seconds |
| ALGO | Model (the algorithm): SOLAR, VOID, GLASS, STRING, CHOIR, BELL, SHEPARD, FOLD |
| KNOB 1–4 | The page's four values (below), shown in a row under the knobs |
| MASTER | Volume |

## Pages (KNOB 1 · 2 · 3 · 4)
| Page | 1 | 2 | 3 | 4 |
| --- | --- | --- | --- | --- |
| HOME | MASS (bright) | DRIFT (wander) | SPACE (the hall) | TIDE (ebb and swell) |
| BODY | MODEL | SHAPE | SUB | AIR |
| NOISE | LEVEL (0 = off) | COLOUR (brown → white) | WIND (how fast the band wanders; how often a gust) | BAND (wide bed → narrow on the newest note → past half, breath on every body) |
| NOISE 2 | TEXTURE (hiss → rain → crackle) | GUST (the weather's depth) | DUCK (back under a chord) | WIDTH |
| ENVELOPE | SWELL | FADE | PRESS | GLIDE |
| LEAD | ATTACK | RELEASE | LIFT (semitones up) | LEVEL |
| MOTION | TIDE RATE | WOBBLE | WALK | WIDTH |
| MOTION 2 | SPREAD (the bodies unlike each other) | CROSS (pairs bend each other) | TIMBRE (each body's MASS walks) | SWAY (each body drifts across the field) |
| FX ENSEMBLE | ENSEMBLE (mode) | ENS MIX | – | – |
| FX DELAY | TYPE: PING (left, right), TAPE (centre, worn), HEADS (three a time), REVERSE (swells), RISE (octaves up), FALL (octaves down), GRAINS (pieces scattered) | DLY TIME (50 to 800 ms; from 100 the beat's division 1/16 1/8 1/4T 1/8D 1/4 1/4D 1/2, from the runner's RATE or the MIDI clock, halved when too long; GRAINS: how often a piece) | DLY FDBK | DLY MIX |
| FX DELAY 2 | COLOUR (repeats dark and full to bright and thin) | DIFFUSE (repeats smeared into a cloud) | WOW (the tape's wander, clean to seasick) | CHOP (only a slice of each DLY TIME goes in: a held chord pulses) |
| FX REVERB | REV SIZE (decay 0.6–8 s) | REV DAMP | SHIMMER | SPACE (the send, as on HOME) |
| ORBIT | MODE | WANDER, CHORD, SCATTER, CYCLE, ECHO, PULSE: EVERY; ARP: RATE (BPM, EXT on MIDI clock) | CHANCE; SCATTER: BODIES; ARP: GATE; ECHO: DECAY | STEPS UP TO; CHORD: DISTANCE; ARP: ORDER; CYCLE: CHORDS; ECHO: JUMPS; PULSE: ROLL |
| ORBIT 2 | HOME PULL | OCTAVES (ARP) | – | – |
| GLOBAL | SCALE | ROOT | VIEW | MORPH |
| MIDI | IN CH | OUT CH | LOCAL | VELOCITY |
| USB | SPEAKER (ON; AUTO: silent while a computer records; OFF) | USB LEVEL (the recording, 0 to -18 dB) | CLOCK OUT (24 a beat while the runner runs from RATE) | – |

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

## Views (VIEW on the GLOBAL page)
BEAMS (the default): every body a beam rising from the key that struck it, its level the height, a white cap when
latched, dim while it fades; ORBIT slides a beam to the neighbouring key and marks where it was struck. A body
below or above the keys stands at the edge. The noise layer is a soft wide beam where its band sits, brown to white.
MOUTH: an RX bottle tumbles out a pill per body; the pills hang over their keys; a let-go pill flies into the mouth.
ORBITS: planets = bodies (radius = pitch, size = level, colour = pitch class), sun = output.
NEBULA: the harmonics as glows, low left, high right.
SCOPE: radar, blips = bodies by pitch class and octave, figure = left vs right.

## Screen
Header: scene slot and name (orange = unsaved); octave, BLE, bodies/8 and CPU at the right; the three states as
pills named for their buttons (SEL LATCH, PLAY ORBIT, SEQ FRZ); E D R S N lit for the effects heard and the noise; a morph bar
along the bottom.
View: the bodies, in the view chosen; every knob or button large over it for a moment; FX held names the switches
over their keys.
Keys strip: the 27 keys as they are lit (sounding, fading, in the scale, a yellow frame when latched).
Footer: the page band (the button that opened the page, its name, the SELECT arrows; model and scale at the
right), then the four knobs in a row: name, big value, bar.

## Factory scenes
F1 FIRST LIGHT · F2 VOID TIDE · F3 GLASS ORBIT (made for ORBIT) · F4 BOWED SKY · F5 PULSAR · F6 NEBULA ·
F7 DEEP VOID · F8 STARLIGHT · F9 CATHEDRAL · F10 LOW CHOIR · F11 GAMELAN (made for ORBIT) · F12 ICE BELLS ·
F13 ASCENT · F14 DESCENT · F15 FOLDED SUN · F16 TIDE POOL. U1..U16 are yours.

## MIDI
In: notes hold while on (CC 64 keeps them), any channel by default. Out: key down = note on, let go = note off (later if latched), ORBIT moves too.
CC 16–19 MASS DRIFT SPACE TIDE · 20 NOISE LEVEL · 74 MASS · 71 SHAPE · 72 FADE · 73 SWELL · 91 SPACE · 93 ENS MIX · 94 DLY MIX · 102 DLY TYPE · 103 WOW · 104 CHOP · 1 wobble · 7 volume · 123 all off.
Program change 0–15 = F1–F16, 16–31 = U1–U16.
