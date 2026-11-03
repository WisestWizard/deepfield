# Drone and ambient instruments: the research, the evaluation, and DEEP FIELD's point of view

Written 2026-10-31 (overnight) for the FM-1 drone firmware. Part A is the survey (web research, with sources).
Part B evaluates each family for live jamming. Part C is the position DEEP FIELD takes and how it maps onto the
FM-1's 27 keys, 7 encoders, 14 buttons and 240 x 240 screen. Part D is what the screen does and why.

The constraint behind every choice: 44.1 kHz stereo, a float-only FPU, 96 KB of RAM plus a 336 KB pool, Bluetooth
MIDI resident (45 KB of RAM, 24 KB of pool), about eight voices of DSP, and a player whose hands are on the keys
and knobs, not on a menu.

---

## A. The survey

### A.1 Catalogue

| Name | Platform | Synthesis core | How it is played live | Screen | Open? | Price |
| --- | --- | --- | --- | --- | --- | --- |
| Mutable Instruments Plaits (v1.2) | Eurorack, STM32F3 | 24 macro models: VA, folder, 2-op FM, formant, additive, wavetable, chords, speech, grains, noise, mini-Rings, drums, DX7 FM, wave terrain | HARMONICS / TIMBRE / MORPH, meaning per model; internal LPG; two models drone without a trigger | none | MIT | ~$250 |
| Mutable Instruments Rings | Eurorack | modal, sympathetic strings, inharmonic Karplus-Strong; 1/2/4 voices, notes overlap | STRUCTURE / BRIGHTNESS / DAMPING / POSITION; strum a chord by overlap | none | MIT | ~$250 |
| MI Clouds (+ Parasites, Kammerl) | Eurorack | granular 1 to 8 s buffer (µ-law for the long one), FREEZE; Parasites adds Oliverb and Resonestor | POSITION / SIZE / PITCH / DENSITY / TEXTURE / BLEND, Freeze button | none | MIT | ~$300 |
| Monome Norns drone scripts (Dronage, nc01 "drone in three worlds", planetary, dnd, Grendy, Jala, otis, SMASH) | Pi + SuperCollider, Lua | Plaits-like voices with Euclidean swells (Dronage); sines + noise (dnd); softcut layers | 3 encoders, 3 keys; the nc01 brief: E1 volume, E2 brightness, E3 density, K2 evolve, K3 world | 128 x 64 OLED, a visual per world | per script | Norns ~$900, shield ~$150 |
| Dirtywave M8 | Teensy 4.1 | tracker: wavesynth, macrosynth (Plaits), FM, sampler | 8 buttons, tables and chains | 2.8" IPS, dense text | closed | $600+ |
| Organelle + Orac ("Drone 1", Wasis, Dust) | Pi + Pure Data | Pd patches | 25 keys, 4 knobs, 1 encoder | 128 x 64 OLED, one line of text | patches open | $600 |
| Bastl Kastle 1.5 / Kastle Drum | 2 x ATtiny85 | PM, noise, track and hold; stepped random generator | 6 knobs, patch wires | none | open | ~$80 |
| Daisy Seed projects (DaisyDrone, Audrey-II, Hothouse) | STM32H7 | 18 sines + wavefold; Karplus-Strong feedback drone | sliders, pedal knobs | none | MIT | $30 to $200 |
| Soma Lyra-8 | analogue | 8 organ-style oscillators in pairs, Hyper-LFO, dual modulated delay, Total FB | 8 metal touch sensors, HOLD per group (slow or fast envelope), no keyboard | none | no | ~$840 |
| Soma Pulsar-23 | analogue | 4 drum/bass voices, looper, delay + reverb | 100+ pins, clips, touch | none | no | ~$2000 |
| Landscape Stereo Field | analogue | 4 stereo feedback circuits | 8 touch plates, skin conductivity | none | no | $445 |
| Folktek Mescaline / Conduit / Matter | analogue | touch-key poly; a delay feeding a filter | touch plates, pin matrix | none | no | $300 to $900 |
| Make Noise Strega / 0-Coast | analogue | triangle to pulse oscillator, "time/filter" feedback delay; west-coast folder + LPG | Tonic, Tone, Activation, Blend, Decay, Agitation touch plates | none | no | $599 / $499 |
| Ciat-Lonbarde Cocoquantus / Plumbutter | analogue + 8-bit | 5 chaotic oscillators into dual 8-bit loopers | bananas, touch nodes | none | no | $800 to $1500 |
| Moog Subharmonicon / Spectravox | analogue | 2 VCOs + 4 subharmonic dividers + polyrhythm; 10-band filter bank | knobs, dividers; Spectral Shift | none | no | $700 / $599 |
| Lorre-Mill Double Knot; Error Instruments Drone Bank | analogue | 2 zero-attack voices + generative sequencer; 4 triangles that modulate each other | bananas; knobs and light sensors | none | no | ~$699; $100 to $400 |
| Hologram Microcosm | DSP pedal | 11 granular / micro-loop / glitch / multidelay families, looper | Activity, Repeats, Shape, Space, Time, Mix, Filter, Hold | none | no | $449 |
| Chase Bliss Mood MkII / Blooper | DSP pedals | always-listening micro-looper + reverb/delay; 8-layer looper with modifiers | knobs, dip switches | none | no | $399 |
| Strymon NightSky / BigSky | DSP pedals | reverb + Shimmer (±1 oct, input or regen) + Freeze / Infinite + 8-step sequencer | knobs, Hold footswitch | none | no | $429 / $479 |
| Empress Zoia | DSP pedal | ~90 modules, virtual modular | 8 x 5 button grid | small OLED + RGB grid | no | $499 |
| Red Panda Particle 2 / Meris Hedra / OTO BAM | DSP | granular delay with auto-freeze; 3-voice pitch shifter; 7 reverb algorithms | knobs, momentary freeze | none | no | $300 to $500 |
| Teenage Engineering OP-1 / OP-1 field | handheld | 11 engines, 4-track tape, Tombola; the "tape speed down" drone trick | keys + 4 colour-coded encoders | 320 x 160 OLED with an animation per engine | no | $1300 to $2000 |
| Yamaha Reface CS | keyboard | Multi-Saw with TEXTURE (sub) and MOD (detune) | sliders, no menus | none | no | $400 |
| Korg Volca Modular | semi-modular | 2 oscillators + folder, 2 function generators, Woggle sample-and-hold | pin patching, Randomize | 7-segment | no | $200 |
| Arturia MicroFreak | keyboard | Plaits engines, Harmonic OSC, 16-band vocoder; paraphonic | flat touch keys with pressure | small OLED | no | $350 |
| Critter & Guitari 201 / Pocket Piano | handheld | Harmonic Sweeper, Octave Cascade, FM arp | 18 keys, 3 knobs | one RGB LED | no | $200 |
| Ableton Drone Lab | Max for Live | 8-voice Harmonic Drone Generator, equal or just intonation | mouse | screen | no | $59 |
| Droneo (Henry Lowengard) | iOS | additive harmonic drone, "Tone Spiral" | touch spiral | spiral | no | ~$5 |
| Bloom / Scape / Reflection (Eno, Chilvers) | iOS | rule-based generative | tap to seed (Bloom); nothing at all (Reflection) | expanding circles | no | $4 |
| Borderlands / Tardigrain / Fluss / Spacecraft | iOS | granular | multitouch grain clouds, XY pads | the grains themselves | Borderlands GPL | $5 to $15 |
| Madrona Labs Kaivo / Aalto | desktop | granular + physical; west-coast | patchable UI | animated cords | no | $89 / $129 |

