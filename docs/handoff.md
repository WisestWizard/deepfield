# Handoff — DEEP FIELD, 2026-11-09

The one page to read before the next session. Status first, then how to resume, then what's next.

## Start here: 1.4 is released and attested; LOOP no longer fades out on its own (2026-11-11)

**The bug Kyle reported: "the loop just stops playing after a while."** A phrase caught by playing with LATCH off
lost a twentieth of its level on **every pass, whatever DECAY said** - `echo_vel *= 1 - 0.45*decay*0.01 -
(loop_have ? 0 : 0.05)`. Measured on the wire over four minutes: velocity 56 down to 10, then silence, with
nothing on the panel able to hold it. A phrase recorded with REC never did this (`loop_have` is set only by
`dr_loop_rec`), which is why it looked intermittent. DECAY is now the only thing that fades a phrase:
`echo_vel *= 1 - (0.45 + (loop_have ? 0 : 0.25)) * decay * 0.01`. At 0 nothing fades either way; turned up, a
caught phrase still dies faster than a recorded one (40 notes against 60 at DECAY 40). Before and after on the
same test: **105.6 -> 11.3** became **117.0 -> 117.0**.

**The guard took three goes to make honest**, which is the lesson worth keeping:

1. Inserted mid-suite, it broke four later tests - `basic.txt` is sequential and stateful.
2. Moved to the end, it **passed with the fault still in**: `loop_have` carries over from an earlier take in the
   suite, so the phrase counted as recorded. It now opens with an empty REC pair to clear it.
3. `out` has to come **before** `tap PLAY`, or the first replay lands inside the tap and is never captured - the
   same trap as the LOOP length work.

Checked by reverting the fix and running the whole suite: the guard fails without it. 785 -> 787 checks, goldens
unchanged. Three existing expectations moved too, because they pinned faded velocities.

**1.4 is out.** Tag pushed, `.github/workflows/release.yml` built it, and the release carries
`deepfield-1.4.fwsc` (614,938 B, identity `FM-1_14010400`) and `SHA256SUMS`. The asset's SHA-256 is
`1f0e1fa3c6055e7f`, the same the attestation names.

**Verifying it needs the signer spelled out**, because the shared workflow signs it, not this repository - the
plain form fails with a bare "verifying with issuer sigstore.dev", which says nothing:

    gh attestation verify deepfield-1.4.fwsc --repo WisestWizard/deepfield --signer-repo zvenson/fm1-build

**Not done:** the site still serves 1.3. `deepfield/tools/publish_site.sh 1.4` is refused from here and needs
running by hand - and note it builds its own package, which will not be byte-identical to the released one (the
build is not reproducible across machines: the same 0.0 source gave `492afd54` in CI and `ec24382f` here). The
unit still runs the pre-LOOP-fix build; 1.4's app image is 424,004 B, build 7021ECFB.

## Start here: a tag now builds and releases itself (2026-11-10)

`.github/workflows/release.yml` calls the Firmware Hub's shared workflow,
`zvenson/fm1-build/.github/workflows/build.yml@v1`, which is what thefirmwarehub.com requires for the "Built from
source" badge: it builds from the repository, attests the package with `actions/attest-build-provenance`, and on a
tag publishes it as that tag's release. **It closes both of the hub's open gaps at once** - there were no GitHub
releases at all, and nothing attested.

What the shared workflow gives us: Python 3.12 and Pillow, JieLi's Linux toolchain from pkgman.jieliapp.com, the
AC79 SDK from gitee at the fixed tag, and `JIELI_TOOLCHAIN` / `AC79_SDK` set, which is exactly what
`base/tools/build.py` reads. It runs on ubuntu-24.04 x86-64, where `use_docker()` is false, so the toolchain runs
natively - the container is only for this Mac.

**Three things the worked example in that workflow does not cover here**, each found by running it rather than by
reading it:

1. **`base` is a submodule and its checkout does not fetch submodules** - so the build command initialises it.
2. **`base` carries felucca-bsp as a submodule of its own**, and the loader is built from `base/bsp/loader/`. The
   first run failed on `no such file or directory: 'base/bsp/loader/crt0_ldr.S'`. It is `--init --recursive` now.
3. **The tags here are bare versions** (`1.3`, not `v1.3`), so the trigger is `tags: ['[0-9]*']` and the version
   is `$GITHUB_REF_NAME` with no `v` to strip.

There is also a `workflow_dispatch` for a dry run. Off a tag `GITHUB_REF_NAME` is the branch, which is not a
version, so the build falls back to `0.0`; it builds, tests and attests, and the release step skips itself.

**Proved, not assumed.** Run 42768501953 is green: build, tests, SHA-256, attestation, artifact. The tests are the
same two that run here - the simulator's whole suite and all sixteen goldens - and **both passed on Linux
x86-64**, so the renders are bit-identical to the macOS build.

**1.3 has no release, and cannot get one without you.** The tag was already pushed before the workflow existed, so
it never fired, and deleting a remote tag is refused from here as destructive. Either re-push it:

    git push origin :refs/tags/1.3 && git push origin 1.3

or leave 1.3 as a site-only release and let 1.4 be the first attested one. Still open for the hub after that: its
three install tests and the mail to zven@thefirmwarehub.com.

## Start here: 1.3 is published and live (2026-11-09)

**The site is out.** Kyle ran `deepfield/tools/publish_site.sh 1.3` himself - the permission classifier refuses
it from here, since the script force-replaces gh-pages. Verified live afterwards: the landing page, the manual,
the cheat sheet, the installer, the emulator and `firmware/deepfield-1.3.fwsc` all return 200; the custom domain
answers with a valid certificate and does not redirect to github.io, so the CNAME survived; the landing page
reads Version 1.3; the manual's HOME row reads TIMBRE / DRIFT / SUB / TREMOLO and GLOBAL carries TEMPO. The
package served has the same SHA-256 as the one built here, `81693571bc29b0e0`.

**Two build ids, one source.** The unit runs **2A0C45ED** at the dev identity `FM-1_1400`; the site serves
**5708AE30** at the release identity `FM-1_14010300`. Same commit - the release build bakes in the version and
identity, which changes the image. The unit, the tag and the site are all the same source now.

**Committed and pushed**, five commits, `1cb1d8f..d1edea1` on master, tag **1.3** pushed:

- `0ff0a44` the panel: GEN 2's knobs by mode, SUB on HOME in REVERB's place, TEMPO on GLOBAL, the bigger
  E D R S N, one percent a click, and the second core's state in the log
- `8160fd8` the engine: the ensemble, WOW, the hall, the shimmer, `dly_fade`, the filter corners, the wavetable
  threshold, FM 4-OP's patch dip, and the DC blockers
- `b25c677` the handoff for all of it
- `d1edea1` the docs back in step (`deepfield/README.md` still called the four macros TIMBRE, DRIFT, REVERB,
  TREMOLO; the CC table had 51 out of order; the FM 4-OP row now says the voice dips as the patch changes)

**The site is built and checked but not published.** `deepfield/tools/publish_site.sh 1.3 --local` ran clean:
five PHOSPHOR pages, `install/` at identity **FM-1_14010300**, `firmware/deepfield-1.3.fwsc` (614,938 B), the
CNAME, and the manual's HOME row reading TIMBRE / DRIFT / SUB / TREMOLO with TEMPO on GLOBAL. `firmware.json` at
the root is refreshed, description exactly 600 characters; the spec says it carries no version by design.

**The real publish was refused** - the permission classifier blocks it as a public surface, since the script
force-replaces the gh-pages branch. It needs running by hand:

    deepfield/tools/publish_site.sh 1.3

**For thefirmwarehub.com**, from `docs/fm1-research-findings.md` section 67: a listing needs `fmhub.json` and a
release built by the shared workflow `zvenson/fm1-build/.github/workflows/build.yml@v1` with a matching SHA-256
for the "Built from source" badge.

- `fmhub.json` is at the root and current (what, author, kinds, play, cheat, image, color).
- **There are no GitHub releases at all** - `gh release list` is empty - so the 1.3 package is not downloadable
  from the repository, only from the site.
- **There is no `.github/workflows/`**, so nothing is built by the hub's shared workflow and the badge cannot be
  earned as things stand.

Still open from before: the hub's three install tests and the mail to zven@thefirmwarehub.com.

## Start here: every voice checked for a moving zero point (flashed 2026-11-09; on the unit, not pushed)

**On the unit:** flashed 2026-11-09 with `fm1_install.py deepfield/build/deepfield.fwsc --yes`, image 424,020 B,
build 2A0C45ED, dev identity `FM-1_1400`. `screen` reads FIRST LIGHT on HOME with **HARMONICS 37, DRIFT 29, SUB
27, TREMOLO 27** - the HOME swap is live. The log line reads `cpu 5% b0 ADDITIVE s0 x0` with **no `C` field, so
the second core is running**; after clearing the flash's own 32 late halves, a quiet baseline reads `late 0`,
`gap 162 us`, `lock 0`. Everything below is in this build.

The first pass measured one note per model at default settings, which understates it - SYNC's own comment says
the offset "moves with the ratio", and SYNC measured -51.9 at defaults while PULSAR, which is SYNC, was -31.4.
So: **all thirteen models x three TIMBRE x three SHAPE x two NOISE, 234 renders**, one sustained note each, no
effects, measuring DC and the energy below 5 Hz against the audible band.

**Splitting the sweep by NOISE is what made it readable.** With NOISE low - the model's own behaviour - exactly
one model is an outlier:

    FM 4-OP   -25.4 dB     <-- at TIMBRE 110 SHAPE 110
    BOWED     -51.7
    WAVEFOLD  -53.8
    RESONATOR -54.6
    ... the rest from -62.5 down to ADDITIVE at -92.3

With NOISE full, WAVEFOLD (-33.2), BOWED (-34.8) and RESONATOR (-47.6) join it - and that is **not a defect**.
White noise spreads its energy evenly, so the 5 Hz under the audible band is about 5/22050 of it, which is
-36 dB by arithmetic. Those models are simply the ones with the most audible noise in them. Flagged, not
changed: high-passing it would thin BOW NOISE, HISS and STRIKE for no fault.

**FM 4-OP needed a second blocker section.** One 20 Hz pole took it from -19.9 to -41.0 at default settings but
left -25.4 at TIMBRE and PATCH full, where every other model sits under -51. Cascaded: **-25.4 -> -46.7 at the
worst corner, -36.1 -> -58.5 in the middle.**

**Every preset now.** The worst is BOWED SKY at -47.5 dB (its bow noise), then RAIN CLOCK at -48.9, and the other
eleven are below -55. DC is under 0.6 everywhere except FOLDED SUN at 2.2. Against where this started: RAIN CLOCK
DC 40.7 -> -0.01 and below-5 Hz -18.6 -> -48.9; PULSAR 12.8 -> under the top five.

780 checks, goldens re-recorded, device image 424,020 B, build 2A0C45ED. RAIN CLOCK's rms moved 2637.4 -> 2587.0,
0.17 dB, which is the infrasonic leaving. **Not committed, not flashed.**

## Start here: the zero point was moving (built 2026-11-09, not flashed)

Kyle's expert said the clicking sounded like a zero point problem. It was, and nothing in the click hunt would
ever have found it, because it is not a discontinuity - it is the baseline the whole signal sits on wandering
under everything else.

**The evidence that was sitting there all along.** scratch.wav had a DC offset of 1855 on both channels. I
measured it on the first pass and dismissed it as 0.022 % of full scale. Measuring the engine's own renders:

| preset | DC before | energy below 5 Hz before |
| --- | --- | --- |
| RAIN CLOCK (FM 4-OP) | **40.7** | **-18.6 dB** |
| PULSAR (SYNC) | **12.8** | **-31.4 dB** |
| FIRST LIGHT | 0.4 | -70.7 dB |

A fifth of a percent of RAIN CLOCK's energy was below 5 Hz, where nothing can reproduce it. **PULSAR is the
preset Kyle reported both of his pops on.** An offset that *moves* is worse than a fixed one: the level, the
tremolo, the limiter, the pan and every knob multiply the signal, so each of them steps by wherever the offset
happens to be at that instant. Nothing lands at a zero crossing.

**Three sources, found by measuring one sustained note per model with no effects at all** (energy below 5 Hz
against the audible band): ADDITIVE -95.1, FM -100.5, and then **FM 4-OP -19.9** and **RESONATOR -34.5**.

- **FM 4-OP had no DC blocker.** Four operators with feedback leave an offset that wanders at the breath's pace.
  Added, 20 Hz corner, before the sub oscillator so the lowest notes keep their sine. **-19.9 -> -41.0 dB**, and
  RAIN CLOCK's DC went **40.7 -> -0.38**.
- **RESONATOR had none.** The coupled resonators settle off zero. **-34.5 -> -53.0 dB**.
- **SYNC had one, in the wrong place.** `render_tear` blocks DC right after the sync reset - the comment there
  says "the reset leaves an offset that moves with the ratio", so someone knew - but the signal then goes through
  the ring modulator and a soft clip, which put an offset straight back, and nothing blocked it again. A second
  blocker at the very end: **PULSAR's DC 10.84 -> -0.27 and its sub-5 Hz -31.6 -> -72.6 dB.**

**What I ruled out on the way**, each by measurement rather than by reading: the oscillators themselves (every
model under 0.0033 DC/rms on a single note), the reverb, the ensemble, the noise layer, and all seven delay types
(every one under 0.0002 DC/rms). PULSAR's offset survived turning each effect off in turn, which is what pointed
back at the voice.

What is left below 5 Hz is musical: RAIN CLOCK now peaks at 2-5 Hz (-35.7 dB) because its arp steps at about
2.7 Hz, and 0-1 Hz is down at -48.2 dB.

780 checks, goldens re-recorded, device image 424,036 B, build 2537BFE1. **Not committed, not flashed.**

## Start here: the last five off the review's list (built 2026-11-09, not flashed)

**Three measured, two preventive. The difference matters, so it is written down.**

Measured, by the largest single-sample step across the change against the same signal elsewhere:

| | |
| --- | --- |
| REVERB DAMPING, one fast detent | **0.51x** - below the signal's own slew, so nothing to hear |
| SHIMMER, one fast detent | **0.52x** |
| delay TYPE away and back inside the fade | level a second later **795.7 vs 795.7**, identical to never touching it |

- **The hall's damping and its eight line gains** are inside the feedback loop, so a step in either does not just
  change what comes out, it changes the decay of everything already ringing. Both slide over 12 ms now, and
  `out_g` follows the gains actually in use rather than the ones asked for.
- **The shimmer's direct term** was `shim_lp * shimmer * 6.0f` behind a hard `shim_on`, which FREEZE and the
  reverb bypass both flip, and `shim_lp` keeps its last value when the shimmer stops running - so the term
  appeared and vanished at full size. The gain slides per sample now; x6 also made one detent of SHIMMER a long
  way to move.
- **`dly_fade`** only ever fell, and was restored to 1.0 at the bottom. Change TYPE and change it back inside the
  12 ms fade and it was stranded part way down until the buffer emptied - the delay sitting quiet, and the next
  TYPE change then fading from there in less than the 12 ms the fade exists for. It comes back up now.

**Preventive - correct by inspection, but I could not build a case where either changes the output:**

- **The noise layer's chord bands.** The four bands are picked afresh every block by level and the resonator
  states are indexed by slot, so a slot handed to another body was retuned with the previous body's energy still
  in it, at a Q of up to 17. A slot that changes hands now starts clean, and the pan follows the body rather than
  the slot so a body cannot change sides. **Could not trigger it:** the replacement rule only fires when a later
  body gets *louder* than the quietest of the first four, and in a six-body swell the later ones are always
  behind. Six bodies at BANDPASS full rendered bit-identical before and after.
- **WAVETABLE's mip switch.** It rises at 19 kHz of harmonic and now falls only six percent under, so a glide
  still crosses and a wobble does not. **The review's reason for it looks wrong**: it said `f` carries the vibrato
  into the mip choice, but D4 with VIBRATO at full renders bit-identical before and after, so the vibrato is
  applied below this point and never moved the mip at all. The hysteresis is still right - glide and bend do
  reach it - but nothing observed was caused by this.

780 checks, goldens re-recorded, device image 423,332 B, build 8D6410AE. All fifteen presets moved against the
previous build by at most -66.3 dB with identical rms, which is the hall's new ramps settling. **Not committed,
not flashed.**

