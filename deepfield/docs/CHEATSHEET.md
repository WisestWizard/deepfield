# DEEP FIELD cheat sheet

Three rules: a page button always lands on the same page and SELECT steps between pages; a lit button is a mode;
the screen mirrors the panel (the keys strip is your keys, the four knob cells are your four knobs).

## Keys
| Do | Result |
| --- | --- |
| LATCH on (SEL lit, the default) | A key strikes a voice that stays. The key it sounds on (lit) lets it go as you let go, however slowly you pressed. |
| How hard you strike (VELOCITY on, the default; MIDI page) | The voice's level over 18 dB, how bright it is (as if you had moved TIMBRE) and how fast it arrives (a soft note eases in at up to half ATTACK's pace). A firm press is already full: the keybed counts its hardest 15 % as a full strike. Off: every key strikes full. |
| Hold a lit key and turn a knob | VOICE opens: KNOB 1–4 = its own LEVEL (−24..+6 dB, MUTE) · TONE (its TIMBRE ±50 %) · TUNE (±50 cents) · LOCK (OFF / ON: never moved by GEN, out of the arp, a pedal). Several keys: all of them. Let go: back to the page, the voice kept; saved with the preset; out as NRPN |
| LATCH off (SEL dark, the screen says KEYS PLAY) | The held chord stays; a key plays a lead over it: the preset's model struck short (LEAD page), held while the key is down, released after. A latched key plays a lead on top of its voice (an accent). |
| Aftertouch (press harder on a held key) | Brighter, more vibrato (at the LFO's own rate), while you press. |
| Hold SEL (the chord's button, no timer) | + a key (pill SEL TRANS): the latched chord moves there, the lowest note to it, gliding over GLIDE · + PRESETS: the preset's chord comes in over yours (the old release, the new swell) · + PLAY: GEN's next move now · + HOME: ALL NOTES OFF, every voice let go, the keys strike again at once. Let go of SEL: the keys strike again, LATCH as it was. Held with nothing done: nothing. |
| Keys light | One meaning a level: LIT = a voice sounds here (tap: let go; hold + knob: its values), a lead while held; MID = a voice fading, the held chord under LATCH off, a lead releasing; GLOW = the scale. The light moves with a moved voice. A lead's strike winks its key off. The screen's keys strip shows the same. |

## Buttons
| Button | Tap | Hold |
| --- | --- | --- |
| HOME | HOME page (the four macros); with SEL held: every voice let go | 1 s: the preset back as saved (its sound, effect switches and GEN values, over MORPH); the chord, LATCH, GEN on/off and FREEZE stay; SAVE goes dark |
| EDIT | OSC page (SELECT for NOISE, again for NOISE 2) | |
| ENV | ENVELOPE page (SELECT for LEAD) | |
| LFO | LFO page (SELECT for VOICES) | |
| FX | The effects pages: ENSEMBLE, then SELECT for DELAY, DELAY 2 and REVERB | Keys F3 G3 A3 B3 = ensemble, delay, reverb, shimmer on / off on every preset (one at zero comes in at a usable amount), C4 = the noise layer, shown large (saved with the preset) |
| ARP | GEN page (SELECT for GEN 2) | |
| GLO | GLOBAL page (SELECT for MIDI, again for USB) | |
| SEL | LATCH on / off (lit while on) | The chord's button: + key moves the chord, + PRESETS brings the preset's chord, + PLAY GEN's next move now, + HOME every voice let go |
| PLAY | GEN on / off (as it goes down; on, it opens the GEN page unless GEN 2 is open; off, the page stays); with SEL held, the next move now; its colour is the MODE: green WALK (one voice moves), orange SHIFT, RANDOM and PROG (the chord moves: along the scale, scattered, or through a progression), red ARP (an arpeggio of leads over the held chord, on after the chord is let go; the bar's first step accented), and orange LOOP (your last notes replayed at the velocities you played them, fading; it needs nothing latched and ignores CHANCE, and the pill counts the phrase or reads EMPTY); the pill fills toward the next move, a line reaches from a beam toward where it is going; MIDI clock in times both | |
| SEQ | FREEZE on / off (lit): the hall holds forever | |
| SAVE | Save dialog: PRESET picks U1..U16, SAVE again saves, HOME cancels | |
| OCT− / OCT+ | Octave −2..+2; the button half-lights one octave out, lights two out | |
| REC | On the MIDI page: connect the device ALGO picks, or disconnect it. Elsewhere: scan, ALGO picks, REC again connects | 1 s: Bluetooth on / off |

The page's button lights (mid while FX is held). A page stays until a button or SELECT moves it. SEL and REC held
blink from a third of a second until their hold fires. PLAY's colour while GEN runs: green WALK, orange SHIFT,
orange RANDOM winking at each move, red ARP winking at each step. SAVE lights for a sound or a setting changed (not
browsing, LATCH or the octave), and blinks in its dialog. An effect switched off reads OFF on its knobs; a turn brings it back on.

## Encoders
| Encoder | Does |
| --- | --- |
| SELECT | The page before or after (HOME, OSC, NOISE, NOISE 2, ENVELOPE, LEAD, LFO, VOICES, FX ENSEMBLE, FX DELAY, FX DELAY 2, FX REVERB, GEN, GEN 2, GLOBAL, MIDI, USB) |
| PRESET | Scroll the presets (the view lists them: neighbours, the 32 slots as marks, a line filling); 0.7 s after you stop, its sound morphs in over MORPH under your chord, each voice crossing into its model; LATCH, GEN (and its page while it runs), FREEZE stay. With SEL held: + CHORD, its chord comes in over yours (LATCH on, its GEN values too) |
| ALGO | Model (the algorithm): ADDITIVE, SUPERSAW, FM, BOWED, FORMANT, BELL, SHEPARD, WAVEFOLD, WAVETABLE, FM 4-OP, SYNC, TAPE, RESONATOR (each voice crosses over in 0.8 s) |
| KNOB 1–4 | The page's four values (below), shown in a row under the knobs; each sends its CC as it turns. Slow: 3 a click (43 a sweep); fast: 8 (16 a sweep); TEMPO 1 BPM a click; few-value knobs one value a click |
| MASTER | Volume: the speaker, the headphones and the USB recording (USB LEVEL trims the recording under it) |

## Pages (KNOB 1 · 2 · 3 · 4)
| Page | 1 | 2 | 3 | 4 |
| --- | --- | --- | --- | --- |
| HOME | TIMBRE (bright) | DRIFT (wander) | SUB (an octave below, as on OSC) | TREMOLO (ebb and swell) |
| OSC | MODEL | SHAPE (named for the model) | SUB | NOISE (named for the model) |
| NOISE | LEVEL (0 = off) | COLOR (brown → white) | RATE (how fast the band wanders; how often a gust) | BANDPASS (wide → narrow on the newest note → past half, breath on every voice) |
| NOISE 2 | TYPE (hiss → rain → crackle) | GUST (the swells now and then) | DUCK (under a chord) | WIDTH (its own) |
| ENVELOPE | ATTACK (ms below 1 s) | RELEASE | AFTERTOUCH | GLIDE |
| LEAD | ATTACK | RELEASE | TRANSPOSE (semitones up) | LEVEL |
| LFO | TREM RATE | VIBRATO (to 40 cents either way, DRIFT its rate: 3.6 to 6.8 Hz) | RANDOM | WIDTH |
| VOICES | SPREAD (the voices unlike each other) | CROSS MOD (pairs bending pitch) | TIMBRE DRIFT (each wandering) | PAN DRIFT (each across the field) |
| FX ENSEMBLE | ENSEMBLE (mode) | ENS MIX | – | – |
| FX DELAY | TYPE: PING PONG (left, right), TAPE (centre, worn, wavering), DUB (one echo, darker and ringing more each pass, wide; near full FDBK it sits and drones), REVERSE (swells; a held chord breathes in), OCT UP (octaves up), OCT DOWN (octaves down), GRANULAR (pieces scattered, at the note, an octave either way or a fifth up) | DLY TIME (50 to 800 ms; from 100 the beat's division 1/16 1/8 1/4T 1/8D 1/4 1/4D 1/2, from ARP's TEMPO or the MIDI clock, halved when too long; GRANULAR: how often a piece) | DLY FDBK | DLY MIX (full: the first echo about as loud as the note) — a DLY TIME move takes the repeats out for 12 ms and lands them on the new time, so it no longer swoops; TAPE still slides, inside a tone and a half |
| FX DELAY 2 | TONE (repeats dark and full to bright and thin) | DIFFUSION (repeats smeared into a cloud) | WOW (the tape's wander, clean to seasick) | GATE (only a slice of each DLY TIME goes in: a held chord pulses) |
| FX REVERB | DECAY (0.6–8 s) | DAMPING | SHIMMER (the octave up read twice, 35 ms apart, one a side: wide) | REVERB (the send, as on the FX mixer) |
| GEN | MODE | WALK, SHIFT, RANDOM, PROG, LOOP: RATE (0.1–60 s; under MIDI clock in beats, shown; LOOP from 1 s; no destination shown under 0.5 s); ARP: TEMPO (BPM, EXT on MIDI clock) | CHANCE; RANDOM: VOICES; ARP: GATE; LOOP: DECAY | RANGE; SHIFT: RANGE; ARP: PATTERN; PROG: CHORDS (ROUND, UNDERTOW, SLOW RISE, TWO ROOMS, OPEN DOOR, COLOUR, BREATH, STONES - 8 to 15 steps, unequal lengths, the chord led one or two voices at a time while the rest hold, nothing pinned - LOCK a voice yourself for a pedal); LOOP: OCT JUMP. REC on this page with LOOP showing records a phrase from the keys (LATCH lifted for the take, the rhythm AND each note's length kept, REC again to close and play; closing sets RATE to the phrase's own length; DECAY 0 never fades, recorded or caught). Selecting LOOP with nothing recorded says REC THEN PLAY |
| GEN 2 (the mode's knobs, the rest empty) | RETURN (WALK, SHIFT, RANDOM) | ROAM (PROG: 0 plays the table as written, up develops it; LOOP: how far each pass travels); OCTAVES (ARP) | EUCLID (ARP: 8/8 5/8 3/8 7/12 5/12 4/9 7/16 5/16, hits spread evenly, rests between) | DIV (ARP: 1/16 1/16T 1/8 1/8T 1/8D 1/4 1/4T 1/2 1 BAR of TEMPO's beat) |
| MIX (GLO) | BED (everything latched) | LEAD (what you play over it) | OUT (a trim under the MASTER pot, speaker and USB alike) | – |
| FX MIX (GLO, SELECT once) | ENS MIX | DLY MIX | REVERB | SHIMMER — every effect's level on one screen |
| GLOBAL (GLO, SELECT twice) | SCALE | ROOT | MORPH | TEMPO (40–240 BPM, EXT on MIDI clock; ARP steps at DIV of it and a synced DLY TIME follows it) |
| MIDI (the view shows the Bluetooth device: CONNECTED and its name, or NOT CONNECTED and the devices found, searched for while the page is up) | IN CH | OUT CH | LOCAL | VELOCITY (on by default) |
| USB | SPEAKER (ON; AUTO: silent while a computer records; OFF) | USB LEVEL (the recording, 0 to -18 dB; 0 by default, so it matches the speaker) | CLOCK OUT (24 a beat while ARP runs from TEMPO) | – |


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
Every model once, every GEN mode on two, every delay type on one; chords on C, in ROOT; SEL held + PRESETS brings the chord.
F1 FIRST LIGHT (ADDITIVE, WALK) · F2 VOID TIDE (SUPERSAW, WALK, wind) · F3 GLASS ORBIT (FM, WALK, OCT UP; made for GEN) ·
F4 BOWED SKY (BOWED, SHIFT, REVERSE) · F5 PULSAR (SYNC, ARP 5/8, gated PING PONG) · F6 NEBULA (ADDITIVE cluster, RANDOM,
GRANULAR, wind) · F7 DEEP VOID (TAPE, one low note, LOOP, TAPE delay, crackle) · F8 STARLIGHT (WAVETABLE, WALK) ·
F9 CATHEDRAL (FORMANT, PROG) · F10 RAIN CLOCK (FM 4-OP, ARP, DUB, rain) · F11 GAMELAN (BELL, RANDOM, gated
PING PONG) · F12 BRONZE BOWL (RESONATOR, WALK, GRANULAR) · F13 ASCENT (SHEPARD, SHIFT, OCT UP) · F14 DESCENT (SHEPARD,
PROG, OCT DOWN, wind) · F15 FOLDED SUN (WAVEFOLD, LOOP, TAPE delay) · F16 INIT (every value at its start, no chord).
U1..U16 are yours. Switching on: the name falls into a vortex (3.4 s; any key, button or knob skips it, and does what
it does), then the last preset's sound in silence, no chord.

## MIDI
In: notes hold while on (CC 64 keeps them), any channel by default; clock times GEN. Out: key down = note on at the
struck velocity (VELOCITY only decides whether the engine listens), let go = note off (later if latched), GEN moves too.
Every knob sends its CC as it turns and takes it (a CC in never lights SAVE); few-valued knobs spread over 0..127:
2 SCALE · 3 ROOT · 5 GLIDE · 9 MODEL · 15 CHORDS · 51 ROAM · 16–19 TIMBRE DRIFT REVERB TREMOLO (74, 91 in) · 20 NOISE LEVEL ·
21 SUB · 22 NOISE (OSC) · 23–25 COLOR RATE BANDPASS · 26–29 TYPE GUST DUCK WIDTH · 30 AFTERTOUCH · 52–53 LOOP DECAY, OCT
JUMP · 54 BED · 55 EUCLID · 56 DIV · 71 SHAPE · 72 RELEASE · 73 ATTACK · 75–78 LEAD ATTACK RELEASE TRANSPOSE LEVEL ·
79–82 TREM RATE VIBRATO RANDOM WIDTH · 83–86 SPREAD CROSS MOD TIMBRE DRIFT PAN DRIFT · 87 ENSEMBLE · 93 ENS MIX · 88 DLY
TIME · 89 DLY FDBK · 94 DLY MIX · 102 DLY TYPE · 90 TONE · 105 DIFFUSION · 103 WOW · 104 GATE · 106 DECAY · 107 DAMPING ·
108 SHIMMER · 109–113 MODE RATE CHANCE RANGE RETURN · 114 SHIFT RANGE · 115 VOICES · 116–119 TEMPO GATE PATTERN OCTAVES.
Switches (64 up = on): 57 LATCH · 58 GEN · 59 FREEZE · 60–63 ensemble, delay, reverb, shimmer · 12 the noise layer.
Triggers (64 up): 13 the preset as saved (HOME held) · 14 GEN's next move now (SEL + PLAY). 1 vibrato · 7 volume · 123 all off.
NRPN (99 · 98 = the voice's note · 6), in and out: 1 LEVEL (0 MUTE, 1–31 = −24..+6 dB) · 2 TONE (64 ± 50) · 3 TUNE (64 ± 50 cents) · 4 LOCK (64 up ON).
Program change 0–15 = F1–F16, 16–31 = U1–U16: the sound, as PRESET (never a chord).
SysEx (USB): F0 7D 44 46 01 ss F7 asks for slot ss (64 every U, 65 every slot); each comes as F0 7D 44 46 02 ss 05 data F7
(118 bytes packed 7-to-8); send one back to load user slot ss (the bank saves a second after the last). tools/fm1_bank.py dump / load / show.