### A.2 Notes and sources, by family

**Open-source Eurorack firmware.** Plaits proves that three knobs cover two dozen engines when the screen (or the
manual) names the knobs' meaning per engine; its additive model gives HARMONICS = spectral bumps, TIMBRE = peak
harmonic, MORPH = bump shape, and two models drone without a trigger
([manual](https://pichenette.readthedocs.io/en/latest/modules/plaits/manual/),
[v1.2 models](https://synthanatomy.com/2022/12/mutable-instruments-plaits-new-firmware-adds-8-models-incl-dx-7-fm-synthesis.html)).
Rings' STRUCTURE / BRIGHTNESS / DAMPING / POSITION is the clearest "one knob, one dimension" vocabulary in
hardware, and its polyphony where notes overlap rather than cut is chord memory without a sustain pedal
([manual](https://pichenette.readthedocs.io/en/latest/modules/rings/manual/),
[review](https://ask.audio/articles/review-mutable-instruments-rings)). Clouds' FREEZE and its µ-law 8 s buffer show
how to hold seconds of sound in tens of kilobytes; the common complaint is "what does this knob do now"
([Softube manual](https://cdn.softube.com/user-manuals/mutable-instruments-clouds),
[Parasites](https://www.synthtopia.com/content/2016/03/01/mutable-instruments-alternate-firmware/),
[Kammerl](https://www.gearnews.com/mutable-instruments-clouds-gets-beat-mangling-firmware-update-kammerl/)).

**Norns.** The community's "norns circle 01: drone in three worlds" brief is nearly a specification for this
firmware: E1 volume, E2 brightness, E3 density, K2 evolve, K3 world, a visual per world
([brief](https://llllllll.co/t/norns-circle-01-drone-in-three-worlds/28582),
[repo](https://github.com/monome-community/nc01-drone)). Tom Armitage's *planetary* draws its three worlds with
boxes, lines and circles scrolling at different speeds per layer, and he writes that the screen "is treated as part
of the instrument, not decoration"; half its 300 lines are graphics ([infovore](https://infovore.org/tag/instruments/)).
Dronage is an "ambient drone workstation" of four voices with fades and a Euclidean trigger per voice
([lines](https://llllllll.co/t/dronage-norns/74914)); dnd is three sines and noise
([lines](https://llllllll.co/t/dnd-drone-drama-for-norns/50568)). The recurring criticism of the platform:
"scripts are usually made for a specific use case by a specific person"
([Elektronauts](https://stage2.elektronauts.com/t/norns/55444?page=12)).

**Trackers and patchers.** The M8 is the anti-pattern for ambient on a small screen: "if I have to squint to read the
display it's a no-go" ([Elektronauts](https://www.elektronauts.com/t/m8-tracker/125103?page=235)). Organelle's one
line of text plus keys and four knobs is the "no menus" baseline that people like
([patches](https://www.critterandguitari.com/organelle-patches?7d19f03b_page=6)). Teletype and Orca are text
live-coding: not hands-on.

**Small MCU boxes.** Kastle's 8-bit phase modulation plus a stepped random generator makes convincing drones on two
ATtinys ([Kastle 1.5](https://synthanatomy.com/2017/12/bastl-instruments-kastle-1-5-is-now-black-powered-by-micro-usb-features-3-new-synthesis-modes.html),
[Kastle Drum](https://bastl-instruments.com/instruments/kastle-drum)). On Daisy, cutlasses' DaisyDrone sums 18
sines under six sliders ([GitHub](https://github.com/cutlasses/DaisyDrone)) and Synthux's Audrey-II is a
Karplus-Strong "string feedback horrorscape" ([GitHub](https://github.com/Synthux-Academy/Audrey-II)).

**Lyra-8.** Voices in pairs sharing envelope, modulation depth and FM; HOLD per group with a slow (about 30 s) or
fast envelope; a Hyper-LFO of two square LFOs combined by AND / OR; two delay lines with shared feedback modulated
by the signal itself; Total FB. Praised: woodwind timbres, "one pair plays while another drones", the delay's
self-oscillation. Criticised: the pitch knob is stable only near the top, tuning while sounding is "disorienting",
Total FB is "destructive", the delays are noisy, nothing is quantised, nothing is stored, the mod knobs are "very
sensitive to the minutest turn", it is "un-repeatable" ([Waveform](https://waveformmagazine.com/waveform-reviews/soma-laboratory-lyra-8/),
[SOS](https://www.soundonsound.com/reviews/soma-laboratory-lyra-8),
[users](https://www.thomann.de/be/soma_lyra_8_white_angel_reviews.htm),
[vi-control](https://vi-control.net/community/threads/cute-wild-noise-boxes.121005/page-2)).

**Strega, Subharmonicon, the touch boxes.** Strega's legends are "deliberately cryptic"; it is "hard to make
anything boring" yet "can quickly become boring if used solo"
([SOS](https://www.soundonsound.com/reviews/make-noise-strega),
[users](https://www.thomann.pt/make_noise_strega_reviews.htm)). Subharmonicon is "perfectly suited to ambient
generative" but "if you are looking for melodic control you are best off with a different solution", and users
find it "hard to get musical results without understanding intervals"
([MusicRadar](https://www.musicradar.com/reviews/moog-subharmonicon),
[ModularGrid](https://modulargrid.net/s/forum/posts/index/13907)). Stereo Field is "messy, atonal, random"
([Gearnews](https://www.gearnews.com/caress-sounds-landscapes-stereo-field-touch-plate-instrument)). Double Knot's
zero-attack envelopes and Error Instruments' four mutually modulating triangles are loved for the same reason
Lyra is: the sound moves by itself.

**Pedals.** NightSky separates Freeze (a static tail) from Infinite (keeps adding), puts the shimmer's octave either
at the input or in the regeneration, and lets an 8-step sequencer repitch a frozen tail
([MusicTech](https://musictech.com/reviews/strymon-nightsky-review/),
[MusicRadar](https://www.musicradar.com/reviews/strymon-nightsky)). Mood's micro-looper "always listens" so a
freeze captures what just happened ([Chase Bliss](https://chasebliss.com/mood)); Particle freezes automatically on
a threshold ([SOS](https://www.soundonsound.com/node/4929475)). Microcosm is criticised for unclear knobs
("Activity", "Repeats") and for drifting into wash while you lose track
([Engadget](https://www.engadget.com/hologram-electronics-microcosm-guitar-effect-pedal-review-ambient-music-170002707.html)).
Zoia's grid is "dense at first... quick and tactile with practice"
([Waveform](https://waveformmagazine.com/waveform-reviews/empress-effects-zoia/)).

**OP-1, Reface, MicroFreak, Pocket Piano.** OP-1's per-engine animations are called "an absolute highlight" and
its colour-coded encoders tell you which knob does what; the OP-Z without a screen felt "unappealing"
([Gearnews](https://www.gearnews.com/teenage-engineering-op-1-field-review/),
[Elektronauts](https://www.elektronauts.com/t/teenage-engineering-op-z/37946?page=3),
[op1tips drone recipe](https://github.com/ratbag98/op1tips)). Reface CS's Multi-Saw (TEXTURE adds a sub, MOD adds
detune) with chorus is the Juno pad recipe ([MusicRadar](https://musicradar.com/reviews/tech/yamaha-reface-cs-629147)).
MicroFreak's paraphony makes its pads "brittle" ([MusicTech](https://musictech.com/reviews/arturia-microfreak/)).

**Software.** Drone Lab is an 8-voice harmonic drone with equal or just intonation
([Ableton](https://www.ableton.com/en/packs/drone-lab/)); Droneo is an additive drone played on a tone spiral
([App Store](https://apps.apple.com/app/id313811077)); Bloom seeds repeating tones that bloom as circles,
Reflection takes no input at all and varies forever ([Tape Op](https://tapeop.com/interviews/137/peter-chilvers)).
SmartDrone (NIME 2020) re-voices a drone to follow the player ([NIME](https://nime.org/proc/nime20_92)).

### A.3 The DSP techniques and what they cost on the FM-1

| Technique | Seen in | Cost on the FM-1 | Verdict |
| --- | --- | --- | --- |
| Additive harmonic drone, per-partial breathing | Droneo, Drone Lab, DaisyDrone, dnd | a sine table lookup per partial: 8 voices x 8 partials is trivial | in: SOLAR |
| Detuned saw stack through a dark filter | Reface CS Multi-Saw, MicroFreak Superwave | 3 DPW saws + 2 one-pole stages per voice | in: VOID |
| Two-operator FM with a slowly breathing index | Plaits, OP-1, Kastle | 3 table lookups per sample | in: GLASS |
| Karplus-Strong kept ringing by a bow (noise + a sine) with a soft limiter in the loop | Rings, Audrey-II | one int16 line per voice (4 KB), a one-pole, a cubic | in: STRING |
| Modal resonator bank | Rings modal | 16 SVFs per voice: fits, but takes the whole budget | out for now |
| Granular freeze of the input | Clouds, Microcosm, Particle | memory: 1 s at 8-bit µ-law = 22 KB | out: no audio input on the FM-1; the reverb freeze covers the drone use |
| Spectral freeze (phase vocoder) | Microcosm-like | FFT frames and tables: too much RAM and FPU | out |
| Plate reverb (Dattorro) with int16 lines | Pocket Poly's fx.c | 68 KB of pool, ~15 % of a core | in, with FREEZE |
| Shimmer: an octave-up granular shifter in the reverb's feedback | NightSky, BigSky | 8 KB line, two interpolated reads per sample | in |
| Tape delay with wow and flutter | Lyra's delays, Mood | 72 KB of pool | in (from Pocket Poly) |
| Juno ensemble | Reface CS + chorus, Lyra "made stereo with chorus" | 4 KB, cheap | in |
| Random walk / Woggle drift | Volca Modular, Strega, Jala | two multiplies per voice per tick | in: WALK, DRIFT |
| Scale lock | Plaits chords, SmartDrone | a 12-bit mask | in, on by default |
| Slow generative note changes with gravity home | Bloom, Reflection, Jala, Subharmonicon | a timer and a random pick | in: ORBIT |
| Swell instead of attack, a release in seconds | Lyra HOLD, Double Knot | a one-pole per voice | in: SWELL, FADE |
| Shepard / Risset glissando | criware, Cycling '74 | trivial | not yet (a candidate "view" and model) |
| Cross-FM between voice pairs, Hyper-LFO | Lyra-8 | cheap | not yet |
| Lo-fi "age" (sample-rate and bit reduction) | Mood's Clock, Lossy | trivial | not yet |

---

## B. Evaluation: each family for live jamming

| Family | What it gets right | Where it fails the jam |
| --- | --- | --- |
| Analogue touch boxes (Lyra-8, Strega, Stereo Field, Folktek, Ciat-Lonbarde) | the sound moves on its own; the gesture is a touch and a hold, not a note; feedback and self-oscillation are the instrument | nothing is in tune with anything else; nothing is recalled; a knob's sweet spot is a hair wide; they "need a delay with a filter" to sound finished; "boring if used solo" |
| Eurorack open firmware (Plaits, Rings, Clouds) | a few knobs with musical names; chords by overlap; freeze | they are modules: no keys, no scenes, no screen; "what does this knob do now" without a display |
| Norns scripts | the nc01 brief (volume, brightness, density, evolve, world) and a visual per world; sound toys that do one thing | each script is one person's use case; a 1-bit 128 x 64 screen; a laptop-class computer |
| Trackers and patchers (M8, Zoia, Orac) | depth | eyes on the screen, dense text, patching before playing |
| Pedals (NightSky, Mood, Particle, Microcosm) | freeze that captures what just happened; shimmer; infinite vs frozen as distinct states | they need an input; knobs with unclear names; "easy to lose track while turning knobs" |
| OP-1 | animations that explain the engine; colour-coded knobs; tape-speed drones | drones are a trick, not a mode; price |
| Generative apps (Bloom, Reflection, Drone Lab) | hands-off evolution; just intonation; self-seeding | no hands-on override; no instrument feel |

What people consistently praise: a hold that keeps playing while the hands move on; swells instead of attacks;
a handful of knobs whose names are musical and whose meaning stays put; a drift amount that lives on one knob;
a freeze; scenes; and a screen that shows the state of the sound rather than a list of numbers. What they
consistently dislike: states that cannot be recalled, pitch that is not in tune with anything, cryptic knobs,
dense text, and instruments that only come alive through external effects.

---

## C. DEEP FIELD's point of view

**A drone machine for live jamming is a system you steer, not a patch you edit.** The instrument should keep
sounding and keep changing while both hands are free, every gesture should be a musical one, and the screen
should tell you what the system is doing from across the room.

The design follows from the evaluation:

1. **Every key is a body that stays.** A key strikes a body that swells in over SWELL seconds and stays, latched,
   until the same key lets it go (it fades over FADE seconds). This is Lyra's HOLD and Rings' overlapping notes on
   a keyboard, so a chord is held by nothing and the hands are free. SEL turns LATCH off for momentary swells.
   Velocity is the body's weight, pressure brightens it and adds wobble while you lean on it.
2. **Four forces, not forty parameters.** HOME puts MASS (how much of the body is there: harmonics, cutoff, FM
   index, string brightness, by model), DRIFT (how much everything wanders: detune, random walk, wobble, breathing
   harmonics), SPACE (how far away it is: reverb, delay, width, the dry falling away) and TIDE (how much each body
   ebbs and swells on its own cycle) on the four knobs. The other pages refine them. Every knob has a musical name;
   the meaning never changes with the page you are on because the page is named on the screen.
3. **In tune by default.** PRESET picks a scale and the keys snap to it; chromatic is one click away. ORBIT moves
   within the scale. This answers the single most repeated complaint about Lyra and Subharmonicon.
4. **The system moves on its own, and you can always override it.** ORBIT (PLAY) moves a latched body to a
   neighbouring scale note every PERIOD seconds with CHANCE, within RANGE, gliding there over GLIDE; GRAVITY pulls
   it home. TIDE breathes, DRIFT wanders. Nothing you did not latch is touched, and a key always wins.
5. **Everything is recalled, and recall is a performance.** A scene is every parameter and the latched bodies.
   SELECT browses 24 scenes and a change morphs over MORPH seconds (the new bodies swell in while the old fade,
   the values crossfade), so a scene change is a transition, not a cut. This is NightSky's sequencer and Hedra's
   morph pedal on an encoder.
6. **Freeze and shimmer are built in, not borrowed from a pedal.** SEQ freezes the plate (the tank rings with no
   decay, no damping and no input, and comes up to the level the dry had); SHIMMER feeds the plate's output back
   in an octave up. The FX hold-layer (FX + F3 G3 A3 B3) bypasses ensemble, delay, reverb and shimmer live.
7. **Four models, level-matched, each a different physics.** SOLAR (eight breathing harmonics), VOID (three detuned
   saws through a dark filter and a sub), GLASS (two-operator FM with a breathing index and a detuned second
   carrier), STRING (a bowed comb filter with air). A model change restarts the bodies through a short fade, so it
   never clicks.
8. **The screen is a view of the system.** See D.

What DEEP FIELD does not try to be: a sampler or looper (the FM-1 has no audio input), a sequencer, a tracker, a
modular. Those are the roads the research shows lead to eyes on the screen.

Candidates for later, in the order they would earn their place: Lyra-style cross-FM between body pairs on one
knob; a Shepard / Risset model with a spiral view; a lo-fi AGE knob; just intonation per scale; MIDI clock sync for
ORBIT's period; velocity to SWELL rate as an option.

---

## D. The screen

Three views, cycled by pressing HOME while on HOME, all drawn on a phosphor layer that fades (eight hues, 32
levels, a lookup table to RGB565), 25 frames a second in two 240 x 72 strips so the pool keeps its room for the
delay and reverb lines.

- **ORBITS.** A sun at the centre pulses with the output level. Each body is a planet on an ellipse whose radius is
  its pitch, whose speed falls with the radius (Kepler, loosely) and rises with the body's breathing, whose size is
  its level and whose colour is its pitch class. A fading body dims; a body ORBIT has moved shows a bright point.
  A starfield drifts behind at a speed set by DRIFT, with parallax by depth and a twinkle. FREEZE draws a dashed
  ring of ice round the whole sky.
- **NEBULA.** The bodies' harmonics (which the engine knows, no FFT) as glows along a wavy spine, left to right by
  frequency on a log scale, coloured by body, with a mark under each fundamental. The phosphor smears them into dust.
- **SCOPE.** A radar: three rings, a sweep, each body a blip at its pitch class round the dial and its octave
  along the radius, lit as the sweep passes, and the output's left channel against its right as a phosphor
  figure, so stereo width and beating are visible.

The header names the scene, says MORPH while one is in progress, and shows LATCH, ORBIT, FRZ and the octave.
The footer is the page: four knob names, values and segmented bars, the model and scale on the right, bodies and
CPU in the corner. No lists, no numbers on the sky.