## Start here: FM 4-OP's PATCH knob no longer slams (built 2026-11-09, not flashed)

The loudest step any knob made in the engine. Every detent of PATCH is another patch (`ui.c` `knob_shown` gives
that knob exactly one per detent), and it swapped four operators' ratios, levels, algorithm and feedback in a
single tick with up to 4.9 dB of `DX_GAIN` riding on it. Sliding the output gain alone only took it from -4.4 dB
to -5.2 dB, because the gain was the small part.

**The voice dips instead.** `dx_patch()` in `drone.c`: out over 6 ms, take the new patch at the bottom, back over
12 ms, the dip interpolated per sample alongside the level (a dip in steps of a tick would be the very thing it is
there to stop). Measured on the output itself, not the residual: the largest single-sample step across the change
was **2.48x the signal's own slew, and is 1.31x now**.

The dip is also what lets the operator envelopes restart, which they had to. `v->dxs[k]` was never reset on a
patch change, so an operator that had reached stage 4 (holding L3) stayed at the **old patch's** L3 for the rest
of the note and the new patch sounded with the old one's balance. The stage machine runs again now, **from the
level the operator is at** - setting the level to zero as well restarts it from silence, which is worse.

**Two things this broke on the way, both instructive:**

1. **A preset morph sweeps PATCH itself.** RAIN CLOCK lands on SHAPE 72 and the morph takes `pcur[P_SHAPE]`
   across several patch boundaries getting there, so the dip and the envelope restart fired again and again
   during the blend: rms 2637 -> 1797, peak 12793 -> 19263, the preset wrecked. `dx_patch` now leaves the patch
   alone while `morph_pos < 1.0f` - a morph is already a crossfade. RAIN CLOCK is back at rms 2637.4.
2. **Isolating it needed the two halves tested apart.** Dip on + reset on was -1.5 dB; dip off + reset on was
   *+2.4* dB. Neither number alone would have pointed at the morph; the pair did.

780 checks, goldens re-recorded, device image 423,124 B, build C3AEC3B8. Only RAIN CLOCK (-47.6 dB) and GAMELAN
(-69.0 dB) moved against the previous build, both with identical rms. **Not committed, not flashed.**

## Start here: the knob clicks, found properly this time (built 2026-11-09, not flashed)

A DSP consultant reviewed the engine and the web turned up four sibling firmwares that fought the same battle.
Both changed the picture.

**The measurement was wrong, and that is the lesson.** The sweep's pass criterion was "the largest single-sample
step, against a control". A click of -14 to -30 dB is only 0.10x to 0.52x of a held chord's *own* largest
single-sample step, so that criterion is structurally blind to every one of them, however many parameters it
sweeps. It will always come back clean. **The test that works is an A/B subtraction**: the engine is bit
deterministic, so render the same script twice, change one parameter at a known millisecond in the second, and
subtract. A smooth change leaves a residual that ramps over tens of samples; a click leaves one that lands in a
few. `scratchpad/ab.py`, and the shape is unmistakable:

    BED LEVEL (interpolated per sample)   -1 -2 -4 -6 -8 -10 -12 ...   ramps over 20+ samples
    TAPE BANDWIDTH, one fast detent        0 -11 -20 -25 -28 -30 -32   lands in 6 samples, -26 dB
    PULSAR REV DECAY, one fast detent      0  12  12  12  12           lands in 2 samples

**Fixed, all measured by residual peak at the change:**

| what | where | before | after |
| --- | --- | --- | --- |
| the per-voice filter corner, six models | `drone.c` `voice_fc` | 86 | 16 |
| the hall's output gain `out_g` | `fx.c` | 20 | 12 |
| WAVETABLE's AIR copy flipping at the middle of the table | `drone.c` `render_scan` | 487 | 396 (and 1.25 ms -> 2.27 ms) |

`voice_fc` is the big one: TAPE, SUPERSAW, WAVEFOLD, WAVETABLE and SYNC all take TIMBRE through an exponential
(5.4 to 7.6 octaves across the knob) and recompute the one-pole coefficient every control tick with the filter
state loaded, so the output steps by the coefficient change times the filter's own high-frequency content. The
corner slides over 20 ms now. **I had built this exact fix earlier in the session and reverted it**, because the
old metric showed it doing nothing.

The WAVETABLE one is worth knowing for a different reason: `pos2` flipped from `pos + 7` to `pos - 7` the instant
`pos` crossed the table's middle, swapping frame 31 for frame 17 - opposite ends of the VOX table. **DRIFT's LFO
and the timbre walk carry `pos` across that line on their own**, so a WAVETABLE body sitting near the middle
ticked at the drift rate with nobody touching anything. It crosses over eight frames now.

**Found, measured, and NOT fixed: FM 4-OP's PATCH knob, -4.4 dB.** The loudest discontinuity in the engine by a
wide margin, and `ui.c`'s `knob_shown` gives that knob exactly one patch per detent, so **every** detent crosses a
boundary: all four operators' ratios, levels, algorithm and feedback swap in one tick, and `DX_GAIN` jumps up to
4.9 dB with them. Sliding the output gain (which is in, `v->dxcg_s`) only takes it to -5.2 dB, because the gain is
the small part. The real fix is to route a patch change through the voice hand-over crossfade that a MODEL change
already uses (`xf_seq`, `handover()`), which is a design change and Kyle's call.

**A second DX bug, found and deliberately left alone.** `v->dxl[k]` and `v->dxs[k]` are not reset when the patch
changes, so an operator parked at the old patch's L3 (stage 4) stays there for the rest of the note - the new
patch sounds with the old one's balance. Resetting them measured *worse* (-0.3 dB), because restarting four
envelopes mid-note is a bigger event than the gain jump. It needs the crossfade first.

