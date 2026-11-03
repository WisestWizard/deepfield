# DEEP FIELD

DEEP FIELD is an ambient drone machine for the M-VAVE FM-1. Every key strikes a *body*: a voice that swells in
while the key is held, stays when you latch it, breathes, drifts, and fades out when you let it go. Eight bodies at
a time. Four
forces on the knobs shape them all at once. ORBIT moves them to neighbouring notes by itself. The reverb freezes
and shimmers. Scenes recall everything and morph into each other. The screen shows the bodies as planets round a
sun, as a nebula of their harmonics, or on a radar scope. MIDI in and out over USB, Bluetooth (both ways) and the
TRS jack.

![The screen at rest: F6 NEBULA in the header with the SEL LATCH pill lit, four bodies as beams over the keys strip, the HOME page's four forces in a row below](img/screen-home.png "DEEP FIELD at rest")

---

## Contents

1. Install
2. The first minute
3. The screen
4. The keys: bodies, LATCH, the scale
5. The four forces (HOME)
6. The pages
7. ORBIT
8. FREEZE and the effects layer
9. Scenes: browsing, morphing, saving
10. The views
11. MIDI and Bluetooth
12. The screen reader
13. Troubleshooting

---

## 1. Install

**From a terminal:** `python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc`, with the FM-1 connected by
USB. Hold OCT− (or OCT− and OCT+ for five seconds) at power-on for update mode.

**From the browser:** in Chrome or Edge, the installer page writes the same package; it can put M-VAVE's own
firmware back too.

Installing is at your own risk. An FM-1 that only shows up as the chip's `WL82 UBOOT1.00` disk is recovered with
felucca-bsp's `fm1_rescue`.

---

## 2. The first minute

1. Power on. The scene F1 FIRST LIGHT loads and its three bodies swell in. The screen shows them as beams rising
   from their keys; SEL is lit, and the SEL LATCH pill with it: LATCH is on, so a key strikes a body that stays.
2. Press a few keys, one at a time. Each body swells in and stays; its key lights and its beam rises.
3. Press one of those keys again. That body fades out; its key dims while it fades and its beam sinks.
4. Turn KNOB 1 (MASS): every body gets brighter and darker, and the value shows large over the beams. KNOB 3
   (SPACE) pushes them away. The four cells under the beams are your four knobs.
5. Press PLAY. ORBIT lights green: every ten seconds or so one body slides to a neighbouring note of the scale,
   and the screen says where it went.
6. Press SEQ. FRZ lights: the reverb holds what it has. Press SEQ again to let it move.
7. Turn PRESET. The scene names scroll large; the one you stop on morphs in over four seconds (the bar under the
   header shows the morph).
8. Hold SEL for a second: every body is let go. Tap SEL: LATCH goes off, and a key now sounds only while held.

---

## 3. The screen

The screen mirrors the panel: what is on it sits where your hands are.

| Part | What it shows |
| --- | --- |
| Header | The scene's slot (F1..F16, U1..U16) and name (orange while it has unsaved changes). At the right: OCT when the octave is not zero, BLE and DEV when Bluetooth is connected, bodies sounding and CPU; LOAD in orange if the render ever ran out of time (a body is shed). Below, the three states as pills named for their buttons, SEL LATCH, PLAY ORBIT and SEQ FRZ, filled when on, and the letters E D R S lit for the effects heard. A bar along the bottom edge while a scene morphs in. |
| The view | The bodies, as beams over their keys or in one of the other views (section 10). Any knob or button shows its name and value large here for a moment. With FX held, the switches are named over their keys. |
| The keys strip | The 27 keys as they are lit: a body's colour while it sounds, dim while it fades, lighter when in the scale, a yellow frame while latched. |
| Footer | The page band: the button that opened the page, its name, and the SELECT arrows (`<` and `>`) for the pages before and after; the model and the scale at the right. Then the four knobs in a row, under the four real ones: name, value, bar. |

---

## 4. The keys: bodies, LATCH, the scale

A **body** is one voice. There are eight. A key strikes one at the key's note; it swells in over SWELL seconds to
the weight the key landed with (velocity), and leaning on the key (pressure) brightens it and adds wobble while
you lean.

**LATCH** (the SEL button, lit while on; on from the factory): a key strikes a body that stays after the key is
released, and the same key pressed again lets it go (it fades over FADE seconds). Its key lights while it sounds
and dims while it fades; on the keys strip a latched key has a yellow frame. Tap SEL to switch LATCH off: a body
then sounds while its key is held and fades when it is released; a body that was latched goes on sounding only if
its key is held at that moment, and the others are let go. Hold SEL for a second to let every body go.

A scene's bodies come in latched, and loading one switches LATCH on. The keys in the scale glow.

**VELOCITY** (MIDI page) is off by default, so every body lands at the same weight and a chord is even; on, a key's
velocity is its body's weight.

A ninth body steals the oldest one (it fades quickly and the new one swells). **Every body is let go** by holding
SEL for a second (the screen says EVERY BODY LET GO).

**The scale.** SCALE on the GLOBAL page picks a scale (CHROMA, MAJOR, MINOR, DORIAN, PHRYGIA, LYDIAN, MIXO,
PENTA+, PENTA−, WHOLE, HARM−) and ROOT its root. A key outside the scale plays the scale note just below it, so
every key is in tune. CHROMA plays the keys as they are. The scale also bounds ORBIT's moves.

**OCT−, OCT+** move the keys by octaves (−2 to +2). The button for the direction half-lights one octave out and
lights two out, and the header says OCT±N.

---

## 5. The four forces (HOME)

HOME is the page the knobs rest on. Each force shapes every body at once.

| Knob | Force | What it does |
| --- | --- | --- |
| KNOB 1 | MASS | How much of the body is there. SOLAR: how many harmonics and how bright; VOID: the filter's cutoff (shown in Hz); GLASS: the FM index; STRING: the string's brightness. Pressure on a key adds to it for that body. |
| KNOB 2 | DRIFT | How much everything wanders: detune, the random walk of pitch, the wobble, the breathing of the harmonics, and how fast the stars go by. |
| KNOB 3 | SPACE | How much of the hall the bodies are heard in: the reverb send (and a little more delay). The dry sound stays as it is. |
| KNOB 4 | TIDE | How much each body ebbs and swells on its own slow cycle. At full, each body goes all the way down and back. TIDE RATE (MOTION page) sets the cycle. |

MASTER is the volume.

---

## 6. The pages

A button opens its page in the footer; the knobs then set those four values, shown in a row under the knobs. A
button always lands on the same page, and the page's button lights. SELECT turns to the page before or after, in
the order of the table, so DELAY and REVERB are a click or two past ENSEMBLE and MIDI one past GLOBAL; the page band shows `<`
and `>` where SELECT can go. A page stays until a button or SELECT moves it. Every turn of a knob shows its name
and value large over the view.

| Button | Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
| --- | --- | --- | --- | --- | --- |
| HOME | HOME | MASS | DRIFT | SPACE | TIDE |
| EDIT | BODY | MODEL (also on the ALGO encoder) | SHAPE | SUB | AIR |
| EDIT, then SELECT | NOISE | LEVEL (0 is off) | COLOUR (brown to white) | WIND (how fast the band wanders) | BAND (a wide bed at 0; narrowing, it sits on the newest body's note) |
| ENV | ENVELOPE | SWELL (0.03 s to 20 s) | FADE (0.03 s to 20 s) | PRESS (how much pressure brightens) | GLIDE (how long an ORBIT move takes) |
| LFO | MOTION | TIDE RATE | WOBBLE (vibrato) | WALK (the random walk of pitch) | WIDTH (stereo spread, by note) |
| FX | ENSEMBLE | ENSEMBLE (OFF, CHOR I, CHOR II, ENSEMBLE) | ENS MIX | – | – |
| FX, then SELECT | DELAY | DLY TIME | DLY FDBK | DLY MIX (the delay's level) | – |
| FX, then SELECT twice | REVERB | REV SIZE (the decay, 0.6 s to 8 s) | REV DAMP (how fast the highs die) | SHIMMER | SPACE (the send, the same knob as on HOME) |
| ARP, or PLAY held | ORBIT | EVERY (1 s to 60 s between moves) | CHANCE | STEPS UP TO (1 to 4 scale steps) | HOME PULL |
| GLO | GLOBAL | SCALE | ROOT | VIEW | MORPH (0 to 30 s; NOW) |
| GLO, then SELECT | MIDI | IN CH (OMNI, 1..16) | OUT CH (1..16) | LOCAL | VELOCITY |

**The models** (ALGO, or KNOB 1 on BODY): SOLAR is eight harmonics that breathe, each on its own slow cycle; SHAPE
moves from odd harmonics (hollow) through all of them to even ones (organ-like). VOID is three detuned saws and a
sub through a dark filter; SHAPE is the detune. GLASS is two-operator FM with a breathing index and a detuned
second carrier; SHAPE picks the ratio (1, 2, 3, 3.5, 4, 5, 7, 1.41). STRING is a bowed string: noise (AIR) and a
sine bow a feedback line; MASS is its brightness. CHOIR is a buzz through three vowel formants: SHAPE morphs the
vowel through A, E, I, O and U, MASS shifts the formants up and brings the upper two forward, AIR is breath.
BELL is four partials, each breathing on its own: SHAPE stretches them from the harmonic series to a bell's
ratios, MASS brightens the upper partials, AIR a dark hiss of the strike. SHEPARD is six octaves of sine under a
moving window, the endless rise: SHAPE below the middle falls, above it rises, further from the middle faster,
at the middle it stands still; MASS leans the window high or low. FOLD is a sine through a wavefolder: SHAPE is
how hard it folds, MASS the filter after it, AIR noise into the fold, and DRIFT makes the fold breathe. Changing
the model restarts the bodies through a short fade.

![The BODY page: MODEL STRING, SHAPE, SUB, AIR](img/screen-body.png "The BODY page")

![The NOISE page on F6 NEBULA: LEVEL, COLOUR, WIND, BAND, the wind bed as a soft beam over the keys](img/screen-noise.png "The NOISE page")

SUB adds an octave below on every model. AIR is breath on STRING and a little noise colour on the others' spectra
shown in NEBULA.

---

**The noise layer** (the NOISE page, EDIT then SELECT) is a stereo noise under the bodies, or alone with no key
held: wind, sea, breath. LEVEL is its switch (0 is off) and its loudness; COLOUR runs from brown to white; WIND is
how fast its band wanders; BAND is the band's width, a wide bed at 0 and, past a third, a narrowing band that
moves onto the newest body's note, so it breathes with the chord. The forces shape it too: MASS brightens it,
DRIFT widens the wander, SPACE sends it to the hall, TIDE breathes it on its own cycle; leaning on a key brightens
it, and it swells and fades at SWELL and FADE like a body. On the FX layer, C4 switches it off and back to its
level (from zero, to a third), and N lights in the header while it is heard. CC 20 is its LEVEL.

## 7. ORBIT

PLAY runs ORBIT: the button lights green, the screen says ORBIT ON, EVERY 5S, and the PLAY ORBIT pill fills
toward the next move and flashes when it comes. While it runs, EVERY so many seconds (give or take a quarter),
with CHANCE, one latched body moves to a neighbouring note of the scale, up to STEPS UP TO scale steps away,
gliding there over GLIDE seconds (ENVELOPE page). With HOME PULL, a body that has wandered is pulled back to the
key it was struck at. Bodies held over MIDI are left alone. Each move goes out over MIDI as a note off and a note
on; the screen says where the body went (E4 > F#4), the body's beam slides to the new key, and its key light
walks with it (the home key stays half-lit). Press PLAY again to stop; the bodies stay where they are.

Hold PLAY (or press ARP) for the ORBIT page.

![The ORBIT page: EVERY 5S, CHANCE, STEPS UP TO, HOME PULL](img/screen-orbit.png "The ORBIT page")

On the ORBITS view a moved body shows a bright point beside it and its orbit's radius changes as it glides.

---

## 8. FREEZE and the effects layer

**FREEZE** (SEQ, lit while on): the hall holds what it has, with no decay, no damping and nothing new going in,
and comes up towards the level the dry sound had. The bodies keep sounding dry (and through the delay). Press SEQ
again to release it. A frozen view is framed in ice.

![Frozen: the ice frame round the beams, SEQ FRZ lit, ORBIT running](img/screen-frozen.png "FREEZE on")

**SHIMMER** (the REVERB page) adds the reverb's output an octave up, heard at once and fed back into the reverb: a
rising cloud. The feedback is held where it cannot run away, whatever the size.

**The effects layer:** hold FX and the four lowest white keys show and switch the effects. F3 ensemble, G3 delay,
A3 reverb, B3 shimmer, and C4 the noise layer; while FX is held the screen shows the five switches large, ON or
OFF (the noise with its level), in the order of their keys, and the keys light teal while heard. The switches are part of the scene (SAVE keeps them), and the letters
E D R S N in the header show what is heard at any time. A key pressed while FX is held starts no body. A tap of FX
with no key pressed opens the effects pages instead: ENSEMBLE, then SELECT for DELAY and REVERB, one page per
effect in the same order as the keys.

![FX held: ENS, DLY, REV and SHM large, ON or OFF, over the four lowest white keys](img/screen-fxlayer.png "The effects layer")

![The REVERB page: REV SIZE, REV DAMP, SHIMMER, SPACE](img/screen-fx-reverb.png "The REVERB page")

---

## 9. Scenes: browsing, morphing, saving

A scene is every parameter and the bodies that were latched when it was saved. Slots F1..F16 are the factory scenes,
U1..U16 are yours.

**Browsing:** turn PRESET. The scene names scroll large over the view; the one PRESET rests on loads, morphing in
over MORPH seconds (GLOBAL page, 4 s by default; NOW switches at once): the values crossfade, its bodies swell in,
the old scene's latched bodies fade, a body in both stays. A bar along the bottom of the header shows the morph.
A MIDI program change does the same.

**Saving:**

1. Press SAVE. The footer says SAVE SCENE TO and a user slot.
2. Turn PRESET (or SELECT, or KNOB 1) to pick the slot U1..U16.
3. Press SAVE again. The live scene with the bodies latched right now goes into that slot, the settings are saved
   with it, and the header says SAVED. (Or press HOME to cancel.)

![The save dialog: SAVE SCENE TO U1 USER 1](img/screen-save.png "Saving a scene")

The settings (octave, scale, root, LATCH, the view, MORPH, the MIDI channels, Bluetooth on or off) are saved
along with every scene save, and the effect switches with the scene. SAVE lights while there is something unsaved.

Factory scenes: F1 FIRST LIGHT (SOLAR, a warm major), F2 VOID TIDE (VOID, low and breathing), F3 GLASS ORBIT
(GLASS with shimmer and delay, made for ORBIT), F4 BOWED SKY (STRING, wide), F5 PULSAR (GLASS, fast tide),
F6 NEBULA (SOLAR, hollow, very slow swells, drowned, a wind bed), F7 DEEP VOID (VOID, sub and long delay, a low
wind), F8 STARLIGHT (STRING, bright and airy, high), F9 CATHEDRAL (CHOIR, an open vowel in the biggest hall),
F10 LOW CHOIR (CHOIR, a closed vowel low, sub and a wind), F11 GAMELAN (BELL, bell ratios, shimmer, made for
ORBIT), F12 ICE BELLS (BELL, high and wide, white air), F13 ASCENT (SHEPARD rising), F14 DESCENT (SHEPARD
falling, dark, a low wind), F15 FOLDED SUN (FOLD, bright, ensemble), F16 TIDE POOL (FOLD, dark, deep tide,
delay).

---

## 10. The views

VIEW on the GLOBAL page picks the view. BEAMS is the one to play by; the others are skins over the same keys.

| View | What you see |
| --- | --- |
| BEAMS | Every body a beam rising from the key that struck it, in its colour: its level is the height, so you watch it swell, breathe with TIDE and sink as it fades (dim). A white cap on a latched beam: it stays. ORBIT slides a beam to the neighbouring key and leaves a mark at the key it was struck at. A body below or above the keys (a MIDI note, or after an octave change) stands thin at the edge. The noise layer is a soft, wide beam where its band sits, brown to white, that drifts with WIND. Stars drift behind at DRIFT's speed; FREEZE frames the view in ice. |
| MOUTH | An orange RX bottle on the left pops its cap and tumbles out a pill for every new body; a mouth on the right (red lips, chunky teeth) opens with the output. The pills hang over their keys, high notes high, two-tone (its colour and white; a yellow outline once latched). A body that is let go flies into the mouth and is eaten; now and then the mouth takes a snack and spits the pill back out. |
| ORBITS | A sun that pulses with the output. Each body is a planet on an orbit: the radius is its pitch, its speed its breathing, its size its level, its colour its pitch class. A fading body dims. Stars drift behind at DRIFT's speed. |
| NEBULA | The bodies' harmonics as glows along a wavy band, low notes left, high right, coloured by body, with a mark under each fundamental. |
| SCOPE | A radar: each body a blip at its pitch class round the dial and its octave along the radius, lit as the sweep passes; the output's left channel against its right as a figure. Width and beating show as the figure's shape. |

![MOUTH: the bottle, four pills over their keys, the mouth](img/screen-mouth.png "The MOUTH view")

![ORBITS: four bodies as planets round the sun](img/screen-orbits.png "The ORBITS view")

![NEBULA: the harmonics of four bodies as coloured glows](img/screen-nebula.png "The NEBULA view")

![SCOPE: the radar with the bodies as blips and the output as a figure](img/screen-scope.png "The SCOPE view")

Everything in the view but MOUTH is drawn on a phosphor that fades, so motion leaves trails.

---

## 11. MIDI and Bluetooth

MIDI comes in over USB, Bluetooth and the TRS jack, on IN CH (OMNI by default). A MIDI note holds a body while it
is on and lets it go at its note off (the sustain pedal, CC 64, keeps them). Poly and channel pressure brighten.
The keys go out over USB and Bluetooth on OUT CH: a key down is a note on, and the note off goes when its body is
let go (at the key's release, or later if it was latched). ORBIT's moves go out too. LOCAL off keeps the keys from the engine: they only send MIDI.

Controllers: 1 wobble (wheel), 7 volume, 64 sustain, 16 MASS, 17 DRIFT, 18 SPACE, 19 TIDE, 20 NOISE LEVEL, 74 MASS, 71 SHAPE,
72 FADE, 73 SWELL, 91 SPACE, 93 ENS MIX, 94 DLY MIX, 120 silence now, 121 controllers to rest, 123 every body let
go. A program change loads a scene (0 = F1, 16 = U1).

**Bluetooth:** Bluetooth MIDI starts with the FM-1. A computer, phone or tablet connects to it as DEEP FIELD FM-1.
To connect the FM-1 to a Bluetooth MIDI keyboard or controller:

1. Press REC. The header says BLE SCANNING, then names each device found; turn ALGO to pick one.
2. Press REC again to connect (CONNECTING, then CONNECTED). Press REC once more to let it go.

Hold REC for a second to switch Bluetooth off or on (kept with the settings).

---

## 12. The screen reader

With fm1-reader listening, every message is spoken, and asking for the screen reads the scene, the page and its
four values. The words file is `words.tsv`.

---

## 13. Troubleshooting

| Problem | Likely cause |
| --- | --- |
| A key does nothing | LATCH is on (SEL lit) and that body was sounding: the press let it go. Or LOCAL is off (MIDI page). |
| A body stops when I let the key go | LATCH is off (SEL dark). Tap SEL. |
| A key plays the wrong note | The scale: the key snapped to the scale note below. SCALE (GLOBAL page) to CHROMA plays the keys as they are. |
| The knobs do something else | Look at the page band above them: another page is open. HOME is one press. |
| Bodies move by themselves | ORBIT is running (PLAY lit green, the PLAY ORBIT pill filling). |
| The reverb will not die away | FREEZE is on (SEQ lit). |
| No sound from a silent scene | TIDE at full ebbs each body to nothing on its cycle; the morph is still in progress. |
| LOAD in the header | The render ran out of time once; one body fewer until it has been calm for a while. |
