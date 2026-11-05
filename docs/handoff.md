# Handoff — Pocket Poly, end of 2026-10-31

The one page to read before the next session. Status first, then how to resume, then what's next.

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
3. Publish 0.2 once Kyle likes it (`base/tools/publish_site.sh deepfield 0.2`) and submit the site to
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
ends. **Never flashed** (Kyle's rule: only on "Flash it"); nothing committed. The design and every module
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
- **Resume:** `./build.sh gnomnom --host && gnomnom/build/host/gnomnom_host gnomnom/tests/basic.txt
  gnomnom/build/host`, `python3 gnomnom/tools/levels.py`, `./build.sh gnomnom --web`, `./build.sh gnomnom`
  (colima running), `python3 base/tools/fm1_install.py gnomnom/build/gnomnom.fwsc` only on "Flash it". Device
  instructions for Kyle: 1. PLAY starts a run on the title (OCT−/OCT+ pick the stage, SEL toggles JAM, EDIT
  toggles FLASH). 2. Eat a mushroom with the white key of its lane as it reaches the gnome; the LEDs under the keys
  show which lanes have something coming. 3. The lowest three black keys use the stashed power-ups. 4. PLAY pauses,
  PLAY again counts in; HOME from the pause quits. First things to hear: stage 1's Mountain King with the band,
  the house drop at stage 2, the acid line at stage 4.