**Ruled out, with evidence.** Not an underrun (`late` 0 across two of Kyle's own events). Not the second core: a
`plat_cpu2_wait` failure does zero the hall for a block and is not counted by `late`, but `fm1_cpu1_wait` retires
the core permanently on timeout, so it is one click per boot, not a repeating one - the log now prints `C1`..`C4`
if it ever happens. Not output truncation (the sibling firmwares' end-of-note buzz): ours truncates at a 1<<22
full scale, about -138 dBFS.

**Still open from the review, in rank order:** the hall's `dampc` and per-line `g[k]` (in the feedback loop);
the shimmer's `shim_lp * shimmer * 6.0f` direct term behind a hard `shim_on` gate, which FREEZE and the reverb
bypass both flip; the noise layer's chord bands, whose resonator state is indexed by slot rather than by body, so
five or more sounding bodies retune a Q-17 resonator and flip its pan when they overtake each other in level;
SCAN's mip switch at 296.9 / 593.8 / 1187.5 / 2375 Hz, which vibrato straddles; `dly_fade` stranded below 1.0 if
TYPE is changed and changed back inside 12 ms; and `dr_param`'s three chained stores racing the morph
interpolator.

780 checks pass, goldens re-recorded, device image 423,220 B, build 0B0CFBE4. STARLIGHT moves -27 dB against the
previous build (it is the WAVETABLE preset, so the `pos2` crossover is most of it) with peak 16353 -> 16365 and
rms 2288.5 -> 2290.7, so the level and character are unchanged. **Not committed, not flashed.**

## Start here: the crunch while turning knobs - found, measured, fixed (built 2026-11-09, not flashed)

Kyle: "its when I am turning encoders always ... it happens pretty regularly when i am actively playing the synth
and a lot is goin gon", on headphones, the speaker and the USB recording alike.

**It is not the CPU.** The build already counts dropouts: `fm1b_dbg.late`, printed by `fm1_remote.py log`, which
**reads and clears**, so each reading is a delta. `audio_commit.c` says a render that misses its deadline leaves
the half the DMA plays as it was - "a click at every late half". Zeroed the counters, Kyle played until he heard
it on WOW, read back: **late 0**, CPU 42 %, the longest the app held the interrupts 7 us. Again after the
ensemble: **late 0**. So no underrun, and the screen-blit and USB theories die with it. No diagnostic build was
needed; `debug_stats` went in and came back out.

**The metric is the hard part, and I got it wrong twice.** `hf/rms` measures brightness, not clicks - it cannot
see a step. **Max single-sample step** can. But it is confounded whenever the knob changes the spectrum: sweeping
a filter open really does make the waveform steeper, which is why TAPE's BANDWIDTH read 2.86x and nothing I did
moved it. The honest test is a **fast wiggle** - alternate between two values 8 apart every 30 ms, so the average
sound is unchanged and anything left is the discontinuity. Under that protocol BANDWIDTH, REVERB, REV SIZE and
NOISE LEVEL are all clean (0.91-1.09x).

**Three real ones, all in fx.c, all fixed:**

| what | before | after |
| --- | --- | --- |
| ENSEMBLE knob 1, OFF <-> CHOR I (and mode to mode) | 4.32x | 0.90x |
| ENS MIX near the bottom of its travel | 1.74x | 1.04x |
| DELAY 2's WOW | 1.62x | 0.89x |

The ensemble was the loud one and it was two bugs. Switching it off dropped out of the wet branch with the gains
still up, so the effect vanished between one sample and the next; the wet and dry gains now slide over 15 ms and a
fade counts as still running. Changing chorus moved the read head to a different LFO in one sample; the wet now
ducks, the mode swaps while it is down, and it comes back. WOW recomputed the head wander from the knob every
block, so a detent moved every head at once - it lands over 20 ms now.

**What did not work, and is not in the tree.** A general glide of every parameter's heard value (`dr_param` sets
`pcur` outright) measured 0.0909 against 0.0909 on a wiggle - it fixes nothing, and the discrete choices never go
through `pcur` anyway. Per-voice smoothing of the five models' filter corners: 2.86x to 2.64x, which was the
confounded number. Both reverted.

**The sweep.** 63 parameters x 2 ranges, wiggled against a control with the effects running: **120 of 126 clean**.
Per-model: TIMBRE, SHAPE and NOISE wiggled on all thirteen models, nothing above 1.55x and all of those
confounded. A page audit by byte-identical renders: every DELAY knob does something on every one of the seven
types; on OSC the only knobs that do nothing are the ones the UI already blanks (NOISE on ADDITIVE, SUPERSAW and
FM; SHAPE on BOWED). FM 4-OP's FEEDBACK looked dead but only because BRASS's own feedback is already at the
ceiling - it works on the other six patches.

**Still not reproduced:** Kyle's pops on BANDWIDTH with TAPE on PULSAR, and on REVERB in the same setup. Matched
preset, model and knob in the host and they come back clean. Next time one of those happens, **read the log before
touching anything else** - if `late` is non-zero there, those are a different fault from the three above.

**Goldens re-recorded** (the ensemble really does sound different now - that is the point). Measured against the
old renders, twelve of sixteen move less than -59 dB; STARLIGHT -37 dB and FOLDED SUN -49 dB are the largest, a
small phase shift of the tape wander rather than a change of character. 780 checks pass, device image 421,812 B,
build 4CE6A3DF. Only `deepfield/engine/fx.c`, `golden.txt` and one manual row changed. **Not committed, not
flashed.**

## Start here: one percent a click, bigger E D R S N, TEMPO on GLOBAL (built 2026-11-09, not flashed)

**The knobs skipped values.** Kyle: "on a lot of the encoders when you are going really slow it take two clicks -
it should be one unit per click when slow and the screen shouldnt skip values". Measured on the host with `turn`
(the real path, not `spin`): a slow detent moved a 0..127 knob by 3 raw, so DRIFT read 32, 34, 37, 39, 41, 44 -
two shown values stepped over on most clicks. `knob()` now steps the knobs that are *shown as a percent* (K_PCT
and K_TEXTURE) by one percent a detent, through `pct_raw(p) = (p * 127 + 99) / 100`, the lowest raw that reads
p%. Same probe after: 30, 31, 32, 33, 34, 35, 36, 37, 38, 39. Fast (a detent within 40 ms of the last) still
moves 8 raw, so a sweep is 16 clicks as before; a slow sweep is now 100 clicks instead of 43, which is the price
of not skipping and is what was asked for.

**Not every knob, and why.** Times and frequencies (ATTACK, RELEASE, GLIDE, a free DLY TIME, SUPERSAW's and
VOID's TIMBRE) span too wide a range for every value they can display to have a detent, so they keep `knob_slow`'s
2 or 3; TEMPO stays 1 raw, about 1.5 BPM, which is as fine as 128 steps over 40..240 allows. If Kyle reports
skipping on a *time* knob next, the honest fix there is a coarser display, not a finer step.

**The device's own decoder is upstream.** `fm1_enc_take` in `base/bsp/boards/fm1/fm1_input.h` counts four
transitions to a detent and drops a click it only saw one transition of (`n = |sub| >= 2 ? (|sub| + 2) / 4 : 0`).
That is where a genuinely dropped click would come from, it is in the pinned upstream base, and the host does not
run it - so it was not touched and has not been ruled out. If two clicks per unit is still felt on the unit after
this build, that decoder is the next place to look, not `ui.c`.

**E D R S N made large.** Kyle asked for it, because with REVERB off HOME those letters are the only thing that
says the reverb is on. FONT_T at 9 px pitch -> FONT_B (the bold 16 px face) at 11 px pitch, x 184 and y 28, which
clears the SEQ FRZ pill (ends at 176) and the morph bar (y 44) and stops inside 240.

**TEMPO is on GLOBAL now.** It was already the one tempo in the firmware - a DLY TIME set to a division of the
beat reads `dr_arp_bpm(P_ARP_RATE)` when no MIDI clock is coming in - but it only appeared on the GEN page with
MODE on ARP. GLOBAL's knob 4 was empty, so it is SCALE, ROOT, MORPH, TEMPO now, the same parameter in both
places. The generator's RATE in the non-ARP modes is still a time in seconds, not a division of TEMPO; following
an external clock it already counts clocks. Making RATE a division of TEMPO would be a real change and was not
made.

780 checks pass, all 16 goldens match, device image 421,408 B, build 233E6605. The KNOB STEPS test now asserts
the shown percent with `said 40% / 41% / 42%` rather than raw numbers alone. Manual, both cheat sheets and the
site cheat sheet updated; **all 24 screenshots re-shot**, since the header letters changed in every one. **Not
committed, not flashed, not published.**

## Start here: GEN 2 shows only the mode's knobs, and SUB takes REVERB's place on HOME (built 2026-11-09, not flashed)

Kyle asked how EUCLID and DIV work and whether they work. They do - measured on the wire at TEMPO 120: DIV 1/8
gives 249 ms steps and 1/4 gives 499, EUCLID 3/8 strikes on steps 0, 3, 6 of 8 and 5/8 on 0, 2, 4, 5, 7, and all
eight patterns give exactly their named hit count with the accent (127 / 110 / 94) landing on the cycle's first
and middle steps. What was wrong was where they lived.

**GEN 2 was showing knobs the mode never reads.** `arp_tick` only runs in ARP, so OCTAVES, EUCLID and DIV did
nothing in the other five modes; RETURN is read only by the wander and chord branches of `orbit_plan`, and ARP
returns before the countdown, so RETURN did nothing in ARP, PROG or LOOP. WALK was four knobs of which one
worked. `page_param` now gives GEN 2 the mode's knobs the way the GEN page already did: RETURN in WALK, SHIFT and
RANDOM; ROAM in PROG and LOOP; OCTAVES, EUCLID and DIV in ARP; the rest empty (`PID_NONE` draws a rule mark and
`knob()` returns). Values are kept while hidden and MIDI can still set any of them.

**Proving a knob inert needs a long run.** The first probe said RETURN did nothing in WALK either. It was a 7 s
render at a slow RATE with the voices still at home, so gravity had nothing to pull on - the measurement, not the
firmware. At 30 s and a fast RATE it clearly acts. The inert results (ARP, PROG) are from the long run; the
method is two renders that differ only in the knob, compared by checksum.

**HOME: REVERB out, SUB in.** Kyle: "do we need to have reverb on the home page now that we have all of the
dedicated FX - what other voice control could go there - maybe the sub?" REVERB was on HOME while it was the only
effect; it is also FX REVERB knob 4 and FX MIX knob 3, so nothing is lost. HOME now reads as the voice itself:
TIMBRE, DRIFT, SUB, TREMOLO - what it is made of, how it wanders, how heavy it is, how it moves. One line in
`PAGES`. The R in the header's E D R S N still shows the reverb is on, which is what a new user has to go on.

**Every preset already ships with reverb on**, which Kyle asked about: all 16 read `fxon2 1`, REVERB 22 to 88,
median about 60. The driest three are RAIN CLOCK 22, PULSAR 25 and GAMELAN 28, dry on purpose. Nothing was
changed there; lifting those three is an open question and would re-record their goldens.

**A test moved rather than died.** The block that checked "an effect switched off on the FX keys reads OFF on its
knobs, and turning one brings it back on" did that on HOME's REVERB knob. It runs on the FX mixer (page 15) now,
with the REVERB page half unchanged.

775 checks pass, all 16 goldens match, device image 421,360 B, build B2C577A9. Manual, both cheat sheets, the
site cheat sheet and the landing page updated; `screen-home`, `screen-frozen`, `screen-browse` and
`screen-rhythm` re-shot (screens.txt now switches to ARP before the rhythm shot, since NEBULA is in RANDOM and
the page would otherwise be three blanks). **Not committed, not flashed, not published** - the unit is still on
the pedal build and the site on 1.2.

## Start here: PROG no longer freezes the bass (flashed 2026-11-09; on the unit, not pushed)

Kyle, on the pedal that every progression sat over: "I don't know if it was the best choice to never chagne the
lowest root note on motion - lets discuss". He was right. `cycle_pedal()` pinned the lowest voice for as long as
PROG ran, which meant every chord of every progression was an inversion over one bass note, no progression could
arrive anywhere, and it cost a voice out of eight the whole time. It is **deleted**, with `cyc_pin` and its call
from `dr_orbit()`. The bass now follows the same common-tone rule as every other voice: it holds when it is a note
of the chord, which is most steps, and moves when the root really changes. Measured on a struck C3 G3 C4 E4,
ROUND reads `F A C D` / `F A B D` / `E G B D` / `E G A C` - four pitch classes a step, one or two voices moving,
the bass walking 53 -> 52 -> 50. For a pedal you now **LOCK a voice yourself** (hold its key, KNOB 4), which PROG
leaves alone and leads the chord above - or below, on a sinking progression.

**Removing the pin exposed a worse problem.** The pinned voice was doubling as the floor (or the ceiling) that
bounded the register contour. With nothing pinned the contour's octave displacement had only the +-12 home leash
to stop it, so it threw a voice to the end of its leash on the first step and parked it there: SLOW RISE came out
spanning 45 to 76, two chords a fifth apart rather than one chord that moved. The register pass now refuses an
octave move that would stretch the chord more than an octave wider than it was struck (`span + 12`). After that
both contours survive the pin's removal: UNDERTOW's centre 58.5 -> 53.8 (sinks), SLOW RISE 62.0 -> 65.0 (climbs
monotonically, no bass collapse). `cyc_top` moved into the CYCLE branch of `orbit_plan` so it is read per step and
follows CHORDS as the knob turns.

**Four CYCLE tests encoded the old pedal.** `pending 1` -> 2, `90 40 7F` -> `90 45 7F`, `pitch60 60` -> 57,
`pitch67 64` -> 69. Rewritten to the measured values with a comment saying what the block is now for: nothing is
pinned, the bass holds because C is a note of the chord, not because PROG froze it.

**Still lumpy, and not the pin's fault.** The contour moves the chord by displacing one voice an octave at a
time, so the chord passes through wide voicings on the way - ROUND still parks a voice at 72 for several steps.
That is the pre-existing mechanism, not something the pin was hiding. Worth a listen before deciding it needs work.

**On the unit:** flashed 2026-11-09 with `fm1_install.py deepfield/build/deepfield.fwsc --yes`, image 421,328 B,
build A9D18294, dev identity `FM-1_1400`. `log` clean, `screen` reads FIRST LIGHT on HOME, CPU 5%. 756 checks
pass, all 16 goldens match. Committed **f40772d**, **not pushed**; the site still serves 1.2, which has neither
this nor the LOOP length fix, so **the unit is two changes ahead of the published build**. Next release step is
1.3 once Kyle has heard the harmony.

## Start here: LOOP keeps how long you held each note (flashed 2026-11-09; on the unit, not pushed)

Kyle on 1.2's LOOP: "none of the dynamics/length of notes or anything seem to be recorded - a user plays something
like BAAAAAAAAAAAAA AAAAAAAAAAAAH AAAAAAAAAAAAAAAH and the loop is BA AA AA" - phonetic, as he said: long held
notes coming back as blips. He also asked for a prompt on the screen when LOOP is first selected.

**Measured first, on the wire.** The host writes a timestamped `midi_out.txt` beside its WAV, which is the tool for
this: a note held 251 ms and a note held 1802 ms both came back at 168 ms. The take kept each note's pitch,
velocity and the gap *before* it, and nothing else; every replay used one fixed gate,
`echo_off[j] = 1 + orbit_per / (2 * (echo_n + 1))`, which has no per-note term in it at all.

**Three things were wrong.** The lengths were never captured: a key down now stamps its slot (`loop_open`,
`loop_live`) and the key up closes it into `echo_len`, with `loop_close(0, 1)` ending anything still held when the
take closes. Then, even with the lengths kept, **RATE was squeezing the phrase**: the loop fitted a 2.55 s take
into whatever RATE was set to (1.0 s in the probe), so a long note could not fit however well it was recorded.
Closing a take now sets RATE to the phrase's own span - the gaps plus the last note's length - so it loops at the
speed it was played; turning RATE afterwards stretches the gaps and the lengths together. `tot` is that same span
now, so the stretch factor is 1 when RATE follows the take. Measured after: 1501 ms played came back 1707 ms and
the short note stayed 64 ms.

**And a bug of mine, found on the way.** The first cut put `loop_live[i] = 0;` after a braceless `for` in
`orbit_reset`, so it ran once with `i == 8` and wrote one past the array, clobbering the static beside it -
`loop_have`. A recorded phrase then faded at DECAY 0, which the manual says never happens, and ROAM's development
of the phrase (gated on `loop_have && roam`) was silently dead. It showed up as replay velocities reading 120,
114, 108 where they should have been a flat 127, and as one test failing on some runs and not others. Braced, and
the velocities are flat across passes now.

**The prompt.** Landing on LOOP with nothing recorded says `LOOP` / `REC THEN PLAY` instead of `MODE LOOP`, and
the phrase is in `words.tsv` for the screen reader as "press record, then play a phrase".

**A test lesson.** The old take block asserted `expect_out USB 90 3C 7F`, pinning a velocity byte; in a long
stateful run that byte is not stable, and the check passed or failed by the run. It asserts `expect leadnote 60`
and `expect_no_out USB 80 3C 00` now - the note is sounding and has not been let go a quarter second in, which is
the regression for this bug and has no velocity in it. Also: `out` must come before `tap REC`, because the tap
itself advances time and the first replay strikes inside it.

**On the unit:** flashed 2026-11-09 with `fm1_install.py deepfield/build/deepfield.fwsc --yes`, image 421,744 B,
build 5772138A. It is **back to the dev identity `FM-1_1400`**, not the release `FM-1_14010200` the site serves -
this is a dev build, so the unit is now ahead of the published 1.2 by these LOOP fixes. `log` is clean and
`screen` reads FIRST LIGHT on HOME. 756 checks pass, goldens unchanged. Committed **1c8dd69**, not pushed, and the
site is still serving 1.2 without these fixes.

## 1.2 is published and on the unit (published and flashed 2026-11-09)

**The site is 1.2.** `deepfield/tools/publish_site.sh 1.2` built the release package and pushed gh-pages:
https://deepfield.fog.baby serves Version 1.2 with the CNAME intact, the landing page, the cheat sheet, the manual,
the installer and the emulator, and `firmware/deepfield-1.2.fwsc` (614,938 B, identity `FM-1_14010200`) at 200.
The site had been at 0.13 since 2026-11-08 while 1.0, 1.1 and 1.2 were each built and flashed without going out, so
the public pages jump 0.13 -> 1.2 and the 1.0 and 1.1 packages were never published.

**ROAM was on CC 14, which is already the "next move now" trigger.** Found while checking the docs before
publishing: the manual's CC table had two rows numbered 14, on adjacent lines. In `midi_cc` the trigger's `case
CC_GEN_NOW:` returns before `cc_param` is consulted, so a CC 14 coming in could never set ROAM, and the ROAM knob
*sent* CC 14 as it turned - a second DEEP FIELD listening would have jumped a chord. **ROAM is CC 51 now** (the
only free number next to LOOP's 52 and 53). Verified by turning the knob in the simulator and reading what went
out: `B0 33 03` five times as it moved, and nothing on `B0 0E`.

**And ROAM's knob was not reachable in LOOP.** The manual and both cheat sheets said ROAM sits on GEN 2 for LOOP as
well as PROG, and `orbit_tick` does read it in the LOOP path - but the knob slot was gated on `OM_CYCLE` alone, so
in LOOP that slot still showed ARP's OCTAVES, which does nothing there. The gate takes `OM_ECHO` too now. Verified
the same way: in LOOP the knob sends CC 51 and no longer sends OCTAVES' CC 119.

**The site cheat sheet was three passes stale** - it is hand-written HTML (`deepfield/site/cheatsheet/index.html`),
separate from `docs/CHEATSHEET.md`, and had not been touched since 0.13. It still listed STRUM as a GEN mode,
described PROG as "I IV V and seven more" in Roman numerals, had GLO opening GLOBAL rather than the two mixer
pages, and knew nothing of LOOP's recorder or the delay's quiet retime. All seven blocks rewritten. The README,
the landing page and `fm1.toml`'s hub description now name the chord generator as well (the description has a
600-character ceiling the build enforces; it sits at 600).

**Where it stands:** 751 checks pass, all 16 golden renders match, the device image is 419,856 B. The manual's
screenshots are not deterministic - re-running `tests/screens.txt` rewrites six or seven PNGs with the same
content at a different drift phase, so leave them as committed unless a screen really changed.

**The unit runs the published release.** On Kyle's "Flash it", `fm1_install.py deepfield/build/deepfield-1.2.fwsc
--yes` wrote the same package the site serves, so the FM-1 reports `FM-1_14010200` rather than the dev identity
`FM-1_1400` it had carried through every build until now. The installer checks that identity itself after the
restart. `log` is clean (boot, `sound back: ready`, `cpu 5% b0 ADDITIVE s0 x0`, no crash record) and `screen` reads
`DEEP FIELD. FIRST LIGHT. HOME. HARMONICS 37%. DRIFT 29%. REVERB 45%. TREMOLO 27%. VOICES 0. CPU 5%`. Note that
`fm1_remote.py crashes` and `stats` both answer "no answer" on this build - the debug stats are off, which is their
normal state; `log` and `screen` are the working read-backs.

The two fixes were proved in the simulator, not on the hardware: confirming them on the device would mean injecting
button presses to walk to GEN 2, which is the one thing not to do over USB while Kyle has the instrument.

Still open: Kyle's listening pass on the harmony (the eight progressions, UNDERTOW's sink, SLOW RISE's climb, ROAM,
the pedal, a take recorded with REC on the GEN page). Still not built: a per-step chord template and a per-step
accent. Not done: the GitHub release with the `.fwsc` attached, the hub's three install tests, and the mail to
zven@thefirmwarehub.com.

## Start here: the two limits closed, and LOOP records a phrase (flashed 2026-11-08; 1.2 is on the unit)

**On the unit:** installed 2026-11-08 with `fm1_install.py deepfield/build/deepfield.fwsc --yes`, image 419,840 B,
build 4EA946D2. It came back as `FM-1_1400`, `log` shows no crash and Bluetooth MIDI advertising as FM-1 F9FC, and
1.2 reads back where 1.1 cannot: with MODE on PROG the GEN page shows `CHORDS ROUND` and GEN 2 shows `ROAM 0%` in
the slot that holds OCTAVES for every other mode. What is left is Kyle's listening pass - the eight progressions,
UNDERTOW's sink and SLOW RISE's climb, ROAM's development of the written progression, the pedal, and a take
recorded with REC on the GEN page with LOOP showing.

Kyle read the two honest limits at the end of the PROG pass and said "fix this too", and chose LOOP over ARP for
the phrase recorder.

**A progression can now really sink or climb.** The limit was that a step names pitch classes, so a degree said
nothing about register and the signed table bought nothing. Each progression now carries a contour as well -
`PROG_R`, where each step wants to sit in semitones from the struck chord's middle. The leading reads it as a
pull, and a voice already on a note of the chord keeps its pitch class and takes the octave nearer the mark: the
harmony is untouched and the body of the sound moves, one voice a step. The first cut only biased the *search*
direction, which did nothing at all, because a voice already on a correct note holds - measured, SLOW RISE drifted
up four semitones and UNDERTOW did not move. Then the register move was blocked downward by the pedal: with the
lowest note pinned at 60, nothing above it can get under it. **For a progression whose body sinks the pedal is now
the ceiling** (`prog_falls`, `cyc_top`) - the highest note is pinned and the chord falls away beneath it, which is
the held-ceiling sound the ambient composer asked for. Measured after: UNDERTOW's centre goes 64 down to 56 and
back in one move, SLOW RISE climbs to 71.

**ROAM has the phrase memory now.** A turn of the table is a phrase and four turns are a section: A the theme, A'
the theme with its last two chords turned home, B fresh and furthest out, A" the theme with about one chord in
seven redrawn. Step lengths and the contour are kept from the table throughout, so the rhythm and the rise and
fall rhyme where the chords do not. Verified by printing the phrases: written ROUND is `0 5 3 1 6 4 2 0`; A came
out `0 5 1 2 0 4 2 0`, A' `0 5 1 2 0 4 0 0` (the theme, ending turned home), B `5 1 3 1 6 4 2 0`, A" `0 5 3 2 6 4
1 0`. The first cut produced `0 0 0 0 0 0 5 0` - a whole phrase picked against state that never advanced, so every
chord came out as home; `cyc_step_seen` now walks the anti-oscillation, coverage and home-debt state with the
phrase as it is written. A second wart found the same way: the theme array was only filled when ROAM was already
up, so turning ROAM up mid-flight developed an empty progression; it is seeded from the table now.

**LOOP records a phrase and moves it.** With the GEN page up and MODE on LOOP, REC opens a take: the screen says
LOOP RECORDING, the pill counts the notes, and **LATCH comes off for the take** so the keys play a melody instead
of piling up a drone, going back on by itself when the take closes. REC again closes it and it loops at once. The
gaps between the notes are kept (`echo_gap`, in sixty-fourths of a sample count against a clock added to
`orbit_tick`), so the phrase comes back in the rhythm it was played in, stretched to fit RATE rather than spread
evenly over it. At DECAY 0 a recorded phrase never fades; a phrase merely caught from passing leads still does.
ROAM carries each pass to another degree of the scale using the same weights PROG uses. REC is the Bluetooth
button everywhere else, so the gesture is gated on the page and the mode. Measured: 300 ms and 700 ms played came
back as 244 ms and 505 ms inside a one-second period, and with ROAM up the passes transpose. Two bugs on the way:
the gap was being counted after the note rather than before, so the first two notes of every phrase landed on the
same instant; and `loop_clk` first lived where it only advanced while the generator ran, so a whole take was
stamped zero and fell back to the even spread.

**ARP, settled.** Its engine was correct all along (struck 68, 60, 64: PLAYED gives 68 60 64 80 72 76, UP gives 60
64 68 72 76 80). What was wrong was the manual: it said "ARP and LOOP play from the keys, so they need nothing
latched", which is false for ARP - it arpeggiates the latched voices, so following the manual made it silent. That
line is fixed, and the phrase feature Kyle wanted went into LOOP instead, where the capture and replay already
existed.

**Where it stands:** 751 checks pass (twelve new ones for the take), goldens unchanged and levels in band, manual
and cheat sheet carry the recorder, the contours and the phrase form. Still not built: a per-step chord template
(a seventh or a sixth where a step-move needs one) and a per-step accent, both of which the harmony specialist
ranked third and fourth. Nothing is tagged, the site is still at 0.13.

## PROG rebuilt on voice leading (built 2026-11-08; NOT flashed; 1.2)

Kyle on the 1.1 progressions: "they are so short they just kind of beep back and forth - we need things that
progress nicely - a user should be able to set the global scale and then use at least some of the chords, motion
settings to have sequences play out across that scale - generated or in a fixed manner". He asked for the three
musician personas to be consulted, so an ambient composer, a generative/modular musician and a harmony and
voice-leading specialist were each briefed on the engine as it stood and asked for concrete, implementable advice.

**All three independently named the same fix, and it was not longer tables.** A step must name a target *chord*
and each voice must move to its nearest note of it or hold. The old code moved every held voice by the same number
of degrees, so no note was ever retained; the harmony specialist's framing was that harmony is heard as a held
pitch being re-interpreted, and with nothing held the ear parses eight voices as one timbre with a moving
fundamental - a pitch bend, not a progression. The 0.13 s glide made it worse: eight voices sliding a sixth
together *is* portamento, where one voice sliding a step against held ones is harmony.

**What was measured before anything was written.** CATHEDRAL sat on VAMP `{0,5,0,5}` at 8 s a step - two chords
alternating every eight seconds for ever, six of the eight tables being two-position oscillators. In MINOR
PENTATONIC the note went 60 -> 72 -> 60 -> 72: five degrees in a five-note scale is an octave, so CATHEDRAL there
was an octave bouncing, not a chord change. In WHOLE TONE a degree move left the chord's pitch classes unchanged
(60 -> 64 -> 60), so PROG was harmonically inert. And the table was `uint8_t`, so every progression could only
leave home upward.

**The degree restriction was wrong, and this was checked against the firmware's own scale masks.** The old comment
held that degrees 0, 2, 3, 4 and 5 are the five consonant in major and minor alike. Classifying every degree's
0-2-4 stack in all ten scales: DORIAN's degree 5, PHRYGIAN's 4, LYDIAN's 3 and MIXOLYDIAN's 2 are the diminished
one and were all *inside* the safe set, while each mode's signature chord (MIXOLYDIAN's flat seventh, PHRYGIAN's
flat second, LYDIAN's second, minor's flat seventh) sat outside it. Right in two scales of ten, inert in three,
wrong in four - and it was removing exactly the chords that make the modes sound unlike each other. `dr_scale` now
derives the diminished degree and the dominant degree from the mask and caps each at one period.

**Built.** `cycle_plan` in `engine/drone.c` leads the chord: the target is a pitch-class set, a voice already on
one of its notes holds, the rest take the nearest reachable note, no voice crosses another, one or two move per
step (three on a long step), moves over a major third are refused as suspensions and the leash opens to a fifth on
a long step so a marooned voice can rejoin. Chord coverage is tracked so voices do not pile onto the same two
notes - without it a triad wore down to a bare C-G-C shell inside a dozen steps. The outer voices are asked to
close up or open out when the spread drifts more than five semitones from the struck spread. Eight new
progressions of 8 to 15 steps with unequal step lengths; the pedal pins the lowest note when three or more are
held; ROAM (GEN 2 knob 2 in PROG, CC 14, STRUM's old slot so saved banks still read) develops the written
progression by weight - theme pull, anti-oscillation on the last two degrees, coverage pressure, and a home
gravity that grows with the steps spent away - re-centring a fifth between turns and coming home every fourth.

**Two bugs of my own that the measurements caught.** The motion budget cancelled the *smallest* moves first, so
the one voice with the biggest leap did all the moving and the voices below it never changed; it now gives up the
furthest. And auto-pinning the pedal with a single note held left nothing to lead, so PROG sat silent - it now
needs three held notes before it claims one.

**Honest limits.** Because a step names pitch classes and voices take the nearest, a degree and that degree plus
the scale's length are now the same thing, so the signed table buys nothing and real descent - a chord sinking
away over a progression - would need the heads biased downward over a run of steps, which is not built. The
generator has the weights, the moving home and the anti-oscillation but not the phrase memory the generative
musician specified (A A' B A", with the chord rhythm remembered and the ending varied). Measured over the same
half minute, ROAM full gave thirteen distinct chords against the written table's four, with six of twenty-six
changes a swing back to the chord before last - better than the old table and not yet as directed as it could be.

**ARP, and what is still owed.** Kyle: "we also need to fix the played arp mode or make it make more sense how to
do it". ARP's engine is correct - struck 68, 60, 64 in that order, PLAYED gives 68 60 64 80 72 76 and UP gives 60
64 68 72 76 80. What he actually wants is a different feature: "it should have a way for a user to play a sequence
in that the arp then imitates and moves", and his control sketch is to select the mode, hit REC, play a sequence
possibly including chords, and hit REC again to close it. **This is designed and agreed but NOT built.** The
decided shape: REC means record-a-phrase only while the GEN page is up with mode ARP and PATTERN PHRASE (REC is
the Bluetooth button everywhere else, so the context has to be narrow), the pill reads `REC 3` while armed, keys
stop latching so a melody can be played rather than a drone accumulated, notes within 60 ms are one chord step,
REC again closes and starts it looping, 16 steps closes it itself. One question for Kyle before it is written: it
may belong in LOOP rather than ARP, since LOOP already captures played notes with their timing and only lacks the
transposition, and that would also answer his 1.1 complaint that LOOP reads EMPTY with LATCH on. Also still
wrong: MANUAL.md line 411 says "ARP and LOOP play from the keys, so they need nothing latched", which is false for
ARP - it arpeggiates the latched voices - and that alone would make ARP look broken.

**Where it stands:** 739 checks pass, all 16 goldens re-recorded and levels in band (peaks -6.5 to -15.5 dBFS),
screens regenerated, manual and cheat sheet carry the new progressions, the pedal, ROAM and the degree change. The
DESCENT preset moved from the old DESCENT table to UNDERTOW and CATHEDRAL to ROUND. Nothing is tagged, the site is
still at 0.13, and the delay and mixer work below is in the same uncommitted tree.

## The delay's time change, and the mixer moved to GLO (built 2026-11-08; NOT flashed; 1.2)

Kyle played 1.1 and said two things. "A lot of the delays when you change the delay time it makes a pretty loud
sound - I think for this synth it should be more chill somehow", and "the mix controls should be together - after
you hit global they should be the first two pages and then the scale and root etc".

**The loud sound, found and fixed.** It was never the crunch it looked like in 1.0. The unit's own log settled
that first: after twenty seconds of turning DLY TIME under a latched chord, every `cpu N% bM MODEL sS xK` line
read **x0** - no xruns - with `late 00000000` and the CPU at 40%. So nothing was dropping out. The cause was the
glide itself. `dly_read` chased the new time at `(tgt - dly_read) * 0.0008` a sample, and in the synced part of
the knob one detent is a jump of about 5,500 half-rate samples: the read pointer set off at **2.2 samples a
sample**, which replays the whole delay buffer at 3.2x - twenty semitones up - and further turns reach 5.4x and
9.5x, running the buffer *backwards*. The comment called it "like tape", but no tape slid two octaves in sixty
milliseconds. Measured in the simulator with five notes latched and DLY MIX at 77%, one detent raised the band
above 4 kHz by **+3.9 to +4.7 dB** in the 10 ms at the turn on PING PONG, DUB, OCT DOWN, REVERSE and GRANULAR.

The fix is two parts. A commanded move of more than 300 samples takes the wet path out over 12 ms (the feedback
coefficient with it, so nothing of the loop is written back while it is down), lands the heads on the new time,
resets REVERSE's swell and GRAINS' pieces so they do not start mid-window, and fades back. Anything smaller
glides as before, but the walk is clamped to **0.19 samples a sample** - the content's pitch stays inside a tone
and a half. TAPE is excluded from the mute and keeps its slide, bounded by the same clamp: verified directly, its
walk would have been +4.4 (reversed at 3.4x) and is now held to 0.19. After the fix the spike at the turn is
**+2.2 to +2.9 dB**, which is where TAPE and OCT UP already sat - the drone's own variation. GRANULAR's high band
did not move (+4.9), and its broadband level is flat across the turn (-32.3 to -32.0 dB): that number is the
randomness of its own grains, not a surge. A `dly_snap` flag makes the first time it is ever told land rather
than slide, so a boot does not creep into place over a second.

**Two measurement traps worth remembering.** FIRST LIGHT ships with **DLY MIX at 0**, so a script that does not
set it measures a silent delay - four rounds of before/after came back byte-identical before a probe on the branch
showed `send 0.000`. And **P_DLY_MIX is 19**, not the 16 a quick read of the enum suggests; `cc -I. ` on a
four-line program that prints the constants is the way to get an index right.

**The mixer under GLO.** MIX and FX MIX were the HOME button's second page and the FX button's fifth; they are
now the GLO button's first two, with GLOBAL, MIDI and USB after them. The page enum and `PAGES` moved with them,
`B_GLO` opens `PG_MIX`, and **`PAGE_BTN` was fixed**: it still had 17 entries for 19 pages from the 1.1 work, so
every page from index 1 on lit the wrong button and the last two indexed past the end. Its existing values happen
to be right for the new order, so it needed only two more `B_GLO`.

**Where it stands:** 738 checks pass (the suite's page numbers remapped, each `tap GLO` advanced two pages to
reach the scale, and the MIX block rewritten to walk MIX -> FX MIX -> GLOBAL). The screens script takes the two
mixer shots under GLO now. 14 of the 16 goldens moved and all 16 levels sit in the same band (peaks -6.5 to
-17.5 dBFS): a preset load is a 12 ms mute-and-land instead of a swoop, which changes the first instants of every
render that uses the delay. The manual and the cheat sheet carry the new page order and what a time change does.
**Nothing is tagged and the site is still at 0.13**: 1.2 goes out once Kyle has heard it.

## What Kyle heard on 1.0, and the pass that answered it (flashed 2026-11-08; 1.1 was on the unit)

He played the 1.0 flash and came back with five things. Four are built and verified here; the fifth needs twenty
seconds of his hands on the unit before anything is written.

1. **"FM amount control does nothing."** True, on the **FM 4-OP** model only (on FM it always worked). Proved in
   the simulator before the fix: RAIN CLOCK rendered at FM AMOUNT 0, 64 and 127 came back with the same level and
   the same brightness to three figures (0.035, 0.035, 0.035; the hashes differed in the noise). The cause was the
   operator law in `render_dx`: a modulator's level and its envelope were both applied as attenuations and summed
   in the DX's log domain, so E.PIANO's modulators sat about 60 dB down - the knob was moving something nobody
   could hear, and every FM 4-OP patch was in truth a stack of plain sines. A modulator's level is now an index in
   turns of phase (MASS squared, DRIFT breathing it, each operator taking its share by its own level and
   envelope); carriers keep the DX gain law, so the voices' loudness and balance are untouched. Measured after:
   brightness 0.035 / 0.100 / 0.383 across the knob, level flat within 0.2 dB. ORGAN, the one patch with no
   modulator, gets a tilt of its upper three sines instead, so the knob is never dead.
2. **"Crunchy or clipping noises when I use encoders to change the delay."** **Not reproduced, not fixed.** Two
   hypotheses were tested in the simulator and both failed: a held chord with DLY MIX, TONE, DIFFUSION, FDBK and
   DLY TIME each jumped the whole way produced no discontinuity at all (the worst sample step after a jump was
   1.0x to 1.3x the slope before it - no click), and eight keys held with DLY MIX full peaked at -3.1 dBFS with
   nothing over 32000. So it is not parameter stepping and not digital clipping in the engine. What is left is
   the device: audio xruns while the screen redraws under a turning encoder. `fm1_remote.py log` prints
   `cpu N% bM MODEL sS xK` - **K is the xrun count**. The test: latch a thick chord, turn DLY TIME back and forth
   for twenty seconds, then read the log and see whether `x` has moved. Nothing should be written until it has.
3. **"I still have trouble hearing the delay."** His own diagnosis, and it is right: a thick latched chord buries
   it. Hence the mixer, which he asked for and specified: **MIX** (HOME, SELECT once) with **BED**, **LEAD** and
   **OUT**, and **FX MIX** (FX, SELECT four times) with **ENS MIX**, **DLY MIX**, **REVERB** and **SHIMMER** on
   one screen. BED scales the latched voices (CC 54, in the preset, squared law, unity at the top); LEAD is the
   LEAD page's LEVEL under its mixer name; OUT is a trim in the settings, under the MASTER pot, on the speaker and
   on a USB recording alike. The settings record grows no byte: OUT took the spare one, DEF5 reads straight as
   DEF6 with OUT at unity.
4. **"We need to replace MULTI TAP, it does not sound good."** Replaced by **DUB**: one head whose feedback runs
   through a resonant low-pass that darkens and rings a little more each pass, the two sides 11 ms apart. TONE
   sets where the filter sits (180 Hz to 3.5 kHz) and how hard it rings; near full FDBK it stops decaying and
   drones. Its first cut had a Q of 2.3 and the filter's own peak took the loop's gain over 1: RAIN CLOCK's render
   came up 8 dB over twenty seconds. The Q is 1.0 to 1.3 now and the preset renders flat (-8.9 dB peak at the
   start, -8.3 in the middle, decaying to -37 in the tail, in line with the other fifteen). MULTI TAP's onset gate
   went with it.
5. **"I don't like many of the chord progressions"** and **"I don't know what strum does at all."** The eight
   progressions were cadences (I IV V, I VI IV V...), which is why they sounded unfinished under a drone: they are
   built to resolve, and a drone never does. They are loops now - VAMP, PLAGAL, MEDIANT, SUSPENDED, DESCENT,
   RISE, ROUND, DRIFT - each moving one or two notes with nothing leaning on a dominant, and each built only from
   the scale degrees that stand as a full chord in a major and a minor scale alike (0, 2, 3, 4 and 5; 1 and 6 are
   diminished in one of the two). **No Roman numerals anywhere.** Two of his names could not be kept honest: a
   DORIAN or a LYDIAN LIFT would need degree 1 or 6 and would read as a different chord in half the scales, so
   RISE and ROUND took their places. **STRUM is gone** (it was a roll of volume dips, not a strum at all);
   STARLIGHT and BRONZE BOWL, which used it, run WALK now, and GEN has six modes. **LOOP now shows itself**: the
   pill counts the phrase (LOOP 5) and lights while it replays, and reads **LOOP EMPTY** when nothing is caught -
   which is what it always was with LATCH on, since the keys make voices then and no lead is ever played. That is
   why it looked broken.

**On the unit:** installed 2026-11-08 with `fm1_install.py deepfield/build/deepfield.fwsc --yes` (the fast
install: only the changed sectors). It came back as `FM-1_1400`, `log` shows no crash and Bluetooth MIDI
advertising as FM-1 F9FC, and the new page reads back - one SELECT detent from HOME: `MIX. BED 100%. LEAD 59%.
OUT 100%`, which only 1.1 has. **Without `--yes` the installer cancels** when it has no terminal to ask in.

**Where it stands:** host and device builds warning-free, image 414,804 B (672 B over 1.0), build **F99E3E13**;
`basic.txt` 736 checks (728 plus eight for the mixer and DUB), `screens.txt` writes the manual's shots including
the two new pages, and the 16 golden renders differ in exactly two places, both expected: RAIN CLOCK (the FM 4-OP
fix and DUB) and GAMELAN's first instant (RAIN CLOCK's tail leaks into it: the renders run in one pass). The
manual and the cheat sheet carry the mixer, DUB, the progressions and the FM 4-OP knob. **The site is not
published and no release is tagged**: 1.1 goes out once Kyle has heard this build, so the public pages never
describe something nobody has played.

**What is left:** his listening pass on 1.1, and the twenty-second xrun test in item 2 above. Then
`deepfield/tools/publish_site.sh 1.1`, the tag, the GitHub release with the `.fwsc` attached, the hub's three
install tests on the unit, and the mail to zven@thefirmwarehub.com.

## The 1.0 flash, and the MIDI pass before the hub (flashed 2026-11-08; 1.0 is on the unit)

Kyle: "every part of this project from the device, to github, to docs, website etc needs to get alligned so
everythign is up to date on the same version - after that I want to do a last deep dive for bugs and then publishe
to the firmware sharing site". His three answers: the hub release is **1.0**, Pocket Poly and GNOMNOM **move out to
their own repositories first**, and the dive covers **DEEP FIELD plus the fm1-base paths it drives**.

### Where the versions stood

The unit, the site and the docs were all on the 0.13 pass; git was two steps behind it (6d3ad70 unpushed, the whole
0.13 tree uncommitted) and had no tag or release, ever. That is closed: 0.13 is **fdd6cff**, pushed, and the
repository now carries `docs/img/screen-home.png`, so the picture a hub fetches is no longer a 404.

### The dive: five bugs, all in MIDI out or the state behind it

Every one was proved in the simulator before the fix and is now a check in `deepfield/tests/basic.txt` (728 checks,
up from 705). The manual's own contract is the standard they failed ("a key sends a note on ... and a note off when
its voice starts to fade"), and every fix is MIDI only: **all 16 golden renders are unchanged**.

1. **A lead's note off went out at the note TRANSPOSE moved it to.** With LIFT up an octave the key sent note on
   C4 and the engine sent note off C5, so C4 stayed held on the computer for ever. Measured: `90 3C 75` then
   `80 48 00`. F7 DEEP VOID is LIFT 61 (an octave) with LOOP, so every replay leaked one -- and because LIFT morphs
   when a preset lands, the off landed on whatever note the morph had reached (an off at D4 in the trace).
   `lead_event` now releases at `v->home`, the note the note on used.
2. **Every voice let go while a lead was held sent no note off.** SEL + HOME, and CC 123, which is what a DAW's
   panic sends: `all_off` dropped the lead by clearing `held`, bypassing the path that owes the off. Both leaked.
3. **CC 120 (all sound off) the same**, and the platform's own silence before a backup, a restart or an update:
   `dr_panic` zeroed the voices and sent nothing. It now releases what the unit owes first.
4. **A note's pressure outlived its voice.** `press[]` is per note and nothing cleared it, so poly aftertouch sent
   for a note (or a key leaned on) coloured the next voice struck there, with no way back. Measured on a dry C4 at
   AFTERTOUCH full: a strike six seconds later rendered 4.4 dB off a clean one, and the probe showed `press[60]`
   still 127. Both strike paths (`voice_strike` and the revived fading body) now start from no pressure.
5. **GEN reported moves for voices a computer struck.** `orbit_apply` sent note off and note on for any latched
   voice it moved, while `let_go` and `transpose_to` stay quiet for MIDI's own notes; a preset's chord can latch a
   voice a keyboard is holding, and then the move's note on was never released. Guarded like the other two.

Two more things the dive found and left alone, for Kyle to decide:

- **A preset's chord sends no note ons, but letting it go sends note offs.** The keys, GEN's moves, the arp and
  LOOP all report themselves; `load_bodies` does not, so a computer sees offs for notes it never saw start (the
  suite's log shows six on 48 and 55). Harmless on a synth, but layering a preset's chord on one does nothing. The
  manual does not promise either way.
- **`fx_pre` assumes an even block** (it steps the delay two samples at a time and divides by `n / 2`). Every path
  calls `app_render` with APP_BLOCK = 256, so nothing can reach it today.

Smaller, fixed on the way: the old-bank migration now refuses a stored scene size that claims more parameters than
this build has (it stayed inside the struct, but could have scrambled a user slot), and `scenes.c` said slots 0..7
were F1..F8 when there are sixteen of each.

The host build is warning-free, the device build too: image 414,116 B (192 B under 0.13), RAM 77,692 of 98,304,
pool 334,032 of 344,064, identity FM-1_1400, build **E273E52A**. `screens.txt` passes and the 16 renders match
golden to the byte.

### The hub's six rules, re-read today (thefirmwarehub.com/fmhub.md and /submit)

Met: the open licence with the source public, the FM-1_14 identity nobody else uses, the update loader in the
package (and the loader has no JEDEC gate of its own, while the data area takes both the Puya and the UC25HQ80, so
a new unit saves and can install another firmware afterwards).

Committed 2026-11-08: **`fmhub.json`** at the repository root -- the hub reads only that file, eight
fields and no others, "no file, no listing" -- with `what` (268 characters), `author`, `kinds` ["synth"], `play`
the emulator, `cheat` the cheat sheet, `image` `docs/img/screen-home.png` (240 x 240, 173 KB, inside the 300 KB
cap) and `color` #E5482E, the site's own accent. The root **`firmware.json`** is DEEP FIELD's now (it was Pocket
Poly's, which the hubs and RemoteFM1 would have read in a repository called deepfield), and `publish_site.sh`
refreshes it from each release build so the two cannot drift. The root README leads with DEEP FIELD.

### The split, done 2026-11-08 ("Yes, create them")

`WisestWizard/pocketpoly` and `WisestWizard/gnomnom` exist and are pushed. Each carries the history filtered out
of this repository (`git filter-branch` over all 40 commits, empty ones pruned) plus one commit that stands its
root up: pocketpoly 5 commits with `app/`, the root `tools/` and `tests/`, `docs/roadmap.md` and the research
docs; gnomnom 5 commits with the firmware renamed to `app/` in every commit, so a release writes `firmware.json`
at the repository root (base only does that when APP_DIR is named `app`). Both have the `base` submodule at the
same upstream pin b0976f2 with `base/bsp` initialised, a `build.sh` defaulting to `app`, their own `.gitignore`
and a README; `app/fm1.toml`'s `repo` is pocketpoly's own now, and GNOMNOM's docs, tests and tools had their
`gnomnom/` paths and `./build.sh gnomnom` lines rewritten. Verified in their new homes: Pocket Poly builds
`--host` and passes 310/310 of `app/tests/basic.txt`, GNOMNOM builds `--host` and passes 50/50. The working
copies are `/Users/kylemccann/Claude/Projects/pocketpoly` and `/Users/kylemccann/Claude/Projects/gnomnom`.

This repository has dropped `app/`, `gnomnom/`, the root `tools/` and `tests/` (all three of those tools and the
golden hashes are Pocket Poly's, which is why they went with it) and `docs/roadmap.md` (Pocket Poly's plan). The
root is now what the README's table says and nothing else.

Both were created private, because `gh repo create --public` is refused by this session's permission check as a
public surface; Kyle ran the two `gh repo edit --visibility public` commands himself the same evening. All three
repositories are now public MPL-2.0 with master as the default branch, both new ones clone with
`--recurse-submodules` from a clean directory (base b0976f2, `base/bsp` present, `app/fm1.toml` there), and this
repository's master is pushed through **2c8058b**. The root README's links to them resolve.

Each of the two carries its own `firmware.json` at the root now (**f652620**, **f1d6ca0**). The base writes that
file whenever APP_DIR is `app`, so without it a plain `--host` build left the tree dirty and a flasher reading
either root found no card at all. Checked from GitHub rather than from the push: both clone with
`--recurse-submodules` into an empty directory (base b0976f2), build `--host` on the system cc and pass their own
`app/tests/basic.txt` -- 310/310 and 50/50, exit 0 -- with nothing untracked left behind. Pocket Poly's card
points its screenshot at `app/docs/img/screen-home.png`, which is in the repository, so it will not 404 the way
this one's did.

**The 1.0 package is built** (2026-11-08, Kyle's "Yes, build 1.0"): `./build.sh --release 1.0` ->
`deepfield/build/deepfield-1.0.fwsc`, 614,938 B, identity **FM-1_14010000** (the first release identity; dev
builds were FM-1_1400), build **F816D1E2**. Image 414,132 B (16 B over the dev build, the version string), RAM
.data+.bss 77,692 of 98,304, .ram_text 9,520 of 32,768, pool 334,032 of 344,064 -- the same figures as the build
the suite was run against, and every guardrail check passes (radio calls in RAM, no 64-bit multiply, no
soft-double, register access through felucca-bsp only). The build's `firmware.json` is byte-identical to the one
committed at the root, so the two have not drifted.

**Flashed 2026-11-08** on Kyle's "Flash it", from the running FM-1_1400 over USB-MIDI: the loader took the fast
install (only the changed sectors), the unit restarted into **FM-1_14010000** and the screen reader shows
DEEP FIELD / FIRST LIGHT / HOME, CPU 5%, no voices; the log is clean (BT stacks 512 768, heap 11488/24576, no
crash record). Kyle has not played it yet. One oddity, harmless: macOS still lists the unit's USB-MIDI port as
**AMB-1 FM-1**, a CoreMIDI name cached from an earlier AMB-1 install -- the firmware's own `usb_product` is
DEEP FIELD FM-1, and `--info` reads the right identity through that port. Live will show the old name.

Still to do, in order:

1. **Hear 1.0 on the unit** (the five test steps below); it is flashed and waiting.
2. **The hub's install tests, on the unit:** from the stock V15 firmware, from another firmware, and another
   firmware installed from this one.
3. **Publish the site at 1.0** (`deepfield/tools/publish_site.sh 1.0`), **tag v1.0** and make the **GitHub release**
   with `deepfield-1.0.fwsc` attached. The hub takes the newest release carrying a `.fwsc`, so no release in this
   repository may ever carry Pocket Poly's or GNOMNOM's package.
4. **The mail to zven@thefirmwarehub.com**, which Kyle sends: the name, his name, the repository, the FM-1_
   identity, the licence, and that it was tested on a real FM-1 and leaving it again works. Nothing else goes in
   the mail -- the text, the picture and the links come from `fmhub.json`.

### How to test the flash (numbered for the unit)

1. Headphones in, F1 FIRST LIGHT, play a chord: it should sound exactly as the build you have been playing. Nothing
   in this pass touches the sound, and the renders prove it to the byte.
2. USB-C to the Mac, a track armed in Live on DEEP FIELD FM-1, LATCH off (SEL tapped), LEAD page, TRANSPOSE up an
   octave: play a few notes. Live should show them starting and stopping; before, every note you played stayed
   stuck on.
3. F7 DEEP VOID with GEN on (LOOP): let it replay for half a minute with Live recording. No held notes should pile
   up.
4. Hold SEL and tap HOME while a lead sounds: the note should stop on the computer too.
5. Live's panic button (or stop): everything should release.
6. Lean on a key hard, let it go, then play the same key gently: it should sound as gentle as any other key -- the
   old build kept the aftertouch on that note and the next voice there came back bright.
7. The FP-30X over Bluetooth, as always: pair, play, and the keyboard's own notes should be unaffected.

## The volume law put back, the keys easier, MASTER on the recording (flashed 2026-11-08; site 0.13; committed fdd6cff)

Kyle, after playing the 11-07 build: "the velocity sensitivity of the keys is a little off - you have to press
really hard to get it to full volume - also we backed off the overall volume too much when we changed the master";
then, on where to land it: "Speaker is too qeuiet overall - on headphones or recording it was too quiet at the low
and not quite loud enough in the upper ranges. basically we went too far when we alteredt the volume before and I
want to just back that off a bit"; then "I also noticed when hooked into ableton via usbc the master knob did
nothing - that needs to wokr always".

### The volume, in order

| | output's full scale | MASTER's law | a third up | halfway | full |
| --- | --- | --- | --- | --- | --- |
| 0.9 and before | 8.7 M (ceiling -0.1 dBFS) | squared | -19 dB | -12 dB | 0 dB |
| 11-08 (7234746, flashed) | 4.35 M (-6 dB) | cubed | -35 dB | -24 dB | -6 dB |
| now | 6.15 M (-3 dB) | squared | -22 dB | -15 dB | -3 dB |

The 11-08 pass turned two dials at once to stop the USB recordings redlining, and that is what went too far: the
trim took 6 dB off everything and the cubed law took another 10 dB out of the bottom half of the knob, which is
exactly the two things he described (quiet at the low end, not loud enough at the top). The law is back to squared,
so the knob feels as it did, and the trim is 3 dB instead of 6: **every position is now exactly 3 dB under where
0.9 had it**, and 3 to 13 dB over the build he has been playing (+3 at full, +9 halfway, +13 a third up).

### What changed

- **`dr_render` (engine/drone.c): the output's full scale 4.35 M -> 6.15 M.** The limiter's ceiling -6.1 -> -3.1
  dBFS on every output. The sound is untouched: all 16 renders are the old ones times 1.178 to within 1 LSB.
- **`df_gain` (app.c): MASTER cubed -> squared,** as it was before 11-08.
- **`key_vel` (ui.c): the keybed's own curve.** The sensing's reading is scaled by 127/108 and clamped, so a strike
  it reads as 108 is already a full one and everything lighter reads that much heavier. The softest touch is where
  it was, so nothing came off the bottom of the 18 dB law; the key reads the same to the engine and to MIDI out.
- **`rec_gain` (app.c): what a computer records follows MASTER,** with USB LEVEL's trim under it. It used to render
  at USB LEVEL alone, so with SPEAKER on AUTO and a track armed in Live the knob did nothing anywhere: the speaker
  is muted by AUTO and the recording ignored the knob. The speaker's own MASTER is applied by the platform after
  the recording is taken (`uac_mix`), and `df_gain` returns 0 in that case, so MASTER never lands twice.

### Measured

The voice's level as the simulator hears it (1/1000 of full) by the velocity the sensing reads, before -> after:

| struck | 1 | 20 | 40 | 60 | 80 | 90 | 100 | 108 | 127 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| before | 127 | 173 | 241 | 334 | 463 | 546 | 643 | 733 | 1000 |
| after | 127 | 182 | 270 | 393 | 583 | 698 | 849 | 1000 | 1000 |
| | +0.0 | +0.4 | +1.0 | +1.4 | +2.0 | +2.1 | +2.4 | +2.7 | +0.0 dB |

So a full strike is no louder than it was -- it arrives at 108 instead of 127 -- and firm playing in the top third
is 2 to 2.7 dB up, with the output's 3 dB on top of all of it.

The renders: the 4.35 M build reproduces the committed goldens exactly, and every render at 6.15 M is that one
times 1.178 to within 1 LSB, checked sample by sample, so the output change moves nothing but the level. Goldens
re-recorded. Peaks at MASTER full are now -5.6 (CATHEDRAL) to -14.3 dBFS (INIT); eight voices at MASTER half peak
-20.5 dBFS (`basic.txt`'s bound moved -30 -> -22).

The recording, CATHEDRAL latched, the same window at three knob positions (identical runs, only `master` differing):
-16.0 dBFS at full, -28.1 at half, -35.2 a third up -- the squared law to the hundredth of a dB, where before it
sat at one level whatever the knob did. The hardest playing at MASTER full reaches -9 dBFS at the -6 dB USB LEVEL
default, -3 at 0 dB.

705 simulator checks pass (six new on `recgain`), and screens.txt. `basic.txt` also moved with the keybed curve:
`level60` 333 -> 393, the keys' MIDI out 0x64 -> 0x75 (a strike read as 100) and 0x3C -> 0x46 (60). Device package:
image 414,308 B, RAM 77,692 of 98,304, pool 334,032 of 344,064, identity FM-1_1400, build CFFC4244.

### How to test (numbered for the unit)

1. Headphones in, MASTER a third of the way up, F1 FIRST LIGHT, latch a chord: it should be 13 dB louder than the last
   build at that position, and the bottom half of the knob useful again.
2. Sweep MASTER from a third to full: the climb should feel even, not back-loaded.
3. Speaker, MASTER full: 3 dB louder than the last build and 3 dB under 0.9. Say if the top still is not enough --
   it is one constant.
4. Play a chord as softly as you can: as quiet as it was. Then play firmly, not hard: full weight now (brightest,
   fastest attack, loudest). The old build needed a dig.
5. USB-C to the Mac, arm a track in Live on DEEP FIELD FM-1 (the header says USB MUTE): turn MASTER and watch
   Live's meter follow it. At MASTER full, playing hard should peak around -9 dBFS and never clip.
6. While that track is armed, the FM-1's own speaker and headphones stay silent -- that is SPEAKER AUTO, unchanged.
   GLO, SELECT twice, SPEAKER ON if you want to hear the unit while Live records.
7. A keyboard on TRS or Bluetooth: MIDI in is untouched, its velocities read as they always did.

Flashed on Kyle's "Flash it" (`/usr/bin/python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc --yes`,
the 11:24 package, newer than every source, carrying 6.15 M and the squared law; a fast install, only the changed
sectors). Booted clean: the unit says DEEP FIELD. FIRST LIGHT. HOME. HARMONICS 37%. DRIFT 29%. REVERB 45%.
TREMOLO 27%. VOICES 0. CPU 5%; `log` records no crash and Bluetooth advertises as FM-1 F9FC (heap peak 11,352 of
24,576). Not yet heard by Kyle.

### firmware.json's metadata, filled in on the way (Kyle: "also fill in the info")

Since the base bump a `--release` build refuses an fm1.toml without `author`, `licence`, `tagline`, `description`,
`tags`, `install`, `repo` and a square screenshot PNG (`base/tools/firmware_json.py`, `base/docs/FIRMWARE-JSON.md`),
and `publish_site.sh` runs `--release`, so publishing 0.13 would have failed. `deepfield/fm1.toml` now carries all
of them: author Kyle McCann, title DEEP FIELD (the USB manufacturer is "Deep Field", so the hub card would have
read that), a 565-character description, tags synth / ambient / generative / fm, install
https://deepfield.fog.baby/install/, site https://deepfield.fog.baby/, repo
https://github.com/WisestWizard/deepfield, screenshot `docs/img/screen-home.png`. The host build is warning-free
and `deepfield/build/firmware.json` carries the lot.

Both warts that turned up with it are fixed (Kyle: "fix the warts first"):

- **The screenshot a hub would fetch.** `repo_root` takes an APP_DIR not named `app` as the repository's root, so
  the build resolves `screenshot` inside `deepfield/` while a hub resolves it against the GitHub repository, where
  `deepfield/` is a subfolder: `raw.githubusercontent.com/WisestWizard/deepfield/vX.Y/docs/img/screen-home.png`
  would have 404'd. Moving the app to `app/` is not open (Pocket Poly is there), so the repository now carries its
  own `docs/img/screen-home.png`, and `publish_site.sh` copies it from `deepfield/docs/img/` before every release
  build, so the two cannot drift -- the manual's shots are regenerated by `deepfield/tests/screens.txt`, which
  writes only into `deepfield/docs/img`. Proved by deleting the repository's copy and running
  `deepfield/tools/publish_site.sh 0.13 --local`: it came back and the site built.
- **Pocket Poly's own metadata.** `app/fm1.toml` was missing the same keys and would refuse a release. It now has
  author, a 569-character description, tags synth / fm / sequencer / generative, the repo URL and
  `app/docs/img/screen-home.png` (relative to the repository's root, which for an APP_DIR named `app` is also the
  hub's frame, so there is no wart to fix there). `install` and `site` are deliberately still missing, and the
  build still names `install`: Pocket Poly has no published site, `app/site/landing.md` goes nowhere yet, and a
  URL that 404s in a hub's catalogue is worse than a key a dev build warns about. Fill it in when the site exists.
  The repository-root `firmware.json` is its generated file, now carrying all of the above.

Published on Kyle's "Publish it": `deepfield/tools/publish_site.sh 0.13` pushed gh-pages, and
https://deepfield.fog.baby serves Version 0.13 with the CNAME intact -- the landing page, the cheat sheet (the new
MASTER line), the manual (the squared law and the keybed's 15 %), the installer (FM-1_14001300), the emulator and
`firmware/deepfield-0.13.fwsc`, all 200. The gh-pages commit says "from 6d3ad70", which is the base bump: the
published content is from the uncommitted tree, so committing this work tidies that provenance up.

**What is not done:** nothing committed, nothing pushed. Until it is, the repository's own
`docs/img/screen-home.png` is local only -- `raw.githubusercontent.com/WisestWizard/deepfield/master/docs/img/
screen-home.png` is still a 404, and a hub fetches it at the release's tag, so a listing also wants a tag.

## The base bump to upstream main, with #44 merged (built 2026-11-08; committed 6d3ad70; not flashed)

charlesvestal merged the Bluetooth MIDI pairing fix as charlesvestal/fm1-base#44 (merge `b0976f2`), with one commit
of his own on top: `ff0a007` widens the 10 s give-up to a link that encrypts but never turns its notes on (it stands
down as soon as the CCC write succeeds) and drops the per-note log line. So the fork carries nothing upstream lacks,
and on Kyle's "ok go ahead" the submodule was pointed back at charlesvestal/fm1-base and moved from `a2db753` to
upstream main `b0976f2`: 142 commits, 245 files, +18,935/-831, and `base/bsp` with it from `305c372` to felucca-bsp
`5b0879b`. `.gitmodules` no longer carries the fork's note, and `base/` has one remote, upstream. The fork
github.com/WisestWizard/fm1-base can go.

Built, not flashed. All three apps build on the device toolchain: deepfield 414,292 B (it was 408,324 B, so the
upstream work costs about 6 KB), RAM 77,692 of 98,304, pool 334,032 of 344,064; pocketpoly and gnomnom likewise. The
deepfield image still carries the pairing fix's own strings (`ccc %x pair %d`, `paired: no answer`, `ccc refused,
let go`). Simulator checks: deepfield 699, pocketpoly 310, gnomnom 50, none failing.

**The golden renders moved, and the old ones were wrong.** 12 of deepfield's 16 and 5 of pocketpoly's 16 changed
their hash on the bump. The cause, found by bisect and confirmed from both ends: `8a3acf0` ("A fourth plat_data
window on request") also made build.py give the host's and the web's o2 units the optional parts' flags, "as the
device's always did". Both apps set `cpu2 = true` and build their engine as an o2 unit, so until that commit the
host compiled `drone.c` / `engine.c` **without** FM1B_CPU2 while the device always compiled them with it: the host
ran the one-core path, where the hall runs inside the block, and the device ran the two-core path, where the hall
runs a block behind. The goldens had been recording an engine the unit never ran. Confirmed: upstream main with
`FM1B_CPU2=0` matches the old goldens exactly, and with it on 12 change, so 142 commits of upstream change the sound
by nothing at all and what moved is the host catching up to the device. Both golden files were re-recorded on the
two-core build (`python3 tools/render_all.py --golden`, `python3 deepfield/tools/render_scenes.py --golden`); from
here a render difference means something real. Nothing Kyle has heard on the unit changes.

**One real regression, found and fixed.** GNOMNOM's host simulator segfaulted on the bump (`gnomnom/tests/basic.txt`,
exit 139 against upstream main, exit 0 at the old pin). Same root cause: with the host finally compiling FM1B_CPU2,
`bd_job` ran for the first time off the device, and `band.c` was handing it the job pointer through
`plat_cpu2_run`'s `arg`, a `uint32_t` -- fine on the device's 32-bit core, truncated to `0xed378` on a 64-bit host.
`bd_job` now reads the job through a file-scope `bd_job_line`, as DEEP FIELD's `half_job` already reads its statics,
and the arg is 0. Device behaviour is unchanged; the host sim runs its 50 checks again.

**What is not done:** not flashed. Committed as 6d3ad70. The unit is still on the 11-07 16:36 package built from
the fork. After a flash the one thing to try is the FP-30X over Bluetooth, since `ff0a007` changed when a paired
link is let go.

## The sound and dynamics pass, with the Bluetooth retry (flashed 2026-11-07; committed 598feb0; site 0.12)

Kyle: "things are sounding pretty good - what can we do to polish the sound or improve the dynamics - within the
bounds of what we have already built are there any ways per voice/ effect/ motion to improve the sound or dynamics -
also while you are doing that conduct another bug finding mission". Two calls were put to him and he took them both:
VELOCITY **on out of the box**, and both of the character changes offered (a stereo shimmer and a real vibrato).

**Flashed twice from this same tree, within a minute of each other, by two sessions that both had Kyle's word.**
The Bluetooth session rebuilt and flashed on his "Wait, then flash" (its package kept as
`scratchpad/deepfield-both.fwsc`, sha1 d53853e7...); this session then rebuilt and flashed on his "Flash it"
(`AC79_SDK=~/fw-AC79_AIoT_SDK JIELI_TOOLCHAIN=~/.jieli/toolchain ./build.sh deepfield`, the 16:36 package, newer
than every source; `/usr/bin/python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc --yes`). The sources
were identical, so the firmware is: this pass and the Bluetooth session's le_midi.c pairing fix (below) together,
image 408,324 B, RAM 77,628 B, 699 checks.

Flashed once more at 23:39 on his "Flash it" after the submodule was settled, from the same sources (image
408,324 B again, with the pairing fix's log strings in it); booted clean, `no crash recorded`, `BT ready:
advertising as FM-1 F9FC`, `cpu 5% b0 ADDITIVE s0 x0`, `late 00000000`, the screen reader the same FIRST LIGHT
line. The unit is on that package. Verified after it: the installer said `done: the FM-1 runs FM-1_1400`; the log
has no crash recorded, `BT ready: advertising as FM-1 F9FC` and `cpu 5% b0 ADDITIVE s0 x0`; the screen reader reads
`DEEP FIELD. FIRST LIGHT. HOME. HARMONICS 37%. DRIFT 29%. REVERB 45%. TREMOLO 27%. VOICES 0. CPU 5%`. Not yet heard
by Kyle, and neither this pass's test steps nor the piano test have been run on it. Committed on his "Commit and
publish" as 598feb0 (not pushed), and site 0.12 published the same way (`deepfield/tools/publish_site.sh 0.12`,
release FM-1_14001200): deepfield.fog.baby serves the new landing page, the manual, both cheat sheets, the emulator
on this engine and `deepfield-0.12.fwsc`. The Bluetooth session then committed its le_midi.c fix (base a2db753) and
pointed the submodule at it (2101207). Master pushed through 293f52b on Kyle's "Push master".

**The submodule pointed at a fork for a day, because the pin carried a commit upstream did not have.** base
`a2db753` (the Bluetooth MIDI pairing fix) lived only on this machine, so the pushed repository could not be cloned:
`git submodule update --init base` failed `upload-pack: not our ref a2db7534...`. On Kyle's "Fork it" the commit went
to github.com/WisestWizard/fm1-base and `.gitmodules` was repointed at the fork (2eebe7b), then the fork was made the
one fm1-base to track (6ae6a07) since upstream and the fix could not both be had until #44 landed. The fix was
offered upstream on Kyle's "Open the PR" as charlesvestal/fm1-base#44, cherry-picked onto upstream/main as
`midi-central-pairing` rather than sent from `deepfield-pin`, which was 139 commits behind and would have read as
reverting all of it. The PR said it had not been compiled against current main, because `tools/build.py template` in
a standalone fm1-base checkout failed on a missing `bsp/loader/crt0_ldr.S`; charlesvestal's answer was that this is
the bsp submodule not checked out (`git submodule update --init --recursive`), not something absent from the repo.
**All of this is now history:** #44 is merged, the submodule points at upstream again, and the fork can be
deleted.

**The one thing to confirm on the unit first:** the DEF4 -> DEF5 settings migration has no check behind it. GLO,
SELECT once should read VELOCITY ON, and the octave, scale, root, channels and Bluetooth should be as Kyle left
them. If they came back at their defaults instead, the migration dropped the record, and that is the thing to fix
(either way VELOCITY ends up on, since the defaults carry it, so the switch alone does not tell you).

### The dynamics: a strike now means something

Every key struck the engine at one level (velocity reached it only through a switch most players never find, and
even then it moved the level over 9 dB and nothing else). Now:

- **VELOCITY is on by default** (`settings_defaults`, scenes.c). The settings record is DEF5; a DEF4 record is read
  and everything in it kept, with VELOCITY switched on, so Kyle's unit gets it on the next flash without losing his
  octave, scale, channels or Bluetooth. The switch is still GLO, SELECT once, KNOB 4, if the keybed's sensing turns
  out to read badly. With it off a key strikes at 127 (full) where it used to strike at 100, so nothing is held back
  when the switch is down.
- **The level law is dB-linear over 18 dB**, a full strike unity (`vel_gain`, drone.c); it was `0.35 + 0.65 v/127`,
  9 dB from a floor. Measured on a held C4 of SUPERSAW: vel 20 to 127 is +17.0 dB (the law alone +15.2; the rest is
  the filter opening).
- **A strike's weight is also its colour and its attack.** `v->vtouch` is the strike as -1..0 and feeds `vmass`
  (0.26 of TIMBRE) and the swell's and the lead attack's rate (half as fast at the softest). Measured, vel 20 to
  127: SUPERSAW at MASS 90, the spectral centre 356 -> 558 Hz (+3.9 dB of brightness); ADDITIVE at F1's own MASS 48,
  299 -> 363 Hz (+1.7 dB). Where TIMBRE's filter already sits at the note it is clamped there (`fc < f * 1.2`), so
  on a filtered model low down the colour moves less; the level and the attack always move.
- **The factory bank keeps its balance.** A preset's chord is now struck full (127) with its weight in each voice's
  own LEVEL, -3 to 0 dB (`scene_make` takes a dB trim, not a velocity), because a velocity is now a performance and
  not a trim. Measured over the 15 presets with chords: median RMS -24.5 dB and the spread -1.9..+2.2 dB before,
  -24.5 and -2.0..+1.8 after; peaks -17.1..-8.0 dBFS before, -17.2..-8.6 after, so nothing is nearer the limiter.
- **A save stores the strike, not the level the voice happened to be at** (`dr_body_t.vel`, `scene_capture`): a
  chord saved while it was still swelling used to come back quieter than it was played.
- **The runner has a downbeat.** ARP struck every step at one velocity. The first step of each EUCLID bar is 127,
  the one halfway 110, the rest 94 (`arp_acc`), and the accents go out over MIDI.
- **LOOP replays the velocities you played** (`echo_ring_vel`), instead of every note at a flat 100.
- **MIDI out carries the strike, not the gain.** GEN's moves and SEL's transposes sent `v->vel * 127` (the gain as a
  velocity: a key struck at 100 went out as 109). They now send the velocity the voice was struck at.

### The sound: the two character changes he picked

- **A real vibrato.** VIBRATO's rate was welded to its own depth, `0.1 + 6 W²` Hz, so at every factory setting
  (WOBBLE 8 to 30) it was a 0.1-0.3 Hz drift that RANDOM already does, and the only way to a 5 Hz vibrato was to
  turn it full, where it is a 6 Hz warble. The rate is now DRIFT's, 3.6 to 6.8 Hz, and the depth runs to 40 cents
  (`b->wob_inc`, `wob_depth * 0.40f`). Measured on C4 with DRIFT 35: VIBRATO 20 (what most presets carry) is 6.2
  cents either way at 4.27 Hz, VIBRATO 127 is 39.5 cents at 4.02 Hz, VIBRATO 0 is flat. The mod wheel and aftertouch
  ride the same LFO, so leaning on a key is a vibrato now and not a slow bend.
- **A stereo shimmer.** The octave up was one pair of heads added to both channels: a mono cloud in the middle of a
  wide hall. It is now two pairs, the right reading 35 ms further back with its grain a quarter out of step, one a
  side (`shim_pair`, `SHIM_B`). Measured on the tail of a big hall, left against right: the hall alone +0.320, the
  mono shimmer +0.558, this one +0.483 (above 500 Hz, +0.268 / +0.535). It costs one more pair of taps and one more
  one-pole a sample while SHIMMER is up: too small to read on the host, **not yet read on the unit**.
- **STRUM's dip slides.** It multiplied the envelope by a quarter inside one control tick (0.73 ms), which is a
  click. It now slides over 34 ticks: measured on one latched voice, 14.4 dB deep, 90 % to 10 % in 23 ms, steepest
  -2.5 dB a millisecond.

### The bug hunt: five, each reproduced before it was fixed

1. **LOOP left notes sounding for ever.** A pass was forgotten with a replay still held -- PLAY off, or GEN NOW,
   between a replay and its note off -- and that note never stopped and never sent its note off. Reproduced: a
   latched chord, two leads, LOOP, PLAY, PLAY off 150 ms in; 30 s later `leads` was still 1, at note 62. Fixed with
   `echo_hush`, called from `orbit_reset` and before each new pass.
2. **LOOP played nothing unless something unrelated was latched.** `orbit_plan` gave up at `if (!cnt) return` before
   it reached the phrase, and the phrase is the leads' own. The branch moved above the candidate gathering.
3. **LOOP was silenced at random by a knob it does not show.** The mode has no CHANCE knob, but the CHANCE left over
   from another mode gated it: at INIT's 70, three passes in ten were silent for no visible reason. It now ignores
   CHANCE.
4. **A note off sent as a note on at velocity 0 reached only half the engine.** `0x80` goes to both the voice and the
   lead path; `0x90 .. 00` went to whichever LATCH named, so switching LATCH between a note on and its note off left
   the note sounding with nothing left to stop it. Reproduced over USB; fixed in `midi_in`.
5. **Two fields of the header's change detector shared bits** (the noise layer with the move's flash, GEN's mode with
   the recording flag), so two of them turning over together left the header undrawn. Each has its own bits now.

Tidied while there: `dr_lead_body` left `target` unset (nothing read it).

### Measured

699 simulator checks (671 once the existing ones were brought to the new velocities, 647 before; new: the velocity
law and the switch off, LOOP with nothing latched and ignoring CHANCE, LOOP's velocities, LOOP let go with PLAY, the
note-on-at-nothing note off, a chord saved and recalled at its strike, the runner's accents through a bar, STRUM's
dip and its recovery). All 16 renders rewritten as golden (the bank's velocities and trims, the vibrato and the
shimmer all move them). Sanitizer clean but the known `fastmath.h:58`. Screenshots regenerated (a voice's LEVEL now
opens at the preset's trim: NEBULA's reads -7 dB where it read -4). Device image 408,324 B (+1.7 KB), RAM 77,628 of
98,304 (+16 B), pool 334,032 (unchanged, 10.0 KB spare). Docs: manual (sections 4, 6, 7, 9, 10), both cheat sheets,
README, BUDGET, landing page, this handoff.

Two things are **not** covered by a check: the DEF4 -> DEF5 settings migration (it needs a stored record to run
against) and the device's CPU with the stereo shimmer. Both want the unit.

One oddity to expect in the renders: F16 INIT has no chord, so `render_scenes.py` plays it with three MIDI notes at
velocity 100, which under the new law is 2.5 dB under a full strike. Its render sits that much lower than before;
the preset itself is unchanged.

### How to test (numbered for the unit)

1. On F1 FIRST LIGHT, play one key as softly as you can, then as hard as you can. The soft note is quiet, dark and
   eases in; the hard one is loud, bright and arrives. Play a chord and lean into one note of it: it stands out.
2. GLO, SELECT once, KNOB 4 to OFF. Play the same two ways: both strike full and alike. Turn it back ON.
3. EDIT, ALGO to SUPERSAW, HOME, TIMBRE about three quarters up. Soft and hard strikes now open and close the filter
   like a knob.
4. LFO, VIBRATO from 0 to about a quarter: a gentle 4-5 Hz vibrato, each voice a little apart. Turn HOME's DRIFT up
   and it gets faster. Lean on a held key: aftertouch is a vibrato now, not a bend.
5. F3 GLASS ORBIT or F6 NEBULA on headphones: the shimmer sits out at the sides with the hall, not in the middle.
6. F5 PULSAR, PLAY: the arpeggio has a downbeat. Record it into Live: the accents are in the clip.
7. Tap SEL (LATCH off), play four notes, ARP, MODE to LOOP, PLAY, with **nothing latched**: the phrase comes back,
   each note as hard as you played it. Press PLAY again mid-phrase: everything stops, nothing hangs.
8. From Live, with LATCH off, hold a MIDI note, tap SEL (LATCH on), then release the note: it stops.
9. F8 STARLIGHT, PLAY (STRUM): each voice dips without a click.
10. Turn PRESETS through the bank: every preset sits where it did, none louder or quieter than before.

## Bluetooth MIDI: the MIDI page's panel, and pairing when a keyboard refuses its notes (flashed 2026-11-07, the retry with the sound pass; site still 0.11)

Flashed on Kyle's "Flash it" (`scratchpad/deepfield-btpanel.fwsc`, sha1 05be0139..., no source newer); the installer
said `done: the FM-1 runs FM-1_1400`; booted clean: `no crash recorded`, `BT ready: advertising as FM-1 F9FC`,
`cpu 5% b0 ADDITIVE s0 x0`, the screen FIRST LIGHT on HOME.

**The piano test on that flash confirmed the cause**, and showed the fix half right. The log: `ccc f pair 0`, so the
FP-30X answers the notes-on write with 0x0F, insufficient encryption. First try: `pairing 5` (twice, folded),
`encryption 6` (key missing), `paired: 6`, and the FM-1 let go at once. After Kyle switched Bluetooth off and on:
`sm: just works`, `encryption 0`, `paired: 0`, then the write sent at once was refused again (`ccc f pair 2`),
`ccc refused, let go`. So the piano refuses notes until the link is encrypted, and still says unencrypted to a write
sent the instant the encryption comes up (no SM "pairing complete" sub-event exists to wait for: le_user.h has
only RECONNECT_START, PIN_KEY_MISS, PAIR_FAIL, PAIR_TIMEOUT, ENCRYPTION_FAIL, SEND_DISCONN, ADD_LIST_*).

**Fixed since, flashed with the sound pass above, and it works** (le_midi.c only). Kyle connected the FP-30X from
the MIDI page and played: the log shows `notes 1: 97 a4 90` and `notes 2: 9f a5 90` (BLE MIDI note-ons), then `cpu
..% bN` climbing to 8 voices on ADDITIVE, FM and BOWED; the pairing lines had already rolled out of the 32-line log.
What was built: after `encryption 0` the write waits 0.6 s (`mc_due`, sent
from `ble_midi_poll`), and a refusal after that is asked again every 0.7 s up to five more times before letting
go; a failed encryption no longer lets go at once (the stack's `pair_redo` gets its second try; the 10 s guard
still ends a link that never encrypts). The device package built clean, image 407,748 B, but it also carries the
"Sound polish and dynamics" session's engine edits (drone.c 15:48, drone.h 15:47, mid-work), so it was NOT
flashed: the next build from this folder carries both. Expected log when it works: `ccc f pair 0`, `paired: 0`,
`ccc 0 pair 2` (or a `ccc f pair 2` or two first), then `notes 1: ...`.

Kyle wanted to play DEEP FIELD from his Roland FP-30X over Bluetooth MIDI. Found: the FM-1 found it (a first "no
device" was the piano not yet advertising), connected and said CONNECTED, but nothing played. The log
(`fm1_remote.py log`) showed `central: MIDI ready` twice and the voice count staying at 0, no pairing lines. In
`base/ble/midi/le_midi.c` the central wrote the MIDI characteristic's CCC (value handle + 1) and declared READY at
once, never reading the answer, and it never pairs (`sm_set_master_request_pair(0)`, kept for the OXI E16).

A logging build was flashed on Kyle's "Flash it" (the 15:06 package, kept apart as
`scratchpad/deepfield-blelog.fwsc`; booted clean, no crash). With the piano connected and played it logged the
MIDI characteristic at value 0x0c (properties 1e) and one answer through `user_client_report_data_callback`:
packet type 0xA0 (GATT_EVENT_QUERY_COMPLETE) on handle 0x0d, length 1, and no notes after it. Its status byte was
cut off: **the radio log keeps 32 lines of 39 characters and folds consecutive lines whose first five characters
match** (radio.c `shim_log`), so log lines need short, distinct starts. Root cause NOT confirmed: the guess is that
the FP-30X refuses the CCC write with insufficient authentication/encryption until the link is paired (as Roland
pianos ask iPads to pair). Kyle picked "fix and log together".

Built (Kyle took every default of three questions: the panel on the MIDI page, searching by itself there, fix and
log together):

1. **Pairing on refusal** (le_midi.c, in the base submodule, uncommitted there; it reaches every firmware's next
   build): the CCC write's answer is read (`blob[0]` of the 0xA0 report on value + 1). 0x05, 0x08 or 0x0F, once a
   link: `sm_api_request_pairing(mc_handle)` (Just Works, the master's IO caps already NO_INPUT_NO_OUTPUT), and on
   `HCI_EVENT_ENCRYPTION_CHANGE` for that link the CCC is written again (`mc_notes_on`). Any other error, a second
   refusal, a failed encryption, or no encryption within 10 s (`ble_midi_poll`) lets the link go, so the screen
   never says CONNECTED over a link without notes for long. Devices that accept the write (the OXI E16) never pair.
   Log lines: `ccc S pair P` (the answer), `paired: S`, `paired: no answer`, `ccc refused, let go`, `notes N: ...`
   (the first two notifications and every 200th).
2. **The MIDI page's Bluetooth panel** (ui.c `ble_panel`, drawn by `sky_draw` in place of the view on PG_MIDI
   while no HUD is up): BLUETOOTH DEVICE; CONNECTED in green with the name and REC DISCONNECTS; CONNECTING and REC
   CANCELS; NOT CONNECTED in orange with up to three devices found (the pick marked, `N FOUND`) and ALGO PICKS
   REC CONNECTS, or SEARCHING... and TURN ON BLUETOOTH ON THE KEYBOARD; BLUETOOTH OFF and how to turn it on. The
   page searches by itself while it is up and nothing is connected (`btdev_page(ui.looking || ble_panel_up())`);
   ALGO picks the device there, not the model; REC there connects at once (no first search press); REC elsewhere
   as before. Bluetooth messages on the MIDI page are only spoken (`bt_say`), not drawn over the panel; the
   reader's page text gains BLUETOOTH CONNECTED X / NOT CONNECTED ... (speak's buffer 200).

Checks: 666 (14 new at the end of basic.txt: the search starts on the page and stops off it, the stand-in found
and said, ALGO leaves MODEL alone, REC connects and lets go, a BLE-DEV note plays, the spoken panel); two new
screenshots, screen-midi-found.png and screen-midi-connected.png (the others regenerated: only the header's
CPU figure and the later shots' timing moved). Renders not rerun (no sound code touched). Device image 406,596 B
(+1.9 KB), RAM 77,612 B as before. The package is kept apart as `scratchpad/deepfield-btpanel.fwsc` too, because
the "Sound polish and dynamics" session was running in this folder (it had changed no file when this was built).
Docs: manual (the page table, section 10's Bluetooth steps and pictures, two troubleshooting rows), both cheat
sheets, README, BUDGET. Not committed: the work of the sessions below is still uncommitted too, and was not
committed apart while another session was running.

How to test (numbered for the unit):

1. On the FP-30X: Bluetooth on, no phone or tablet app connected to it. Turn its volume down.
2. On the FM-1: GLO, then SELECT once. The panel reads NOT CONNECTED and lists FP-30X MIDI within a few seconds.
3. ALGO to it, press REC: CONNECTING, then CONNECTED in green. DEV lights in the header.
4. Play: DEEP FIELD should sound. Then read `fm1_remote.py log`: `ccc 5 pair 0` (or f) then `paired: 0` then
   `ccc 0 pair 2` and `notes 2: ...` confirms the pairing cause; `ccc 0 pair 0` with no notes means it is not
   pairing and the cause is still open.
5. Press REC on the MIDI page: back to NOT CONNECTED and searching. HOME: the search stops.

## The flash before: PLAY lands on the GEN page (flashed 2026-11-08; committed 5980167; site 0.11)

Kyle: "when you hit play I think it should go to the arp page - what do you think?", then "build it now and then
flash and update". Built: PLAY turning GEN on opens the GEN page (`button()` B_PLAY: `page_to(PG_ORBIT)` after
`orbit_toggle` when it came on), unless GEN or GEN 2 is already open (GEN 2 stays, so its EUCLID and DIV are not lost
under the hand); PLAY turning GEN off leaves the page as it is (no auto-return: rule 1). CC 58 turning GEN on changes
no page (a sequencer must not flip pages). The site cheat sheet's stale "hold PLAY: GEN page" line (nothing ever did
it) is replaced. Checks: the PLAY block expects page 12 on and after, and HOME back to 0; screens.txt taps HOME after
PLAY before the FREEZE shot. 647 checks, renders match, screenshots regenerated, image 404,676 B (+16 B), RAM and
pool as the panel build. Manual (sections 2 and 7), both cheat sheets, this handoff. Flashed on his word (the 13:58
package); see the log line below. Site 0.11 published. Committed with the panel build below as 5980167 on
2026-11-07, so the sound pass above stands as a diff of its own.

How to test (numbered for the unit):

1. On HOME, latch a chord, press PLAY: the band reads ARP GEN, the knobs MODE RATE CHANCE RANGE. Press PLAY again:
   GEN stops, the page stays GEN. Tap HOME.
2. Tap ARP, turn SELECT once (GEN 2), press PLAY: GEN 2 stays under the knobs.
3. From Live, send CC 58 at 127 on HOME: GEN starts, the page stays HOME.

## The flash before: the panel's six (flashed 2026-11-08; committed 5980167; site 0.10)

Flashed on Kyle's "build and fix all six then flash and update" (`/usr/bin/python3 base/tools/fm1_install.py
deepfield/build/deepfield.fwsc --yes`, the 13:33 package); the installer said `done: the FM-1 runs FM-1_1400`, the
log `no crash recorded` and `cpu 5% b0 ADDITIVE s0 x0`: booted clean on F1. Not yet heard by Kyle. Every factory
preset was dumped back from the unit over SysEx after the flash (`tools/fm1_bank.py dump --all`): 32 frames, the
names as built (the tool first asked before CoreMIDI's input was ready and heard nothing: it now waits 0.3 s and
asks up to three times). `fm1_remote.py screen` printed nothing this session while `log` answered; not chased. Site
0.10 published (`deepfield/tools/publish_site.sh 0.10`, from the uncommitted tree). The level work of the other
session was committed apart first as 7234746, so this build sits on it; this build and PLAY's page were committed
together as 5980167 on 2026-11-07, before the sound pass above.

Where it came from: a four-musician panel (a laptop-free live performer, an Ableton studio musician, a score
artist, a generative patcher who sells preset packs) reviewed 0.9 in two rounds; the synthesis is the artifact
"DEEP FIELD Panel Review" (claude.ai/artifact/UBdjYdTRiq3ndNbuVQFGLk). Kyle: "build and fix all six", the top six of
its ranked list. All six are in this build:

1. **A real MIDI implementation.** Every value has a CC, in and out (`CC_OF` in ui.c; the manual's table in
   section 10): a knob turned sends its CC (`cc_send` from `knob`), a CC in sets the value (`midi_cc` in app.c looks
   the CC up in the same table). Few-valued knobs are spread over 0..127 in equal parts and sent from the middle of
   their part (`cc_out_val` / `cc_in_val`), so what comes back is what went out. Switches and triggers (57 LATCH, 58
   GEN, 59 FREEZE, 60-63 the FX keys, 12 the noise layer, 13 AS SAVED, 14 GEN NOW, and 2 SCALE / 3 ROOT) are queued
   from the render for the main loop (`mcmd_*`, `midi_cmd_apply`), applied as the buttons would with nothing sent
   back (`cc_quiet`). A CC in is playback: it no longer lights SAVE (`midi_param` and the NRPN path drop
   `scene_dirty`). The keys send the struck velocity whatever VELOCITY says (`key_on`: `evel` for the engine, `vel`
   for MIDI). The presets travel as SysEx: `F0 7D 44 46 01 ss F7` asks (64 every U, 65 every slot), `02 ss 05 data`
   answers and loads (user slots only; 118 bytes packed 7-to-8; the bank saved 0.8 s after the last frame:
   `ui.bank_save_t`); `app_sysex` via APP_SYSEX, `dfx_*` in app.c (named away from fm1-base's own `sx_frame`);
   `tools/fm1_bank.py dump / load / show`; the host's `sxloop A B` round-trips a frame for the checks.
2. **`FM1B_UAC_DEEP`** set in app.h (unity.c includes app.h first, so the define reaches it): the USB audio
   input's ring 4 KB in the pool (pool spare 14.1 -> 10.0 KB, RAM -1.8 KB as the ring left it). The header shows
   ` U<n>` in orange after the CPU figure while `uac.underruns` is nonzero (capped at 999); the log already had `uN`.
   Not yet measured on the unit under a recording: that is the test below.
3. **ALL NOTES OFF off the SEL timer.** SEL + HOME (either order) lets every voice go; SEL held with nothing done
   does nothing, however long (a tap under 0.6 s switches LATCH); the keys strike again at once with SEL still down
   (`sel_cleared`). SEL blinks no more. HOME tapped with SEL held opens no page and starts no reset count.
4. **SEL is the chord's button.** SEL + PRESETS: the preset lands with its chord over the chord playing
   (`ui.sel_chord`, `scene_go(.., chord 1)`: `load_bodies` lets the voices not in it go and strikes the new, as it
   always did for an empty latch), + CHORD shown in the list and said. PRESETS alone never brings a chord now (the
   "+ CHORD when nothing is latched" rule is gone; a program change never did). SEL + PLAY: GEN's next move now
   (`dr_orbit_now`, queue kind 9 -> `orbit_now`: the pending plan applied, or one forced past CHANCE
   (`orbit_force`); ARP the next step; LOOP and STRUM the period over; the countdown restarts), CC 14 the same.
5. **The factory bank rebuilt** (an agent in a worktree, merged; scenes.c): every model once (ADDITIVE and SHEPARD
   twice), every GEN mode the MODE of two presets, every delay type on one, chords beyond triads (a cluster, quartal,
   open fifths, a minor ninth, a 6/9, one low note under TAPE), the hidden eight set on most, five noise beds, F16
   INIT (`scene_init` exactly, no chord). F1 keeps its sound and its 3-note chord. Names: RAIN CLOCK (F10), BRONZE
   BOWL (F12) and INIT replace LOW CHOIR, ICE BELLS and TIDE POOL. Level-matched by the per-preset velocity: chord
   RMS within +-2.4 dB of the median (-18.9 dBFS), peaks -2.0 to -11.1. The manual's table has every row.
6. **The hidden eight on two pages:** NOISE 2 (EDIT, SELECT twice: TYPE GUST DUCK WIDTH) and VOICES (LFO, SELECT
   once: SPREAD CROSS MOD TIMBRE DRIFT PAN DRIFT). These are the MOTION 2 / NOISE 2 pages the 2026-11-07 cut pass
   took off the panel, back under standard names because without them no user preset library can exist (the
   panel's case; Kyle said build all six). Pages are now 17; the simulator's page numbers moved (ENV 4, LEAD 5,
   LFO 6, VOICES 7, FX 8-11, GEN 12-13, GLOBAL 14, MIDI 15, USB 16).

Measured: 647 checks (was 598: the SEL gestures, the CC map in and out, the switches as CCs, AS SAVED by CC, the
SysEx round trip, the bank's test re-pointed to F3's OCT UP), 16 renders rewritten as golden (the bank changed; the
renders are the bank), sanitizer clean but fastmath.h:58, screenshots regenerated (screens.txt now brings the F6
chord with SEL held), device image 404,660 B (+2.3 KB), RAM 77,612 B, pool 334,032 B (10.0 KB spare). Docs: manual
(sections 1-4, 6, 7, 9, 10, 12), both cheat sheets, README, BUDGET, landing page, words.tsv, this handoff.

Not in this build (the panel's 7-10, Kyle's call): settings saved without a user slot (hold SAVE), MIDI NOTES
HOLD/LATCH, Stop pausing GEN and CLOCK OUT in every mode, JITTER and CHANCE on ARP, LOW CUT on the hall, effects
fading in from zero, preset names on the save dialog, the three-way PRESET CHORD setting the Ableton panelist
wanted instead of the SEL gesture (program change still never brings a chord).

How to test (numbered for the unit):

1. Switch on: F1 FIRST LIGHT, silent. Hold SEL and turn PRESETS one click forward and back: the list says + CHORD
   under the name, and C3 G3 E4 come in latched. Turn PRESETS alone to F2: the sound morphs, the chord stays.
2. Hold SEL and turn PRESETS to F5 PULSAR: its four notes come in as yours let go. Press PLAY: a hard-sync
   arpeggio in 5/8 over a gated ping-pong echo. Hold SEL and press PLAY: the next step comes at once.
3. Hold SEL for three seconds and let go: nothing changes, LATCH stays lit. Hold SEL and tap HOME: ALL NOTES OFF,
   every beam sinks; the page does not change.
4. Tap EDIT, turn SELECT twice: NOISE 2 (TYPE GUST DUCK WIDTH). Tap LFO, SELECT once: VOICES (SPREAD CROSS MOD
   TIMBRE DRIFT PAN DRIFT); on F6 NEBULA turn SPREAD from 0 to full and hear the cluster open up.
5. Turn PRESETS through the bank: sixteen names end in INIT; press PLAY on each to hear its GEN mode (PLAY's colour
   changes with it); F7 DEEP VOID is one low note on tape, F10 RAIN CLOCK an electric piano under rain.
6. USB to the Mac, Live open, a MIDI track on DEEP FIELD FM-1 armed: turn any knob and a CC lane records; play it
   back and the knob's value on the screen follows, SAVE stays dark. Send CC 59 at 127 from a clip: FREEZE lights.
7. Arm an audio track on LOW CHOIR's heir, F10 RAIN CLOCK, with eight voices and GEN on, record a minute: no
   crackle, and no U in the header; `fm1_remote.py log` has no `u` figure on its cpu line.
8. In a terminal: `/usr/bin/python3 deepfield/tools/fm1_bank.py dump bank.syx` lists U1..U16 and writes the file;
   save a sound to U1 on the unit, dump again, `show bank.syx` names it.

## The flash before: DEEP FIELD's output backed off (flashed 2026-11-08; committed 7234746)

Flashed on Kyle's "Flash it" (`/usr/bin/python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc --yes`,
the 12:34 package, newer than every source, carrying the 4.35 M constant and not 8.7 M); booted clean: the unit
says DEEP FIELD. FIRST LIGHT. HOME. HARMONICS 37%. DRIFT 29%. REVERB 45%. TREMOLO 27%. VOICES 0. CPU 5%; the log
records no crash. Not yet heard by Kyle.

Kyle: "levels are way too hot in general on the device even turned up only 1/3 it can be redlining in ableton";
he records over the USB cable into Live on the Mac, and wants the headphones fixed too.

Measured before the change (host renders, MASTER full): a factory preset alone peaks -2 to -10 dBFS (RMS about
-18); four MIDI notes at velocity 127 on top reach the limiter at -0.1 dBFS. Neither the bus soft-clipper (under
1 dB) nor the limiter (at most 1 dB, CATHEDRAL and LOW CHOIR) was squashing anything: the output simply sat at the
top of the scale. The FM-1 has no headphone detect and the codec has no gain register, so speaker and headphones
are one signal and the firmware's own gain is the only lever.

What changed:
- `dr_render` (engine/drone.c): the output's full scale 8.7 M -> 4.35 M, so the limiter's ceiling is -6.1 dBFS on
  every output (speaker, headphones, USB). The sound is untouched: all 16 renders are the old ones halved to within
  1 LSB, goldens rewritten.
- `df_gain` (app.c): MASTER cubed instead of squared. A third of the way up -29 dB (was -19), halfway -18 (was -12),
  full unchanged. With the trim, a third of the way up is 16 dB quieter than before.
- USB LEVEL keeps its -6 dB default: the hardest playing now peaks at -12 dBFS in the recording; 0 dB reaches -6.
- Cost: the speaker at MASTER full is 6 dB quieter than on 0.9.
- basic.txt's 8-voice chord at MASTER half: bound -18 -> -30 dBFS (it measures -28.3; the same 1.7 dB margin as
  before). 598 checks pass. Manual: MASTER's law and USB LEVEL's levels.

How to test (numbered for the unit):

1. Headphones in, MASTER a third of the way up, latch a chord on F1 FIRST LIGHT: quiet but clear, no longer loud.
2. Sweep MASTER to full: the climb is gradual through the middle and the top quarter brings it up to full.
3. USB to the Mac, arm an audio track in Live on DEEP FIELD FM-1 (the header says USB MUTE): latch a chord and play
   hard over it on CATHEDRAL (F9). Live's meter peaks around -12 dB and never clips.
4. GLO, SELECT twice, USB LEVEL to 0 dB, repeat: peaks around -6 dB, still no clip.
5. Unplug the headphones: the speaker at MASTER full is quieter than before; say if it's now too quiet.

## The flash before: standard names for every DEEP FIELD setting (flashed 2026-11-08; committed 0b55c1b, site 0.9)

Flashed on Kyle's "Flash it" (`/usr/bin/python3 base/tools/fm1_install.py deepfield/build/deepfield.fwsc --yes`, the
11:49 package, newer than every source); booted clean: the unit says DEEP FIELD. FIRST LIGHT. HOME. HARMONICS 37%.
DRIFT 29%. REVERB 45%. TREMOLO 27%. VOICES 0. CPU 5%; the log `cpu 5% b0 ADDITIVE s0 x0`, no crash recorded. Site
0.9 published on Kyle's "publish/ commit update all docs" (`deepfield/tools/publish_site.sh 0.9`, release
FM-1_14000900), from 0b55c1b; Kyle played the unit after the flash (log: up to eight voices, 31 to 37 %, no
xruns). Master pushed to github.com/WisestWizard/deepfield on Kyle's "Push master" (09384cf through this note).

Kyle: "in any case where we use a non standard word for a setting I want to instead use the standard ... I don't
care about the words fitting a theme". He took every default of the proposal and kept two words: CHANCE and PLAYED.
The work below this section (the delay rework, the views cut) was committed first, apart, as 530340c; this build
sits on top of it and carries it, so one flash brings both.

What a player now sees, hears (words.tsv) and reads (manual, both cheat sheets, README, landing page, BUDGET):

| Was | Now |
| --- | --- |
| body / bodies, scene, bed, runner, the four forces | voice / voices, preset, the held chord, the arpeggio, the four macros |
| pages BODY, MOTION, ORBIT, ORBIT 2 | OSC, LFO (the band names it once), GEN, GEN 2 |
| HOME: MASS, SPACE, TIDE (DRIFT kept) | TIMBRE, REVERB, TREMOLO |
| ENVELOPE: SWELL, FADE, PRESS; LEAD: LIFT | ATTACK, RELEASE, AFTERTOUCH; TRANSPOSE |
| LFO: TIDE RATE, WOBBLE, WALK | TREM RATE, VIBRATO, RANDOM |
| NOISE: COLOUR, WIND, BAND; hidden TEXTURE | COLOR, RATE, BANDPASS; TYPE |
| DELAY types PING, HEADS, RISE, FALL, GRAINS | PING PONG, MULTI TAP, OCT UP, OCT DOWN, GRANULAR |
| DELAY 2: COLOUR, DIFFUSE, CHOP; REVERB: REV SIZE, REV DAMP, SPACE | TONE, DIFFUSION, GATE; DECAY, DAMPING, REVERB |
| GEN modes WANDER, CHORD, SCATTER, CYCLE, ECHO, PULSE (ARP kept) | WALK, SHIFT, RANDOM, PROG, LOOP, STRUM |
| GEN knobs EVERY, ARP's RATE, STEPS UP TO, DISTANCE, BODIES, ORDER, JUMPS, ROLL | RATE, TEMPO, RANGE, RANGE, VOICES, PATTERN, OCT JUMP, SPEED |
| GEN 2: HOME PULL, RHYTHM | RETURN, EUCLID |
| a voice's ORBIT MOVES / PINNED | LOCK OFF / ON |
| hidden CROSS, TIMBRE, SWAY | CROSS MOD, TIMBRE DRIFT, PAN DRIFT |
| models SOLAR, VOID, GLASS, STRING, CHOIR, FOLD, SCAN, DX, TEAR (BELL, SHEPARD, TAPE, RESONATOR kept) | ADDITIVE, SUPERSAW, FM, BOWED, FORMANT, WAVEFOLD, WAVETABLE, FM 4-OP, SYNC |
| scales CHROMA, PHRYGIA, MIXO, PENTA+, PENTA-, WHOLE, HARM- | CHROMATIC, PHRYGIAN, MIXOLYDIAN, MAJ PENTA, MIN PENTA, WHOLE TONE, HARM MINOR |
| messages EVERY BODY LET GO, ORBIT ON, SCENE + CHORD, SAVE SCENE TO, pill PLAY ORBIT | ALL NOTES OFF, GEN ON, PRESET + CHORD, SAVE PRESET TO, PLAY GEN (PLAY SHIFT ... by mode) |

Calls made in the build (Kyle's "per model" pick for the model-dependent knobs):
- TIMBRE, SHAPE and the OSC page's NOISE are named on the screen for what they do on each model (`param_label`'s
  BY_MODEL table in ui.c): HARMONICS / ODD/EVEN on ADDITIVE, CUTOFF / DETUNE on SUPERSAW, FM AMOUNT / RATIO on FM,
  BRIGHTNESS / BOW NOISE on BOWED, FORMANT / VOWEL / BREATH, BRIGHTNESS / INHARMONIC / STRIKE on BELL, CENTER /
  RISE/FALL / NOISE on SHEPARD, CUTOFF / FOLD / NOISE, CUTOFF / POSITION / UNISON on WAVETABLE, FM AMOUNT / PATCH /
  FEEDBACK on FM 4-OP, CUTOFF / RATIO / RING MOD on SYNC, BANDWIDTH / SOURCE / HISS on TAPE, BRIGHTNESS / MATERIAL /
  NOISE on RESONATOR. The docs and MIDI call them TIMBRE, SHAPE and NOISE.
- A knob a model doesn't use is now empty and does nothing (`page_param`): NOISE on ADDITIVE, SUPERSAW and FM, SHAPE
  on BOWED. Before, AIR and SHAPE showed a value there that changed nothing. CC 71 still sets SHAPE's stored value.
- DX's SHAPE read VOICE; with voice now meaning a sounding note it reads PATCH.
- GEN's start message drops the word: WALK 5.1S (was WANDER EVERY 5.1S), as ARP's reads ARP 120 BPM.
- Names a page shares: ATTACK and RELEASE (ENVELOPE, LEAD), RATE (NOISE, GEN), GATE (DELAY 2, ARP); the manual says
  so where COLOUR's note was.
- A word too wide for its knob cell in the XS font now falls to FONT_T (`cell_value`): MIXOLYDIAN, HARM MINOR,
  WAVETABLE, RESONATOR (RESONATOR overflowed into SHAPE's cell before).
- Code identifiers (P_MASS, OM_WANDER, PG_ORBIT ...), comments, factory preset names (GLASS ORBIT, VOID TIDE ...),
  the screenshot file names and research.md keep the old words. Prose keeps British spelling; only the COLOR knob
  is American, as panels print it.
- Measured: 598 checks (seven spoken-word checks renamed), all 16 renders match golden, sanitizer clean but
  fastmath.h:58, every page and the longest names rendered and checked by eye (no label or value overruns).

How to test (numbered for the unit):

1. Switch on: HOME reads HARMONICS, DRIFT, REVERB, TREMOLO on F1 FIRST LIGHT (ADDITIVE). Turn ALGORITHM to SUPERSAW:
   KNOB 1 reads CUTOFF in Hz.
2. Tap EDIT: the band reads EDIT OSC, knobs MODEL, ODD/EVEN, SUB and an empty fourth. ALGORITHM through the models:
   the second and fourth knobs rename (VOWEL / BREATH on FORMANT, PATCH / FEEDBACK on FM 4-OP); BOWED's SHAPE is empty.
3. Tap LFO: the band reads LFO once; TREM RATE, VIBRATO, RANDOM, WIDTH.
4. Latch C4 E4 G4, tap ARP: MODE WALK, RATE, CHANCE, RANGE. PLAY: the pill reads PLAY GEN, the screen GEN ON WALK and
   the rate. KNOB 1 to SHIFT, RANDOM, ARP (TEMPO, GATE, PATTERN), PROG, LOOP, STRUM.
5. Hold E4 and turn KNOB 4: the band reads VOICE E4, the fourth knob LOCK OFF / ON.
6. GLO, KNOB 1 to MIXOLYDIAN: the word fits its cell in a smaller face. Hold SEL a second: ALL NOTES OFF.

Then the delay and views below: that build, with the SPACE fix (turning an effect's knob switches it back on), went to the unit first (the Transpose session's 11:44 package, under the old names); this flash carries it.

## The flash before: the delay's quiet types reworked, the views gone (2026-11-08; committed 530340c, flashed from the Transpose session)

Its test steps are below.

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

1. Hold SEL a second, tap SEL (LATCH off), REVERB down. FX, SELECT once (DELAY): DLY TIME about 500 MS, FDBK 70 %, MIX full.
   Tap C4 with each TYPE: TAPE wavers and darkens, MULTI TAP taps three times, REVERSE swells backwards, GRANULAR
   scatters pieces at octaves and a fifth (the names as renamed in the build above).
2. Tap SEL (LATCH on), hold C4 E4 G4, step TYPE again: TAPE beats against the chord, MULTI TAP pulses
   left-centre-right, REVERSE breathes in each DLY TIME, GRANULAR glitters.
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
  storage with migration, 22 pages, screen reader, manual with screenshots. §1 of `docs/roadmap.md` in WisestWizard/pocketpoly has the table.
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
