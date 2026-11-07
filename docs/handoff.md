# Handoff — Pocket Poly, end of 2026-10-31

The one page to read before the next session. Status first, then how to resume, then what's next.

## Start here: built, waiting for the flash: the delay's quiet types reworked, the views gone (2026-11-08; not flashed, not committed)

When Kyle asks "what's next", open with these and how to test them, then the flash below.

- **The delay** (Kyle: "rise and fall you can really hear and ping is ok but the rest I can't hear at all"). Measured
  first: on a short note with nothing held, TAPE, HEADS and REVERSE echoed as loud as PING; over a held chord they
  vanish, because an echo of a steady sound is the same sound (RISE and FALL change octave, PING moves). GRAINS was
  broken outright: its pieces 26 dB under PING and silent a second after a note (four pieces, nearly all from the last
  80 ms). Now each kind changes what comes back (engine/fx.c):
  - TAPE sways deeper: wow 600 x (0.12 + WOW) half-rate samples, about 22 cents either way at WOW 40, 57 at full;
    darker (lpc x 0.4) and harder (drive 2.0) each pass;
  - HEADS slices its input: open for the first 30 % of each DLY TIME, a quarter the rest, a note's onset (a fast
    follower over a slow one) opening it whole for 120 ms: a held chord pulses as a triplet left, centre, right;
  - REVERSE one head, each window the last DLY TIME backwards, swelling from a quarter to full and cut in its last 4 %,
    alternate swells leaning left and right: a held chord breathes in;
  - GRAINS six pieces, one every half DLY TIME, 40 to 150 ms, from anywhere 80 ms to 1.8 s back, each at the note, an
    octave up or down, or a fifth up (its head running against the line), feedback 0.45 so octaves do not climb;
  - DLY MIX above 60 % climbs harder (x (1 + 12 (mix - 0.6)^2): +9.3 dB at full), so full brings the first echo about to
    the note's level; every factory scene sits at 55 or less, so the goldens match.
  Measured on a held C major chord (level swing / left-right swing over 30 ms windows, dB; before -> after, DLY MIX
  full): TAPE 0.95/0.14 -> 1.64/0.22, HEADS 0.88/0.53 -> 1.21/1.50, REVERSE 0.82/0.57 -> 1.23/1.16, GRAINS 0.52/0.26 ->
  1.58/0.90 (PING 0.80/1.43 -> 1.59/2.95, no delay 0.17/0.02; part of each rise is the louder top of DLY MIX; the
  measure misses pitch, which is how RISE and FALL stand out). On short notes all seven now echo -45 to -52 dB against
  the note's -23.7 (GRAINS -66.6 before). Auditions after the rework, each kind on short notes and over a held chord:
  deepfield/build/renders/delays/*.wav.
- **The views** (Kyle: "remove all views except for beams and remove the view setting"): MOUTH, ORBITS, NEBULA and SCOPE
  gone with their sprites, palette and state (ui.c), dr_scope and dr_harmonics and the per-tick spectra only NEBULA drew
  (drone.c; SOLAR keeps its own); GLOBAL is SCALE, ROOT, MORPH, -. `settings_t.view` stays for the saved layout, set 0
  at load, and no longer resets the settings when an older build saved another view. Manual section 10 (the views) cut,
  BEAMS described in section 3, sections renumbered; cheat sheets, README, words.tsv, screens.txt and four screenshots.
- Measured: 598 checks (the view checks kept as timing with KNOB 4 so the later dice don't move; new: GRAINS brings a
  note back), renders match, sanitizer clean but fastmath.h:58, device image 402,052 B (-15.7 KB), RAM 79,436 (-1.6 KB).

How to test (numbered for the unit):

1. Hold SEL a second, tap SEL (LATCH off), SPACE down. FX, SELECT once (DELAY): DLY TIME about 500 MS, FDBK 70 %, MIX full.
   Tap C4 with each TYPE: TAPE wavers and darkens, HEADS taps three times, REVERSE swells backwards, GRAINS scatters
   pieces at octaves and a fifth.
2. Tap SEL (LATCH on), hold C4 E4 G4, step TYPE again: TAPE beats against the chord, HEADS pulses left-centre-right,
   REVERSE breathes in each DLY TIME, GRAINS glitters.
3. GLO: the GLOBAL page reads SCALE, ROOT, MORPH; nothing on KNOB 4; the screen is always BEAMS.

## The last flash, the bed 3 dB down, ORBIT's audit fixed, GLIDE 0.13 s, HOME held (flashed 2026-11-08; not committed)

Its test steps are below.
Flashed on Kyle's "flash and update everything after you are done building" (`/usr/bin/python3
base/tools/fm1_install.py deepfield/build/deepfield.fwsc --yes`, through first time; the package newer than every
source, carrying AS SAVED and BEATS); booted clean: FIRST LIGHT, HOME, 0 bodies, 5 % CPU, no crash recorded. Site 0.8
published on Kyle's "Publish 0.8" (`deepfield/tools/publish_site.sh 0.8`, release FM-1_14000800; the first try with
"update everything" was refused by the permission check). Not committed (the 0.6 tree is saved as a patch in the
session's scratchpad, flashed-0.6/tree.patch).

- **The bed 3 dB down** (Kyle: "bed too loud"; he had asked first about the keys' velocity, which is already MIDI 100,
  79 %, and only spans 9 dB of level): `BED_GAIN` 0.708 on every body and on the noise layer, the leads left where
  they were, so LEAD LEVEL's default (59 %) now sits about 2 dB over a body (50 % matches one, full is 9 dB over).
  CROSS divides the trim back out of its bend. The factory scenes peaked at the limiter (NEBULA -0.1 dBFS, ICE BELLS
  -0.1, CATHEDRAL -0.2): every scene now 2.9 to 3.0 dB lower in RMS, peaks -1.9 to -9.7 dBFS. Goldens rewritten.
- **ORBIT's audit** (Kyle: "chord mode, EVERY 2.1 s, changing every 1 second"; a subagent measured every setting in
  the simulator, scripts in the scratchpad's audit/): with ORBIT keeping its own time EVERY was right (1.64 to 2.48 s
  at 2.1 s, the designed quarter either way). The 1 s came from MIDI clock arriving doubled (the same clock on two
  inputs) or bunched in pairs (Bluetooth): dr_clock counted every clock but left zero gaps out of the tempo, so the
  tempo read half and EVERY's beats halved; reproduced exactly (1.00 s) before the fix. Fixed:
  - the clock followed from one input (`clk_src`, dr_clock(port)), the tempo read over the last 24 clocks
    (`clk_hist`): every case the audit tried (steady 60 to 240 BPM, duplicates on two ports or 2 to 12 ms apart,
    pairs, computer buffers of 11.6 to 46 ms, Bluetooth intervals of 15 to 45 ms) now moves every 2.0 s at EVERY 2.1;
  - EVERY reads in beats under clock (4 BEATS, 1/2 BEAT: `dr_orbit_clocks`), so the screen says what ORBIT does;
  - under clock `orbit_per` is set from the clock, so PULSE rolls again (C4, E4, G4 dipping 0.6 s apart at ROLL full
    instead of all at once) and ECHO spreads its phrase;
  - the runner lost a whole block every step (it kept no leftover and the firing block went uncounted): 1/16 at 120
    BPM stepped every 130.5 ms, now 124.72 for 124.7;
  - ECHO's one-second floor applied after the spread (passes never under 1.004 s; they came 0.76 s apart);
  - WANDER's HOME PULL no longer lands on a note another body holds (33 in 1200 samples, now 0);
  - a turn of EVERY cuts the countdown under way (60 s to 2.1 s used to wait out the 60);
  - BODIES counts the latched bodies that may move ("2 OF 3"), as the engine does, instead of "N OF 8".
  Left as designed: CYCLE's three-chord progressions (I IV V, I II V, I VII IV) rest on I for two periods across the
  loop (the test calls it I IV V I).
- **GLIDE** 40 (0.13 s) on INIT and so every factory scene (was 60, 0.33 s; Kyle: "too much portamento"), GLASS
  ORBIT 50 (0.21 s, was 75, 0.68 s). Saved user scenes keep theirs.
- **HOME held a second**: the scene back as saved (`scene_reset`: its values, effect switches and ORBIT values over
  MORPH; the chord, LATCH, ORBIT on or off, FREEZE, octave and scale stay; SAVE dark; the screen says the scene and
  AS SAVED); HOME blinks while it counts; a tap still only opens HOME. Kyle: "a clear way to set each preset back to
  the default".
- Measured: 598 simulator checks (new: HOME held, bunched and two-input clocks, EVERY in beats, EVERY cut short,
  BODIES' count, the runner's 20 s; the moved-body check now rides CYCLE so the dice don't move it; a `level60` probe
  in app.c), renders match the rewritten goldens, sanitizer clean but fastmath.h:58, device image 417,748 B, RAM 81,036
  of 98,304 (+96 B for the clock history).

How to test (numbered for the unit):

1. Tap C4 E4 G4 on F1: quieter than before by about 3 dB; LATCH off and a lead stands out a little more.
2. ARP, KNOB 1 to CHORD, KNOB 2 (EVERY) to 2.1S, PLAY: a move about every two seconds (1.6 to 2.6). If a computer or
   a Bluetooth device sends clock, EVERY now reads BEATS instead of seconds.
3. KNOB 2 from 60S down to 2.1S while ORBIT runs: the next move comes within a couple of seconds, not a minute.
4. Watch a CHORD or WANDER move: a short slide (0.13 s), not a swoop.
5. Turn MASS and SPACE, switch the delay off on FX held + G3, change EVERY. Hold HOME a second: HOME blinks, then the
   scene's own values come back under the chord and SAVE goes dark.
6. ARP with RHYTHM 8/8 at 120: steady, and in time with a drum machine following CLOCK OUT.

## The flash before, three new models, TEAR, TAPE and RESONATOR (flashed 2026-11-07 late; not committed)

Flashed on Kyle's "Flash it" (`/usr/bin/python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc --yes`,
through first time; the package newer than every source, carrying RESONATOR, MATERIAL, BRONZE, SOURCE, SEL TRANS);
booted clean: the screen reader read FIRST LIGHT, HOME, 0 bodies, 5 % CPU after the vortex; the log only the radio's
lines, no crash. Not yet heard; device CPU with eight bodies of RESONATOR not read. Site 0.7 published on Kyle's
"Publish 0.7" with `deepfield/tools/publish_site.sh 0.7` (release FM-1_14000700), from the uncommitted tree.

Not heard yet; its test steps are below.
Kyle wanted more sounds; SCAN and DX (from Pocket Poly, on the unit since 878f35f) he has not heard yet. Pocket Poly's
other models are plain waves VOID covers and PD; its TEAR (sync and ring) was the one left. He said "Build all
three". ALGO now runs SOLAR ... DX, TEAR, TAPE, RESONATOR (M_TEAR 10, M_TAPE 11, M_RESON 12: older scenes unchanged).
No factory scene uses them, nor SCAN or DX: which five factory scenes make room for them is Kyle's call, once heard.

- **TEAR** (`render_tear`): a saw hard-synced to the note, SHAPE the ratio 1 to 8 (`dr_tear_ratio`, shown X1.0 to
  X8.0 as RATIO), DRIFT breathing it; every edge, the saw's own and the reset's, gets a two-point polyBLEP one sample
  late (Pocket Poly's sync smoothed only the saw's own), a DC blocker after; AIR rings it with the note's sine (all
  harmonic), MASS a low-pass that follows the note and a little drive. Measured: the energy between harmonics 96 dB
  under them at 1.6 kHz (SOLAR's additive sines 93), 60 dB at 392 Hz with the ratio at 8.
- **TAPE** (`render_tape`): a soft organ of three sines to a saw (SHAPE as SOURCE), saturated, through the tape's
  bandwidth (MASS); DRIFT the wear: wow (each body its own, 0.35 Hz and up), flutter, and dropouts at random times
  per body (3 to 12 s apart, sooner when worn; level down fast and back over 0.38 s, the top darkening with it; the
  first not before 2 s); AIR the hiss. Measured at full DRIFT over 30 s: five dropouts, 3 to 8 dB, no step over 6 dB
  in 50 ms.
- **RESONATOR** (`render_reson`): white noise into five coupled-form resonances (a rotation a sample: in tune low
  down in single precision), fed at the square root of their width, their power normalised across MASS's tilt;
  SHAPE the MATERIAL (PURE harmonic, GLASS a wine glass's rim, BOWL a singing bowl, BRONZE a plate's crowded modes);
  DRIFT a shimmer of up to 3 cents and their levels breathing; AIR the scrape (a darker noise, the resonances wider).
  Six bodies with every knob full, two octaves up or down: the bus stays between 0.29 and 0.83.
- **Levels** against SOLAR at F1's settings (a C-major chord, the model chosen before the strike), across SHAPE and
  MASS: TEAR -1.0 to -0.2 dB (dark MASS -2.9), TAPE -1.2 to +1.8, RESONATOR +0.4 to +0.5; single notes G3 and G5
  within 2.6 dB. (The older models are matched in their own scenes, not at F1's settings: VOID -4.8, CHOIR +8.9
  there, by design.)
- **CPU** (host, eight bodies, 120 s simulated, less idle): TEAR 0.21 s (SOLAR 0.23), TAPE 0.28, RESONATOR 0.36
  (CHOIR 0.36, the heaviest; LOW CHOIR reads 47 % on the unit). Device not read.
- Screen: a glyph each in art.h (a synced saw, a cassette, a singing bowl), SHAPE named RATIO / SOURCE / MATERIAL,
  RESONATOR's material by name; words.tsv has them. 572 checks (ALGO through all thirteen, each sounding; the three
  SHAPE readouts), renders unchanged, sanitizer clean, device image 416,580 B (+9 KB), RAM unchanged. Auditions at
  F1's settings, SHAPE 0, 64 and 127: deepfield/build/renders/models/*.wav.

How to test (numbered for the unit):

1. Tap C4, E4, G4. Turn ALGO to TEAR (11th): EDIT, turn KNOB 2 (RATIO) slowly from X1.0 to X8.0: the sync sweep.
   KNOB 4 (AIR) up: ringing, hollow. HOME, DRIFT up: the formant slides by itself.
2. ALGO one more: TAPE. KNOB 2 (SOURCE) from organ to saw. HOME, DRIFT to full and wait: the pitch sways, a body
   drops out now and then. AIR (EDIT, KNOB 4): hiss.
3. ALGO one more: RESONATOR. KNOB 2 (MATERIAL) through PURE, GLASS, BOWL, BRONZE. KNOB 1 (MASS on HOME) up:
   brighter rings. AIR: scrape.
4. Try each on F6 NEBULA (long swells) and with LATCH off for leads (TEAR is the one with bite).

Questions to ask after: do TEAR's leads have the bite he wanted; how worn should TAPE be at DRIFT's default; which
materials earn their place; which five factory scenes give way to scenes for SCAN, DX, TEAR, TAPE and RESONATOR.

## The last flash, DEEP FIELD's cut pass and the boot vortex (flashed 2026-11-07; not committed)

Flashed on Kyle's word ("Flash it") with `/usr/bin/python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc
--yes`, through first time (the package checked newer than every source, carrying SEL TRANS and SCENE + CHORD, no
CROSSFADE); booted clean: the screen reader read FIRST LIGHT, HOME, 0 bodies, 5 % CPU after the vortex, nothing in
the log. Not yet seen or heard by Kyle; the vortex's frame rate on the unit not read. Site 0.6 published on Kyle's
"Publish 0.6" with `deepfield/tools/publish_site.sh 0.6` (release FM-1_14000600; the first try had been refused by
the session's permission check), from the uncommitted tree. Not committed.


**The boot vortex** (Kyle, before the flash: "a cool vortex look with our color scheme and deepfield getting sucked
into the vortex in a spiral"). `boot_frame(t)` in ui.c, played by ui_frame for 3.4 s from ui_init: violet arms,
cyan nearer in, a white-hot ring round a dark eye (a log spiral of three arms worked out per 2 x 2 cell, dithered
with the beams' Bayer matrix, colour bounds dithered too); DEEP FIELD in FONT_L, each glyph pixel a mote, the letters
peeling off left to right from 0.62 s, 118 ms apart, falling along a spiral that turns faster near the eye with two
trailing samples for streaks; the eye flares at 2.45-3.15 s and the field collapses into the sky by 3.4 s. Nothing
kept between frames. Any key, button or knob ends it (and acts). On the unit `app_splash` (APP_SPLASH) draws its
first frame instead of the cream name page, and ui_init no longer clears to black, so the boot runs straight into
it. 558 checks (the vortex playing at boot, ending by itself, a key ending it and striking), renders unchanged,
sanitizer clean, image 407,700 B (+4 KB), RAM unchanged. The unit's frame rate is not read: each frame sends the
whole screen (21 ms) plus the maths (estimated under 15 ms), so expect about 25-30 fps. Test: switch on and watch;
switch on again and tap a key at once: the vortex gives way and the key plays.


Its test: the vortex, then the ten steps below.
Kyle: "I feel like we may have made this too complex now." A three-persona review (a first-hour player, a live
drone performer, an instrument designer; each read the manual, cheat sheet and code without my suggestions) agreed
the core works and the weight was in gestures and hidden states: SEL had five gestures, a ⅓ s timer decided whether
a key stopped or edited a body, TRANSPOSE stayed on after SEL was let go (with a bug: SEL held a second, then a key,
entered it with nothing latched, every key saying LATCH FIRST), the GLO blend could not be saved (it stopped at
0.999), the key lights carried 14 meanings, 17 pages. Kyle's four picks: ORBIT keeps all seven modes (untouched);
a body opens on a held key plus a knob turn, no timer; MOTION 2 and NOISE 2 leave the panel; one meaning per light
level. The persona majority's calls were built as defaults. Built on top of PHOSPHOR TUBE, which this session
committed first on Kyle's word (09384cf, not pushed).

- **SEL is LATCH and a hold-layer.** Tap: LATCH. Held: each key touched moves the latched chord there
  (`dr_transpose_to`, engine unchanged), pill SEL TRANS only while held; let go and the keys strike again, LATCH as
  it was. Held a second with no key: every body let go, and the keys strike at once (`sel_cleared`). `ui.shift`,
  the sticky mode and SEL+PRESET are gone.
- **A body is reached where it sounds.** `dr_body_at(note)` finds the latched body whose target is that key;
  `dr_let_go(home)` (queue kind 8) lets it go. Released without a knob turn, however slowly: let go. A knob turned
  while it is held: BODY opens, values kept at release. Pressing the key a moved body was struck on strikes a new
  body there; the moved one is re-homed where it sounds (`note_event`), and `find_body` prefers, between two known
  by one note, the one sounding there.
- **PRESET with nothing latched brings the scene's chord** (+ CHORD in the list and the HUD); with a chord latched,
  sound only, as before. Program change: sound only, always. Switching on is still silent.
- **Cut:** the GLO blend (`dr_morph_hold*`, the GLO layer; GLO is a page button, keys play under it), PLAY-hold
  (PLAY toggles ORBIT as it goes down), the MOTION 2 and NOISE 2 pages (their eight values stay in every scene and
  the engine; no CC had them), the destination LED, the home-key trace and the end-key stand-in.
- **Lights:** LIT a body sounds here (by its target), MID fading or the bed under LATCH off or a lead releasing,
  GLOW the scale; the keys strip the same. SAVE lights for a sound change or a GLOBAL/MIDI/USB setting
  (`ui.cfg_dirty`), not browsing, LATCH or the octave (still saved). An effect switched off reads OFF on SPACE,
  SHIMMER, DLY MIX and ENS MIX; a turn of that knob switches it back on (`knob()`, 2026-11-07: Kyle found SPACE
  "stuck at OFF" after the reverb was switched off).
- **ALGO crosses** each body over in 0.8 s through the scene morph's twins (`dr_param(P_MODEL)`), instead of the hard
  restart: measured at 10 ms steps the old cut fell to 3/1000 within 20 ms, the cross never under 105.
- **Not built from the plan:** FX opening its page on press. It would move the knobs to ENSEMBLE every time FX is
  held to switch an effect mid-set, the performer's main gesture; FX still opens its page on a tap. Left for later
  (persona findings, not picked): five ORBIT modes share PLAY's orange; ECHO needs LATCH off and a bed; SCATTER and
  ECHO use a CHANCE they don't show; a synced DLY TIME follows ARP's RATE, shown only in ARP; the designer's delay
  cut (PING, TAPE, GRAINS) waits until Kyle has heard the seven; views, models, the FX page layout, LIFT.

Measured: 550 simulator checks (499 before; TRANSPOSE, the body hold, the GLO blend, PLAY-hold, SEL+PRESET and the
page walks rewritten; new: SEL held a second then a key, SEL's release, a slow press, key plus knob, PRESET bringing
the chord, a moved body stopped where it sounds and its home key striking anew, ALGO's cross with no gap, the OFF
readouts, SAVE's light); the simulator's new `param N V` and `land N` commands (app.c). Sanitizer clean but the
known `fastmath.h:58`. Goldens rewritten: render_scenes.py brought each chord in with SEL+PRESET, now `land` on the
same millisecond; the committed 09384cf engine run with the new driver gives hashes identical to this build's, so
the sound is unchanged. Device package built: image 403,508 B, RAM 80,940 of 98,304 (17,364 B spare), pool 14,128 B
spare. The browser emulator built and played: silent boot, PRESET's chord, a 1.5 s press letting a body go, hold
plus knob opening BODY C4, SEL TRANS while held, the lit key following a moved body and stopping it. Manual
653 → 633 lines; cheat sheets, README, BUDGET and words.tsv updated; screenshots regenerated (screen-noise2.png
removed). Site not published (`publish_site.sh 0.6` with the commit).

How to test (numbered for the unit):

1. Switch on: F1 in silence. Turn PRESETS one click and back: + CHORD under the name, the line fills, F1's chord
   comes in.
2. Press E4 and hold it two seconds, then let go: it fades (no blink, no BODY).
3. Tap E4 back in. Hold it and turn KNOB 1 left a few clicks: BODY E4, quieter. Let go: it stays, quieter.
4. Hold SEL and touch A3, then D4: the chord moves to each, the pill reads SEL TRANS. Let go of SEL: SEL LATCH; tap
   G5: a new body.
5. Hold SEL a second without a key: every body goes. With SEL still down, tap C4: it sounds.
6. Tap E4 and G4 too. ARP, KNOB 2 (EVERY) to about 1 s, PLAY: a body moves and its light goes with it, the old key
   dark. PLAY off. Tap the lit key: that body fades. Tap the key it started on: a new body.
7. Turn ALGO one click: the chord crosses into VOID with no gap.
8. Hold FX, press A3 (reverb off), let go; turn KNOB 3 on HOME: the reverb comes back, SPACE reads its amount.
9. Hold GLO and press a key: it plays. SELECT from MOTION goes straight to ENSEMBLE, from NOISE to ENVELOPE.
10. Browse PRESETS with a chord latched: the sound changes under it, SAVE stays dark.

Questions to ask after: does stopping on any release feel right, or does he miss holding to peek at a body; is
MOTION 2 or NOISE 2 missed; should the key a moved body started on strike a new body, as now.

## The last flash: PHOSPHOR TUBE, DEEP FIELD's new screen (flashed 2026-11-07; site 0.5 the same day)

Flashed on his word ("Flash it"), through first time; booted clean (FIRST LIGHT, Kyle already playing: 2 bodies,
31 % CPU, nothing but the platform's boot lines in the log). Site 0.5 published with `deepfield/tools/publish_site.sh
0.5` (release FM-1_14000500). Committed as 09384cf (not pushed) before the cut pass. Not yet judged on the panel by Kyle.

Kyle asked for a design review of the screen (a UI/UX designer, a pixel artist, an FM-1 expert as agents; the review
page is the artifact https://claude.ai/artifact/XufR1cQRyPw3Ma1hJKiebq) and picked its recommended direction ("Build
Phosphor Tube"). Light means sound: only the bodies glow; one meaning per colour (a body's colour = that body, sun =
latched, white = your hand, cyan = where you are, teal = an effect on, orange = look). Sound unchanged (the goldens
match), 499 checks pass, the sanitizer run is clean, the device build passes (image 395 KB, pool 14.1 KB spare).

- **The glow fixed:** the colour table is eight ramps keyed at levels 31/26/20/13/7/2 that rise from the sky (the old
  fade from black drew the faintest 5 to 11 levels below the sky: black moats round every beam, ORBIT's ghost a
  black slot, the noise in a black box). The HUD dims by lowering every level to two fifths (the old mask kept a
  level's low three bits and blacked out the beams). Body green moved to lime, blue off violet.
- **The beams:** a 7 px core rising from level 20 to the key's pure hue through a 4x4 ordered dither, edges and a
  dithered halo, a white-hot head three rows deep, breathing with the body's spark; latched a sun spark, pinned a
  sun bar, open white frames; an ARP step flashes the head; ORBIT's ghost dithered at the destination; the noise a
  mist mound (warm or pale) that drifts with WIND; stars twinkle on slow sines with glints; FREEZE holds everything
  and draws four ice brackets; a dithered horizon at the bottom of the view.
- **The layout:** header 46 (the model's glyph from art.h, slot and name in M, an orange mark when edited, pills at
  fixed places with ORBIT's countdown along the PLAY pill's foot, ticks over the white keys, the morph line), view
  84, keys 36 (dark glass lit by the bodies, sun caps, white frames for the open body), footer 74 (the page's icon,
  numbers in M with units in T, bars by kind: segments, a cell per choice, two cells for a switch, RHYTHM's hits,
  centred for TONE and TUNE; SHAPE reads VOICE on DX and TABLE on SCAN). The 19 empty rows at the bottom are used.
- **Browsing PRESET:** neighbours, the name large, the 32 slots as marks (the one playing cyan, the candidate tall
  white), the landing line; + CHORD in sun while SEL is held.
- Not built from the review: stage mode (the red palette and backlight dimming, which needs a test on the unit),
  the boot screen and the LED language, Star Chart and Aurora.

How to test (numbered for the unit): 1. Switch on, tap C4 E4 G4: three beams in their keys' colours with white
heads and sun sparks, the keys lit from below them. 2. Turn MASS: the value large, the beams dimmed but visible
behind it. 3. Turn PRESET slowly: the list of scenes and the line filling. 4. PLAY with CHORD on a short EVERY: the
dotted ghosts and lines before each move. 5. SEQ: the brackets, everything still. 6. Hold E4: the white frames.
7. Look at it in a dark room and at an angle: does the navy read, is the small text readable, is the mist visible
(raise NOISE LEVEL)?

## The flash before (DEEP FIELD, flashed 2026-11-07 early; site 0.4 published the same night)

Kyle's six asks of 2026-11-06 ("Build all six"), each as the expert pass proposed it. Flashed on his word ("Flash
it") with `/usr/bin/python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc --yes` (through first time);
booted clean: FIRST LIGHT in silence, 0 bodies, 5 % CPU, nothing in the log. Not yet heard, device CPU not read with
SCAN or DX. Committed as 878f35f and pushed; the site went up as 0.4 just before (`deepfield/tools/publish_site.sh
0.4`, release package FM-1_14000400), so its publish note names ca3e6b1, but it is this code. The 11-06 flash's
delay types (below) were never heard either; they are in this build.

- **A body's own values.** LATCH on, hold a latched body's key a third of a second (it blinks; a knob turn opens it
  at once): the footer reads BODY and its note, the four knobs are its LEVEL (-24..+6 dB a dB a click, MUTE below),
  TONE (its own MASS ±50 %), TUNE (±50 cents) and ORBIT (MOVES / PINNED). A tap still lets a body go, now at the
  release. Several keys held: all of them, each from where it is. Pinned: ORBIT, PULSE and the arp leave it alone,
  TRANSPOSE still moves it, a white bar at its beam's foot. Saved with the scene (bank SCN5, older banks read), out
  and in as NRPN (99 = 1..4, 98 = the body's note, 6 = the value).
- **Arp RHYTHM and DIV** (ORBIT 2, knobs 3 and 4): Euclidean 8/8 5/8 3/8 7/12 5/12 4/9 7/16 5/16 (the notes move on
  only at a hit), and the step 1/16 1/16T 1/8 1/8T 1/8D 1/4 1/4T 1/2 1 BAR of RATE's beat; MIDI clock steps at DIV
  too (it was stuck on eighths). CHANCE, CHORDS, ACCENT and RATCHET from the same proposal are not built.
- **EVERY from 0.1 s** on the same curve (old scenes moved onto it: new = 0.64 old + 46), tenths shown below 10 s;
  ECHO never under 1 s; under MIDI clock a half and a quarter beat below one; no destination drawn under 0.5 s.
- **Knob steps.** Time-based acceleration: a detent within 40 ms of the last moves 8, else 3 (43 a sweep, Kyle's
  25 % finer); 2 for VOID's MASS, a free DLY TIME and ATTACK; 1 for RATE; one shown value a click for LIFT, STEPS UP
  TO, DISTANCE, BODIES, the delay's divisions and DX's voices. SWELL, FADE and GLIDE show ms below a second. (Before,
  the device never used its fast step: every knob moved a flat 4.)
- **SCAN and DX**, from Pocket Poly. SCAN: its three wavetables as one 48-frame sweep under SHAPE, generated at build
  time into flash (`deepfield/tools/gen_wt.py`, 61 KB), MASS a filter that follows the note, a loudness gain per
  frame and per MASS so the sweep and MASS keep the level (within 1.5 dB of SOLAR, the bright GLASS end at high MASS
  3.8 dB under), AIR a second copy elsewhere. DX: four-operator FM, SHAPE one of eight voices (GLASS PAD, BRASS, ORGAN,
  BELL PAD, E.PIANO, WOOD, STRINGS, CLUSTER), envelopes at SWELL's pace, AIR the feedback; each voice within 0.8 dB
  of SOLAR at F1's MASS. TEAR (sync and ring) and more polyphony were not built.
- **A scene is a sound.** Landing on a scene morphs the sound over MORPH under the chord being played; each body
  crosses into the new model through a twin (with eight bodies, one restarts at a time); LATCH, ORBIT (and its page
  while it runs), FREEZE, octave and scale stay; a delay or hall switched off rings out (spillover, also on the FX
  keys); power-on loads the sound in silence; PRESET's 0.7 s rest is a bar filling under the name. SEL held while
  PRESET turns brings the scene's chord in at SEL's release (LATCH on, its ORBIT values). Every factory chord now
  stands on C and plays in ROOT (VOID TIDE, GLASS ORBIT, BOWED SKY, PULSAR, DEEP VOID, GAMELAN, ICE BELLS and
  TIDE POOL moved the nearer way; NEBULA and DESCENT up, so their beams stay on the strip); a saved chord keeps its
  ROOT. Program change = the sound. The GLO crossfade is unchanged.

How to test (numbered for the unit):

1. Switch on: F1's sound, nothing playing, SEL lit.
2. Tap C4, E4, G4 (the white keys 5, 7 and 9 from the left): three bodies. Tap E4 again quickly: it fades as you let go.
   Tap it back in.
3. Hold E4: it blinks, then the footer reads BODY E4. Turn KNOB 1 left a few clicks (quieter, its beam lower), KNOB 3
   to -14 CT (the third goes pure). Let go: the HOME page is back and E4 keeps its values.
4. Hold C4, turn KNOB 4 right: PINNED (a white bar under its beam). Tap ARP, KNOB 1 to CHORD, tap PLAY: the chord
   moves over C4, which stays. Tap PLAY to stop.
5. Turn PRESET one click: the bar fills under F2 VOID TIDE, then your chord keeps playing as it crosses into VOID over
   four seconds.
6. Hold SEL, turn PRESET one click forward and one back, let go of SEL: F2's own chord comes in (on C).
7. ARP: KNOB 1 to ARP, tap PLAY. Turn SELECT once (ORBIT 2): KNOB 3 RHYTHM to 3/8, then 5/16; KNOB 4 DIV to 1/16, then
   1/4T. Tap PLAY to stop.
8. ORBIT page, KNOB 1 to WANDER, KNOB 2 (EVERY) all the way left: 0.1S. PLAY: the chord warbles inside the scale. Up
   to 0.5S: a slow melody. PLAY off.
9. Knob feel: turn KNOB 1 slowly (small steps), then fast (big jumps).
10. ALGO to SCAN (ninth): EDIT, KNOB 2 (SHAPE) from ANALOG 1 through VOX to GLASS 16. ALGO to DX: SHAPE one click at a
    time through the eight voices. On F6 NEBULA's long SWELL, DX's voices evolve slowly.
11. FX held + G3 with a delay on a held chord: the echoes ring out instead of stopping.

Questions to ask after: is the 40 ms threshold right for fast turning; do the factory chords on C (and following
ROOT) suit; which DX voices and which SCAN regions earn their place; whether LEVEL needs more than +6 dB.

Measured: 499 simulator checks (377 before, every older one passing again after the scene changes), the sanitizer
run clean but the known fastmath.h:58 note, the 16 scene renders within 0.7 dB of the committed build's on their
settled 6-14 s (new goldens), the hand-over equal-power (eight bodies: the deepest 50 ms dip 2.1 dB against its
second). Host CPU, eight bodies: SCAN about SOLAR, DX about CHOIR (LOW CHOIR reads 47 % on the unit); device CPU
not read. Device build: image 389 KB of the 568 KB slot, RAM 81,036 of 98,304, pool 13.2 KB spare (all checks ok).
The browser emulator builds.

## The flash before (DEEP FIELD, flashed 2026-11-06 evening; site 0.3 published the same night)

Superseded by the flash above, which carries all of this. Flashed on his word ("update the device now") with `/usr/bin/python3 base/tools/fm1_install.py
deepfield/build/deepfield.fwsc --yes` (the first try died at CoreMIDI client creation, -304, before writing anything;
the second went through). Booted clean: FIRST LIGHT, 3 bodies, 31 % CPU, no xruns, no crash. Not yet heard by Kyle;
device CPU at full load not read. Committed as acbd7d7 (not pushed). The site went up as 0.3 the same night
(`deepfield/tools/publish_site.sh 0.3`, release package FM-1_14000300) in the PHOSPHOR theme Kyle picked:
`deepfield/site/theme.css` and `beams.svg`, put in every page by `deepfield/tools/site_theme.py` (fm1-base's front
page, manual, installer and emulator page, and the cheat sheet; base/ untouched); `publish_site.sh X.Y --local`
builds the site into deepfield/build/site without pushing, for the "deepfield-site" preview.

Kyle on the 11-04 flash: the echoes are heard on a latched drone, but less than on a played key. Measured: the send is
the same, but a steady drone's echo sits 12 to 14 dB under it in every 50 ms window and adds no movement, so it
fuses; a played key's echo tops the dry in the gaps by up to 10 dB. His answer: a delay TYPE, COLOUR to DELAY 2,
two new DELAY 2 controls. This build:

- **DELAY page: TYPE, DLY TIME, DLY FDBK, DLY MIX.** TYPE: PING (the delay he has heard), TAPE (one centre echo,
  darker and harder each pass, wandering), HEADS (three echoes at a third, two thirds and the whole time, left
  centre right), REVERSE (each echo backwards, a swell), RISE (an octave up each repeat), FALL (an octave down),
  GRAINS (pieces of the last two seconds, one every DLY TIME, scattered). Turning TYPE fades the echoes out in 12 ms
  and starts the new kind fresh.
- **DELAY 2 page: COLOUR, DIFFUSE, WOW, CHOP.** WOW: the tape's wander, 0 clean, 31 % (40) what it always had, full
  seasick. CHOP: only a slice at the start of each DLY TIME goes in (full: a tenth), louder as it shortens; a held
  chord comes back as a pulse.
- **Synced PING lands on the beat:** the right echo used to come at 2.38 times the division; now at 2 times.
- CC 102 TYPE, 103 WOW, 104 CHOP. Every scene, saved ones included, loads PING, WOW 40, CHOP 0.
- **ARP PLAYED fixed** (Kyle: "I don't think PLAYED works"). Every upward order began on its second note, so a bed
  struck G C A E ran C A E G, which sounds like UP; with the bed let go PLAYED fell back to low to high; and a body
  added or let go mid-run made any order repeat or skip a step. Now G C A E runs G C A E from the first step, keeps
  that order after the bed is let go, and carries on from the note just played when the chord changes.

How to test (numbered for the unit):

1. F1 FIRST LIGHT: hold FX, press G3 (DELAY ON). Tap FX, turn SELECT once: the DELAY page, knob 1 reads PING.
2. Turn knob 1 one step at a time and listen to the latched chord on each: TAPE (wobbly, centre), HEADS (a little
   rhythm), REVERSE (swells), RISE (a climbing shimmer), FALL (a low shadow), GRAINS (a sparkle). Knob 4 (DLY MIX)
   up makes any of them louder.
3. Back to PING. Turn SELECT once more (DELAY 2). Knob 4 (CHOP) to the top: the chord comes back as a pulse. Knob 2
   on the DELAY page (DLY TIME) sets the pulse's speed.
4. DELAY 2 knob 3 (WOW) to the top on TAPE: seasick. Back to about a third.
5. Tap SEL off (LATCH OFF), play short keys on REVERSE and GRAINS: the echoes are clearest on single notes.
6. F3 GLASS ORBIT (synced 1/8D): play a short key with LATCH off; the right echo is now in time with the left.
7. ARP PLAYED: hold SEL a second to clear, latch G4 C4 A4 E4 in that order, tap ARP, knob 1 to ARP, knob 4 to
   PLAYED, tap PLAY: the runner plays G C A E from the first step. Hold SEL a second: it keeps G C A E.

Questions to ask after: which types earn their place; is CHOP's pulse the drone echo he wanted; is WOW's range right.

Measured: every type within 1.5 dB of PING's wet level on a held chord and a played key; no click on a TYPE change
(the largest sample step the same as with none); RISE's echo at 900 zero crossings a second against the drone's
290, FALL's at 242; REVERSE's first echo rises 25 dB to its peak over 140 ms. Host cost of the delay stage 0.3 to
1.4 us a block for every type (about 1 % of the device); device CPU not read.

**Waiting on Kyle (2026-11-06): his other asks, each with an expert proposal (details in the DEEP FIELD section,
"Proposals of 2026-11-06"):** per-body edits by holding a latched key (LEVEL, TONE, TUNE, ORBIT pin); arp options
(RHYTHM first); EVERY below one second; finer knob steps (the device never uses the fast step); more voices from
Pocket Poly (SCAN wavetables, then a DX model); what a scene change should carry over. Two arp questions from the
bug pass: bodies held over MIDI (LATCH on) are never latched, so the arp plays nothing over a chord held from a
controller (as the manual says); RANDOM can repeat a note at once.

## Where it stands

- **The FM-1 runs the review-fix build** (image 279,040 B, identity FM-1_1300, flashed 2026-10-31 evening). It
  booted clean (no crash recorded), idles at 4 %, and sits on F1 INIT / HOME. Its settings were reset once by the
  PPL4 settings format earlier that day; nothing has been saved on it since except what Kyle did at the keys.
- **Everything built so far:** VA/digital-hybrid engine with wavetables and PD, six-operator FM with DX7 import,
  Juno ensemble / tape delay / plate / limiter that rest when silent, a 12-mode arpeggiator with Euclid, Turing,
  chance, ratchets, latch and MIDI clock both ways, seeded discovery (DICE / MUTATE / MORPH / history), full MIDI
  (USB, TRS in, Bluetooth both roles, channel filter, CC map, sustain, LOCAL, clock out), settings/bank/FM-bank
  storage with migration, 22 pages, screen reader, manual with screenshots. `docs/roadmap.md` §1 has the table.
- **Verified:** 310 simulator checks (`app/tests/basic.txt`), sanitizer build clean, clang analyzer clean, 16
  golden renders (`tests/golden.txt`), device CPU: 8 VA voices 50–55 %, FM 81 %, idle 4 %. Three code reviews
  (37 findings) — 33 fixed, 4 deferred into the roadmap. Details: `docs/platform-decision.md` "Review pass".
- **Committed 2026-11-02** (the whole tree in one commit, build outputs and the generated words JSON ignored).
  No `Co-Authored-By` trailers when committing (Kyle's rule).
- **Kyle's verdict so far:** the FM voices after sound pass 1 are untested by ear beyond "the arp works now"; his
  overall read is that the instrument is *boring* — correct engine, placeholder sounds. That is the brief for
  the next phase.

## Standing rules (Kyle's)

- Never flash without his explicit "Flash it"; he powers the unit on for tests only. Never inject keys/buttons
  over USB while he is playing (`base/tools/fm1_remote.py screen` and `log` are fine any time).
- MIDI (USB, TRS, Bluetooth both roles) stays in every build; never cut for resources.
- Device instructions as numbered steps naming buttons and what the screen shows.
- Terse replies, no emojis, no recaps; debugging needs an executable confirmation before a fix.
- Every commit or flash (2026-11-06): bring all the docs up to that build first (MANUAL, both cheat sheets: the
  markdown and `site/cheatsheet/index.html`, README, BUDGET, this handoff) and republish the site with its emulator
  (DEEP FIELD: `deepfield/tools/publish_site.sh X.Y`, the next version; never the base script).

## How to resume

```
colima start --vm-type vz --vz-rosetta      # once per boot of the Mac (device builds only)
./build.sh --host && app/build/host/pocketpoly_host app/tests/basic.txt app/build/host
python3 tools/render_all.py                 # must say "all 16 renders match golden"
JIELI_TOOLCHAIN="$HOME/.jieli/toolchain" AC79_SDK="$HOME/fw-AC79_AIoT_SDK" ./build.sh    # device package
python3 base/tools/fm1_install.py app/build/pocketpoly.fwsc --yes                        # only on "Flash it"
```

Sanitizer run: copy the tree (minus `app/build`) to a scratch folder and build there with
`CC=<wrapper that execs clang -fsanitize=address,undefined -fsanitize-recover=all -g> ./build.sh --host`; the one
expected UBSan report is `base/include/fastmath.h:58` (fm1-base, benign on the target).

Level check of the renders: `tools/render_all.py` writes `app/build/renders/*.wav`; a peak/RMS-over-time script
is the quickest way to compare patches (one lived in the scratchpad this session — rewrite it in `tools/` when
the sound pass starts, it is ten lines of `wave` + `array`).

## Next: v0.6, starting with what is heard first

Agreed direction (2026-10-31 evening): skip to the playable work, plumbing later.

1. **Factory sound pass** — 16 patches (bass ×3, pad ×3, keys ×3, lead ×3, pluck ×2, FM ×2) designed as sounds,
   not demos; every render within ±3 dB of the loudest; Kyle's notes from sound pass 1 are the taste guide
   (`memory: fm1-sound-taste`): no per-note pan jumps, bodies that sustain, fast brass/pad attacks, consonant
   chords, leads that bite, levels matched to the VA patches.
2. **Unison STACK** — two detuned voices per note in stereo, polyphony 4; the "full" sound without positional
   panning. CPU guardrail: worst patch with STACK ≤ 85 % at 8 voices → likely 4-voice cap when on.
3. **Chord memory and scale lock on the keys** — one key plays a stored voicing; scale lock snaps to a scale.
4. Then: ladder filter, LFO tempo sync, velocity curve, the wavetable alias fix (sine fallback above the lowest
   mip's range).
5. v0.7 discovery (favourites, breed, category dice, section lock, LED feedback, arp→phrase) right after.

Guardrails for every change: RAM ≥ 8 KB and pool ≥ 8 KB spare (today 14.0 KB and 11.5 KB), goldens rewritten only
for intended sound changes, a simulator check per new parameter, manual row per new page or knob.

## Open items to keep in view

- Unreproduced incident (arp dead + no USB answer until power cycle, 2026-10-31): if it recurs, note whether the
  footer CPU/voice blocks still move, read `log` before power-cycling.
- BNK4 → BNK5 bank migration never exercised on a device that held real user saves.
- Codec noise floor / headroom unmeasured; hub identity prefix is still the development one (FM-1_1300).
- Licence: MPL-2.0 recommended for the app's files, Kyle has not confirmed.

---

# DEEP FIELD — second firmware, built overnight 2026-10-31 → 11-01

**Status, 2026-11-03 (read this first):** the FM-1 still runs the build of 2026-11-02 evening (image 300 KB,
identity FM-1_1400). The **playable pass** of 2026-11-03 and everything that followed it that day (items 1-9 below) are flashed
(image 317 KB, identity FM-1_1400, booted clean), committed and pushed to github.com/WisestWizard/deepfield; the
site at wisestwizard.github.io/deepfield carries 0.1 (the commit before item 9). Kyle has played the lead layer
and the runner only briefly; nothing else since the morning has been heard. Before this day he liked the sounds
of the 11-02 build but found it bland. Resume with "What's next" at the end of this section. On it: the three-rule interface (one button one
page, SELECT between pages; a lit button is a mode; the screen mirrors the panel with the BEAMS view), the FDN hall
on the second core a block behind the bus, the noise layer, eight models, sixteen factory scenes, the effects
retuned by measurement, 21 review fixes including the watchdog reset's cause. Device CPU with eight bodies: 29–37 %
by scene, no xruns; STRING ×8 not read since the hall moved. Budgets: `deepfield/docs/BUDGET.md`. Verified: 170
simulator checks, 16 goldens, sanitizer clean. Everything committed on master. Next, Kyle's TODO at the end of this
section: more voices (cheap now that bodies ride on core 2), more scenes and models; and the GAMELAN NOISE page
read COLOUR/WIND/BAND at zero on the unit once, where the factory scene says 50/40/30 — not reproduced, possibly
Kyle's own knob turns; worth a look if it recurs. Resume with the commands under "How to resume" above, with
`deepfield` as the app.


A separate app in `deepfield/` on the same fm1-base (not a change to Pocket Poly). Kyle's brief: an ambient drone
machine for live jamming with a sci-fi space look, after researching the existing drone/ambient firmwares.

- **Research and point of view:** `deepfield/docs/research.md` (catalogue, pros and cons with sources, DSP costs,
  the position: every key a latched body, four forces, scale-locked, ORBIT generative moves, scenes that morph,
  freeze and shimmer built in, the screen as a view of the system).
- **Built:** eight bodies in four models (SOLAR additive, VOID saws, GLASS FM, STRING bowed comb), LATCH (SEL),
  ORBIT (PLAY), FREEZE (SEQ), SHIMMER, scenes with morph (SELECT), ALGO model, PRESET scale, FX hold-layer on
  F3/G3/A3/B3, three phosphor views (ORBITS, NEBULA, SCOPE), MIDI in/out USB + TRS + Bluetooth both roles, CC map,
  program change, screen reader words, manual with screenshots.
- **Verified:** 85 simulator checks (`deepfield/tests/basic.txt`), 8 golden renders level-matched (RMS −13 to
  −17 dB, `deepfield/tools/render_scenes.py --levels`), freeze holds (no runaway), shimmer at maximum bounded,
  ASan/UBSan clean but for the known `fastmath.h:58` report, device package built: image 258 KB, RAM 75.9 of 96 KB,
  pool 310 of 344 KB (`deepfield/build/deepfield.fwsc`, identity FM-1_1400).
- **Not done / not verified:** never flashed (Kyle's rule), so device CPU is unmeasured — estimate from Pocket
  Poly's figures is 40–60 % with the second core taking four bodies; no listening test; the web emulator build
  (`--web`) not tried. Nothing committed (same as Pocket Poly).
- **Next:** flash on "Flash it", read `plat_cpu_pct` through the footer on each factory scene with eight bodies,
  listen (levels, the STRING model's bow, shimmer taste), then candidates in `research.md` §C (cross-FM pairs,
  a Shepard model and view, an AGE knob, just intonation, clock-synced ORBIT).

**UX pass (2026-11-01, after Kyle found it hard to use):** header 32 px with LATCH / ORBIT / FRZ pills, sky 80 px,
footer 128 px with 24 px values in a 2 x 2 grid; a HUD shows every knob or button large over the sky; keys are
momentary and SEL latches what is held (a latched key pressed again lets it go); velocity off by default
(VELOCITY on the MIDI page); factory swells shortened; EDIT cycles BODY / ENVELOPE / MOTION and any page returns
HOME after 8 s; SELECT scrolls and loads after 700 ms with a morph bar; in-scale keys glow; one turn per force;
OCT- + OCT+ together lets every body go (HOME hold removed). 100 simulator checks, goldens rewritten (the swell
and velocity change the renders).

**Lag pass (2026-11-01):** Kyle felt lag "switching between voices". Measured in the simulator: a key sounds within
10 ms of the press; the felt lag was the swell (exponential, 0.3 s to full in the factory scenes) and the model
switch (a 20 dB dip for 250 ms while bodies restarted). Fixed: a straight swell that reaches full level at SWELL,
factory SWELL 20-35 (80-200 ms; NEBULA keeps 95), a 7 ms quick swell after a restart (dip now ~30 ms). The screen:
the footer is one 240 x 128 transfer (canvas 240 x 128), the phosphor is faded and composed in one pass, and the
sky is skipped on any frame that sent the header or footer, so the main loop never waits on a transfer in a knob
frame (host profile: sky 31 us, footer 25 us per draw on the Mac; the device's cost was the DMA waits, 15-20 ms).

**Art pass (2026-11-01):** Kyle hated the vector-line look. The MOUTH view (the default) is now sprite-based pixel
art drawn at 2x onto the canvas (120 x 46 game pixels; `spr()` in ui.c, sprites as rows of characters with a
character palette `spal`): an RX bottle whose cap opens in three frames (then the hand idea was dropped), a front-view mouth in three
frames (open, half, shut) for the chomp, two-tone pills that tumble by a quarter turn, chunky stars and a ringed
planet. The other views keep the phosphor. The sky grew to 92 rows (footer cells 50, canvas 240 x 116). To change
the art, edit the ASCII sprites in ui.c (BOTTLE, CAP_*, MOUTH_*, PILL, PLANET) and the palette in spal_build.
Encoders now: PRESET scenes, ALGO model, SELECT scale.

**Interface pass (2026-11-02, after Kyle's "still confusing"):** three personas' critique, then a rebuild on three
rules. (1) A page button always lands on the same page (HOME, EDIT BODY, ENV, LFO MOTION, FX SPACE 1, ARP ORBIT,
GLO GLOBAL) and SELECT steps between the nine pages (SPACE 2 and MIDI are a click away); no cycles, no 8 s
auto-return, the page's button lights. (2) A lit button is a mode: SEL is LATCH (on by default: a key strikes a
body that stays, the same key lets it go; off: momentary; held 1 s: every body let go; the OCT chord is gone), PLAY
ORBIT, SEQ FREEZE; pills are named for their buttons (SEL LATCH, PLAY ORBIT, SEQ FRZ). (3) The screen mirrors the
panel: header 34 (scene, status, pills, E D R S for the effects heard), a 240 x 84 view, a 240 x 36 keys strip (the
27 keys as lit, in-scale lighter, latched framed yellow, FX held = the four switches named over their keys), footer
86 (page band with `<` `>`, four knob cells in a row). The default view is BEAMS: each body a beam over the key
that struck it (height = level, white cap = latched, dim = fading, ORBIT slides it and marks its home); MOUTH
(pills over their keys), ORBITS, NEBULA, SCOPE stay as skins (VIEW on GLOBAL). Effect switches are now the
scene's (`scene_t.fx_off`, bank magic SCN2) and LATCH a setting (DEF3): a device with old settings loads
defaults. Performance: the view is 20 % fewer pixels than the old sky (40 KB a frame at 25 fps), the keys strip is
drawn only when its signature changes (17 KB), the header once a second for CPU (16 KB); beams cost ≤ 6.4 k
phosphor dots a frame. 137 simulator checks, goldens unchanged (no sound change), sanitizer clean. Device package built (image 281 KB, RAM 76.4 of 96 KB, pool 317 of 344 KB), not flashed; device CPU of the beam view unmeasured.

**Octave and ORBIT passes (2026-11-02, after Kyle played it):** OCT− / OCT+ half-light one octave out and light two
out. ORBIT made visible: the key light walks with a moved body (home key half-lit, on the LEDs and the keys strip),
the PLAY ORBIT pill fills toward the next move in eighths (`dr_orbit_next`) and flashes on a move, PLAY held opens
the ORBIT page, PLAY says ORBIT ON · EVERY 5S, and the page reads EVERY / CHANCE / STEPS UP TO / HOME PULL (same
engine values). 140 checks (137 → the octave LEDs, 140 → PLAY held), goldens unchanged.

**Effects pass (2026-11-02):** Kyle could not find the effects' parameters and found the FX-held labels tiny. The
two SPACE pages became three pages named for the effects in the order of the FX keys (FX ENSEMBLE: mode, mix; FX
DELAY: time, feedback, mix; FX REVERB: size, damp, shimmer, SPACE), empty knobs drawn as a dash; FX held shows the
four switches large (name, ON/OFF, bar) in four columns over the view in place of the HUD. 142 checks.

**Effects level pass (2026-11-02, Kyle: "the effects are hard to hear"):** measured in the simulator by rendering
each scene with and without one effect bypassed (`deepfield/tools/fx_levels.py`; the method: the FX-held key
toggles the bypass, both renders get the same key commands so they stay aligned, the difference is the wet). At
factory F1 the delay was 20 dB under the rest, the reverb 22 dB, the shimmer 43 dB; even SPACE max left the reverb
18 dB down because the dry fall-off also starved the sends. Fixed: ensemble wet 0.8·√mix (dry 1 − 0.4·mix), delay
send √(DLY MIX)·(0.6 + 0.4·SPACE) (DLY MIX is the delay's level now, not a share of SPACE), delay feedback
0.95·√fdbk, reverb out_g 4.0·(1 − decay) (was 0.695·), dry fall-off to −5 dB at SPACE max (was −8), a direct
shimmer path (6·shimmer·octave-up) beside the loop, the loop held at the measured bound (shim line ×0.175, gain ≤
0.45: a let-go chord dies away at every size; ×0.25 ran away at sizes 0 and 70), the bus 3 dB lower into the effects
and the master 3 dB up (room before the limiter). Now at factory F1: delay −5 dB vs the rest, reverb −6, ensemble
−7 at mix 40; at the knobs' max: delay +1, feedback +7, ensemble −1, shimmer −6. Factory trims: GLASS ORBIT and
PULSAR delay/shimmer down (they pinned the limiter), STARLIGHT ensemble down, VOID TIDE and DEEP VOID MASS up.
Goldens rewritten; scene RMS −12.6 (GLASS ORBIT) to −17 (VOID TIDE), the VOID scenes 4 dB under the loudest by
RMS, dark on purpose. Not yet heard by Kyle.

**The hall (2026-11-02, Kyle: the reverb "makes the whole sound more quiet rather than feel like it is reverbing";
he chose the hall over the quick pass):** the Dattorro plate is replaced by an eight-line feedback delay network
(`engine/fx.c`): lines 45 to 131 ms (int16, the same 33803-sample budget as the plate), Hadamard mix, stereo in
(L to the even lines, R to the odd, alternating signs) and out (even lines left, odd right), a 30 ms pre-delay, two
diffusing allpasses a side, a one-pole high-pass near 120 Hz on the way in (the sub stays dry), damping in each
line, four lines wandering ±6 samples at 0.45 Hz. REV SIZE is the decay time 0.6 to 8 s (each line's gain set from
it, so every length decays alike); REV DAMP the damping. Freeze: gain 0.9998, no damping, no input, output up to
3.5× (a frozen F3 chord holds at −13 dB RMS, losing 0.2 dB every 2 s, measured). Shimmer as before (grain octave-up of the output, fed
back at ≤ 0.3 and heard direct at 6×shimmer); bounded at every size with shimmer at max (a let-go chord dies away
within 8 s). With it: the dry no longer falls away with SPACE (SPACE only adds the hall), the limiter's release is
250 ms (was 80). Measured at F1: hall wet 6 dB under the rest at the factory SPACE, 3 under at SPACE max;
shimmer 8 under at max. Goldens rewritten; scene RMS −12.6 to −18.6. Not yet heard by Kyle; the plate is in git
history only once committed, so the old fx.c is gone from the tree.

**The noise layer (2026-11-02, Kyle: "a noise generator that can play independently or along with any other
voice", revised by the three personas):** one stereo noise in the engine (`noise_render` in drone.c), under the
bodies or alone: two generators, a colour tilt (brown to white, COLOUR), a high-pass under 40 Hz, a trapezoidal
SVF band that wanders with WIND (and DRIFT) or, as BAND passes a third, moves onto the newest body's note; MASS
and pressure brighten it, TIDE breathes it on its own phase, it swells and fades at SWELL/FADE. LEVEL is its one
state (0 = off, law level^1.5: 30 sits 15 dB under the bodies, full 3 dB over, measured with the scratch script
`nzlevel.py`). Page NOISE after BODY (EDIT then SELECT); on the FX layer C4 toggles it (from zero to 35 %), the
readout has five columns, the header letters are E D R S N; a soft wide beam on the keys strip where its band
sits. CC 20. Scenes: four new bytes, bank SCN3 with a migration from SCN2 (user scenes kept; the old block is
read back from the end); NEBULA and DEEP VOID carry a bed. COLOUR swung the level by 10 dB (14 weighted above
400 Hz), Kyle heard it: a lift table by colour (20, 18, 10, 4, 0 dB at 0, 25, 50, 75, 100 %) now holds the
plain RMS within 1.5 dB and the weighted level within 5.4 dB across COLOUR (brown a little quieter to the ear, on purpose), the base 6 dB lower, then another 3 dB on Kyle's ear (scratch scripts nzlevel.py and colour.py). The delay-at-half-send idea from the persona pass was
dropped: the effects run on one bus, a separate noise path would double the sends. 151 checks, goldens rewritten
(NEBULA, DEEP VOID, and STARLIGHT, which starts with DEEP VOID's bed fading). Not yet heard by Kyle.

**Four more models and eight more scenes (2026-11-02, Kyle: "more diverse sound choices"; the first half of the
TODO below):** CHOIR (a parabolic-saw buzz, AIR noise, through three trapezoidal SVF band-passes at vowel
formants; SHAPE morphs A E I O U, MASS shifts the formants ×2^((m−½)·0.8) and lifts the upper two), BELL (four
sine partials, ratios from harmonic to 1 / 2.76 / 5.40 / 8.93 with SHAPE, each breathing on its own hb phase,
MASS a tilt, AIR a dark hiss), SHEPARD (six sine octaves from f/8 to 4f under a raised-cosine window that slides
with `v->gp`; SHAPE below the middle falls, above rises, up to 0.35 octaves/s; MASS leans the window), FOLD (a sine
of a sine: drive 0.15 + 2.2·SHAPE turns, DRIFT breathes the drive, AIR noise into it, a one-pole LP from MASS).
Each has a spectrum for NEBULA. voice_t grew by fs[6] and gp. Factory scenes F9..F16: CATHEDRAL, LOW CHOIR,
GAMELAN, ICE BELLS, ASCENT, DESCENT, FOLDED SUN, TIDE POOL; NFACTORY 16, program change 0..15 factory, 16..31
user (the manual says so; the save test expects slot 18 = U3). Host timing of eight bodies in each model is in
the same band as the old four. Level-matched by `render_scenes.py --levels` (CHOIR evened across vowels by (300 Hz / F1)^0.45 and lifted, CATHEDRAL weighted 84 and LOW CHOIR 115, SHEPARD ×1.5 after
the first render); the dark noise beds were brought down (NEBULA 18, DEEP VOID 16, LOW CHOIR 16, DESCENT 14)
because the colour lift had raised them. Goldens: 16 renders. Not yet heard by Kyle.

**Review and hardening pass (2026-11-02, Kyle: "deep dive, fix bugs, make it more performant, evaluate the
device's capabilities"):** three reviews (engine, UI and scenes, platform use), every finding reproduced before
the fix. Fixed: the ORBIT message's stack overflow on sharp notes (a[8] → a[16]); OUT CH reaching 0 (corrupt MIDI
and the settings record rejected at boot); a scene's saved effect switches not applied at power-on; a failed
SAVE clearing the dirty state; an FX-layer key within 80 ms of FX lost; crumb_log one byte short; the load log
line's buffer; the MOUTH pill-spacing loop that was the watchdog reset (bounded to eight retries, regression check
in basic.txt); dr_overload now lets the oldest body go so the load falls at once; a stolen body's fade was dead
code (it cut to silence in one sample); VOID and CHOIR started with a full-scale spike (the saw's DPW state now
begins at x² of its phase); BELL and SHEPARD partials above 18 kHz now stand still instead of running their phase
away; dr_panic drops the queue with one store; every-body-let-go goes through the event queue; dr_load lowers
the flag while it copies; the delay counts as heard whenever DLY MIX is up. Performance (host, per 256-frame
block): the hall's wander and the delay's wow once a block, one diffusing allpass a side, the "still ringing" test
without fabs: effects 12.7 → about 8 µs, the hall 8.4 → 5.3; the second core takes five bodies (core 1 has the
effects); the canvas is the footer's 86 rows (14 KB of pool back); `vc` and the arena on 32-byte lines; a load
line in the log and the CPU in the spoken page. Goldens rewritten (the spike and the steal change renders).
`deepfield/docs/BUDGET.md` has the device's budgets and the app's use; the audio CPU on the unit, read the
same evening while Kyle played: SOLAR ×8 bodies 41 %, BELL ×8 42 %, STRING ×8 47 %, VOID ×4 39 %, 42 % with no
body but the tails ringing, 3 % at rest. The active effects are about 40 % of core 1 by themselves; eight bodies
add 2 to 7 points because five render on core 2. Worst case mid-40s, under the 70 % line in BUDGET.md; the hall
is the lever if more is ever needed (run it on core 2 one block late). Review harnesses and repros sat in the scratchpad (not kept).

**The hall on the second core (2026-11-02, after the readings showed the effects were core 1's load):**
fx_process is split into fx_pre (ensemble, delay, in place), fx_hall (the hall's wet of a block, written to its own
buffers) and fx_post (the limiter). dr_render builds the block's fx_params_t before the core-2 run (`g_fxp` in the
pool); the second core's job runs fx_hall on the bus of the block before (hall_in, 2 × 256 floats) into hall_out,
then its five bodies; core 1 renders three bodies, the noise, the bus, fx_pre, stores the bus as the next hall_in,
adds hall_out, fx_post. With no second core (the host, or after a fallback) fx_hall runs on core 1 at the same
point, so both paths render the same samples (goldens hold either way). The wet is one block (5.8 ms) later, so the
hall's pre-delay dropped by 256 samples (RV_PRE 1080) and the total stays 30 ms: F1 renders at the old level to
0.1 dB. A given-up core-2 block zeroes hall_out for that block rather than running the hall twice. Arena +4 KB
(DR_ARENA_BYTES 214 K), the hall's lines −1 KB. Host: core 1's effect work fell from about 8 µs to about 3 per
block. Goldens rewritten. Flashed: the figure did not move (34 % with three bodies, 40 % with eight, 41 before),
because plat_cpu_pct is the half's wall time including the wait for core 2, which with the hall and five bodies had
become the longer core. Rebalanced to two bodies beside the hall on core 2 and six on core 1: SOLAR ×8 now reads 29–31 % (was 41), CHOIR ×8 36, SHEPARD ×8 33,
VOID ×4 (VOID TIDE, ensemble and wind) 37, no xruns; STRING ×8 not yet read since the move.

**The playable pass (2026-11-03, after Kyle: "a little bland still", "play live over latched keys", "orbit more
variation, maybe a real arp"; a three-persona review first, then four steps):**

1. *The lead layer.* LATCH off (SEL dark) no longer lets the bed go: the keys (and MIDI notes) play **leads**, the
   scene's model struck short, over the latched bodies. Four lead voices beyond the eight (`vc[NV_ALL]`, each voice
   padded to whole 32-byte lines; the leads render on core 2 beside the hall and bodies 0..1), a LEAD page after
   ENVELOPE (ATTACK 1 ms..2 s, RELEASE 30 ms..8 s, LIFT in semitones to +24, LEVEL: 75 level with a body), a
   little MASS of their own (`vmass`, +0.15) so they cut, no tide, never latched, never moved by ORBIT. A key on a
   latched note plays a lead over its body (Kyle: an accent). The screen: a thin white beam over the key, the key
   white on the strip (grey as it releases), the LED lit then dim. The old momentary mode is gone (a long ATTACK
   and RELEASE is the same thing).
2. *ORBIT's modes* (the name kept, Kyle's call): MODE on the ORBIT page, knobs 2-4 by mode (`page_param`):
   WANDER (as before, but never onto a note another body holds or is going to), CHORD (every latched body by the
   same number of degrees up to DISTANCE, the chord shifting along the scale; HOME PULL pulls the chord home),
   SCATTER (BODIES of them at once, each its own way, timing give or take a half), ARP (below). ORBIT 2 under
   SELECT: HOME PULL, OCTAVES. Moves are **planned at the start of the countdown** (`orbit_plan`, `pend_to[]`,
   `dr_orbit_pending`) and made at its end (`orbit_apply`), so the screen shows where each body is going: a ghost
   of the beam at the destination, a line from the beam's top growing toward it with the pill, the destination
   key in the body's dim colour on the strip and glowing (mid) on the panel; the "E4 > F#4" HUD is gone (Kyle:
   smaller). PLAY's colour is the mode: green WANDER, orange CHORD/SCATTER, red ARP; the pill reads PLAY CHORD /
   PLAY SCATTER / PLAY ARP while running.
3. *The runner.* ARP mode strikes leads over the latched notes (low to high, OCTAVES up) at RATE (40..240 BPM, a
   step an eighth), GATE of the step (top = legato), ORDER UP / DOWN / UP DOWN / RANDOM / PLAYED (strike order). It
   keeps the note set, so after SEL held (every body let go) it plays on over nothing until PLAY stops it (Kyle's
   spec). BPM only; MIDI clock sync is a TODO for a full MIDI pass (clock bytes are still dropped in app.c).
4. *The noise pass* (both personas: "a steady bed and nothing else"): GUST, the layer's own weather (Poisson
   gusts at WIND's pace, 36 s to 6 s apart, 0.3-3 s up, 2-10 s down, louder + brighter + higher together, the
   bed between gusts down to -18 dB at GUST full; the layer always starts with a gust; FREEZE holds it; a WIND turn
   shortens a pending wait); BAND past a half is **the chord's breath**: up to four narrow bands on the loudest
   bodies, each as loud as its body, spread left/right in turn; TEXTURE hiss → rain → crackle (Poisson bursts, gain
   matched to the hiss's RMS: measured, `deepfield/tools/noise_levels.py`); DUCK under a chord (to -12 dB); its own
   WIDTH; brown's high-pass down to 25 Hz; C4 on the FX layer comes back to the scene's saved level. NOISE 2 page
   after NOISE. Factory beds: NEBULA gusts, DEEP VOID crackle, LOW CHOIR and ICE BELLS breathe on the chord (and
   ICE BELLS rains), DESCENT gusts dark.

5. *The lights* (the interaction designer's review of every state through the simulator's `leds`): the keys had
   one dim level for five meanings and the runner was invisible. (The host's `leds` prints mid and glow alike and
   not PLAY's colour; a printer that tells them apart was tried in `base/host/plat_host.c` and reverted, since
   `base` is the upstream fm1-base submodule: worth sending upstream.)
   Now three levels, one meaning each: LIT = what the keys address (a body under LATCH on, a held lead), MID =
   the machine's (fading, a moved body's home, the bed under LATCH off, a lead releasing, an off-strip body at
   the end key, ORBIT's destination, which blinks through the last eighth), GLOW = the scale (`plat_lights`'s
   dim keys, which the app never used). A lead's strike winks its key off 60 ms (a runner's step and an accent
   on a lit key are seen); PLAY winks at a SCATTER move and an ARP step; a hold (SEL, PLAY, REC) blinks the
   button from 300 ms until it fires, over the mode light; SAVE blinks in its dialog; the page button sits at
   mid under the FX layer. Left for Kyle (the reviewer's questions): SAVE lights for any settings change (LATCH,
   octave, scale, MIDI) and stays lit all session, so it carries little; the bed at mid under LATCH off is a
   choice; whether the destination light is worth an LED at all.

6. *Recording over USB* (Kyle, recording into Live: "make the FM-1 know when it is hooked up so it doesn't make
   noise in the speaker"): a SPEAKER setting on a USB page under GLO (ON / AUTO / OFF, settings DEF4 with DEF3
   read and brought up). AUTO: the app's `APP_GAIN` returns 0 for the speaker while a computer records, with
   `plat_usb_audio_full` on so the USB input gets the render at full level; the header says USB MUTE. The stream
   state is usb.c's `uac.feed` (alt 1 and the host reading), read from the app because it is compiled in the same
   unity unit; plat.h has no word for it: a `plat_usb_audio_in_use()` belongs upstream. The host simulator takes
   `expect usbrec1` / `usbrec0` to pretend.

7. *The limiter and the recording's level* (Kyle, recording: "clipping type noises", redlining in Live, heard
   on the unit too): measured in the simulator (`expect limiter`, the limiter's deepest gain per half second):
   eight GLASS bodies on F3 held the old limiter 1.5-4 dB into reduction with an instant attack, a hard clipper on
   every peak, the same in the 11-02 build (the renders were identical); over USB at full level it reads
   -0.13 dBFS. Now the limiter has 1.5 ms of lookahead (64 samples, the gain 98 % of the way down when the peak
   comes out) and the same 250 ms release; goldens rewritten (every scene, by a hair). And USB LEVEL on the USB
   page, -6 dB from the factory, trims the render only while a computer records with SPEAKER not ON, so the
   recording keeps headroom and the speaker loses nothing. The USB ring's underruns are in the log line (`uN`)
   for the day the deep ring is needed; none were seen.

8. *From the firmware hub comparison* (Kyle: "ok to all"): **MIDI clock** in (F8/FA/FB/FC through app.c to
   `dr_clock*`): the runner steps every 12 clocks, ORBIT's EVERY counts whole beats at the followed tempo, RATE
   reads EXT, two seconds without a clock restores the knobs; **CLOCK OUT** (USB page, setting) sends 24 a beat
   with start/stop while the runner runs from RATE. **CYCLE**, a fifth ORBIT mode: the chord walks one of eight
   progressions as scale degrees from where it was struck (CHORDS knob, `PROG[]`), the destination lines showing
   the whole move. **The crossfade by hand**: GLO held, a key's pressure blends the heard values toward the next
   scene (`dr_morph_hold`: pstart from what is heard, ptarget the scene, morph_step 0), the blend staying where it
   is left; the keys play nothing while GLO is held. **The browser emulator** builds (`./build.sh deepfield
   --web`, Emscripten from Homebrew, 6.0.13) into deepfield/build/emu; there is no git remote, so nothing is
   published: `base/tools/publish_site.sh` is the path once the app has a GitHub repository.

9. *Motion and depth* (Kyle: "what else can we do to add motion and depth", then "flash and go on"): a MOTION 2
   page (LFO, SELECT): **SPREAD** gives each body fixed differences for its life (up to 14 cents of detune, MASS
   ±15 %, its own tide rate, its own place: `sp_*` in voice_t, from the body's own generator at the strike),
   **CROSS** couples bodies in pairs (the partner's level bends the pitch, up to 180 cents at full: not audio-rate
   FM, the renderers run a voice at a time), **TIMBRE** a slow per-body walk of MASS, **SWAY** each body across the
   field on its own cycle (`pan_live`, which the renderers read now). Two ORBIT modes: **ECHO** (the last eight
   notes the player's leads struck, `echo_ring`, replayed over the period each EVERY, quieter each pass, notes
   dropped and jumped an octave by DECAY and JUMPS, until gone or a new note wakes it; the runner's and ECHO's
   own leads do not count, `lead_machine`) and **PULSE** (each body dips to a quarter and swells back at SWELL,
   low to high, ROLL apart: a strum). Defaults SPREAD 30, TIMBRE 25, SWAY 15 change every render: goldens
   rewritten, scene RMS -12.8 to -16.7; PULSAR and FOLDED SUN carry CROSS. The bank migrates by record size now
   (any of our magics), so a scene saved on an earlier build today survives. Also fixed: a body struck again
   while fading glides home (it kept ORBIT's moved pitch). Device package: image 316,604 B, RAM 79,772 of 98,304
   (18.1 KB spare), pool 324,760 of 344,064 (18.9 KB spare). **The repository**: github.com/WisestWizard/deepfield
   (public, MPL-2.0, the LICENSE file added) and the site with the installer, the manual and the browser demo at
   wisestwizard.github.io/deepfield (`base/tools/publish_site.sh deepfield 0.1`, from the commit before this pass;
   Emscripten from Homebrew). Homebrew's python3 is 3.15 now, without mido: the FM-1 tools run with
   `/usr/bin/python3`.

Scenes grew by 15 bytes (bank SCN4; SCN3 and SCN2 banks migrate through one generic unpack-from-the-end). 287
simulator checks (basic.txt: the lead layer, the three modes and the runner with pending counts and MIDI out,
gust counts, FREEZE holding the weather), goldens rewritten (the beds changed: hpk, DUCK 30 by default, the
factory beds), scene RMS -12.7 to -17.0, sanitizer clean but for `fastmath.h:58`. Device package built: image
314,552 B, RAM 79,188 of 98,304 (18.7 KB spare), pool 324,520 of 344,064 (19.1 KB spare) after items 6-8; the
build with items 1-7 is flashed and boots clean; item 8's build (clock, CYCLE, the crossfade) waits for "Flash it". Device CPU
of the leads, the chord bands and the runner unmeasured (host: the four bands about 1 µs a block; a lead costs
a body).

**Delay pass (2026-11-03 night, after Kyle's "not really feeling it"):** an electronic-musician persona read the
delay and said why: a dark mono repeat of a sustained drone is a comb filter, not an echo; the wet sat dead centre
between a wide ensemble and a stereo hall; and it had no relation to the runner's tempo, then ran into the hall.
Built its first pass. The one 0.836 s line is now two legs of 0.418 s (`fx.c` DLY_HALF): the input into the left,
each leg's repeat crossing into the other with the feedback, the right leg 1.382 times the left so the sides never
coincide, the wet spread by DLY_WIDTH 0.7; two allpasses a leg (7 + 11 ms left, 5 + 13 ms right, 3.2 KB from the
arena, their 18 ms taken off the read so the knob's time is the hop) smear the repeats by DIFFUSE, the DELAY
page's fourth knob (P_DLY_DIFF at the end of the enum: the scene bank migrates by size, old scenes get INIT's 25).
DLY TIME below 100 is free, 50 to 418 ms; from 100 a division of the beat (1/16, 1/8, 1/8D, 1/4T, 1/4;
`dr_dly_div`) timed from the MIDI clock when it is followed, else the runner's RATE, clamped to the leg. Measured
in the simulator on a 120 ms lead over an empty bed (DLY TIME 273 ms, FDBK 55): repeats at about 290 ms left,
then 350 ms later right, then left, then right, each hop 4 to 5 dB down, the sides 15 dB apart; at DIFFUSE 100
the gaps between them fill. Factory: the delay is now absent (DLY MIX 0, also INIT's default) in FIRST LIGHT, VOID
TIDE, BOWED SKY, STARLIGHT, CATHEDRAL, LOW CHOIR, ICE BELLS, ASCENT and FOLDED SUN; in time on GLASS ORBIT (1/8D,
FDBK 50, MIX 40, DIFFUSE 30), PULSAR (1/8) and GAMELAN (1/16, clean); a second dark room on DEEP VOID (418 ms, FDBK
75, DIFFUSE 85) and TIDE POOL (380 ms, DIFFUSE 60); a trailing cloud on NEBULA and DESCENT (MIX 25, DIFFUSE 70).
301 simulator checks (the DELAY page, the sync labels), goldens rewritten (every scene with a delay changed; scene
RMS -11.5 to -17.7), device package built (image 317,996 B, RAM 79,836 of 98,304, pool 324,800 of 344,064), not
flashed, not heard by Kyle. The musician's next steps, in order: a COLOUR in the feedback (a high-pass at about
120 Hz to keep the sub out of the loop, the low-pass brighter than 0.35: "probably a quarter of the mud"),
modulation depth as a knob (the wow from clean to chorus to seasick), half the wet into the hall instead of all of
it; they would skip ducking, reverse and pitch-shifted repeats on this material.

**Second delay pass, FX keys, black keys, TRANSPOSE (2026-11-04 evening):** Kyle heard the three delay tests and
found them underwhelming, asked whether the delay needed more time, and reported that FX+G3 no longer switched the
delay on, that other FX keys did nothing on some scenes, that black keys played the white key beside them, and
asked for a way to play the keyboard to transpose the latched bodies. Causes confirmed in the simulator: (1) the
first pass set DLY MIX to 0 in nine scenes and the FX keys only flipped a bypass, so a stage at zero stayed silent
(FIRST LIGHT: G3 said DELAY OFF, then ON, nothing heard); ensemble and shimmer default to zero, the same; (2) the
delay topped out at 0.418 s, and past 0.30 s the right leg hit its end so the sides collapsed; (3) the default
scale (C MAJOR) snapped every key outside it to the scale note below (F#3 played F3). Done: the FX keys bring a
zero stage in (delay MIX 35, SPACE 45, SHIMMER 30 with the hall unbypassed, ENSEMBLE CHOR I at 40) and switch it off
when heard; a sweep of all four keys on all 16 scenes passes. The delay legs run at half the rate (two one-poles at
7 kHz down, a midpoint and a one-pole up), left 0.80 s and right 1.11 s in 42,240 samples, allpasses halved in
length to keep their milliseconds; DLY TIME free is 50 ms doubling four times to 800 (200 at the middle), the sync
divisions are seven in order of length (1/16 1/8 1/4T 1/8D 1/4 1/4D 1/2, four steps each from 100) and a division
too long at the tempo halves; COLOUR (P_DLY_COLOUR, 56, INIT 50) sets the loop's low-pass 1.5 to 9 kHz and its
high-pass 30 to 200 Hz in place of the fixed 0.35 low-pass and DC block; DELAY page TIME FDBK MIX COLOUR, DIFFUSE
moved to a new DELAY 2 page (page numbers from 10 shift by one). Measured at TIME 99: left repeat 0.84 s, right
+1.06 s, ping-pong, 4 to 5 dB a hop at FDBK 55; COLOUR 0 to 100 thins the repeats' low end by about 8 dB. Factory
times: NEBULA 92 (0.66 s), PULSAR 105 (1/8), DEEP VOID 0.8 s, DESCENT 0.54 s, TIDE POOL 0.6 s; COLOUR GLASS ORBIT 70,
PULSAR 65, NEBULA 45, DEEP VOID 30, GAMELAN 75, DESCENT 40, TIDE POOL 55. The engine arena grew to 235 KB and
drone.c now refuses at compile time an arena too small (the first build crashed the simulator because dr_init
silently refused). Keys play their own note in every scale; the scale glows and bounds ORBIT. TRANSPOSE: SEL held
and a key touched enters it (pill SEL TRANS); each key moves every latched body by one step so the lowest lands
on it (homes move, ORBIT's pending moves dropped, MIDI offs before ons); a SEL tap is LATCH again; SEL held a
second lets go and ends it. 345 simulator checks under ASan and UBSan (the only report is base's fastmath.h left
shift of a negative, which predates this), goldens rewritten (renders after a delayed scene catch its longer
tail), scene RMS -12 to -18, device package built (image 320,604 B, RAM 79,904 of 98,304, pool 329,952 of
344,064), flashed the same evening (booted clean, 30 % with 3 bodies), not heard by Kyle, device CPU at full load
not read. Still open from the musician: a modulation depth
knob (DELAY 2 has three free slots) and half the wet into the hall.

**Delay types pass (2026-11-06, built, not flashed):** P_DLY_TYPE 57 (DT_PING..DT_GRAINS), P_DLY_WOW 58 (INIT 40),
P_DLY_CHOP 59 (INIT 0) at the end of the enum, so older banks unpack with the INIT values (the size-based
migration); TYPE reads dr_scene.p directly, never morphed, like ENSEMBLE. fx.c: the one 42,240-sample buffer is two
legs (PING, RISE, FALL) or one line of 1.91 s (TAPE, HEADS, REVERSE, GRAINS); a TYPE change fades the wet over 256
half-rate samples, then `dly_clear` and the new type (at once when the stage rests or is bypassed); `dly_tap` clamps
its distance inside the leg. WOW is now a distance in samples, one-sided (400 w² for the wow at 0.35 Hz, 14.1 √w
for the flutter at 6.3 Hz; TAPE 2.5× and 1.5×), where it was a fraction of the time: 40 gives the 3 and 12 cents the
delay had at 0.6 s, the same at every time now. CHOP gates the input at the start of each DLY TIME, duty 1 - 0.9
CHOP, 2 ms edges, makeup 1/√duty. RISE/FALL: two heads half a 1,536-sample grain apart on each leg (the leg's
maximum time 70 ms shorter). HEADS: taps at T/3, 2T/3, T, the last fed back. REVERSE: two heads at 2 + 2Ww behind the
write. GRAINS: four voices, one started every T, 1.6 T long (40 to 300 ms), from 80 ms to 1.9 s back (squared
random, the recent more often), smoothstep window, √ pan, feedback 0.7 of the sum. DLY_OG levels each type's wet
with PING's (TAPE 0.5, HEADS 1.16, REVERSE 1.35, RISE 1.55, FALL 1.4, GRAINS 1.15). Synced (`dly_synced`), PING's
right leg slews to the left's length. 372 checks pass (basic.txt's DELAY section rewritten, CC 102-104 checked),
ASan/UBSan clean (also a script cycling all seven types at FDBK, WOW and CHOP extremes), goldens rewritten: rendered
alone, every scene without a delay is bit-identical to 22d5ce3 and the seven with one keep their level within
0.1 dB (the in-sequence render changes more, through tails carried scene to scene). Device package: image 325,460 B,
RAM 80,240 of 98,304, pool 330,064 of 344,064. `tools/fx_levels.py` knob numbers fixed for the new pages.

**ARP fix (2026-11-06, same build):** `arp_gather(played)` keeps `arp_mem`, the latched notes in the order struck
with each body's age, through the bed being let go, sorted low to high only when ORDER is not PLAYED (PLAYED's own
case is gone); each step's body age is kept (`arp_age`) so a changed chord carries on from the note just played;
`arp_pos = ~0u` before the first step (PLAY, MIDI start), DOWN's wrap adjusted to start at the top. OCTAVES and
ORDER now also apply to the remembered notes. basic.txt ends with the check (G4 C4 A4 E4 out in that order; it fails
on 22d5ce3). Left: letting go the very body that just sounded can skip one step once.

**Proposals of 2026-11-06 (expert passes on Kyle's asks; built 2026-11-07 as "Start here" says, except the arp's CHANCE, CHORDS, ACCENT and RATCHET, TEAR, more polyphony, and the GLO crossfade's changes):**

- *Per-body edit* (musician): hold a latched key past a third of a second (or turn a knob while holding) and the
  footer becomes BODY <note> with LEVEL (MUTE, -24..+6 dB), TONE (its own MASS ±50 %), TUNE (±50 cents) and ORBIT
  (MOVES / PINNED: never moved by ORBIT, left out of the arp; the top pick); a tap still lets a body go, on release;
  several keys held edit together. Fits voice_t's spare bytes; saves with the scene (lvl/tone/tune[8], pins in
  rsv[0]; needs a new bank magic SCN5); NRPN for MIDI. Runner-up: per-body TIDE.
- *Arp options* (musician), ranked: RHYTHM (euclidean 8/8 5/8 3/8 7/12 5/12 4/9 7/16 5/16, ORBIT 2 knob 3), DIV
  (1/16..one bar incl. triplets and dotted, a new ORBIT 3 page; also fixes the clocked arp stuck at eighths), CHANCE
  (its own byte), CHORDS (the bed walks a progression under the runner), ACCENT, RATCHET (the only one with a CPU
  cost).
- *EVERY below a second* (musician): the same knob over 0.1..60 s on the same curve, tenths shown below 1 s,
  saved scenes migrated (new = 0.64 old + 46); ECHO floored at 1 s (its phrase breaks below); 1/2 and 1/4 beat
  under MIDI clock; no destination preview below 0.5 s. WANDER/SCATTER/CYCLE gain the most, CHORD turns to mush.
- *Knob steps* (audit): on the device the fast step (8) never fires: `knob()` takes it only for 4 or more detents
  in one read and the loop reads every pass, so every 0..127 knob is a flat 4 a detent (32 a sweep). Proposed:
  time-based acceleration (a detent within 40 ms of the last: 8), slow step 3 for all (43 a sweep, Kyle's 25 %), 2 for
  VOID MASS, free DLY TIME and ATTACK, 1 for RATE, one shown value a detent for LIFT, STEPS, DISTANCE, BODIES and the
  synced divisions; SWELL, FADE and GLIDE in ms below 1 s. No parameter smoothing exists (dr_param sets at once).
- *More voices from Pocket Poly* (survey): SCAN (the three wavetables as one model, SHAPE through all 48 frames;
  61 KB of tables, in flash as const data or in the STRING lines' 48 KB), then DX (FM6 with SHAPE choosing eight
  sustaining voices, capped at four operators; CPU the risk, Pocket Poly's FM BELL ran 80 % against 52 %), TEAR (sync
  and ring, cheap); skip PD. If "voices" means polyphony: CPU has room, the pool does not (4 KB STRING line a voice).
- *Scene changes* (performer, interaction and sound designers): a scene is a sound, not a state. On landing, the
  latched chord, LATCH, ORBIT on/off (and its values while PLAY is lit), FREEZE, octave and scale carry over; the
  values morph over MORPH; each body hands over to the new model through a twin instead of restarting; effect tails
  ring out; boot loads the scene's sound silent. SEL held + PRESET loads the whole scene with its chord (factory
  chords rewritten on C and transposed to ROOT). The 0.7 s browse rest shown as a filling bar.

**What's next (agreed order, 2026-11-03 night):**

1. Kyle's ears on everything since the morning: the leads and the accent, the five new ORBIT modes, the noise
   weather and the chord breath, the lookahead limiter, MOTION 2. And the three lighting questions from the
   review: SAVE lights for any settings change (autosave the settings?), the bed at mid under LATCH off, whether
   the destination LED stays.
2. ~~Device CPU with the new load~~ Read 2026-11-03 night (Kyle's go-ahead to drive the unit over USB): every
   factory scene with eight bodies 29–47 % (LOW CHOIR the heaviest at 47, ICE BELLS 42, DESCENT 41, NEBULA 40,
   STRING scenes 35–37); the full load (STARLIGHT ×8 with CROSS and SWAY at 100, the breath on every body, ORBIT in
   ARP at 120 BPM with CLOCK OUT) 56–57 %, no xruns. The breath costs 11 points, the runner 8, CROSS and SWAY
   nothing readable. Figures in `deepfield/docs/BUDGET.md`. Still under the 70 % line; STACK (item 5) has room.
3. ~~Publish 0.2~~ Done 2026-11-05 from 79bae26, before Kyle had heard the 11-04 flash: the site, the installer
   and the browser emulator at deepfield.fog.baby (GitHub Pages, a Namecheap CNAME to wisestwizard.github.io; the
   front page leads with the emulator for people without an FM-1). Publish with `deepfield/tools/publish_site.sh
   X.Y`, never the base script: its push drops the custom domain. Still to do: submit the site to
   thefirmwarehub.com (its submission form).
4. Sound depth still on the list: the hall breathing with TIDE, just intonation per scale, AGE on the master,
   true audio-rate cross-FM (a pair-wise renderer rewrite; CROSS today is control-rate coupling).
5. STACK: a second model per body (FRASIL stacks three); the second core has room beside the hall.
6. The full MIDI pass: CC map for the new pages, MIDI learn, clock out for ORBIT's moves, and the two platform
   changes to send upstream to fm1-base (a public "a computer is recording" call; the `leds` printer with
   PLAY's colour).
7. Pocket Poly: its v0.6 sound pass never happened; decide whether it stays in the repository or is parked.

**TODO (Kyle, 2026-11-03): the crossfade by hand needs a reason to exist.** Kyle tried GLO + a key and could not
tell what it was for. Fixed the same night: the top row (D5 up) has no pressure sensing, so a held top-row key
now climbs the blend to full in two seconds (flashed). The remaining question is musical: the blend slides the
values toward the next slot and never the model or the notes, so between unrelated factory neighbours (F1 SOLAR
to F2 VOID) it is subtle. Try it where it should be obvious (F6 NEBULA toward F7 DEEP VOID: SPACE 69 to 39,
DRIFT 55 to 23) and as designed pairs in the user slots (a calm U1 and a stormy U2 of the same patch). If it
still does not earn its place: let GLO + PRESET pick the far scene while held, or cut the feature.

**TODO (Kyle, 2026-11-02): more sounds, presets and voices for DEEP FIELD** — more factory scenes than eight,
more models than four (candidates in `deepfield/docs/research.md` §C: cross-FM pairs, a Shepard model, just
intonation), and the voice count above eight if the second core carries it. After the interface has been played.

**Incident closed (2026-11-02 review):** the 2026-11-01 watchdog reset was the MOUTH view: `sky_mouth`'s
pill-spacing loop restarted from j = 0 on every collision, and a pill between one neighbour above row 60 and one
below bounced for ever; nothing in a frame feeds the 8 s watchdog. Reproduced in the simulator (six MIDI bodies in
MOUTH hang the main loop with the render idle, the log's exact picture) and bounded to eight retries; the script
is a regression check in basic.txt.

**Open incident (2026-11-01, as it was seen):** Kyle saw the screen freeze after changing the model and scenes quickly; the unit
then restarted to F1. The log: "reset by the watchdog: task main", "last watchdog reset in a frame" (the 8 s
watchdog fired with the main loop inside app_frame). Checked and ruled out: render cost (host timing: every model
with 8 bodies is 16-24 us a block, under Pocket Poly's INIT at 21 us = 21 % on the device, so the audio cannot
starve the loop); an unbounded loop in ui.c or drone.c (none found by inspection; four low notes and eight pills do
not hang the simulator); memory errors (ASan/UBSan clean on basic.txt and the pixel-art scripts); the LCD wait
(bounded, 4M spins). Not reproduced. The build now carries breadcrumbs in .noinit (app.c `crumb`): the frame's step
(1 frame, 2 input, 3 speak, 4 btdev, 5 scene, 6 header, 7 footer, 8 sky, 9-11 the MOUTH view, 12 HUD, 13 blit) and
whether the render was in progress; the next boot after such a reset logs "deepfield last frame step N render
busy/idle" (fm1_remote.py log). If it recurs: read the log before power-cycling; ask whether the sound kept playing
while the screen was stuck (render idle + sound on = a main-loop loop; render busy = the audio interrupt).


---

# GNOMNOM — third firmware, built 2026-11-03 night → 11-04

**Status, 2026-11-04 (read this first):** a rhythm game in `gnomnom/`: a garden gnome eats mushrooms falling down
seven lanes (white keys C4..G5 by letter), every eaten mushroom plays a note of a public-domain tune, the band
climbs from a straight folk rendition through house, deep house, acid, rave, breaks, IDM and hard across eight
stages, the background warps and cycles while the field stays crisp, a trip meter is the life bar, JAM never
ends. **Never flashed** (Kyle's rule: only on "Flash it"). Committed on master as 3f447b5 (2026-11-04, the whole
tree, build outputs ignored); not pushed. The design and every module
contract: `gnomnom/docs/DESIGN.md` and `gnomnom/gn.h` (gn.h wins). Kyle's brief: concept 1 of the game
discussion re-themed as a gnome eating mushrooms, "really trippy and wavy and colorful at a bunch of points",
music that starts as the normal songs and gets more IDM / house / acid as the levels go on.

- **How it was built:** the design spec and gn.h first, four independent design reviews (hardware budget, game
  feel, music, concurrency) that reshaped both (a field layer over a warped background, falls kept near a second,
  three audio buses, a tune format with alterations, four tune swaps, every cross-context protocol spelled out),
  then six modules written in parallel from briefs (gfx, art, band, music, game, ui) and integrated. The host
  build compiled first time; two fixes on integration (the title music never started at boot; the stage gap
  field was a byte); the test ordering fixed once.
- **Verified:** `gnomnom/tests/basic.txt` passes in the host simulator (title → run → eats, misses, pause with the
  count-in, game over, the saved high score across a reboot, JAM's wobble, knobs, the screen reader); the manual's
  six screenshots in `gnomnom/docs/img`; `gnomnom/tools/levels.py`: every stage peaks at −0.8 dBFS under the
  limiter, RMS −18 (folk) to −12 (hard); the browser emulator (`./build.sh gnomnom --web`, `.claude/launch.json`
  "gnomnom-emu", port 8766) plays: P starts a run, qwertyui/asdfghjk are the white keys; the device package
  builds: image 286,468 B, RAM 72.5 KB of 96, pool 317 KB of 344 (`gnomnom/build/gnomnom.fwsc`, identity FM-1_1500),
  every build check ok. Three code reviews after integration (concurrency, contract conformance, audio and
  renderer) found no blocker; their fixes are in: MIDI-in black notes no longer fire the stash, endless cycles the
  tunes, the boss gets its one-bar riser every stage, the LED bright threshold honours the shorter of half the fall
  and a beat, the saved high score is re-fed to the render, the core-2 hall job takes its buffer by argument, a slid
  acid note gates itself off, FREEZE's stutter slice halves its division until it fits the ring, FLASH OFF also
  gates the PERFECT flash, three envelopes flush denormals, the snare keeps its tone phase on a roll.
- **Not verified:** nothing heard by ear (levels and routing by number only); the device CPU figure (the band's
  host timing extrapolates to 30–40 % on core 1 with the hall on core 2); the two-core path and its fallback ran
  only inline on the host; the tearing of a 21 ms full-screen write on invert/strobe/cycling needs a finger test on
  the unit; the transcriptions are from memory (Hungarian Dance, the later bars of Alla Turca and the Can-Can are
  "that tune at tempo"); the ACID bass slide was fixed twice on integration (it now glides and gates) and only re-rendered by number.
- **Pre-flash bug pass, 2026-11-04 late (committed 2026-11-06 as 673e558, not flashed):** three reviewers (engine, UI, simulator play) plus my own
  read; 20 bugs fixed and each re-checked in the simulator or a harness: early eats at stages 1–2 cut to the
  guide's level (a BN_SOFT note no longer retriggers a sounding eat), STAB ignored BN_NOTEOFF_SELF (boss and BURP
  pad/hoover notes stuck, MIDI too), a REC tap toggled Bluetooth on even ms, buttons in the demo acted on the title,
  a whole-screen flash on every PERFECT from stage 1 (now trip ≥ 700, ≤ 2/s, two frames), FLASH OFF missed the kick
  pulse, the device seed was fixed, MIDI play did not count as activity, an AUTO note sounded after a miss, the
  count-in's HOME quit, pause during FREEZE kept its stutter, FREEZE's stutter half a bar under 109 BPM, lead osc-2
  and ensemble-mode clicks, a flash write on HOME from GAME OVER, two stash keys in one block, K3 replacing the
  title bell, density-4 chain floods, FB (continue) instead of FA on resume, reader state announcements. Host
  CPU against DEEP FIELD by the same simulator figure (0.5 % at stage 8 vs 0.3 % for DEEP FIELD at 33–40 % on the
  unit) puts stage 8 near 55–80 % of core 1: read it first; the fallback is the drums on core 2 beside the hall.
- **Design pass built, 2026-11-04 late (committed with the bug pass, Kyle picked the top six):** (1) a wrong key: a lane press
  that eats nothing while a mushroom is due in another lane, or beside one a key just ate, plays a sour note, ends
  the combo, costs a third of a miss, shows OOPS (judged after CHORD's and WIDE's extra lanes; never under FREEZE);
  at stage 1 honest play makes 0, a neighbour with every note keeps every eat at 29 % of the score, slapping all
  seven keys comes down in two seconds. (2) `BP_TRIP`: below trip 400 a low-pass on the band bus closes toward
  700 Hz and the hats then the stabs drop out at bar lines; above 700 the delay, hall and shimmer sends rise; lead,
  bass and kick untouched (stage 2 recorded: the band's high end falls from 0.35 to under 0.05 of its level between
  trip 280 and 160). (3) Difficulty: an on-ramp (the gap 1.5× over TUNE's first two bars, 1.25× the next two), the
  Canon's gap 250, Bumblebee and Can-Can at density 2, Bumblebee's death caps 8, a bar's rest between chains
  (Bumblebee 31 chains → 12); presses a second by stage 2.25 2.04 2.06 2.36 2.37 3.16 4.02 3.76, the finale with
  the busiest two seconds (6/s) and the most death caps; the first five tunes play every note they have, so only
  the last three climb. (4) Readability: messages in play on a line under the HUD, AUTO notes as grey outlines, a
  dithered dark track behind each lane from stage 5, lane letters lighter with a shadow. (5) Tunes: Mountain King's
  second bar (♯4 2 ♯4 | 4 ♭2 4) and its answer, Korobeiniki A1 A2 A1 A2, Alla Turca's B A G♯ A pair an octave up,
  the Can-Can's own strain opening it, the Canon's first eight bars from the first violin's three entries (the last
  two are the ones to judge by ear). (6) The stash slots stay on their keys. Verified: basic.txt, the clam and WIDE
  harnesses, the stash fill and replace, levels.py (every stage −0.8 dBFS peak, RMS −17.9 … −11.9), screenshots
  regenerated; device package 289,404 B, every check ok.
- **Next (nothing decided yet):** flash on Kyle's word and read the device CPU per stage (`fm1_remote.py log`), listen to
  stage 1's band and the acid line, the tune fixes (the Canon's and Can-Can's openings above all), the band thinning as
  the trip falls, finger-test strobe and invert for tearing, then fix the transcriptions he flags and
  the levels by ear; the design reviews' untaken ideas (a second stutter ring for the wet bus, an ensemble switch
  ramp) are in the review notes, not the code.
- **Resume:** `./build.sh gnomnom --host && gnomnom/build/host/gnomnom_host gnomnom/tests/basic.txt
  gnomnom/build/host`, `python3 gnomnom/tools/levels.py`, `./build.sh gnomnom --web`, `./build.sh gnomnom`
  (colima running), `python3 base/tools/fm1_install.py gnomnom/build/gnomnom.fwsc` only on "Flash it". Device
  instructions for Kyle: 1. PLAY starts a run on the title (OCT−/OCT+ pick the stage, SEL toggles JAM, EDIT
  toggles FLASH). 2. Eat a mushroom with the white key of its lane as it reaches the gnome; the LEDs under the keys
  show which lanes have something coming. 3. The lowest three black keys use the stashed power-ups. 4. PLAY pauses,
  PLAY again counts in; HOME from the pause quits. First things to hear: stage 1's Mountain King with the band,
  the house drop at stage 2, the acid line at stage 4.
