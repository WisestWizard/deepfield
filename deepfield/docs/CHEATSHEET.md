# DEEP FIELD cheat sheet

Three rules: a page button always lands on the same page and SELECT steps between pages; a lit button is a mode;
the screen mirrors the panel (the keys strip is your keys, the four knob cells are your four knobs).

## Keys
| Do | Result |
| --- | --- |
| LATCH on (SEL lit, the default) | A key strikes a body that stays. The same key again lets it go. |
| LATCH off | A body sounds while its key is held and fades when released. |
| Lean on a key | Brighter, more wobble, while you lean. |
| Hold SEL for a second | Every body let go. |
| Keys light | While a body sounds, at the note it is at now (an ORBIT move walks the light; the home key half-lit); dim while it fades; in-scale keys glow. On screen the same, a yellow frame on a latched key. |

## Buttons
| Button | Tap | Hold |
| --- | --- | --- |
| HOME | HOME page (the four forces) | |
| EDIT | BODY page (SELECT for NOISE) | |
| ENV | ENVELOPE page | |
| LFO | MOTION page | |
| FX | The effects pages: ENSEMBLE, then SELECT for DELAY and REVERB | Keys F3 G3 A3 B3 = ensemble, delay, reverb, shimmer on / off, C4 = the noise layer, shown large (saved with the scene) |
| ARP | ORBIT page | |
| GLO | GLOBAL page (SELECT for MIDI) | |
| SEL | LATCH on / off (lit while on) | 1 s: every body let go |
| PLAY | ORBIT on / off (green): bodies wander within the scale; the pill fills toward the next move | ORBIT page |
| SEQ | FREEZE on / off (lit): the hall holds forever | |
| SAVE | Save dialog: PRESET picks U1..U16, SAVE again saves, HOME cancels | |
| OCT− / OCT+ | Octave −2..+2; the button half-lights one octave out, lights two out | |
| REC | Scan for a Bluetooth MIDI device, ALGO picks, REC again connects | 1 s: Bluetooth on / off |

The page's button lights. A page stays until a button or SELECT moves it.

## Encoders
| Encoder | Does |
| --- | --- |
| SELECT | The page before or after (HOME, BODY, NOISE, ENVELOPE, MOTION, FX ENSEMBLE, FX DELAY, FX REVERB, ORBIT, GLOBAL, MIDI) |
| PRESET | Scroll the scenes (the presets); the one you stop on morphs in over MORPH seconds |
| ALGO | Model (the algorithm): SOLAR, VOID, GLASS, STRING, CHOIR, BELL, SHEPARD, FOLD |
| KNOB 1–4 | The page's four values (below), shown in a row under the knobs |
| MASTER | Volume |

## Pages (KNOB 1 · 2 · 3 · 4)
| Page | 1 | 2 | 3 | 4 |
| --- | --- | --- | --- | --- |
| HOME | MASS (bright) | DRIFT (wander) | SPACE (the hall) | TIDE (ebb and swell) |
| BODY | MODEL | SHAPE | SUB | AIR |
| NOISE | LEVEL (0 = off) | COLOUR (brown → white) | WIND (how fast the band wanders) | BAND (wide bed → narrow, on the newest body's note) |
| ENVELOPE | SWELL | FADE | PRESS | GLIDE |
| MOTION | TIDE RATE | WOBBLE | WALK | WIDTH |
| FX ENSEMBLE | ENSEMBLE (mode) | ENS MIX | – | – |
| FX DELAY | DLY TIME | DLY FDBK | DLY MIX | – |
| FX REVERB | REV SIZE (decay 0.6–8 s) | REV DAMP | SHIMMER | SPACE (the send, as on HOME) |
| ORBIT | EVERY (s between moves) | CHANCE | STEPS UP TO | HOME PULL |
| GLOBAL | SCALE | ROOT | VIEW | MORPH |
| MIDI | IN CH | OUT CH | LOCAL | VELOCITY |

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
CC 16–19 MASS DRIFT SPACE TIDE · 20 NOISE LEVEL · 74 MASS · 71 SHAPE · 72 FADE · 73 SWELL · 91 SPACE · 1 wobble · 7 volume · 123 all off.
Program change 0–15 = F1–F16, 16–31 = U1–U16.
