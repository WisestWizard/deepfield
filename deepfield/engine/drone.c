/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann */
/* DEEP FIELD's engine. Eight bodies: each is struck from a key (or MIDI), swells in over SWELL, sustains for as
 * long as it is latched or held, breathes with TIDE, wanders with DRIFT, and fades out over FADE when let go.
 * One of four models sounds a body: SOLAR (eight breathing harmonics), VOID (three detuned saws through a dark
 * filter), GLASS (two-operator FM, the index breathing), STRING (a bowed comb: noise and a sine bowing a feedback
 * line). ORBIT moves latched bodies to neighbouring scale notes now and then, gliding. The four forces of HOME
 * (MASS, DRIFT, SPACE, TIDE) shape every body at once; the rest of the scene refines them. Then fx.c. */
#include <stdint.h>
#include "fastmath.h"
#include "plat.h"                        /* the second core only: plat_cpu2_* */
#include "drone.h"
#include "fx.h"
#include "df_wt.h"                       /* SCAN's wavetables (tools/gen_wt.py, at build time) */

#define FS DR_FS
#define CTL DR_CTL
#define NV DR_NV
#define MAXBLOCK 256u
#define SIN_N 4096u
#define BED_GAIN 0.7079f                 /* the bed (the bodies and the noise layer) 3 dB under where it was: Kyle,
                                          * "bed too loud" (the factory scenes peaked at the limiter); the leads stay,
                                          * so they stand 3 dB further out of it */
#define SIN_MASK (SIN_N - 1u)
#define LINE_LEN 2048u                   /* STRING: down to 21.5 Hz, int16 */
#define LINE_MASK (LINE_LEN - 1u)
#define NQ 32u

scene_t dr_scene;

typedef struct {
    uint8_t on, fading, restart, latched, sustained, midi, quick;
    uint8_t home;                        /* the note it was struck at: the key that lets it go */
    uint8_t model;                       /* the model it sounds in (a change restarts it) */
    uint8_t lead, held;                  /* a lead (LATCH off): held while its key is down, then it releases */
    int8_t ed_lvl, ed_tone, ed_tune;     /* its own LEVEL (dB), TONE (% of MASS), TUNE (cents): dr_body_edit */
    uint8_t pinned;                      /* ORBIT, PULSE and the arp leave it where it is */
    uint8_t xf_out;                      /* handing over to its twin in the scene's new model: no note off, no restart */
    uint8_t dxs[4];                      /* DX: each operator's envelope stage, 1..3 toward L1..L3, 4 holding L3 */
    float ed_gain;                       /* LEVEL as a gain (0: muted) */
    float xf;                            /* a hand-over's crossfade: the envelope's step a tick, up (the twin) or down */
    float dxl[4];                        /* DX: each operator's envelope level, 0..99 */
    float mboost;                        /* a lead cuts: a little MASS of its own */
    float sp_det, sp_mass, sp_tide, sp_pan;   /* its own small differences (SPREAD): detune, MASS, tide rate, pan */
    float twalk, sway_ph, pan_live;      /* the timbre's walk, the sway's phase, the pan heard */
    uint32_t pulse_left;                 /* PULSE: samples to its dip and swell, 0 none */
    float target, pitch;                 /* the note it goes to (ORBIT) and where the glide has it */
    float env, vel, press, pan;
    float walk, tide_ph, wob_ph, breath_ph, spark;
    float lvl_s;
    float ph, sub_ph, ph2, mph;
    float ph3[3], z[3];
    float lp1, lp2, sl, dc_x, dc_y;
    float hb[8], amp[8];
    float fs[6];                         /* CHOIR: the three formant filters' states */
    float gp;                            /* SHEPARD: the glide's place in its octave, 0..1 */
    uint32_t rng, age, lpos;
    uint32_t pad[7];                     /* to a whole number of 32-byte lines: the two cores write different voices */
} voice_t;
typedef char voice_on_lines[(sizeof(voice_t) % 32u) == 0u ? 1 : -1];

typedef struct {                         /* what a block shares between its voices (both cores read it) */
    float mass, drift, shape, sub, air, swell_k, fade_m, press_amt, glide_k, tide_inc, tide_depth, wob_depth, wob_inc,
          walk_depth, width, bend, wheel, chan_press, breath_inc, hinc;
    float attack_k, rel_m, lead_level, lift;   /* the leads' envelope and place */
    float spread, cross, twalk_depth, sway_inc, sway;   /* MOTION 2 */
    uint32_t model, n;
} blk_t;
static blk_t g_blk PLAT_POOL;

#define NL DR_NL
#define NV_ALL (NV + NL)                 /* the bodies, then the leads */
static voice_t vc[NV_ALL] __attribute__((aligned(32)));   /* each voice on its own cache lines: the second core
                                                           * writes bodies 0..1 and the leads */
static uint32_t age_ctr, voices_max = NV;
static float pcur[P_COUNT], pstart[P_COUNT], ptarget[P_COUNT];   /* 0..1: the values heard, and the morph's ends */
static float morph_pos = 1.0f, morph_step;
static float bend, wheel, chan_press, volume = 1.0f, gain_now;
static uint8_t press[128];
static uint32_t sustain;
static uint32_t scale_mask = 0xFFFu, scale_root;
static uint32_t orbit_on, frozen, fx_bypass;
static uint32_t orbit_left, orbit_per, orbit_rng = 0x2545F491u;
static uint8_t pend_to[NV];              /* ORBIT: a body's destination note, 0 none */
/* CYCLE: progressions as scale degrees from where the chord was struck (0 = home); the chord shifts along the
 * scale to each in turn, so in a major key the first is I IV V I, in a minor key i iv v i */
static const uint8_t PROG[DR_PROGS][4] = {{0, 3, 4, 0}, {0, 5, 3, 4}, {0, 4, 5, 3}, {0, 5, 2, 6},
                                          {0, 3, 0, 4}, {0, 1, 4, 0}, {0, 2, 5, 3}, {0, 6, 3, 0}};
static uint32_t cyc_pos;                 /* the step the chord stands on */
/* ECHO: the last eight lead notes the keys (or MIDI) played, replayed each period over the period, quieter each
 * time, notes dropped and jumped an octave with DECAY and JUMPS, until the phrase is gone or new notes come */
static uint8_t echo_ring[8], echo_n, echo_w, lead_machine;
static float echo_vel = 1.0f;
static uint32_t echo_due[8], echo_off[8];
static uint8_t echo_note[8];
static uint32_t arp_notes[NV * 3u], arp_n, arp_pos, arp_dir, arp_left, arp_step, arp_count, arp_due;
static uint32_t arp_age[NV * 3u];        /* each step's body, by its age: the chord changing, the arp goes on from it */
static uint32_t arp_mem[NV], arp_mem_age[NV], arp_mem_n;   /* the latched notes as struck, kept once the bed is let go */
static int8_t arp_mem_tune[NV], arp_tune[NV * 3u];   /* each note's own TUNE: the step sounds where its body does */
static uint32_t arp_hits;                /* notes played (the RHYTHM's rests not counted) */
/* RHYTHM: the runner's hits over a pattern of steps, spread as evenly as they go (Euclid); a rest waits, and the
 * notes move on only at a hit. DIV: a step's length in MIDI clocks (24 a beat) */
const char *const DR_RHYTHM_NAME[DR_RHYTHMS] = {"8/8", "5/8", "3/8", "7/12", "5/12", "4/9", "7/16", "5/16"};
static const uint8_t RHY[DR_RHYTHMS][2] = {{8, 8}, {5, 8}, {3, 8}, {7, 12}, {5, 12}, {4, 9}, {7, 16}, {5, 16}};
const char *const DR_DIV_NAME[DR_DIVS] = {"1/16", "1/16T", "1/8", "1/8T", "1/8D", "1/4", "1/4T", "1/2", "1 BAR"};
static const uint8_t DIV_CLK[DR_DIVS] = {6, 4, 12, 8, 18, 24, 16, 48, 96};
static uint32_t arp_rhythm(void) { return dr_scene.p[P_ARP_RHYTHM] < DR_RHYTHMS ? dr_scene.p[P_ARP_RHYTHM] : 0u; }
static uint32_t arp_div_clk(void) { return DIV_CLK[dr_scene.p[P_ARP_DIV] < DR_DIVS ? dr_scene.p[P_ARP_DIV] : 2u]; }
static uint32_t clk_now, clk_last, clk_period, clk_count, clk_ext, clk_out, orbit_clk_left, orbit_clk_per;
static uint32_t clk_hist[25], clk_hn, clk_hw;   /* the last 25 clocks' arrivals: the tempo over a whole beat, so a
                                                 * clock bunched with the next (a Bluetooth packet, a computer's
                                                 * buffer) still counts once and the tempo does not wobble */
static int32_t clk_src = -1;             /* the input the clock is followed from: the same clock on a second one is
                                          * not counted again */
static volatile uint32_t orbit_reper;    /* EVERY turned: the countdown under way shortened to it (orbit_tick) */
static float clk_acc;
static void (*rt_out)(uint32_t);
static int32_t arp_held = -1;            /* the step's lead sounding, its note */
static uint32_t arp_gate_left;
static void (*note_out)(uint32_t, uint32_t, int);
static volatile uint32_t orbit_moves, orbit_from, orbit_to;
static float *sintab;
static int16_t *lines;
static float *mixl, *mixr, *mixl2, *mixr2;
static float *hall_in_l, *hall_in_r, *hall_out_l, *hall_out_r;   /* the bus a block ago, and the hall's wet of it */
static uint32_t hall_n;
static fx_params_t g_fxp PLAT_POOL;      /* the block's effect settings, read by the second core too */
static int have_arena;
static float pk_bus, pk_out;
static uint32_t ovl_cool, calm_blocks;
static int32_t newest_note = -1;

static struct { uint8_t note, vel, on, kind; } q[NQ];   /* main loop -> render; kind 1: SEL's latch / release,
                                                         * 2: a latched strike, 3: unlatch, 4: every body let go,
                                                         * 5: a lead, 6: the latched bodies transposed, 7: a body's
                                                         * own value (on: which, vel: the value), 8: a latched body
                                                         * let go by its note */
static volatile uint32_t q_head, q_tail;
static scene_t load_scene;               /* a scene loaded with its chord: its bodies are struck and let go by the render */
static volatile uint32_t load_pending;
/* a scene landing in another model: each body sounding hands over to a twin in the new model, the twin rising as
 * the old one falls over the morph; low to high, as voices free up (with eight bodies, one restarts at a time) */
static volatile uint32_t xf_seq, xf_kind;  /* the main loop's request (kind 1: hand over, 0: no more), counted */
static volatile float xf_req_s;
static uint32_t xf_on, xf_seen, xf_wait;
static float xf_secs;

static inline float p01(uint32_t id) { return pcur[id]; }
static inline float clamp01(float x) { return x < 0.0f ? 0.0f : x > 1.0f ? 1.0f : x; }
static inline float tsin(float turns)    /* a sine of a phase in turns, 4096 steps */
{
    return sintab[(uint32_t)((turns + 64.0f) * (float)SIN_N) & SIN_MASK];
}
static inline float softclip(float x)
{
    x = x > 1.5f ? 1.5f : x < -1.5f ? -1.5f : x;
    return x - x * x * x * (4.0f / 27.0f);
}
static inline float frand(uint32_t *rng)  /* -1..1 */
{
    *rng = *rng * 1664525u + 1013904223u;
    return (float)(int32_t)*rng * (1.0f / 2147483648.0f);
}

float dr_time_s(uint32_t p) { return 0.03f * fm_exp2f((float)(p & 127u) * (9.4f / 127.0f)); }       /* 0.03 .. 20 s */
float dr_orbit_s(uint32_t p) { return 0.1f * fm_exp2f((float)(p & 127u) * (9.2288187f / 127.0f)); }   /* 0.1 .. 60 s */
float dr_dly_time_s(uint32_t p)
{
    p &= 127u;
    if (p >= 100u)
        p = 99u;
    return 0.05f * fm_exp2f((float)p * (4.0f / 99.0f));   /* 50 ms doubling four times to 800 */
}
int32_t dr_dly_div(uint32_t p) { p &= 127u; return p < 100u ? -1 : (int32_t)((p - 100u) / 4u); }
const char *const DR_DLY_DIV_NAME[DR_DLY_NDIV] = {"1/16", "1/8", "1/4T", "1/8D", "1/4", "1/4D", "1/2"};
static const float DLY_DIV_BEATS[DR_DLY_NDIV] = {0.25f, 0.5f, 2.0f / 3.0f, 0.75f, 1.0f, 1.5f, 2.0f};
float dr_hz(uint32_t p) { return 60.0f * fm_exp2f((float)(p & 127u) * (7.6f / 127.0f)); }
static float glide_s(float p) { return 0.02f * fm_exp2f(p * 8.6f); }                                  /* 0.02 .. 7.8 s */
float dr_attack_s(uint32_t p) { return 0.001f * fm_exp2f((float)(p & 127u) * (11.0f / 127.0f)); }    /* 1 ms .. 2 s */
float dr_release_s(uint32_t p) { return 0.03f * fm_exp2f((float)(p & 127u) * (8.0f / 127.0f)); }     /* 30 ms .. 7.7 s */
static inline float vmass(const voice_t *v, const blk_t *b)   /* a voice's MASS: the force, pressure, a lead's boost,
                                                               * its own TONE */
{
    return clamp01(b->mass + v->press * b->press_amt * 0.6f + v->mboost + v->sp_mass * b->spread * 0.15f
                   + v->twalk * b->twalk_depth * 0.25f + (float)v->ed_tone * 0.01f);
}

/* ------------------------------------------------------------------ the scale --- */
uint32_t dr_snap(uint32_t note)          /* the scale note at or below */
{
    uint32_t i;
    for (i = 0; i < 12u && note >= i; i++)
        if (scale_mask >> ((note - i + 12u - scale_root) % 12u) & 1u)
            return note - i;
    return note;
}
static int32_t scale_step(int32_t note, int32_t steps)   /* steps scale degrees from note (itself a scale note) */
{
    int32_t d = steps > 0 ? 1 : -1, n = note;
    while (steps) {
        uint32_t guard = 12u;
        do
            n += d;
        while (guard-- && !(scale_mask >> (((uint32_t)n + 1200u - scale_root) % 12u) & 1u));
        steps -= d;
    }
    return n;
}

/* ------------------------------------------------------------------ bodies --- */
static void voice_osc_init(voice_t *v, uint32_t model)
{
    uint32_t i;
    v->model = (uint8_t)model;
    v->ph = v->sub_ph = v->mph = 0.0f;
    v->ph2 = 0.37f;
    for (i = 0; i < 3u; i++) {           /* the saws' state is x^2 at the phase they start at: no spike */
        float x = 2.0f * 0.31f * (float)i - 1.0f;
        v->ph3[i] = 0.31f * (float)i;
        v->z[i] = model == M_GLASS ? 0.0f : x * x;
    }
    v->lp1 = v->lp2 = v->sl = v->dc_x = v->dc_y = 0.0f;
    for (i = 0; i < 6u; i++)
        v->fs[i] = 0.0f;
    v->gp = 0.0f;
    for (i = 0; i < 4u; i++) {           /* DX: every operator's envelope from silence */
        v->dxl[i] = 0.0f;
        v->dxs[i] = 1;
    }
    if (model == M_TAPE)                 /* its first dropout not before two seconds in */
        v->lpos = (uint32_t)(FS * 2.0f) + (v->rng >> 16) * 2u;
    if (model == M_STRING) {
        int16_t *ln = lines + (v - vc) * LINE_LEN;
        for (i = 0; i < LINE_LEN; i++)
            ln[i] = 0;
        v->lpos = 0;
    }
}

static void voice_strike(voice_t *v, uint32_t note, uint32_t vel, uint32_t latch, uint32_t midi)
{
    uint32_t i, steal = v->restart == 2u;   /* stolen: it fades a moment first (voice_tick's restart), then starts */
    v->on = 1;
    v->fading = 0;
    if (!steal)
        v->restart = 0;
    v->quick = 0;
    v->latched = (uint8_t)latch;
    v->sustained = 0;
    v->midi = (uint8_t)midi;
    v->lead = v->held = 0;
    v->mboost = 0.0f;
    v->ed_lvl = v->ed_tone = v->ed_tune = 0;
    v->pinned = 0;
    v->ed_gain = 1.0f;
    v->xf = 0.0f;
    v->xf_out = 0;
    v->home = (uint8_t)note;
    v->target = v->pitch = (float)note;
    if (!steal)
        v->env = 0.0f;
    v->vel = 0.35f + 0.65f * (float)vel * (1.0f / 127.0f);
    v->press = 0.0f;
    v->walk = 0.0f;
    if (!steal)
        v->lvl_s = 0.0f;
    v->age = ++age_ctr;
    v->rng = 0x9E3779B9u ^ (note * 2654435761u) ^ age_ctr;
    v->sp_det = frand(&v->rng);          /* SPREAD: its own small differences, fixed for its life */
    v->sp_mass = frand(&v->rng);
    v->sp_tide = 1.0f + 0.25f * frand(&v->rng);
    v->sp_pan = frand(&v->rng);
    v->twalk = 0.0f;
    v->sway_ph = 0.5f + 0.5f * frand(&v->rng);
    v->pulse_left = 0;
    v->tide_ph = 0.75f + 0.13f * (float)(v - vc);        /* each body breathes from its own point of the wave */
    v->wob_ph = 0.0f;
    v->breath_ph = 0.0f;
    v->spark = 0.0f;
    for (i = 0; i < 8u; i++) {
        v->hb[i] = 0.11f * (float)(i * (v - vc + 1u));
        v->amp[i] = 0.0f;
    }
    {
        float pan = ((float)note - 60.0f) * (1.0f / 24.0f);
        v->pan = fm_clampf(pan, -1.0f, 1.0f);
        v->pan_live = v->pan;
    }
    if (!steal)
        voice_osc_init(v, dr_scene.p[P_MODEL] < M_COUNT ? dr_scene.p[P_MODEL] : 0u);
    newest_note = (int32_t)note;
}

static voice_t *find_body(uint32_t note, int fading_too)   /* a body handing over is never the key's: its twin is */
{
    uint32_t i;
    voice_t *best = 0;
    for (i = 0; i < NV; i++)
        if (vc[i].on && vc[i].home == note && !vc[i].xf_out && (fading_too || !vc[i].fading)) {
            int here = (uint32_t)(vc[i].target + 0.5f) == note, best_here = best && (uint32_t)(best->target + 0.5f) == note;
            if (!best || here > best_here || (here == best_here && vc[i].age > best->age))   /* two known by one note
                                                                                              * (one moved there): the
                                                                                              * one sounding there */
                best = &vc[i];
        }
    return best;
}

static voice_t *alloc_body(void)
{
    uint32_t i, sounding = 0;
    voice_t *oldest_fading = 0, *oldest = 0;
    for (i = 0; i < NV; i++)
        if (vc[i].on) {
            sounding++;
            if (vc[i].fading && (!oldest_fading || vc[i].age < oldest_fading->age))
                oldest_fading = &vc[i];
            if (!oldest || vc[i].age < oldest->age)
                oldest = &vc[i];
        }
    if (sounding < voices_max)
        for (i = 0; i < NV; i++)
            if (!vc[i].on)
                return &vc[i];
    return oldest_fading ? oldest_fading : oldest;
}

static void let_go(voice_t *v)
{
    if (v->on && !v->fading) {
        v->fading = 1;
        if (v->xf > 0.0f)                /* a twin let go before it had risen: an ordinary fade from where it is */
            v->xf = 0.0f;
        v->latched = 0;
        v->sustained = 0;
        if (note_out && v->midi == 0u)
            note_out((uint32_t)(v->target + 0.5f), 0, 0);
    }
}

static void note_event(uint32_t note, uint32_t vel, uint32_t on, uint32_t latch, uint32_t midi)
{
    voice_t *v;
    note &= 127u;
    if (on) {
        v = find_body(note, 0);
        if (v && v->latched && !midi) {  /* latched and its key pressed again: let go */
            uint32_t at = (uint32_t)(v->target + 0.5f);
            if (at == note) {
                let_go(v);
                return;
            }
            v->home = (uint8_t)at;       /* ORBIT took it away: the key strikes a new body, that one is known where it
                                          * sounds */
            v = 0;
        }
        if (v) {                         /* held and struck again (MIDI): keep it, at the new velocity */
            v->vel = 0.35f + 0.65f * (float)vel * (1.0f / 127.0f);
            return;
        }
        v = find_body(note, 1);
        if (v) {                         /* fading: swell back from where it is, gliding home if ORBIT had moved it */
            v->fading = 0;
            v->target = (float)note;
            v->latched = (uint8_t)latch;
            v->sustained = 0;
            v->midi = (uint8_t)midi;
            v->vel = 0.35f + 0.65f * (float)vel * (1.0f / 127.0f);
            v->age = ++age_ctr;
            newest_note = (int32_t)note;
            return;
        }
        v = alloc_body();
        if (!v)
            return;
        if (v->on) {                     /* stolen: it restarts at once, a short fade first; its note off out */
            if (note_out && v->midi == 0u && !v->fading)
                note_out((uint32_t)(v->target + 0.5f), 0, 0);
            v->restart = 2;
        }
        voice_strike(v, note, vel, latch, midi);
    } else {
        v = find_body(note, 0);
        if (!v || v->latched)
            return;
        if (midi && sustain) {
            v->sustained = 1;
            return;
        }
        let_go(v);
    }
}
/* a lead: LATCH off, the keys (or MIDI) play the scene's model with the LEAD envelope over the bed */
static voice_t *find_lead(uint32_t note)
{
    uint32_t i;
    voice_t *best = 0;
    for (i = NV; i < NV_ALL; i++)
        if (vc[i].on && vc[i].held && vc[i].home == note)
            if (!best || vc[i].age > best->age)
                best = &vc[i];
    return best;
}
static void lead_event(uint32_t note, uint32_t vel, uint32_t on, uint32_t midi)
{
    voice_t *v;
    note &= 127u;
    if (on) {
        uint32_t i;
        v = 0;
        for (i = NV; i < NV_ALL; i++)    /* a free lead, else the oldest */
            if (!vc[i].on) {
                v = &vc[i];
                break;
            }
        if (!v)
            for (i = NV; i < NV_ALL; i++)
                if (!v || vc[i].age < v->age)
                    v = &vc[i];
        if (v->on) {                     /* stolen: a short fade first, and its note off out if still owed */
            if (note_out && v->midi == 0u && v->held)
                note_out((uint32_t)(v->target + 0.5f), 0, 0);
            v->restart = 2;
        }
        voice_strike(v, note, vel, 0, midi);
        v->lead = 1;
        v->held = 1;
        if (!lead_machine) {             /* a player's note: into ECHO's phrase, which wakes */
            echo_ring[echo_w] = (uint8_t)note;
            echo_w = (echo_w + 1u) % 8u;
            if (echo_n < 8u)
                echo_n++;
            echo_vel = 1.0f;
        }
        v->mboost = 0.15f;
        v->target = v->pitch = (float)note + g_blk.lift;
        v->walk = 0.0f;
    } else {
        v = find_lead(note);
        if (!v)
            return;
        if (midi && sustain) {
            v->sustained = 1;
            return;
        }
        v->held = 0;
        if (note_out && v->midi == 0u)
            note_out((uint32_t)(v->target + 0.5f), 0, 0);
    }
}
static void latch_hold(void)             /* SEL: latch what the keys hold, or let every latched body go */
{
    uint32_t i, n = 0;
    for (i = 0; i < NV; i++)
        if (vc[i].on && !vc[i].fading && !vc[i].midi && !vc[i].latched) {
            vc[i].latched = 1;
            n++;
        }
    if (n)
        return;
    for (i = 0; i < NV; i++)
        if (vc[i].on && vc[i].latched)
            let_go(&vc[i]);
}

static void queue_put(uint32_t note, uint32_t vel, uint32_t on, uint32_t kind)   /* main loop -> render */
{
    uint32_t h = q_head, nx = (h + 1u) % NQ;
    if (nx == q_tail)
        return;
    q[h].note = (uint8_t)note;
    q[h].vel = (uint8_t)vel;
    q[h].on = (uint8_t)on;
    q[h].kind = (uint8_t)kind;
    q_head = nx;
}
void dr_note(uint32_t note, uint32_t vel, int on) { queue_put(note, vel, on != 0, 0); }
void dr_latch_hold(void) { queue_put(0, 0, 0, 1); }
void dr_note_latched(uint32_t note, uint32_t vel) { queue_put(note, vel, 1, 2); }
void dr_unlatch(uint32_t note) { queue_put(note, 0, 0, 3); }
void dr_transpose_to(uint32_t note) { queue_put(note, 0, 0, 6); }
void dr_let_go(uint32_t home) { queue_put(home, 0, 0, 8); }
int32_t dr_body_at(uint32_t note)
{
    uint32_t i;
    const voice_t *best = 0;
    for (i = 0; i < NV; i++) {
        const voice_t *v = &vc[i];
        if (v->on && !v->fading && !v->xf_out && v->latched && (uint32_t)(v->target + 0.5f) == (note & 127u))
            if (!best || v->age > best->age)
                best = v;
    }
    return best ? (int32_t)best->home : -1;
}
static void body_set(voice_t *v, uint32_t what, int32_t x)   /* one of a body's own values, within its range */
{
    switch (what) {
    case BE_LEVEL:
        x = x < DR_LVL_MUTE ? DR_LVL_MUTE : x > 6 ? 6 : x;
        v->ed_lvl = (int8_t)x;
        v->ed_gain = x == DR_LVL_MUTE ? 0.0f : fm_exp2f((float)x * (1.0f / 6.0206f));
        break;
    case BE_TONE: v->ed_tone = (int8_t)(x < -50 ? -50 : x > 50 ? 50 : x); break;
    case BE_TUNE: v->ed_tune = (int8_t)(x < -50 ? -50 : x > 50 ? 50 : x); break;
    case BE_PIN:
        v->pinned = x != 0;
        if (v->pinned)
            pend_to[v - vc] = 0;         /* a move already planned for it is dropped */
        break;
    default: break;
    }
}
static void body_edit(uint32_t note, uint32_t what, int32_t x)
{
    voice_t *v = find_body(note & 127u, 0);
    if (v && v->latched)
        body_set(v, what, x);
}
void dr_body_edit(uint32_t note, uint32_t what, int32_t v) { queue_put(note, (uint32_t)(uint8_t)(int8_t)v, what, 7); }
void dr_body_edit_now(uint32_t note, uint32_t what, int32_t v) { body_edit(note, what, v); }
int32_t dr_body_value(uint32_t note, uint32_t what)
{
    const voice_t *v = find_body(note & 127u, 0);
    if (!v)
        return 0;
    return what == BE_LEVEL ? v->ed_lvl : what == BE_TONE ? v->ed_tone : what == BE_TUNE ? v->ed_tune : v->pinned;
}
void dr_note_now(uint32_t note, uint32_t vel, int on) { note_event(note, vel, on != 0, 0, 1); }
void dr_lead(uint32_t note, uint32_t vel, int on) { queue_put(note, vel, on != 0, 5); }
void dr_lead_now(uint32_t note, uint32_t vel, int on) { lead_event(note, vel, on != 0, 1); }
static void transpose_to(uint32_t note)  /* every latched body by the same step, gliding there; ORBIT's home moves too */
{
    uint32_t i, low = 128u;
    int32_t d;
    for (i = 0; i < NV; i++)
        if (vc[i].on && !vc[i].fading && vc[i].latched && vc[i].home < low)
            low = vc[i].home;
    if (low > 127u || note == low)
        return;
    d = (int32_t)note - (int32_t)low;
    for (i = 0; i < NV; i++)             /* every note off before any note on: a body moving onto another's old note
                                          * must not be cut by that one's off */
        if (vc[i].on && !vc[i].fading && vc[i].latched && vc[i].midi == 0u && note_out
            && (int32_t)vc[i].home + d >= 0 && (int32_t)vc[i].home + d <= 127)
            note_out((uint32_t)(vc[i].target + 0.5f), 0, 0);
    for (i = 0; i < NV; i++) {
        voice_t *v = &vc[i];
        int32_t cur, h;
        if (!v->on || v->fading || !v->latched)
            continue;
        h = (int32_t)v->home + d;
        if (h < 0 || h > 127)
            continue;
        cur = (int32_t)(v->target + 0.5f) + d;
        if (note_out && v->midi == 0u)
            note_out((uint32_t)(cur < 0 ? 0 : cur > 127 ? 127 : cur), (uint32_t)(v->vel * 127.0f), 1);
        v->home = (uint8_t)h;
        v->target += (float)d;
        pend_to[i] = 0;
    }
}
static void all_off(void);
static void drain_queue(void)
{
    while (q_tail != q_head) {
        uint32_t t = q_tail;
        if (q[t].kind == 1u)
            latch_hold();
        else if (q[t].kind == 2u)
            note_event(q[t].note, q[t].vel, 1, 1, 0);
        else if (q[t].kind == 3u) {
            voice_t *v = find_body(q[t].note, 0);
            if (v)
                v->latched = 0;
        } else if (q[t].kind == 4u)
            all_off();
        else if (q[t].kind == 5u)
            lead_event(q[t].note, q[t].vel, q[t].on, 0);
        else if (q[t].kind == 6u)
            transpose_to(q[t].note);
        else if (q[t].kind == 7u)
            body_edit(q[t].note, q[t].on, (int32_t)(int8_t)q[t].vel);
        else if (q[t].kind == 8u) {
            voice_t *v = find_body(q[t].note, 0);
            if (v && v->latched)
                let_go(v);
        }
        else
            note_event(q[t].note, q[t].vel, q[t].on, 0, 0);
        q_tail = (t + 1u) % NQ;
    }
}
uint32_t dr_latched(void)
{
    uint32_t i, n = 0;
    for (i = 0; i < NV; i++)
        n += vc[i].on && !vc[i].fading && vc[i].latched;
    return n;
}
int dr_body_latched(uint32_t note)
{
    const voice_t *v = find_body(note & 127u, 0);
    return v && v->latched;
}

void dr_pressure(uint32_t note, uint32_t v) { press[note & 127u] = (uint8_t)(v & 127u); }
void dr_channel_pressure(uint32_t v) { chan_press = (float)(v & 127u) * (1.0f / 127.0f); }
void dr_wheel(uint32_t v) { wheel = (float)(v & 127u) * (1.0f / 127.0f); }
void dr_bend(uint32_t b) { bend = ((float)(b & 16383u) - 8192.0f) * (2.0f / 8192.0f); }
void dr_sustain(uint32_t on)
{
    uint32_t i;
    sustain = on > 63u;
    if (!sustain)
        for (i = 0; i < NV_ALL; i++)
            if (vc[i].on && vc[i].sustained) {
                if (vc[i].lead) {
                    vc[i].held = 0;
                    vc[i].sustained = 0;
                } else
                    let_go(&vc[i]);
            }
}
void dr_volume(uint32_t cc) { volume = (float)(cc & 127u) * (1.0f / 127.0f); }
static void all_off(void)
{
    uint32_t i;
    for (i = 0; i < NV_ALL; i++) {
        if (vc[i].lead)
            vc[i].held = 0;
        else
            let_go(&vc[i]);
    }
}
void dr_all_off(void) { queue_put(0, 0, 0, 4); }   /* through the queue: the render owns the bodies */
void dr_panic(void)
{
    uint32_t i;
    for (i = 0; i < NV_ALL; i++) {
        vc[i].on = 0;
        vc[i].env = vc[i].lvl_s = 0.0f;
    }
    q_tail = q_head;                     /* the queue dropped: one store of the render's own word */
}
void dr_scale(uint32_t mask12, uint32_t root)
{
    scale_mask = (mask12 & 0xFFFu) ? mask12 & 0xFFFu : 0xFFFu;
    scale_root = root % 12u;
}
static void orbit_reset(void);
void dr_orbit(int on)
{
    uint32_t was = orbit_on;
    orbit_on = on != 0;
    orbit_left = orbit_per = 0;
    orbit_reset();
    if (clk_out && !clk_ext && rt_out && dr_orbit_mode() == OM_ARP && was != orbit_on) {   /* the runner's start and stop */
        rt_out(orbit_on ? 0xFAu : 0xFCu);
        clk_acc = 0.0f;
    }
}
float dr_orbit_next(void)
{
    if (!orbit_on)
        return 0.0f;
    if (dr_orbit_mode() == OM_ARP) {     /* through the RHYTHM's pattern */
        uint32_t len = RHY[arp_rhythm()][1];
        if (clk_ext)
            return (float)(clk_count % (len * arp_div_clk())) / (float)(len * arp_div_clk());
        return arp_step ? ((float)((arp_count + len - 1u) % len) + 1.0f - (float)arp_left / (float)arp_step) / (float)len
                        : 0.0f;
    }
    if (clk_ext)
        return orbit_clk_per ? 1.0f - (float)orbit_clk_left / (float)orbit_clk_per : 0.0f;
    return orbit_per ? 1.0f - (float)orbit_left / (float)orbit_per : 0.0f;
}
int dr_orbit_on(void) { return (int)orbit_on; }
void dr_freeze(int on) { frozen = on != 0; }
int dr_frozen(void) { return (int)frozen; }
void dr_fx_bypass(uint32_t bits) { fx_bypass = bits; }
uint32_t dr_fx_bypass_get(void) { return fx_bypass; }
uint32_t dr_fx_active(void)
{
    uint32_t a = 0;
    if (dr_scene.p[P_ENS] && dr_scene.p[P_ENS_MIX])
        a |= DR_FX_ENS;
    if (dr_scene.p[P_DLY_MIX])
        a |= DR_FX_DLY;
    if (dr_scene.p[P_SPACE])
        a |= DR_FX_REV;
    if (dr_scene.p[P_SHIMMER] && dr_scene.p[P_SPACE])
        a |= DR_FX_SHIM;
    a &= ~fx_bypass;
    if (dr_scene.p[P_NZ_LEVEL])
        a |= DR_FX_NOISE;
    return a;
}
void dr_note_out(void (*fn)(uint32_t, uint32_t, int)) { note_out = fn; }
uint32_t dr_orbit_moved(uint32_t *from, uint32_t *to)
{
    *from = orbit_from;
    *to = orbit_to;
    return orbit_moves;
}
void dr_overload(void)                   /* the render ran over 90 % of its time: one body fewer from now on, and the
                                          * oldest one sounding let go now, so the load falls at once */
{
    uint32_t i;
    voice_t *oldest = 0;
    if (ovl_cool)
        return;
    if (voices_max > 2u)
        voices_max--;
    for (i = 0; i < NV; i++)
        if (vc[i].on && !vc[i].fading && (!oldest || vc[i].age < oldest->age))
            oldest = &vc[i];
    if (oldest)
        let_go(oldest);
    ovl_cool = 90u;
    calm_blocks = 0;
}

/* ------------------------------------------------------------------ the scene --- */
void dr_param(uint32_t id, uint32_t v)
{
    if (id >= P_COUNT)
        return;
    dr_scene.p[id] = (uint8_t)(v & 127u);
    pcur[id] = pstart[id] = ptarget[id] = (float)(v & 127u) * (1.0f / 127.0f);
    if (id == P_ORB_PERIOD)
        orbit_reper = 1;
    if (id == P_MODEL) {                 /* ALGO: every body crosses into the new model, as a scene's does */
        xf_req_s = 0.8f;
        xf_kind = 1;
        xf_seq++;
    }
}
void dr_load(const scene_t *s, float seconds, int chord)
{
    uint32_t i, model_was = dr_scene.p[P_MODEL];
    dr_scene = *s;
    if (s->p[P_MODEL] != model_was) {    /* the bodies cross into it over the morph; at once: they restart */
        xf_req_s = seconds;
        xf_kind = seconds > 0.01f;
        xf_seq++;
    }
    for (i = 0; i < P_COUNT; i++) {
        pstart[i] = pcur[i];
        ptarget[i] = (float)(s->p[i] & 127u) * (1.0f / 127.0f);
    }
    pcur[P_MODEL] = pstart[P_MODEL] = ptarget[P_MODEL];   /* the model does not morph */
    if (seconds > 0.01f) {
        morph_pos = 0.0f;
        morph_step = (float)CTL / (seconds * FS);
    } else {
        morph_pos = 1.0f;
        for (i = 0; i < P_COUNT; i++)
            pcur[i] = ptarget[i];
    }
    if (!chord)
        return;
    load_pending = 0;                    /* down while the copy is torn, up once it has landed */
    __asm__ volatile("" ::: "memory");
    load_scene = *s;
    __asm__ volatile("" ::: "memory");
    load_pending = 1;
}
static void load_bodies(void)            /* render: the scene's bodies swell in with their own values; the latched
                                          * ones not in it fade */
{
    const scene_t *s = &load_scene;
    uint32_t i, k;
    load_pending = 0;
    for (i = 0; i < NV; i++)
        if (vc[i].on && !vc[i].fading && vc[i].latched) {
            int keep = 0;
            for (k = 0; k < NV; k++)
                if (s->vel[k] && s->note[k] == vc[i].home)
                    keep = 1;
            if (!keep)
                let_go(&vc[i]);
        }
    for (k = 0; k < NV; k++)
        if (s->vel[k]) {
            voice_t *v = find_body(s->note[k], 0);
            if (!v) {
                note_event(s->note[k], s->vel[k], 1, 1, 0);
                v = find_body(s->note[k], 0);
            }
            if (v) {
                v->latched = 1;
                body_set(v, BE_LEVEL, s->lvl[k]);
                body_set(v, BE_TONE, s->tone[k]);
                body_set(v, BE_TUNE, s->tune[k]);
                body_set(v, BE_PIN, (int32_t)(s->pins >> k & 1u));
            }
        }
}
static voice_t *free_body(void)          /* a voice not sounding, within the overload's count; 0 none */
{
    uint32_t i, sounding = 0;
    for (i = 0; i < NV; i++)
        sounding += vc[i].on != 0;
    if (sounding < voices_max)
        for (i = 0; i < NV; i++)
            if (!vc[i].on)
                return &vc[i];
    return 0;
}
static void twin(voice_t *v, voice_t *t, uint32_t model)   /* body v hands over to t, the same body in the new model */
{
    float k = xf_secs > 0.05f ? (float)CTL / (xf_secs * FS) : 1.0f;
    *t = *v;                             /* its note, glide, age, its own values, its breathing's phases */
    voice_osc_init(t, model);
    t->env = t->lvl_s = 0.0f;
    t->xf = k;
    t->xf_out = 0;
    t->restart = t->quick = 0;
    t->fading = 0;
    pend_to[t - vc] = pend_to[v - vc];
    pend_to[v - vc] = 0;
    v->xf = -(v->env > 0.001f ? v->env : 0.001f) * k;   /* from where it is to silence in the same time */
    v->xf_out = 1;
    v->fading = 1;                       /* out of ORBIT's and the arp's reach; its note goes on in the twin */
    v->latched = v->sustained = 0;
}
static void handover(uint32_t model, uint32_t n)
{
    uint32_t i, j, need = 0, busy = 0;
    voice_t *cand[NV];
    if (xf_seq != xf_seen) {
        xf_seen = xf_seq;
        xf_on = xf_kind;
        xf_secs = xf_req_s;
        xf_wait = 0;
    }
    if (!xf_on)
        return;
    for (i = 0; i < NV; i++) {
        voice_t *v = &vc[i];
        if (!v->on)
            continue;
        if (v->xf_out)
            busy++;
        else if (!v->fading && !v->restart && v->model != model) {
            for (j = need++; j && cand[j - 1u]->pitch > v->pitch; j--)   /* low to high */
                cand[j] = cand[j - 1u];
            cand[j] = v;
        }
    }
    if (!need) {
        xf_on = 0;
        return;
    }
    if (xf_wait > n) {
        xf_wait -= n;
        return;
    }
    xf_wait = 0;
    for (i = 0; i < need; i++) {
        voice_t *t = free_body();
        if (t) {
            twin(cand[i], t, model);
            continue;
        }
        if (!busy) {                     /* every voice sounding and none on its way out: this one restarts where
                                          * it is (a moment's dip), the next a little later */
            cand[i]->restart = 1;
            xf_wait = (uint32_t)(xf_secs * FS / (float)need);
        }
        break;
    }
}
int dr_morphing(void) { return morph_pos < 1.0f; }
float dr_heard(uint32_t id) { return id < P_COUNT ? pcur[id] * 127.0f : 0.0f; }
float dr_morph_pos(void) { return morph_pos; }

/* ------------------------------------------------------------------ a tick --- */
static const float LOG2H[8] = {0.0f, 1.0f, 1.5849625f, 2.0f, 2.3219281f, 2.5849625f, 2.8073549f, 3.0f};
static const float GLASS_RATIO[8] = {1.0f, 2.0f, 3.0f, 3.5f, 4.0f, 5.0f, 7.0f, 1.414f};
/* CHOIR: the vowels A E I O U, three formants each (Hz), SHAPE morphing through them; MASS shifts them and
 * brings up the upper two */
static const float VOWEL[5][3] = {{730.0f, 1090.0f, 2440.0f}, {530.0f, 1840.0f, 2480.0f}, {270.0f, 2290.0f, 3010.0f},
                                  {570.0f, 840.0f, 2410.0f}, {300.0f, 870.0f, 2240.0f}};
static void choir_formants(float shape, float mass, float *F, float *G)
{
    float p = shape * 3.999f, t = p - (float)(uint32_t)p, sh = fm_exp2f((mass - 0.5f) * 0.8f);
    uint32_t a = (uint32_t)p, k;
    for (k = 0; k < 3u; k++)
        F[k] = (VOWEL[a][k] + (VOWEL[a + 1u][k] - VOWEL[a][k]) * t) * sh;
    G[0] = 1.0f;
    G[1] = 0.25f + 0.55f * mass;
    G[2] = 0.08f + 0.4f * mass;
    {                                    /* an open vowel passes far more of the buzz than a closed one: evened out */
        float ev = fm_exp2f(-0.45f * fm_log2f(F[0] * (1.0f / 300.0f)));
        for (k = 0; k < 3u; k++)
            G[k] *= ev;
    }
}
/* BELL: partial h's ratio, from the harmonic series to a bell's with SHAPE */
static float bell_ratio(uint32_t h, float shape)
{
    static const float BELLR[4] = {1.0f, 2.76f, 5.40f, 8.93f};
    return (float)(h + 1u) + (BELLR[h] - (float)(h + 1u)) * shape;
}
/* SHEPARD: component k's weight under the raised-cosine window over six octaves, MASS leaning it high or low */
static float shepard_window(uint32_t k, float gp, float mass)
{
    float x = ((float)k + gp) * (1.0f / 6.0f), w = 0.5f - 0.5f * tsin(x + 0.25f);   /* a cosine bump over 0..1 */
    return w * fm_exp2f((mass - 0.5f) * 3.0f * (x - 0.5f));
}

static void voice_tick(voice_t *v, const blk_t *b, float *f_out, float *lvl_out)
{
    float cents, vib, note, f, prs, mass, tide, lvl;
    uint32_t h;
    /* the envelope */
    if (v->restart) {
        v->env *= 0.82f;                 /* a quick fade, then the new model from silence */
        if (v->env < 0.01f) {
            uint32_t was_fading = v->fading;
            voice_osc_init(v, b->model);
            v->env = 0.0f;
            v->restart = 0;
            v->quick = 1;                /* back in a few ms, whatever SWELL says */
            v->fading = (uint8_t)was_fading;
        }
    } else if (v->xf != 0.0f) {          /* a hand-over: the twin rises as the old one falls, over the morph */
        v->env += v->xf;
        if (v->xf > 0.0f && v->env >= 1.0f) {
            v->env = 1.0f;
            v->xf = 0.0f;
        } else if (v->xf < 0.0f && v->env <= 0.0f) {
            v->on = 0;
            v->env = 0.0f;
            v->xf = 0.0f;
            v->xf_out = 0;
        }
    } else if (v->lead) {                /* a lead: up at ATTACK while held, then down at RELEASE */
        if (v->held) {
            v->env += v->quick ? 0.12f : b->attack_k;
            if (v->env >= 1.0f) {
                v->env = 1.0f;
                v->quick = 0;
            }
        } else {
            v->env *= b->rel_m;
            v->fading = 0;
            if (v->env < 0.0005f) {
                v->on = 0;
                v->env = 0.0f;
            }
        }
    } else if (v->fading) {
        v->env *= b->fade_m;
        if (v->env < 0.0005f) {
            v->on = 0;
            v->env = 0.0f;
        }
    } else {
        v->env += v->quick ? 0.12f : b->swell_k;   /* a straight swell: full level at SWELL */
        if (v->env >= 1.0f) {
            v->env = 1.0f;
            v->quick = 0;
        }
    }
    if (v->pulse_left) {                 /* PULSE: a dip, and the swell brings it back */
        if (v->pulse_left > CTL)
            v->pulse_left -= CTL;
        else {
            v->pulse_left = 0;
            if (!v->fading && !v->lead)
                v->env *= 0.25f;
        }
    }
    /* pressure: brighter and more wobble while the key is leaned on */
    prs = (float)press[v->home] * (1.0f / 127.0f);
    prs = prs > b->chan_press ? prs : b->chan_press;
    v->press += (prs - v->press) * 0.08f;
    mass = vmass(v, b);
    /* pitch: the glide, the random walk, the wobble */
    v->pitch += (v->target - v->pitch) * b->glide_k;
    v->walk += frand(&v->rng) * 0.012f - v->walk * 0.0015f;
    cents = v->walk * b->walk_depth * 40.0f + v->sp_det * b->spread * 14.0f   /* SPREAD: up to 14 cents apart */
          + (float)v->ed_tune;
    v->twalk += frand(&v->rng) * 0.003f - v->twalk * 0.0006f;   /* the timbre's own slow walk */
    v->twalk = fm_clampf(v->twalk, -1.0f, 1.0f);
    v->sway_ph += b->sway_inc * (0.8f + 0.4f * v->sp_tide);
    if (v->sway_ph >= 1.0f)
        v->sway_ph -= 1.0f;
    v->pan_live = fm_clampf(v->pan + v->sp_pan * b->spread * 0.5f + tsin(v->sway_ph) * b->sway * 0.7f, -1.0f, 1.0f);
    if (b->cross > 0.0f && !v->lead) {  /* CROSS: the partner's swell bends this body (bodies in pairs) */
        const voice_t *q = &vc[(v - vc) ^ 1u];
        if (q->on && !q->lead)
            cents += (q->lvl_s - v->lvl_s) * (1.0f / BED_GAIN) * b->cross * 180.0f;   /* the trim not in the bend */
    }
    v->wob_ph += b->wob_inc * (1.0f + 0.09f * (float)(v - vc));
    if (v->wob_ph >= 1.0f)
        v->wob_ph -= 1.0f;
    vib = tsin(v->wob_ph) * (b->wob_depth * 0.25f + v->press * b->press_amt * 0.3f + b->wheel * 0.5f);
    note = v->pitch + b->bend + cents * 0.01f + vib;
    f = 440.0f * fm_exp2f((note - 69.0f) * (1.0f / 12.0f));
    if (f > 8000.0f)
        f = 8000.0f;
    *f_out = f;
    /* the breathing */
    v->tide_ph += b->tide_inc * (0.85f + 0.045f * (float)(v - vc)) * (1.0f + (v->sp_tide - 1.0f) * b->spread);
    if (v->tide_ph >= 1.0f)
        v->tide_ph -= 1.0f;
    v->spark = 0.5f + 0.5f * tsin(v->tide_ph);
    tide = v->lead ? 1.0f : 1.0f - b->tide_depth * v->spark;   /* a lead does not breathe */
    lvl = v->env * (v->xf != 0.0f ? 2.0f - v->env : 1.0f)    /* a hand-over: equal power, the pair 0.5 dB up midway */
        * v->vel * tide * (v->lead ? b->lead_level : v->ed_gain * BED_GAIN);
    *lvl_out = lvl;
    v->breath_ph += b->breath_inc;
    if (v->breath_ph >= 1.0f)
        v->breath_ph -= 1.0f;
    /* SOLAR's harmonics this tick (the only model that plays from them) */
    if (v->model == M_SOLAR) {
        float tilt = 2.5f - 2.3f * mass, sum = 0.0f, evenw = b->shape * 2.0f, oddw = (1.0f - b->shape) * 2.0f,
              bd = b->drift * 0.8f, nyq = (float)SIN_N;
        if (evenw > 1.0f)
            evenw = 1.0f;
        if (oddw > 1.0f)
            oddw = 1.0f;
        (void)nyq;
        for (h = 0; h < 8u; h++) {
            float a = fm_exp2f(-tilt * LOG2H[h]);
            if (h) {
                a *= (h & 1u) ? evenw : oddw;   /* h is the harmonic number - 1: odd h = even harmonics */
                v->hb[h] += b->hinc * (1.0f + 0.17f * (float)h);
                if (v->hb[h] >= 1.0f)
                    v->hb[h] -= 1.0f;
                a *= 1.0f - bd * (0.5f + 0.5f * tsin(v->hb[h]));
                if (f * (float)(h + 1u) > 18000.0f)
                    a = 0.0f;
            }
            v->amp[h] = a;
            sum += a;
        }
        sum = sum > 0.05f ? 0.92f / sum : 0.0f;
        for (h = 0; h < 8u; h++)
            v->amp[h] *= sum;
    }
}

/* ---------------------------------------------------------- the inner loops --- */
static void render_solar(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float inc = f / FS, sinc = inc * 0.5f, ph = v->ph, sph = v->sub_ph, l = v->lvl_s, dl = (lvl - l) / (float)m;
    float sub = b->sub * 0.5f, a0 = v->amp[0], a1 = v->amp[1], a2 = v->amp[2], a3 = v->amp[3], a4 = v->amp[4],
          a5 = v->amp[5], a6 = v->amp[6], a7 = v->amp[7];
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i;
    for (i = 0; i < m; i++) {
        float s, p;
        ph += inc;
        if (ph >= 1.0f)
            ph -= 1.0f;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        p = ph * (float)SIN_N;
        s = a0 * sintab[(uint32_t)p & SIN_MASK] + a1 * sintab[(uint32_t)(p * 2.0f) & SIN_MASK]
          + a2 * sintab[(uint32_t)(p * 3.0f) & SIN_MASK] + a3 * sintab[(uint32_t)(p * 4.0f) & SIN_MASK]
          + a4 * sintab[(uint32_t)(p * 5.0f) & SIN_MASK] + a5 * sintab[(uint32_t)(p * 6.0f) & SIN_MASK]
          + a6 * sintab[(uint32_t)(p * 7.0f) & SIN_MASK] + a7 * sintab[(uint32_t)(p * 8.0f) & SIN_MASK]
          + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    v->ph = ph;
    v->sub_ph = sph;
    v->lvl_s = lvl;
}

static void render_void(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float det = (0.3f + b->drift) * (0.04f + b->shape * 0.45f), inc0 = f / FS,
          inc1 = inc0 * fm_exp2f(det * (1.0f / 12.0f)), inc2 = inc0 * fm_exp2f(-det * 0.7f * (1.0f / 12.0f)),
          c0 = 0.25f / inc0, c1 = 0.25f / inc1, c2 = 0.25f / inc2, l = v->lvl_s, dl = (lvl - l) / (float)m;
    float mass = vmass(v, b), fc = dr_hz((uint32_t)(mass * 127.0f)), a;
    float p0 = v->ph3[0], p1 = v->ph3[1], p2 = v->ph3[2], z0 = v->z[0], z1 = v->z[1], z2 = v->z[2];
    float lp1 = v->lp1, lp2 = v->lp2, sph = v->sub_ph, sinc = inc0 * 0.5f, sub = b->sub * 0.6f;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i;
    if (fc < f * 1.2f)
        fc = f * 1.2f;
    a = 1.0f - fm_expf(-6.2831853f * fc / FS);
    if (a > 0.95f)
        a = 0.95f;
    for (i = 0; i < m; i++) {
        float x, sq, s;
        p0 += inc0;
        if (p0 >= 1.0f)
            p0 -= 1.0f;
        p1 += inc1;
        if (p1 >= 1.0f)
            p1 -= 1.0f;
        p2 += inc2;
        if (p2 >= 1.0f)
            p2 -= 1.0f;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        x = 2.0f * p0 - 1.0f;
        sq = x * x;
        s = (sq - z0) * c0;
        z0 = sq;
        x = 2.0f * p1 - 1.0f;
        sq = x * x;
        s += (sq - z1) * c1 * 0.8f;
        z1 = sq;
        x = 2.0f * p2 - 1.0f;
        sq = x * x;
        s += (sq - z2) * c2 * 0.8f;
        z2 = sq;
        s = s * (1.25f / 2.6f) + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        lp1 += (s - lp1) * a;
        lp2 += (lp1 - lp2) * a;
        l += dl;
        s = lp2 * l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    v->ph3[0] = p0;
    v->ph3[1] = p1;
    v->ph3[2] = p2;
    v->z[0] = z0;
    v->z[1] = z1;
    v->z[2] = z2;
    v->lp1 = fm_flush(lp1);
    v->lp2 = fm_flush(lp2);
    v->sub_ph = sph;
    v->lvl_s = lvl;
}

static void render_glass(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float mass = vmass(v, b);
    float ratio = GLASS_RATIO[(uint32_t)(b->shape * 7.99f)], inc = f / FS, minc = inc * ratio,
          det = (0.3f + b->drift) * 0.012f, inc2 = inc * (1.0f + det), sinc = inc * 0.5f;
    float idx = mass * 4.0f * (1.0f - b->drift * 0.45f * (0.5f + 0.5f * tsin(v->breath_ph))) * 0.159155f;
    float ph = v->ph, ph2 = v->ph2, mph = v->mph, sph = v->sub_ph, l = v->lvl_s, dl = (lvl - l) / (float)m,
          sub = b->sub * 0.5f;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i;
    if (minc > 0.45f)
        minc = 0.45f;
    v->z[0] += (idx - v->z[0]) * 0.2f;
    idx = v->z[0];
    for (i = 0; i < m; i++) {
        float mod, s;
        ph += inc;
        if (ph >= 1.0f)
            ph -= 1.0f;
        ph2 += inc2;
        if (ph2 >= 1.0f)
            ph2 -= 1.0f;
        mph += minc;
        if (mph >= 1.0f)
            mph -= 1.0f;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        mod = idx * sintab[(uint32_t)(mph * (float)SIN_N) & SIN_MASK];
        s = sintab[(uint32_t)((ph + mod + 8.0f) * (float)SIN_N) & SIN_MASK]
          + 0.6f * sintab[(uint32_t)((ph2 + mod * 0.7f + 8.0f) * (float)SIN_N) & SIN_MASK];
        s = s * 0.6f + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    v->ph = ph;
    v->ph2 = ph2;
    v->mph = mph;
    v->sub_ph = sph;
    v->lvl_s = lvl;
}

static void render_string(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    int16_t *ln = lines + (v - vc) * LINE_LEN;
    float mass = vmass(v, b);
    float len = FS / f, damp = 0.06f + mass * 0.8f, inc = f / FS, ph = v->ph, sl = v->sl, l = v->lvl_s,
          dl = (lvl - l) / (float)m, air = 0.3f * (0.1f + b->air), bow = 0.05f * (1.0f - b->air * 0.6f),
          dcx = v->dc_x, dcy = v->dc_y, sph = v->sub_ph, sinc = inc * 0.5f, sub = b->sub * 0.4f;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i, pos = v->lpos, rng = v->rng;
    if (len > (float)(LINE_LEN - 4u))
        len = (float)(LINE_LEN - 4u);
    if (len < 4.0f)
        len = 4.0f;
    for (i = 0; i < m; i++) {
        float rp = (float)pos - len, y, w, s, n;
        uint32_t i0, i1;
        int32_t ri;
        if (rp < 0.0f)
            rp += (float)LINE_LEN;
        ri = (int32_t)rp;
        i0 = (uint32_t)ri & LINE_MASK;
        i1 = (i0 + 1u) & LINE_MASK;
        y = ((float)ln[i0] + ((float)ln[i1] - (float)ln[i0]) * (rp - (float)ri)) * (1.0f / 32767.0f);
        sl += (y - sl) * damp;
        rng = rng * 1664525u + 1013904223u;
        n = (float)(int32_t)rng * (1.0f / 2147483648.0f);
        ph += inc;
        if (ph >= 1.0f)
            ph -= 1.0f;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        w = n * air + (sintab[(uint32_t)(ph * (float)SIN_N) & SIN_MASK]
                       + 0.35f * sintab[(uint32_t)(ph * (float)SIN_N * 2.0f) & SIN_MASK]) * bow + sl * 0.995f;
        w = softclip(w);
        w *= 32767.0f;
        ln[pos] = (int16_t)(w > 32767.0f ? 32767.0f : w < -32767.0f ? -32767.0f : w);
        pos = (pos + 1u) & LINE_MASK;
        s = sl - dcx + 0.995f * dcy;     /* no DC from the noise in the loop */
        dcx = sl;
        dcy = s;
        s = s * 1.1f + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    v->ph = ph;
    v->sub_ph = sph;
    v->sl = fm_flush(sl);
    v->dc_x = fm_flush(dcx);
    v->dc_y = fm_flush(dcy);
    v->lpos = pos;
    v->rng = rng;
    v->lvl_s = lvl;
}

static void render_choir(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float mass = vmass(v, b), F[3], G[3], a1[3], a2[3], a3[3], k = 0.14f;
    float inc = f / FS, p0 = v->ph3[0], z0 = v->z[0], c0 = 0.25f / inc, l = v->lvl_s, dl = (lvl - l) / (float)m;
    float air = b->air * 0.5f, sph = v->sub_ph, sinc = inc * 0.5f, sub = b->sub * 0.5f;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i, j, rng = v->rng;
    choir_formants(b->shape, mass, F, G);
    for (j = 0; j < 3u; j++) {           /* three band-passes, trapezoidal, Q about 7 */
        float g = 3.14159265f * (F[j] > 5000.0f ? 5000.0f : F[j]) / FS;
        a1[j] = 1.0f / (1.0f + g * (g + k));
        a2[j] = g * a1[j];
        a3[j] = g * a2[j];
    }
    for (i = 0; i < m; i++) {
        float x, sq, s = 0.0f, n;
        p0 += inc;
        if (p0 >= 1.0f)
            p0 -= 1.0f;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        x = 2.0f * p0 - 1.0f;
        sq = x * x;
        x = (sq - z0) * c0;              /* the buzz: a saw without the aliasing */
        z0 = sq;
        rng = rng * 1664525u + 1013904223u;
        n = (float)(int32_t)rng * (1.0f / 2147483648.0f);
        x = x * 0.5f + n * air;
        for (j = 0; j < 3u; j++) {
            float v3 = x - v->fs[2u * j + 1u], v1 = a1[j] * v->fs[2u * j] + a2[j] * v3,
                  v2 = v->fs[2u * j + 1u] + a2[j] * v->fs[2u * j] + a3[j] * v3;
            v->fs[2u * j] = 2.0f * v1 - v->fs[2u * j];
            v->fs[2u * j + 1u] = 2.0f * v2 - v->fs[2u * j + 1u];
            s += v1 * G[j];
        }
        s = s * k * 8.0f + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];   /* level-matched by render */
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    for (j = 0; j < 6u; j++)
        v->fs[j] = fm_flush(v->fs[j]);
    v->ph3[0] = p0;
    v->z[0] = z0;
    v->sub_ph = sph;
    v->rng = rng;
    v->lvl_s = lvl;
}

static void render_bell(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float mass = vmass(v, b), inc[4], a[4], tilt = 2.5f - 2.3f * mass;
    float ph[4], sph = v->sub_ph, sinc = f / FS * 0.5f, sub = b->sub * 0.5f, l = v->lvl_s, dl = (lvl - l) / (float)m;
    float air = b->air * 0.12f, lp = v->lp1, gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f),
          gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i, h, rng = v->rng;
    ph[0] = v->ph;
    ph[1] = v->ph2;
    ph[2] = v->mph;
    ph[3] = v->ph3[0];
    for (h = 0; h < 4u; h++) {           /* each partial breathes on its own, like SOLAR's harmonics */
        float r = bell_ratio(h, b->shape), fh = f * r;
        inc[h] = fh / FS;
        v->hb[h] += b->hinc * (0.6f + 0.23f * (float)h);
        if (v->hb[h] >= 1.0f)
            v->hb[h] -= 1.0f;
        a[h] = fm_exp2f(-tilt * LOG2H[h]) * (0.55f + 0.45f * tsin(v->hb[h])) * 0.7f;
        if (fh > 18000.0f)
            a[h] = inc[h] = 0.0f;        /* silent, and its phase stands still (it would never wrap) */
    }
    for (i = 0; i < m; i++) {
        float s = 0.0f, n;
        for (h = 0; h < 4u; h++) {
            ph[h] += inc[h];
            if (ph[h] >= 1.0f)
                ph[h] -= 1.0f;
            s += a[h] * sintab[(uint32_t)(ph[h] * (float)SIN_N) & SIN_MASK];
        }
        rng = rng * 1664525u + 1013904223u;
        n = (float)(int32_t)rng * (1.0f / 2147483648.0f);
        lp += (n - lp) * 0.3f;           /* a little dark hiss: the strike's noise */
        s += lp * air;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        s += sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    v->ph = ph[0];
    v->ph2 = ph[1];
    v->mph = ph[2];
    v->ph3[0] = ph[3];
    v->sub_ph = sph;
    v->lp1 = fm_flush(lp);
    v->rng = rng;
    v->lvl_s = lvl;
}

static void render_shepard(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float mass = vmass(v, b), d = (b->shape - 0.5f) * 2.0f;
    float rate = (d < 0.0f ? -1.0f : 1.0f) * 0.35f * d * d * (float)m / FS;   /* octaves this block, falling or rising */
    float inc[6], a[6], ph[6], l = v->lvl_s, dl = (lvl - l) / (float)m, sum = 0.0f, air = b->air * 0.1f, lp = v->lp1;
    float sph = v->sub_ph, sinc = f / FS * 0.5f, sub = b->sub * 0.4f;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i, k, rng = v->rng;
    v->gp += rate;
    if (v->gp >= 1.0f)
        v->gp -= 1.0f;
    if (v->gp < 0.0f)
        v->gp += 1.0f;
    ph[0] = v->ph;
    ph[1] = v->ph2;
    ph[2] = v->mph;
    ph[3] = v->ph3[0];
    ph[4] = v->ph3[1];
    ph[5] = v->ph3[2];
    for (k = 0; k < 6u; k++) {           /* six octaves, the lowest two below the note, sliding together */
        float fk = f * fm_exp2f((float)k - 3.0f + v->gp);
        inc[k] = fk / FS;
        a[k] = fk < 18000.0f && fk > 20.0f ? shepard_window(k, v->gp, mass) : 0.0f;
        if (a[k] == 0.0f)
            inc[k] = 0.0f;               /* silent, and its phase stands still */
        sum += a[k];
    }
    sum = sum > 0.05f ? 1.2f / sum : 0.0f;   /* level-matched by render */
    for (i = 0; i < m; i++) {
        float s = 0.0f, n;
        for (k = 0; k < 6u; k++) {
            ph[k] += inc[k];
            if (ph[k] >= 1.0f)
                ph[k] -= 1.0f;
            s += a[k] * sintab[(uint32_t)(ph[k] * (float)SIN_N) & SIN_MASK];
        }
        rng = rng * 1664525u + 1013904223u;
        n = (float)(int32_t)rng * (1.0f / 2147483648.0f);
        lp += (n - lp) * 0.2f;
        s = s * sum + lp * air;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        s += sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    v->ph = ph[0];
    v->ph2 = ph[1];
    v->mph = ph[2];
    v->ph3[0] = ph[3];
    v->ph3[1] = ph[4];
    v->ph3[2] = ph[5];
    v->sub_ph = sph;
    v->lp1 = fm_flush(lp);
    v->rng = rng;
    v->lvl_s = lvl;
}

static void render_fold(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float mass = vmass(v, b), fc = dr_hz((uint32_t)(mass * 127.0f)), c;
    float drive = (0.15f + b->shape * 2.2f) * (1.0f - b->drift * 0.3f * (0.5f + 0.5f * tsin(v->breath_ph)));
    float inc = f / FS, ph = v->ph, sph = v->sub_ph, sinc = inc * 0.5f, sub = b->sub * 0.5f, lp = v->lp1,
          l = v->lvl_s, dl = (lvl - l) / (float)m, air = b->air * 0.3f;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i, rng = v->rng;
    if (fc < f * 1.5f)
        fc = f * 1.5f;
    c = 1.0f - fm_expf(-6.2831853f * fc / FS);
    if (c > 0.95f)
        c = 0.95f;
    for (i = 0; i < m; i++) {
        float x, s, n;
        ph += inc;
        if (ph >= 1.0f)
            ph -= 1.0f;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        rng = rng * 1664525u + 1013904223u;
        n = (float)(int32_t)rng * (1.0f / 2147483648.0f);
        x = sintab[(uint32_t)(ph * (float)SIN_N) & SIN_MASK] * drive * (1.0f + n * air);
        s = tsin(x * 0.5f);              /* the fold: a sine of a sine, more turns the harder it is driven */
        lp += (s - lp) * c;
        s = lp * 0.8f + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    v->ph = ph;
    v->sub_ph = sph;
    v->lp1 = fm_flush(lp);
    v->rng = rng;
    v->lvl_s = lvl;
}

/* SCAN: Pocket Poly's three wavetables (ANALOG, VOX, GLASS) as one sweep of 48 frames under SHAPE, each body's
 * place wandering with DRIFT and TIMBRE, a band-limited copy for its pitch, MASS a low-pass, AIR a second copy
 * detuned at another place, SUB the sine */
const char *const DR_SCAN_NAME[3] = {"ANALOG", "VOX", "GLASS"};
#define SCAN_FR (DF_WT_TABLES * DF_WT_FRAMES)
static inline const int16_t *scan_frame(uint32_t g, uint32_t mip)   /* frame g (0..47) of the sweep */
{
    uint32_t t = g / DF_WT_FRAMES;
    return DF_WT + ((t * DF_WT_MIPS + mip) * DF_WT_FRAMES + g % DF_WT_FRAMES) * DF_WT_LEN;
}
static inline float scan_read(const int16_t *a, const int16_t *c, float fr, float ph)
{
    uint32_t i0 = (uint32_t)ph, i1 = (i0 + 1u) & (DF_WT_LEN - 1u);
    float t = ph - (float)i0, x = (float)a[i0] + (float)(a[i1] - a[i0]) * t, y = (float)c[i0] + (float)(c[i1] - c[i0]) * t;
    return x + (y - x) * fr;
}
static void render_scan(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float mass = vmass(v, b), fc = f * 3.0f * fm_exp2f(mass * 6.0f), c, pos, fr, pos2, fr2;   /* MASS follows the
                                         * note: 3 to 192 times it, the same brightness all along the keys */
    float inc = f / FS * (float)DF_WT_LEN, inc2 = inc * (1.0f + (0.3f + b->drift) * 0.004f), ph = v->ph, ph2 = v->ph2;
    float air = b->air * 0.8f, sph = v->sub_ph, sinc = f / FS * 0.5f, sub = b->sub * 0.5f, lp = v->lp1,
          l = v->lvl_s, dl = (lvl - l) / (float)m, k = 1.0f / 32768.0f;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    const int16_t *a0, *a1, *c0, *c1;
    uint32_t i, g0, g2, mip = 0;
    while (mip < DF_WT_MIPS - 1u && (float)(64u >> mip) * f > 19000.0f)   /* the copy with as many harmonics as fit */
        mip++;
    v->hb[0] += b->hinc * (0.5f + 0.13f * (float)(v - vc));   /* its own wander through the frames */
    if (v->hb[0] >= 1.0f)
        v->hb[0] -= 1.0f;
    pos = b->shape * (float)(SCAN_FR - 1u) + b->drift * 5.0f * tsin(v->hb[0]) + v->twalk * b->twalk_depth * 8.0f;
    pos = fm_clampf(pos, 0.0f, (float)(SCAN_FR - 1u) - 0.001f);
    g0 = (uint32_t)pos;
    fr = pos - (float)g0;
    pos2 = pos < (float)SCAN_FR * 0.5f ? pos + 7.0f : pos - 7.0f;   /* AIR's copy: another place in the sweep */
    g2 = (uint32_t)pos2;
    fr2 = pos2 - (float)g2;
    {                                    /* every frame as loud, whatever MASS (DF_WT_GAIN, at five of its settings) */
        float mp = mass * 3.999f, mf = mp - (float)(uint32_t)mp, ga, gb;
        uint32_t mi = (uint32_t)mp, g1 = g0 + 1u < SCAN_FR ? g0 + 1u : g0;
        ga = DF_WT_GAIN[g0][mi] + (DF_WT_GAIN[g0][mi + 1u] - DF_WT_GAIN[g0][mi]) * mf;
        gb = DF_WT_GAIN[g1][mi] + (DF_WT_GAIN[g1][mi + 1u] - DF_WT_GAIN[g1][mi]) * mf;
        k *= ga + (gb - ga) * fr;
    }
    a0 = scan_frame(g0, mip);
    a1 = scan_frame(g0 + 1u < SCAN_FR ? g0 + 1u : g0, mip);
    c0 = scan_frame(g2, mip);
    c1 = scan_frame(g2 + 1u < SCAN_FR ? g2 + 1u : g2, mip);
    c = 1.0f - fm_expf(-6.2831853f * fc / FS);
    if (c > 0.95f)
        c = 0.95f;
    for (i = 0; i < m; i++) {
        float s;
        ph += inc;
        if (ph >= (float)DF_WT_LEN)
            ph -= (float)DF_WT_LEN;
        s = scan_read(a0, a1, fr, ph);
        if (air > 0.01f) {
            ph2 += inc2;
            if (ph2 >= (float)DF_WT_LEN)
                ph2 -= (float)DF_WT_LEN;
            s += scan_read(c0, c1, fr2, ph2) * air;
        }
        lp += (s * k - lp) * c;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        s = lp + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];   /* level-matched by render */
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    v->ph = ph;
    v->ph2 = ph2 >= 0.0f && ph2 < (float)DF_WT_LEN ? ph2 : 0.0f;
    v->sub_ph = sph;
    v->lp1 = fm_flush(lp);
    v->lvl_s = lvl;
}

/* DX: four-operator FM after Pocket Poly's six (fm.c), SHAPE picking one of eight voices made to sustain. Each
 * operator has a DX envelope (rates and levels 0..99, up to L3 and holding there) run at SWELL's pace; MASS brightens
 * the modulators, DRIFT breathes them and spreads the detune, AIR is the feedback, SUB the sine. FADE stays the body's */
typedef struct { uint8_t r[3], l[3], coarse, fine; int8_t det; uint8_t level; } dx_op_t;
typedef struct { dx_op_t op[4]; uint8_t alg, fb; } dx_voice_t;
enum { DXA_STACK, DXA_PAIRS, DXA_FAN, DXA_ADD };
#define DB(k) (1u << (k))
static const uint8_t DX_CAR[4] = {       /* each algorithm's carriers (render_dx runs its routing) */
    DB(0),                               /* STACK: 4 > 3 > 2 > 1 */
    DB(0) | DB(2),                       /* PAIRS: 2 > 1, 4 > 3 */
    DB(0) | DB(1) | DB(2),               /* FAN: 4 > 1, 2, 3 */
    DB(0) | DB(1) | DB(2) | DB(3),       /* ADD: four sines */
};
#define DXOP(r1, r2, r3, l1, l2, l3, co, fi, de, lv) {{r1, r2, r3}, {l1, l2, l3}, co, fi, de, lv}
static const dx_voice_t DX_V[DR_DX_VOICES] = {
    {{DXOP(55, 40, 30, 99, 95, 92, 1, 0, 0, 99), DXOP(45, 35, 30, 99, 70, 64, 1, 0, 4, 68),     /* GLASS PAD */
      DXOP(55, 40, 30, 99, 95, 92, 1, 0, -4, 95), DXOP(45, 35, 30, 99, 72, 66, 3, 0, 0, 58)}, DXA_PAIRS, 2},
    {{DXOP(60, 50, 50, 99, 95, 92, 1, 0, 0, 99), DXOP(38, 45, 50, 99, 80, 74, 1, 0, 0, 84),     /* BRASS */
      DXOP(60, 50, 50, 99, 95, 92, 1, 0, 2, 95), DXOP(34, 45, 50, 99, 82, 76, 1, 0, -1, 78)}, DXA_PAIRS, 6},
    {{DXOP(99, 99, 99, 99, 99, 99, 0, 0, 0, 99), DXOP(99, 99, 99, 99, 99, 99, 1, 0, 1, 92),     /* ORGAN */
      DXOP(99, 99, 99, 99, 99, 99, 2, 0, -1, 84), DXOP(99, 99, 99, 99, 99, 99, 3, 0, 0, 74)}, DXA_ADD, 0},
    {{DXOP(99, 25, 20, 99, 85, 80, 1, 0, 0, 99), DXOP(99, 25, 15, 99, 60, 45, 3, 50, 0, 72),    /* BELL PAD */
      DXOP(99, 20, 15, 99, 80, 75, 2, 0, 5, 85), DXOP(99, 25, 15, 99, 55, 40, 7, 1, 0, 62)}, DXA_PAIRS, 3},
    {{DXOP(95, 28, 20, 99, 88, 80, 1, 0, 0, 99), DXOP(99, 45, 25, 99, 60, 30, 14, 0, 0, 52),    /* E.PIANO */
      DXOP(95, 30, 22, 99, 88, 80, 1, 0, 3, 92), DXOP(99, 50, 28, 99, 65, 50, 1, 0, 0, 70)}, DXA_PAIRS, 6},
    {{DXOP(99, 35, 25, 99, 85, 80, 1, 0, 0, 99), DXOP(99, 45, 30, 99, 40, 30, 4, 0, 0, 70),     /* WOOD */
      DXOP(99, 35, 25, 99, 85, 80, 2, 0, 2, 60), DXOP(99, 50, 30, 99, 35, 25, 10, 0, 0, 45)}, DXA_PAIRS, 0},
    {{DXOP(45, 40, 40, 99, 96, 94, 1, 0, 0, 99), DXOP(40, 40, 40, 99, 90, 86, 1, 0, -3, 78),    /* STRINGS */
      DXOP(45, 40, 40, 99, 96, 94, 1, 0, 6, 97), DXOP(40, 40, 40, 99, 88, 84, 1, 0, 3, 76)}, DXA_PAIRS, 5},
    {{DXOP(70, 40, 30, 99, 92, 90, 1, 0, 0, 95), DXOP(70, 40, 30, 99, 92, 90, 1, 41, 0, 88),    /* CLUSTER */
      DXOP(70, 40, 30, 99, 90, 86, 2, 82, 0, 80), DXOP(60, 40, 30, 99, 80, 74, 1, 41, 0, 62)}, DXA_FAN, 4},
};
static const float DX_GAIN[DR_DX_VOICES] = {1.459f, 1.301f, 1.924f, 1.619f, 1.410f, 1.619f, 1.315f, 2.314f};   /* each
                                         * voice as loud as SOLAR at F1's settings (measured) */
const char *const DR_DX_NAME[DR_DX_VOICES] = {"GLASS PAD", "BRASS", "ORGAN", "BELL PAD", "E.PIANO", "WOOD", "STRINGS",
                                             "CLUSTER"};
static void render_dx(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    uint32_t vi = (uint32_t)(b->shape * ((float)DR_DX_VOICES - 0.01f));
    const dx_voice_t *dv = &DX_V[vi];
    uint32_t car = DX_CAR[dv->alg], i, k, nc = 0;
    float inc[4], amp[4], ph[4], fb1 = v->z[0], fb2 = v->z[1], mass = vmass(v, b);
    float rate_x = fm_clampf(1.5f / dr_time_s((uint32_t)(p01(P_SWELL) * 127.0f)), 0.05f, 8.0f) * ((float)CTL / FS);
    float bright = (mass - 0.5f) * 40.0f + b->drift * 10.0f * tsin(v->breath_ph), fbv, fbg, cg;
    float sph = v->sub_ph, sinc = f / FS * 0.5f, sub = b->sub * 0.5f, l = v->lvl_s, dl = (lvl - l) / (float)m;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    fbv = fm_clampf((float)dv->fb + b->air * 4.0f, 0.0f, 7.0f) * (1.0f / 7.0f);
    fbg = fbv * fbv * 0.75f * 0.5f;
    ph[0] = v->ph;
    ph[1] = v->ph2;
    ph[2] = v->mph;
    ph[3] = v->ph3[0];
    for (k = 0; k < 4u; k++) {           /* each operator's envelope a tick, its level and its step */
        const dx_op_t *o = &dv->op[k];
        uint32_t st = v->dxs[k];
        float L;
        if (st >= 1u && st <= 3u) {      /* toward its stage's level at its rate; the third holds */
            float tgt = (float)o->l[st - 1u], step = 99.0f / (0.004f * fm_exp2f((99.0f - (float)o->r[st - 1u]) * (1.0f / 7.5f)))
                                                     * rate_x;
            if (v->dxl[k] < tgt) {
                v->dxl[k] += step;
                if (v->dxl[k] >= tgt) {
                    v->dxl[k] = tgt;
                    st++;
                }
            } else {
                v->dxl[k] -= step;
                if (v->dxl[k] <= tgt) {
                    v->dxl[k] = tgt;
                    st++;
                }
            }
            v->dxs[k] = (uint8_t)st;
        }
        L = (float)o->level;
        if (!(car >> k & 1u))            /* a modulator: brighter with MASS, breathing with DRIFT */
            L = fm_clampf(L + bright, 0.0f, 99.0f);
        else
            nc++;
        L -= 99.0f - v->dxl[k];
        amp[k] = L > 0.0f ? fm_exp2f((L - 99.0f) * (0.75f / 6.0206f)) : 0.0f;
        inc[k] = f / FS * (o->coarse ? (float)o->coarse : 0.5f) * (1.0f + (float)o->fine * 0.01f)
               * (1.0f + (float)o->det * (1.0f + b->drift) * (3.0f / 1200.0f * 0.6931472f));   /* a few cents: 1 + x ln 2 */
        if (inc[k] > 0.45f)
            inc[k] = amp[k] = 0.0f;      /* past the top: silent, its phase still */
    }
    cg = nc ? 2.0f / (1.0f + (float)nc * fm_rsqrtf((float)nc)) * DX_GAIN[vi] : 0.0f;   /* between a coherent and a
                                                                                       * random sum */
    cg *= 0.8f;
#define DXS(p) sintab[(uint32_t)(((p) + 64.0f) * (float)SIN_N) & SIN_MASK]   /* the modulation reaches a few turns */
#define DXPH(k)                                                                                                     \
    do {                                                                                                            \
        ph[k] += inc[k];                                                                                            \
        if (ph[k] >= 1.0f)                                                                                          \
            ph[k] -= 1.0f;                                                                                          \
    } while (0)
    for (i = 0; i < m; i++) {            /* each algorithm straight through: a modulator is always a higher operator */
        float s, o0, o1, o2, o3;
        DXPH(0);
        DXPH(1);
        DXPH(2);
        DXPH(3);
        o3 = DXS(ph[3] + (fb1 + fb2) * fbg) * amp[3];
        switch (dv->alg) {
        case DXA_STACK:
            o2 = DXS(ph[2] + o3) * amp[2];
            o1 = DXS(ph[1] + o2) * amp[1];
            o0 = DXS(ph[0] + o1) * amp[0];
            s = o0;
            break;
        case DXA_FAN:
            o0 = DXS(ph[0] + o3) * amp[0];
            o1 = DXS(ph[1] + o3) * amp[1];
            o2 = DXS(ph[2] + o3) * amp[2];
            s = o0 + o1 + o2;
            break;
        case DXA_ADD:
            s = DXS(ph[0]) * amp[0] + DXS(ph[1]) * amp[1] + DXS(ph[2]) * amp[2] + o3;
            break;
        default:                         /* PAIRS */
            o2 = DXS(ph[2] + o3) * amp[2];
            o1 = DXS(ph[1]) * amp[1];
            o0 = DXS(ph[0] + o1) * amp[0];
            s = o0 + o2;
            break;
        }
        fb2 = fb1;
        fb1 = o3;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        s = s * cg + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];   /* level-matched by render */
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
#undef DXS
#undef DXPH
    v->ph = ph[0];
    v->ph2 = ph[1];
    v->mph = ph[2];
    v->ph3[0] = ph[3];
    v->z[0] = fm_flush(fb1);
    v->z[1] = fm_flush(fb2);
    v->sub_ph = sph;
    v->lvl_s = lvl;
}

/* TEAR: a saw hard-synced to the note, SHAPE its ratio (1 to 8 times the note), DRIFT breathing it, so the formant
 * the reset makes slides; every edge (the saw's own and the reset's) gets a two-point polyBLEP one sample late, so
 * the sweep stays clean high up. AIR rings it with the note's sine (all harmonic: chords stay in tune), MASS a
 * low-pass that follows the note and a little drive, SUB the sine. Pocket Poly's TEAR, with the reset smoothed */
#define TEAR_GAIN 0.66f                  /* as loud as SOLAR at F1's settings (measured) */
float dr_tear_ratio(float shape) { return 1.0f + 7.0f * shape * shape; }
static void render_tear(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float mass = vmass(v, b), r = dr_tear_ratio(b->shape) * (1.0f + b->drift * 0.18f * tsin(v->breath_ph))
                                  + v->twalk * b->twalk_depth * 0.6f;
    float inc = f / FS, sinc, sub = b->sub * 0.5f, air = b->air, fc = f * 3.0f * fm_exp2f(mass * 6.0f), c,
          drive = 0.9f + mass * 0.8f, keep = (1.0f + 0.48f * b->shape) * fm_exp2f((2.55f - 5.15f * mass) * 0.1661f),
          pm = v->ph, ps = v->ph2, sph = v->sub_ph, prev = v->sl, lp1 = v->lp1,
          lp2 = v->lp2, dcx = v->dc_x, dcy = v->dc_y, l = v->lvl_s, dl = (lvl - l) / (float)m;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    float incs;
    uint32_t i;
    if (r < 1.0f)
        r = 1.0f;
    if (f * r > 14000.0f)
        r = 14000.0f / f;
    incs = inc * r;
    sinc = inc * 0.5f;
    c = 1.0f - fm_expf(-6.2831853f * (fc > 16000.0f ? 16000.0f : fc) / FS);
    for (i = 0; i < m; i++) {
        float cur, out, x, ring;
        pm += inc;
        ps += incs;
        cur = 0.0f;                      /* an edge of height J, t samples before this one: the sample before gets
                                          * J t^2 / 2, this one -J (1 - t)^2 / 2 */
        if (ps >= 1.0f) {                /* the saw's own edge: J = -2 */
            float t;
            ps -= 1.0f;
            t = ps / incs;
            prev -= t * t;
            cur += (1.0f - t) * (1.0f - t);
        }
        if (pm >= 1.0f) {                /* the reset: wherever the saw was, back to its start */
            float tm, before, jump;
            pm -= 1.0f;
            tm = pm / inc;
            before = ps - tm * incs;
            if (before < 0.0f)
                before += 1.0f;
            jump = -2.0f * before;
            ps = tm * incs;
            prev += jump * tm * tm * 0.5f;
            cur -= jump * (1.0f - tm) * (1.0f - tm) * 0.5f;
        }
        out = prev;                      /* the sample before, now that both its corrections are in */
        prev = 2.0f * ps - 1.0f + cur;
        dcy = out - dcx + 0.9979f * dcy; /* no DC: the reset leaves an offset that moves with the ratio */
        dcx = out;
        x = dcy;
        ring = x * tsin(pm) * 1.6f;
        x = x * (1.0f - 0.85f * air) + ring * air;
        x = softclip(x * drive);
        lp1 += (x - lp1) * c;
        lp2 += (lp1 - lp2) * c;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        x = lp2 * TEAR_GAIN * keep + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        l += dl;
        x *= l;
        ml[i] += x * gl;
        mr[i] += x * gr;
    }
    v->ph = pm;
    v->ph2 = ps;
    v->sub_ph = sph;
    v->sl = prev;
    v->lp1 = fm_flush(lp1);
    v->lp2 = fm_flush(lp2);
    v->dc_x = fm_flush(dcx);
    v->dc_y = fm_flush(dcy);
    v->lvl_s = lvl;
}

/* TAPE: a loop worn thin. SHAPE the source on the tape (a soft organ of three sines to a saw), MASS the tape's
 * bandwidth, DRIFT its wear: wow (a slow pitch sway, its own for each body), flutter, and now and then a dropout (the
 * level sinks and the top darkens for a third of a second); AIR the hiss, SUB the sine. Everything saturates a
 * little, as tape does. A body's dropouts come at their own random times, so a chord never drops out at once */
#define TAPE_GAIN 0.86f                  /* as loud as SOLAR at F1's settings (measured) */
static void render_tape(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float mass = vmass(v, b), wear = 0.15f + b->drift, cents, k, inc, c0, fc, c, dip;
    float ph = v->ph, z0 = v->z[0], lp1 = v->lp1, lp2 = v->lp2, hiss = v->fs[0], sph = v->sub_ph, sub = b->sub * 0.5f,
          sinc = f / FS * 0.5f, air = b->air * 0.09f, organ = (1.0f - b->shape) * 0.68f, saw = b->shape, l = v->lvl_s,
          dl = (lvl - l) / (float)m;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i, rng = v->rng;
    v->hb[0] += (0.35f + 0.04f * (float)(v - vc)) * (float)m / FS;   /* wow, each body at its own rate */
    if (v->hb[0] >= 1.0f)
        v->hb[0] -= 1.0f;
    v->hb[1] += (6.5f + 0.3f * (float)(v - vc)) * (float)m / FS;     /* flutter */
    if (v->hb[1] >= 1.0f)
        v->hb[1] -= 1.0f;
    cents = wear * (11.0f * tsin(v->hb[0]) + 2.5f * tsin(v->hb[1]));
    inc = f / FS * fm_exp2f(cents * (1.0f / 1200.0f));
    c0 = 0.25f / inc;
    if (v->mph > 0.0f) {                 /* a dropout under way: down fast, back slowly */
        v->mph += (float)m / (FS * 0.38f);
        if (v->mph >= 1.0f)
            v->mph = 0.0f;
    } else if (v->lpos > m) {
        v->lpos -= m;
    } else {                             /* the next one: sooner the more worn the tape */
        rng = rng * 1664525u + 1013904223u;
        v->lpos = (uint32_t)(FS * (3.0f + 9.0f * (float)(rng >> 8) * (1.0f / 16777216.0f)) / (0.25f + wear));
        v->mph = 0.0001f;
        v->gp = 0.35f + 0.35f * (float)(rng & 255u) * (1.0f / 255.0f);   /* how deep */
    }
    dip = v->mph > 0.0f ? v->gp * fm_minf(1.0f, wear * 1.6f)
                          * (v->mph < 0.12f ? v->mph * (1.0f / 0.12f) : (1.0f - v->mph) * (1.0f / 0.88f)) : 0.0f;
    fc = fm_maxf(f * 2.0f, 320.0f * fm_exp2f(mass * 5.4f)) * (1.0f - 0.7f * dip);
    c = 1.0f - fm_expf(-6.2831853f * (fc > 16000.0f ? 16000.0f : fc) / FS);
    k = TAPE_GAIN * (1.0f - dip);
    for (i = 0; i < m; i++) {
        float x, sq, s, p, n;
        ph += inc;
        if (ph >= 1.0f)
            ph -= 1.0f;
        p = ph * (float)SIN_N;
        x = 2.0f * ph - 1.0f;
        sq = x * x;
        s = -saw * (sq - z0) * c0 * 0.8f + organ * (sintab[(uint32_t)p & SIN_MASK] + 0.33f * sintab[(uint32_t)(p * 2.0f) & SIN_MASK]
                                                 + 0.16f * sintab[(uint32_t)(p * 3.0f) & SIN_MASK]);
        z0 = sq;
        s = softclip(s * 1.3f);          /* the tape's warmth */
        lp1 += (s - lp1) * c;
        lp2 += (lp1 - lp2) * c;
        rng = rng * 1664525u + 1013904223u;
        n = (float)(int32_t)rng * (1.0f / 2147483648.0f);
        hiss += (n - hiss) * 0.45f;
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        s = lp2 * k + (n - hiss) * air + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    v->ph = ph;
    v->z[0] = z0;
    v->lp1 = fm_flush(lp1);
    v->lp2 = fm_flush(lp2);
    v->fs[0] = fm_flush(hiss);
    v->sub_ph = sph;
    v->rng = rng;
    v->lvl_s = lvl;
}

/* RESONATOR: noise played into five resonances tuned over the note, like a bowl or a glass kept singing: SHAPE the
 * material (PURE: harmonic; GLASS; BOWL; BRONZE: a gong's crowded modes), MASS how bright the upper ones ring, DRIFT
 * a slow shimmer in their tuning and level, AIR the scrape (raw noise, and the resonances wider), SUB the sine. Each
 * resonance a coupled-form pair (a rotation a sample), which stays in tune low down in single precision; fed in at
 * the square root of its width, so a narrow one is as loud as a wide one */
#define RES_GAIN 1.50f                   /* as loud as SOLAR at F1's settings (measured) */
#define RES_MODES 5u
static const float RES_RATIO[4][RES_MODES] = {
    {1.0f, 2.0f, 3.0f, 4.0f, 5.0f},                  /* PURE */
    {1.0f, 2.32f, 4.25f, 6.63f, 9.38f},              /* GLASS: a wine glass's rim */
    {1.0f, 2.71f, 5.15f, 8.43f, 12.5f},              /* BOWL: a singing bowl */
    {1.0f, 1.59f, 2.14f, 2.30f, 2.65f},              /* BRONZE: a struck plate's modes, close together */
};
const char *const DR_RES_NAME[4] = {"PURE", "GLASS", "BOWL", "BRONZE"};
static void render_reson(voice_t *v, const blk_t *b, float f, float lvl, uint32_t m, float *ml, float *mr)
{
    float mass = vmass(v, b), pos = b->shape * 2.999f, fr = pos - (float)(uint32_t)pos, tilt = 2.4f - 2.2f * mass;
    float cr[RES_MODES], sr[RES_MODES], gin[RES_MODES], ga[RES_MODES], y1[RES_MODES], y2[RES_MODES];
    float *st[2 * RES_MODES] = {&v->fs[0], &v->fs[1], &v->fs[2], &v->fs[3], &v->fs[4], &v->fs[5], &v->ph3[0], &v->ph3[1],
                                &v->ph3[2], &v->z[0]};
    float sph = v->sub_ph, sinc = f / FS * 0.5f, sub = b->sub * 0.5f, scrape = b->air * 0.12f, ex = v->lp1,
          l = v->lvl_s, dl = (lvl - l) / (float)m;
    float gl = tsin(0.25f - (v->pan_live * b->width + 1.0f) * 0.125f), gr = tsin((v->pan_live * b->width + 1.0f) * 0.125f);
    uint32_t i, j, t0 = (uint32_t)pos, rng = v->rng;
    for (j = 0; j < RES_MODES; j++) {
        float ratio = RES_RATIO[t0][j] + (RES_RATIO[t0 + 1u][j] - RES_RATIO[t0][j]) * fr, fj, bw, rad, w;
        v->hb[j] += b->hinc * (0.5f + 0.21f * (float)j);
        if (v->hb[j] >= 1.0f)
            v->hb[j] -= 1.0f;
        fj = f * ratio * (1.0f + b->drift * 0.0016f * tsin(v->hb[j] + 0.25f));   /* up to 3 cents of shimmer */
        bw = (1.6f + 14.0f * b->air) * (1.0f + 0.6f * (float)j);                 /* Hz */
        rad = fm_expf(-3.14159265f * bw / FS);
        w = 6.2831853f * fj / FS;
        cr[j] = rad * fm_cosf(w);
        sr[j] = rad * fm_sinf(w);
        gin[j] = fm_sqrtf(1.0f - rad);
        ga[j] = fj < 16000.0f ? fm_exp2f(-tilt * LOG2H[j]) * (1.0f - b->drift * 0.3f * (0.5f + 0.5f * tsin(v->hb[j])))
                              : 0.0f;
        y1[j] = *st[2u * j];
        y2[j] = *st[2u * j + 1u];
    }
    {                                    /* the modes' power kept whatever MASS leans them to */
        float p = 0.0f;
        for (j = 0; j < RES_MODES; j++)
            p += ga[j] * ga[j];
        p = p > 1e-6f ? fm_rsqrtf(p) : 0.0f;
        for (j = 0; j < RES_MODES; j++)
            ga[j] *= p;
    }
    for (i = 0; i < m; i++) {
        float n, s = 0.0f;
        rng = rng * 1664525u + 1013904223u;
        n = (float)(int32_t)rng * (1.0f / 2147483648.0f);
        ex += (n - ex) * (0.25f + 0.6f * mass);          /* the scrape, darker at low MASS */
        for (j = 0; j < RES_MODES; j++) {
            float a = y1[j], c2 = y2[j];
            y1[j] = cr[j] * a - sr[j] * c2 + n * gin[j];
            y2[j] = sr[j] * a + cr[j] * c2;
            s += y2[j] * ga[j];
        }
        sph += sinc;
        if (sph >= 1.0f)
            sph -= 1.0f;
        s = s * RES_GAIN + ex * scrape + sub * sintab[(uint32_t)(sph * (float)SIN_N) & SIN_MASK];
        l += dl;
        s *= l;
        ml[i] += s * gl;
        mr[i] += s * gr;
    }
    for (j = 0; j < RES_MODES; j++) {
        *st[2u * j] = fm_flush(y1[j]);
        *st[2u * j + 1u] = fm_flush(y2[j]);
    }
    v->lp1 = fm_flush(ex);
    v->sub_ph = sph;
    v->rng = rng;
    v->lvl_s = lvl;
}

static void render_voices(uint32_t lo, uint32_t hi, uint32_t n, float *ml, float *mr)   /* added to ml, mr */
{
    uint32_t i, k;
    for (i = 0; i < n; i += CTL) {
        uint32_t m = n - i < CTL ? n - i : CTL;
        for (k = lo; k < hi; k++) {
            voice_t *v = &vc[k];
            float f, lvl;
            if (!v->on)
                continue;
            voice_tick(v, &g_blk, &f, &lvl);
            if (!v->on)
                continue;
            switch (v->model) {
            case M_SOLAR: render_solar(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_VOID: render_void(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_GLASS: render_glass(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_CHOIR: render_choir(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_BELL: render_bell(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_SHEPARD: render_shepard(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_FOLD: render_fold(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_SCAN: render_scan(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_DX: render_dx(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_TEAR: render_tear(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_TAPE: render_tape(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            case M_RESON: render_reson(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            default: render_string(v, &g_blk, f, lvl, m, ml + i, mr + i); break;
            }
        }
    }
}

static volatile uint32_t job_n;
#define NV_CORE2 2u                      /* the second core's bodies: two beside the hall, so the two cores take about
                                          * the same time (the half's CPU figure is the longer core's, wait included) */
static void zero_mix(float *ml, float *mr, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        ml[i] = mr[i] = 0.0f;
}
#if FM1B_CPU2
static void half_job(uint32_t arg)       /* the second core: the hall on the last block's bus, two bodies, the leads */
{
    (void)arg;
    fx_hall(hall_in_l, hall_in_r, hall_out_l, hall_out_r, hall_n, &g_fxp);
    zero_mix(mixl2, mixr2, job_n);
    render_voices(0, NV_CORE2, job_n, mixl2, mixr2);
    render_voices(NV, NV_ALL, job_n, mixl2, mixr2);
}
#endif

/* ------------------------------------------------------------------ ORBIT --- */
/* PLAY. Four modes (P_ORB_MODE). WANDER: every EVERY seconds (give or take a quarter), with CHANCE, one latched
 * body moves up to STEPS scale degrees, never onto a note another body holds. CHORD: every latched body moves
 * together by the same number of degrees (up to DISTANCE), so the chord shifts along the scale to a related one.
 * SCATTER: BODIES of them move at once, each its own way, the timing looser (give or take a half). HOME PULL sends
 * a body (in CHORD, the chord) back to where it was struck. The moves are chosen when the countdown starts, so
 * the screen can show where each body is going, and made when it ends. ARP: a runner at RATE over the latched
 * notes (and OCTAVES up), each step a lead held for GATE of the step; the notes are remembered, so it goes on
 * after the bed is let go, until PLAY stops it. */

uint32_t dr_orbit_mode(void) { return dr_scene.p[P_ORB_MODE] < OM_COUNT ? dr_scene.p[P_ORB_MODE] : 0u; }
uint32_t dr_orbit_cycle_pos(void) { return cyc_pos; }
float dr_arp_bpm(uint32_t p) { return 40.0f + (float)(p & 127u) * (200.0f / 127.0f); }   /* 40 .. 240 */
int dr_orbit_pending(uint32_t i, uint32_t *to)
{
    if (i >= NV || !pend_to[i] || !orbit_on)
        return 0;
    *to = pend_to[i];
    return 1;
}
int32_t dr_arp_note(void) { return orbit_on && dr_orbit_mode() == OM_ARP ? arp_held : -1; }
int dr_arp_running(void) { return orbit_on && dr_orbit_mode() == OM_ARP && arp_n > 0u; }
uint32_t dr_arp_steps(void) { return arp_hits; }

static int note_held_by_other(const voice_t *v, int32_t note)   /* another body sits (or is going) there */
{
    uint32_t i;
    for (i = 0; i < NV; i++)
        if (&vc[i] != v && vc[i].on && !vc[i].fading
            && ((int32_t)(vc[i].target + 0.5f) == note || (int32_t)pend_to[i] == note))
            return 1;
    return 0;
}
static int32_t orbit_step_from(const voice_t *v, int32_t d)   /* d degrees from where the body is, within an
                                                                * octave of home and the keys' range */
{
    int32_t cur = (int32_t)(v->target + 0.5f), nxt = scale_step((int32_t)dr_snap((uint32_t)cur), d);
    if (nxt > (int32_t)v->home + 12 || nxt < (int32_t)v->home - 12)
        nxt = (int32_t)v->home;
    if (nxt < 12)
        nxt = 12;
    if (nxt > 108)
        nxt = 108;
    return nxt;
}
static void orbit_plan(void)             /* the countdown starts: choose the moves */
{
    uint32_t i, cnt = 0, mode = dr_orbit_mode(), chance = dr_scene.p[P_ORB_CHANCE] * 100u / 127u;
    uint32_t grav = dr_scene.p[P_GRAVITY] * 100u / 127u, range = 1u + dr_scene.p[P_ORB_RANGE] * 4u / 128u;
    voice_t *cand[NV];
    for (i = 0; i < NV; i++)
        pend_to[i] = 0;
    orbit_rng = orbit_rng * 1664525u + 1013904223u;
    if ((orbit_rng >> 8) % 100u >= chance)
        return;
    for (i = 0; i < NV; i++)             /* a pinned body is a pedal: never moved, never dipped */
        if (vc[i].on && !vc[i].fading && vc[i].latched && !vc[i].restart && !vc[i].pinned)
            cand[cnt++] = &vc[i];
    if (!cnt)
        return;
    if (mode == OM_PULSE) {              /* a roll of dips across the chord, low to high, ROLL apart */
        uint32_t j, k;
        float roll = p01(P_ORB_ROLL) * (float)orbit_per * 0.6f / (float)(cnt > 1u ? cnt - 1u : 1u);
        for (j = 0; j < cnt; j++)        /* sorted low to high */
            for (k = j + 1u; k < cnt; k++)
                if (cand[k]->pitch < cand[j]->pitch) {
                    voice_t *t = cand[j];
                    cand[j] = cand[k];
                    cand[k] = t;
                }
        for (j = 0; j < cnt; j++)
            cand[j]->pulse_left = 1u + (uint32_t)(roll * (float)j);
        return;
    }
    if (mode == OM_ECHO) {               /* the phrase, over the period */
        uint32_t j, n = echo_n, jump = dr_scene.p[P_ECHO_JUMP] * 100u / 127u, decay = dr_scene.p[P_ECHO_DECAY] * 100u / 127u;
        if (!n || echo_vel < 0.08f)
            return;
        for (j = 0; j < n; j++) {
            uint32_t note = echo_ring[(echo_w + 8u - n + j) % 8u], at = 1u + (uint32_t)((float)orbit_per * (float)j / (float)(n + 1u));
            orbit_rng = orbit_rng * 1664525u + 1013904223u;
            if ((orbit_rng >> 8) % 100u < decay / 2u)   /* dropped */
                continue;
            orbit_rng = orbit_rng * 1664525u + 1013904223u;
            if ((orbit_rng >> 8) % 100u < jump)
                note = (orbit_rng & 0x100u) && note >= 24u ? note - 12u : note + 12u;
            if (note > 108u)
                note -= 12u;
            echo_due[j] = at;
            echo_note[j] = (uint8_t)note;
            echo_off[j] = 0;
        }
        echo_vel *= 1.0f - 0.45f * (float)decay * 0.01f - 0.05f;
        return;
    }
    if (mode == OM_CYCLE) {              /* the chord to the progression's next degree, from where it was struck */
        uint32_t prog = dr_scene.p[P_ORB_PROG] < DR_PROGS ? dr_scene.p[P_ORB_PROG] : 0u, nxt_pos = (cyc_pos + 1u) % 4u;
        int32_t d = (int32_t)PROG[prog][nxt_pos];
        for (i = 0; i < cnt; i++) {
            int32_t nxt = scale_step((int32_t)dr_snap(cand[i]->home), d);
            if (nxt > 108)
                nxt -= 12;
            if (nxt < 12)
                nxt += 12;
            if (nxt != (int32_t)(cand[i]->target + 0.5f))
                pend_to[cand[i] - vc] = (uint8_t)nxt;
        }
        cyc_pos = nxt_pos;
        return;
    }
    if (mode == OM_CHORD) {              /* the chord along the scale, or home */
        int32_t d, away = 0;
        uint32_t dist = 1u + dr_scene.p[P_ORB_DIST] * 3u / 128u;   /* 1 .. 3 degrees */
        for (i = 0; i < cnt; i++)
            away |= (int32_t)(cand[i]->target + 0.5f) != (int32_t)cand[i]->home;
        orbit_rng = orbit_rng * 1664525u + 1013904223u;
        if (away && (orbit_rng >> 9) % 100u < grav) {
            for (i = 0; i < cnt; i++)
                pend_to[cand[i] - vc] = cand[i]->home;
            return;
        }
        d = (int32_t)((orbit_rng >> 16) % dist) + 1;
        if (orbit_rng & 0x100u)
            d = -d;
        for (i = 0; i < cnt; i++) {
            int32_t nxt = orbit_step_from(cand[i], d);
            if (nxt != (int32_t)(cand[i]->target + 0.5f))
                pend_to[cand[i] - vc] = (uint8_t)nxt;
        }
        return;
    }
    {                                    /* WANDER: one body; SCATTER: BODIES of them, each its own way */
        uint32_t want = mode == OM_SCATTER ? 1u + dr_scene.p[P_ORB_BODIES] * (cnt - 1u) / 127u : 1u, n;
        for (n = 0; n < want && cnt; n++) {
            uint32_t pick;
            voice_t *v;
            int32_t cur, nxt, d;
            orbit_rng = orbit_rng * 1664525u + 1013904223u;
            pick = (orbit_rng >> 10) % cnt;
            v = cand[pick];
            cand[pick] = cand[--cnt];    /* each body once */
            cur = (int32_t)(v->target + 0.5f);
            orbit_rng = orbit_rng * 1664525u + 1013904223u;
            if ((orbit_rng >> 9) % 100u < grav && cur != (int32_t)v->home && !note_held_by_other(v, (int32_t)v->home)) {
                nxt = (int32_t)v->home;
            } else {
                d = (int32_t)((orbit_rng >> 16) % range) + 1;
                if (orbit_rng & 0x100u)
                    d = -d;
                nxt = orbit_step_from(v, d);
                if (note_held_by_other(v, nxt))   /* taken: the other way, else stay */
                    nxt = orbit_step_from(v, -d);
                if (note_held_by_other(v, nxt))
                    nxt = cur;
            }
            if (nxt != cur)
                pend_to[v - vc] = (uint8_t)nxt;
        }
    }
}
static void orbit_apply(void)            /* the countdown ends: the planned moves, if the bodies are still there */
{
    uint32_t i;
    for (i = 0; i < NV; i++) {
        voice_t *v = &vc[i];
        int32_t cur, nxt = (int32_t)pend_to[i];
        if (!nxt)
            continue;
        pend_to[i] = 0;
        if (!v->on || v->fading || !v->latched || v->restart || v->pinned)
            continue;
        cur = (int32_t)(v->target + 0.5f);
        if (nxt == cur)
            continue;
        if (note_out) {
            note_out((uint32_t)cur, 0, 0);
            note_out((uint32_t)nxt, (uint32_t)(v->vel * 127.0f), 1);
        }
        v->target = (float)nxt;
        orbit_from = (uint32_t)cur;
        orbit_to = (uint32_t)nxt;
        orbit_moves++;
    }
}
static void arp_gather(uint32_t played)  /* the latched notes as struck (none: the last remembered), low to high unless
                                          * PLAYED, then OCTAVES up */
{
    uint32_t i, j, n = 0, oct = 1u + (dr_scene.p[P_ARP_OCT] < 3u ? dr_scene.p[P_ARP_OCT] : 2u), notes[NV], age[NV];
    int8_t tune[NV];
    for (i = 0; i < NV; i++)             /* a pinned body is the pedal under the runner, not one of its notes */
        if (vc[i].on && !vc[i].fading && vc[i].latched && !vc[i].restart && !vc[i].pinned) {
            for (j = n++; j && arp_mem_age[j - 1u] > vc[i].age; j--) {
                arp_mem_age[j] = arp_mem_age[j - 1u];
                arp_mem[j] = arp_mem[j - 1u];
                arp_mem_tune[j] = arp_mem_tune[j - 1u];
            }
            arp_mem_age[j] = vc[i].age;
            arp_mem[j] = (uint32_t)(vc[i].target + 0.5f);
            arp_mem_tune[j] = vc[i].ed_tune;
        }
    if (n)
        arp_mem_n = n;
    n = arp_mem_n;
    for (i = 0; i < n; i++) {            /* low to high unless PLAYED: insertion */
        for (j = i; j && !played && notes[j - 1u] > arp_mem[i]; j--) {
            notes[j] = notes[j - 1u];
            age[j] = age[j - 1u];
            tune[j] = tune[j - 1u];
        }
        notes[j] = arp_mem[i];
        age[j] = arp_mem_age[i];
        tune[j] = arp_mem_tune[i];
    }
    arp_n = 0;
    for (j = 0; j < oct; j++)
        for (i = 0; i < n; i++)
            if (notes[i] + 12u * j <= 108u) {
                arp_age[arp_n] = age[i];
                arp_tune[arp_n] = tune[i];
                arp_notes[arp_n++] = notes[i] + 12u * j;
            }
}
static void arp_release(void)
{
    if (arp_held >= 0) {
        lead_event((uint32_t)arp_held, 0, 0, 0);
        arp_held = -1;
    }
}
static void arp_tick(uint32_t n)
{
    uint32_t gate = dr_scene.p[P_ARP_GATE], order = dr_scene.p[P_ARP_ORDER] < AO_COUNT ? dr_scene.p[P_ARP_ORDER] : 0u;
    if (arp_gate_left) {
        if (arp_gate_left > n)
            arp_gate_left -= n;
        else {
            arp_gate_left = 0;
            arp_release();
        }
    }
    if (clk_ext) {                       /* following: the clock says when (dr_clock), the step is DIV in clocks */
        if (!arp_due)
            return;
        arp_due = 0;
        arp_step = clk_period * arp_div_clk();
    } else {
        if (arp_left > n) {
            arp_left -= n;
            return;
        }
        arp_step = (uint32_t)((float)arp_div_clk() * (2.5f / dr_arp_bpm(dr_scene.p[P_ARP_RATE])) * FS);   /* DIV at
                                                                                                        * RATE */
        arp_left = arp_left + arp_step > n ? arp_left + arp_step - n : 1u;   /* from where this step fell due, this
                                          * block's samples counted too, so the runner keeps RATE (and its own CLOCK OUT)
                                          * over time instead of a block long each step */
    }
    {                                    /* RHYTHM: this step a hit or a rest */
        uint32_t r = arp_rhythm(), len = RHY[r][1], at = arp_count % len;
        arp_count++;
        arp_release();
        if ((at * RHY[r][0]) % len >= RHY[r][0])
            return;
    }
    arp_hits++;
    {
        uint32_t k, l_age = arp_pos < arp_n ? arp_age[arp_pos] : 0u, l_note = arp_pos < arp_n ? arp_notes[arp_pos] : 0u;
        arp_gather(order == AO_PLAYED);
        for (k = 0; k < arp_n; k++)      /* a body added or let go: on from the step just played, where it now stands */
            if (arp_age[k] == l_age && arp_notes[k] == l_note)
                arp_pos = k;
    }
    if (!arp_n)
        return;
    {
        uint32_t note;
        switch (order) {
        case AO_DOWN:
            arp_pos = arp_pos && arp_pos <= arp_n ? arp_pos - 1u : arp_n - 1u;
            break;
        case AO_UPDOWN:
            if (arp_n < 2u)
                arp_pos = 0;
            else if (arp_dir) {
                if (arp_pos == 0u) {
                    arp_dir = 0;
                    arp_pos = 1;
                } else
                    arp_pos--;
            } else {
                if (arp_pos + 1u >= arp_n) {
                    arp_dir = 1;
                    arp_pos = arp_n - 2u;
                } else
                    arp_pos++;
            }
            break;
        case AO_RANDOM:
            orbit_rng = orbit_rng * 1664525u + 1013904223u;
            arp_pos = (orbit_rng >> 10) % arp_n;
            break;
        default:                         /* UP, and PLAYED (arp_notes in the order struck) */
            arp_pos = (arp_pos + 1u) % arp_n;
            break;
        }
        if (arp_pos >= arp_n)
            arp_pos = 0;
        note = arp_notes[arp_pos];
        if (note_out)
            note_out(note, 100, 1);
        lead_machine = 1;
        lead_event(note, 100, 1, 0);
        lead_machine = 0;
        {
            voice_t *ld = find_lead(note);
            if (ld)
                ld->ed_tune = arp_tune[arp_pos];
        }
        arp_held = (int32_t)note;
        /* GATE: the lead let go that far through the step; at the top, legato, at the next step */
        arp_gate_left = gate >= 125u ? 0u : (uint32_t)((float)arp_step * (0.05f + 0.95f * (float)gate / 127.0f));
    }
}
static void orbit_reset(void)            /* PLAY: a fresh start, the arp's memory dropped, its lead let go */
{
    uint32_t i;
    for (i = 0; i < NV; i++)
        pend_to[i] = 0;
    arp_n = arp_mem_n = arp_dir = arp_left = arp_step = arp_count = arp_gate_left = arp_due = arp_hits = 0;
    arp_pos = ~0u;                       /* before the first step: it lands on the first note (DOWN: the top) */
    cyc_pos = 0;
    for (i = 0; i < 8u; i++)
        echo_due[i] = echo_off[i] = 0;
    echo_vel = 1.0f;
    orbit_clk_left = orbit_clk_per = 0;
    if (arp_held >= 0)
        arp_release();
}
static float orbit_period_s(void)        /* EVERY; ECHO's phrase needs a second at least */
{
    float s = dr_orbit_s(dr_scene.p[P_ORB_PERIOD]);
    return dr_orbit_mode() == OM_ECHO && s < 1.0f ? 1.0f : s;
}
uint32_t dr_orbit_clocks(uint32_t p)    /* EVERY (p) in clocks at the clock's tempo: whole beats, below one a half or a
                                          * quarter of one */
{
    float bpm = dr_clock_bpm(), s = dr_orbit_s(p), b;
    if (dr_orbit_mode() == OM_ECHO && s < 1.0f)
        s = 1.0f;
    b = s * (bpm > 0.0f ? bpm : 120.0f) / 60.0f;
    if (dr_orbit_mode() != OM_ECHO) {
        if (b < 0.375f)
            return 6u;
        if (b < 0.75f)
            return 12u;
    }
    return (b < 1.5f ? 1u : (uint32_t)(b + 0.5f)) * 24u;
}
static uint32_t orbit_clocks(void) { return dr_orbit_clocks(dr_scene.p[P_ORB_PERIOD]); }
void dr_clock(int port)
{
    uint32_t mode = dr_orbit_mode();
    if (clk_ext && clk_src >= 0 && port != clk_src)   /* the same clock on a second input */
        return;
    clk_src = port;
    clk_hist[clk_hw] = clk_now;          /* the tempo: the time over the last 24 clocks (fewer at the start) */
    clk_hw = (clk_hw + 1u) % 25u;
    if (clk_hn < 25u)
        clk_hn++;
    if (clk_hn >= 2u) {
        uint32_t first = clk_hn == 25u ? clk_hw : (clk_hw + 25u - clk_hn) % 25u, span = clk_now - clk_hist[first];
        if (span >= clk_hn - 1u)
            clk_period = (span + (clk_hn - 1u) / 2u) / (clk_hn - 1u);
    }
    clk_last = clk_now;
    clk_ext = 1;
    if (orbit_on && mode == OM_ARP) {
        if (clk_count % arp_div_clk() == 0u)
            arp_due = 1;
    } else if (orbit_on) {
        if (orbit_clk_left)
            orbit_clk_left--;
        if (!orbit_clk_left) {
            orbit_apply();
            orbit_clk_left = orbit_clk_per = orbit_clocks();
            orbit_per = orbit_clk_per * (clk_period ? clk_period : (uint32_t)(FS / 48.0f));   /* the period in samples
                                                                                           * too: ECHO's phrase and
                                                                                           * PULSE's roll spread over it */
            orbit_plan();
        }
    }
    clk_count++;
}
void dr_clock_start(int port)
{
    if (clk_ext && clk_src >= 0 && port != clk_src)
        return;
    clk_count = 0;
    arp_pos = ~0u;
    arp_dir = 0;
    orbit_clk_left = 0;
    clk_ext = 1;
    clk_last = clk_now;
}
void dr_clock_stop(void) { arp_release(); }
int dr_clock_ext(void) { return (int)clk_ext; }
float dr_clock_bpm(void) { return clk_ext && clk_period ? 60.0f * FS / ((float)clk_period * 24.0f) : 0.0f; }
void dr_clock_out(int on) { clk_out = on != 0; }
void dr_realtime_out(void (*fn)(uint32_t)) { rt_out = fn; }
static void echo_tick(uint32_t n)        /* the scheduled replays: strike, then let go half a step later */
{
    uint32_t j;
    for (j = 0; j < 8u; j++) {
        if (echo_due[j]) {
            if (echo_due[j] > n)
                echo_due[j] -= n;
            else {
                uint32_t vel = (uint32_t)(echo_vel * 100.0f);
                echo_due[j] = 0;
                if (note_out)
                    note_out(echo_note[j], vel, 1);
                lead_machine = 1;
                lead_event(echo_note[j], vel, 1, 0);
                lead_machine = 0;
                echo_off[j] = 1u + orbit_per / (2u * (echo_n ? echo_n + 1u : 2u));
            }
        } else if (echo_off[j]) {
            if (echo_off[j] > n)
                echo_off[j] -= n;
            else {
                echo_off[j] = 0;
                lead_event(echo_note[j], 0, 0, 0);
            }
        }
    }
}
int dr_echo_running(void) { return orbit_on && dr_orbit_mode() == OM_ECHO && echo_n > 0u && echo_vel >= 0.08f; }
static void orbit_tick(uint32_t n)
{
    uint32_t mode;
    clk_now += n;
    if (clk_ext && clk_now - clk_last > (uint32_t)(2.0f * FS)) {   /* the clock stopped coming: our own time */
        clk_ext = 0;
        arp_due = 0;
        orbit_clk_left = orbit_clk_per = 0;
        clk_hn = clk_hw = 0;
        clk_src = -1;
    }
    if (!orbit_on)
        return;
    mode = dr_orbit_mode();
    if (mode == OM_ECHO)
        echo_tick(n);
    if (mode == OM_ARP) {
        if (clk_out && !clk_ext && rt_out) {   /* 24 clocks a beat from RATE */
            float per = 60.0f / dr_arp_bpm(dr_scene.p[P_ARP_RATE]) * FS * (1.0f / 24.0f);
            clk_acc += (float)n;
            while (clk_acc >= per) {
                clk_acc -= per;
                rt_out(0xF8u);
            }
        }
        arp_tick(n);
        return;
    }
    if (orbit_reper) {                   /* EVERY turned: a countdown longer than the new period is cut to it */
        orbit_reper = 0;
        if (clk_ext) {
            uint32_t c = orbit_clocks();
            if (orbit_clk_left > c)
                orbit_clk_left = orbit_clk_per = c;
        } else {
            uint32_t s = (uint32_t)(orbit_period_s() * FS);
            if (orbit_left > s)
                orbit_left = orbit_per = s;
        }
    }
    if (clk_ext)                         /* the clock counts the period (dr_clock) */
        return;
    if (orbit_left > n) {
        orbit_left -= n;
        return;
    }
    orbit_apply();
    {                                    /* the next countdown: the period, give or take a quarter (a half in SCATTER) */
        float j = mode == OM_SCATTER ? 0.5f : 0.25f;
        float per = orbit_period_s() * (1.0f + j * frand(&orbit_rng));
        if (mode == OM_ECHO && per < 1.0f)   /* the phrase needs its second, after the spread too */
            per = 1.0f;
        orbit_left = orbit_per = (uint32_t)(per * FS);
    }
    orbit_plan();
}

/* ------------------------------------------------------------------ the noise layer --- */
/* One stereo noise under the bodies, or alone. Two generators (or, as TEXTURE rises, bursts: rain, then crackle),
 * a colour tilt (brown to white), a high-pass, and a resonant band that wanders with WIND or, as BAND narrows,
 * sits on the newest body's note; past half, BAND is the chord's breath: up to four bands on the bodies sounding,
 * each as loud as its body. GUST is its weather: now and then (at WIND's pace) it swells louder, brighter and
 * higher over a second or three and dies back over several; FREEZE holds the gust where it is. DUCK draws it back
 * under a chord. MASS brightens it, pressure too, TIDE breathes it on its own phase, it swells and fades at SWELL
 * and FADE like a body, WIDTH is its own. */
#define NZ_BANDS 4u
static struct {
    uint32_t rng_l, rng_r;
    float env, tide_ph, press, wander, note, lp_l, lp_r, hp_l, hp_r, s1l, s2l, s1r, s2r, level, shown_note;
    float gust, gust_tgt, gust_ka, gust_kd, duck;   /* the weather: a gust's envelope, its peak, its rates */
    uint32_t gust_wait;                  /* samples to the next gust */
    float burst_l, burst_r;              /* TEXTURE: the bursts under way */
    float cs1[NZ_BANDS], cs2[NZ_BANDS];  /* the chord bands' states */
} nz = {0x1234567u, 0x89ABCDEu, 0.0f, 0.3f, 0.0f, 0.0f, 60.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 60.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0u, 0.0f, 0.0f, {0, 0, 0, 0}, {0, 0, 0, 0}};
static float nz_gust_amt;                /* what GUST does to the level now, 0..1, for the screen */
static uint32_t nz_gusts;

static void noise_render(const blk_t *b, uint32_t n, float *l, float *r)
{
    float target = p01(P_NZ_LEVEL), colour = p01(P_NZ_COLOUR), wind = p01(P_NZ_WIND), band = p01(P_NZ_BAND);
    float tex = p01(P_NZ_TEXTURE), gust = p01(P_NZ_GUST), duck = p01(P_NZ_DUCK), width = p01(P_NZ_WIDTH);
    float tide, prs = 0.0f, want, f, g, k, a1, a2, a3, c, amp, bp_g, band1, cb, hpk, hiss, imp_p, imp_g, bdec;
    float ca1[NZ_BANDS], ca2[NZ_BANDS], ca3[NZ_BANDS], cg[NZ_BANDS], bl = 0.0f;
    uint32_t i, j, cn = 0;
    target *= fm_sqrtf(target);          /* LEVEL as heard: a bed at 30 sits 19 dB under full (measured) */
    if (nz.env == 0.0f && target > 0.0f) {   /* the layer starts: with a gust, the count from here */
        nz.gust_wait = 0;
        nz_gusts = 0;
    }
    if (target > nz.env)                 /* a swell at SWELL, a fade at FADE, like a body */
        nz.env += (target - nz.env) * (1.0f - fm_expf(-(float)n / (dr_time_s((uint32_t)(p01(P_SWELL) * 127.0f)) * FS) * 3.0f));
    else
        nz.env += (target - nz.env) * (1.0f - fm_expf(-(float)n / (dr_time_s((uint32_t)(p01(P_FADE) * 127.0f)) * FS) * 3.0f));
    if (nz.env < 0.0005f && target == 0.0f) {
        nz.env = 0.0f;
        nz.level = 0.0f;
        return;
    }
    nz.tide_ph += b->tide_inc * ((float)n / (float)CTL) * 0.93f;   /* its own breath, a little slower than the bodies' */
    if (nz.tide_ph >= 1.0f)
        nz.tide_ph -= 1.0f;
    tide = 1.0f - b->tide_depth * (0.5f + 0.5f * tsin(nz.tide_ph));
    for (i = 0; i < NV; i++)
        if (vc[i].on && !vc[i].fading) {
            float lv = vc[i].env * vc[i].vel;
            if (vc[i].press > prs)
                prs = vc[i].press;
            if (lv > bl)
                bl = lv;
        }
    nz.press += (prs - nz.press) * 0.1f;
    nz.duck += (bl - nz.duck) * 0.05f;   /* under a chord it draws back, up to 12 dB */
    if (!frozen) {                       /* the weather: a gust every so often, WIND setting the pace */
        uint32_t longest = (uint32_t)(FS * 36.0f * fm_exp2f(-2.6f * wind));   /* WIND turned up shortens a wait */
        if (nz.gust_wait > longest)
            nz.gust_wait = longest;
        if (nz.gust_wait > n) {
            nz.gust_wait -= n;
        } else {
            float att, dec;
            nz.gust_wait = (uint32_t)(FS * 36.0f * fm_exp2f(-2.6f * wind) * (0.6f + 0.4f * (0.5f + 0.5f * frand(&nz.rng_r))));
            nz_gusts++;
            nz.gust_tgt = nz.gust + 0.45f + 0.45f * (0.5f + 0.5f * frand(&nz.rng_r));   /* above where it is now */
            if (nz.gust_tgt > 1.0f)
                nz.gust_tgt = 1.0f;
            att = 0.3f + 2.7f * (0.5f + 0.5f * frand(&nz.rng_r));     /* 0.3 .. 3 s up */
            dec = 2.0f + 8.0f * (0.5f + 0.5f * frand(&nz.rng_r));     /* 2 .. 10 s down */
            nz.gust_ka = 1.0f - fm_expf(-(float)n / (att * FS) * 3.0f);
            nz.gust_kd = 1.0f - fm_expf(-(float)n / (dec * FS) * 3.0f);
        }
        if (nz.gust_tgt > 0.0f) {
            nz.gust += (nz.gust_tgt - nz.gust) * nz.gust_ka;
            if (nz.gust > nz.gust_tgt - 0.03f)
                nz.gust_tgt = 0.0f;
        } else
            nz.gust += (0.0f - nz.gust) * nz.gust_kd;
    }
    nz_gust_amt = gust * nz.gust;
    colour = colour + nz_gust_amt * 0.5f * (1.0f - colour);   /* a gust brightens */
    nz.wander += frand(&nz.rng_l) * (0.01f + 0.12f * wind * wind) * (0.3f + 0.7f * b->drift) - nz.wander * 0.004f;
    nz.wander = fm_clampf(nz.wander, -1.0f, 1.0f);
    want = 60.0f + nz.wander * 24.0f;    /* free: C4 give or take two octaves */
    band1 = band < 0.5f ? band * 2.0f : 1.0f;   /* the one band narrows to half; past that, the chord's bands */
    cb = band > 0.5f ? (band - 0.5f) * 2.0f : 0.0f;
    if (band1 > 0.3f && newest_note >= 0) {   /* narrowing: onto the newest body's note */
        voice_t *v = find_body((uint32_t)newest_note, 0);
        if (v)
            want = want + (v->pitch - want) * fm_clampf((band1 - 0.3f) * 2.0f, 0.0f, 1.0f);
    }
    nz.note += (want - nz.note) * 0.03f;
    nz.shown_note = nz.note;
    f = 440.0f * fm_exp2f((nz.note - 69.0f + (b->mass - 0.5f) * 24.0f + nz.press * b->press_amt * 12.0f + nz_gust_amt * 10.0f) * (1.0f / 12.0f));
    f = fm_clampf(f, 40.0f, 6000.0f);
    g = 3.14159265f * f / FS;            /* the band: a trapezoidal state-variable filter, its width from BAND */
    k = 2.0f * (1.0f - 0.95f * band1);
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
    bp_g = band1 * fm_sqrtf(k) * 1.6f;
    if (cb > 0.0f) {                     /* the chord's breath: the four loudest bodies, a narrow band each */
        float sum = 0.0f, kk = 0.35f - 0.29f * cb;   /* Q about 3 to 17 */
        uint32_t pick[NZ_BANDS];
        for (i = 0; i < NV; i++) {
            float lv;
            if (!vc[i].on || vc[i].fading || vc[i].lead)
                continue;
            lv = vc[i].env * vc[i].vel;
            if (cn < NZ_BANDS) {
                pick[cn++] = i;
            } else {                     /* the quietest of the four gives way */
                uint32_t q = 0;
                for (j = 1; j < NZ_BANDS; j++)
                    if (vc[pick[j]].env * vc[pick[j]].vel < vc[pick[q]].env * vc[pick[q]].vel)
                        q = j;
                if (lv > vc[pick[q]].env * vc[pick[q]].vel)
                    pick[q] = i;
            }
        }
        for (j = 0; j < cn; j++) {
            float fb = 440.0f * fm_exp2f((vc[pick[j]].pitch - 69.0f) * (1.0f / 12.0f)), gb;
            if (fb < 110.0f)
                fb *= 2.0f;              /* a low body: its octave, where the breath is heard */
            if (fb > 6000.0f)
                fb = 6000.0f;
            gb = 3.14159265f * fb / FS;
            ca1[j] = 1.0f / (1.0f + gb * (gb + kk));
            ca2[j] = gb * ca1[j];
            ca3[j] = gb * ca2[j];
            cg[j] = vc[pick[j]].env * vc[pick[j]].vel;
            sum += cg[j];
        }
        for (j = 0; j < cn; j++)
            cg[j] = sum > 0.001f ? cg[j] / sum * fm_sqrtf(kk) * 3.2f : 0.0f;   /* the breath at the bed's RMS (measured) */
    }
    c = 0.01f + 0.99f * colour * colour * colour;   /* the colour: brown at 0, white at full */
    hpk = 0.0035f + 0.0022f * fm_clampf(colour * 4.0f, 0.0f, 1.0f);   /* the high-pass: 25 Hz for brown (rumble), 40 above */
    {                                    /* brown is quieter by nature and to the ear: lifted so every colour is
                                          * heard about alike (measured: within 4 dB above 400 Hz across COLOUR) */
        static const float LIFT_DB[5] = {20.0f, 18.0f, 10.0f, 4.0f, 0.0f};
        float cp = colour * 4.0f;
        uint32_t ci = cp >= 4.0f ? 3u : (uint32_t)cp;
        float lift = LIFT_DB[ci] + (LIFT_DB[ci + 1u] - LIFT_DB[ci]) * (cp - (float)ci);
        amp = nz.env * tide * (1.0f + 0.5f * nz.press) * 2.0f * fm_exp2f(lift * (1.0f / 6.02f));   /* Kyle: full was too loud, 30 % off */
    }
    amp *= (1.0f - gust) + gust * (0.12f + 0.88f * nz.gust);   /* GUST: the bed drops between gusts, up to 18 dB */
    amp *= (1.0f - duck * 0.75f * nz.duck) * BED_GAIN;
    nz.level = nz.env * tide * ((1.0f - gust) + gust * (0.12f + 0.88f * nz.gust)) * (1.0f - duck * 0.75f * nz.duck);
    /* TEXTURE: hiss to a quarter; then bursts, dense as rain, sparse as crackle; each burst a short decaying
     * splash of noise; the bursts' gain keeps the RMS near the hiss's (measured, tools/noise_levels.py) */
    hiss = tex < 0.25f ? 1.0f : tex < 0.5f ? (0.5f - tex) * 4.0f : 0.0f;
    imp_p = tex < 0.25f ? 0.0f : 3000.0f * fm_exp2f(-7.6f * (tex - 0.25f) * (1.0f / 0.75f)) / FS;   /* 3000 .. 15 a second */
    bdec = 0.82f + 0.15f * tex;          /* a burst's decay: 5 samples to 40 */
    imp_g = imp_p > 0.0f ? 2.7f * fm_sqrtf((1.0f - bdec * bdec) / (imp_p * 3.0f)) : 0.0f;   /* 2.7: rain and crackle
                                                                                            * at the hiss's RMS (measured) */
    if (imp_g > 100.0f)
        imp_g = 100.0f;
    for (i = 0; i < n; i++) {
        float xl, xr, v1, v2, v3, yl, yr, m;
        if (hiss >= 1.0f) {
            xl = frand(&nz.rng_l);
            xr = frand(&nz.rng_r);
        } else {
            float nl = frand(&nz.rng_l), nr = frand(&nz.rng_r);
            nz.rng_l = nz.rng_l * 1664525u + 1013904223u;
            if ((float)(nz.rng_l >> 8) * (1.0f / 16777216.0f) < imp_p)
                nz.burst_l = 0.6f + 0.4f * nr;
            nz.rng_r = nz.rng_r * 1664525u + 1013904223u;
            if ((float)(nz.rng_r >> 8) * (1.0f / 16777216.0f) < imp_p)
                nz.burst_r = 0.6f + 0.4f * nl;
            xl = nl * hiss + nz.burst_l * nl * imp_g * (1.0f - hiss);
            xr = nr * hiss + nz.burst_r * nr * imp_g * (1.0f - hiss);
            nz.burst_l *= bdec;
            nz.burst_r *= bdec;
        }
        nz.lp_l += (xl - nz.lp_l) * c;
        nz.lp_r += (xr - nz.lp_r) * c;
        nz.hp_l += (nz.lp_l - nz.hp_l) * hpk;
        nz.hp_r += (nz.lp_r - nz.hp_r) * hpk;
        xl = nz.lp_l - nz.hp_l;
        xr = nz.lp_r - nz.hp_r;
        v3 = xl - nz.s2l;
        v1 = a1 * nz.s1l + a2 * v3;
        v2 = nz.s2l + a2 * nz.s1l + a3 * v3;
        nz.s1l = 2.0f * v1 - nz.s1l;
        nz.s2l = 2.0f * v2 - nz.s2l;
        yl = xl * (1.0f - band1) + v1 * bp_g;
        v3 = xr - nz.s2r;
        v1 = a1 * nz.s1r + a2 * v3;
        v2 = nz.s2r + a2 * nz.s1r + a3 * v3;
        nz.s1r = 2.0f * v1 - nz.s1r;
        nz.s2r = 2.0f * v2 - nz.s2r;
        yr = xr * (1.0f - band1) + v1 * bp_g;
        if (cn) {                        /* the chord's bands, on the mix of both, spread left and right in turn */
            float cl = 0.0f, cr = 0.0f;
            m = (xl + xr) * 0.5f;
            for (j = 0; j < cn; j++) {
                float w3 = m - nz.cs2[j], w1 = ca1[j] * nz.cs1[j] + ca2[j] * w3,
                      w2 = nz.cs2[j] + ca2[j] * nz.cs1[j] + ca3[j] * w3, o;
                nz.cs1[j] = 2.0f * w1 - nz.cs1[j];
                nz.cs2[j] = 2.0f * w2 - nz.cs2[j];
                o = w1 * cg[j];
                if (j & 1u) {
                    cl += o * (1.0f - 0.5f * width);
                    cr += o;
                } else {
                    cl += o;
                    cr += o * (1.0f - 0.5f * width);
                }
            }
            yl = yl * (1.0f - cb) + cl * cb;
            yr = yr * (1.0f - cb) + cr * cb;
        }
        m = (yl + yr) * 0.5f;
        l[i] += (m + (yl - m) * width) * amp;
        r[i] += (m + (yr - m) * width) * amp;
    }
    nz.s1l = fm_flush(nz.s1l);
    nz.s2l = fm_flush(nz.s2l);
    nz.s1r = fm_flush(nz.s1r);
    nz.s2r = fm_flush(nz.s2r);
    for (j = 0; j < NZ_BANDS; j++) {
        nz.cs1[j] = fm_flush(nz.cs1[j]);
        nz.cs2[j] = fm_flush(nz.cs2[j]);
    }
}
float dr_noise_gust(void) { return nz.gust; }
uint32_t dr_noise_gusts(void) { return nz_gusts; }
void dr_noise(float *level, float *note, float *colour, float *band)
{
    *level = nz.level;
    *note = nz.shown_note + (p01(P_MASS) - 0.5f) * 24.0f + nz_gust_amt * 10.0f;
    *colour = p01(P_NZ_COLOUR) + nz_gust_amt * 0.5f * (1.0f - p01(P_NZ_COLOUR));
    *band = p01(P_NZ_BAND);
}

/* ------------------------------------------------------------------ init --- */
#define DR_NEED (SIN_N * 4u + NV_ALL * LINE_LEN * 2u + MAXBLOCK * 8u * 4u + FX_BYTES + 64u)
typedef char dr_arena_check[DR_NEED <= DR_ARENA_BYTES ? 1 : -1];   /* else dr_init refuses and nothing sounds */
int dr_init(void *arena, uint32_t bytes)
{
    uint8_t *a = (uint8_t *)arena;
    uint32_t i, need = DR_NEED;
    for (i = 0; i < P_COUNT; i++)
        pcur[i] = pstart[i] = ptarget[i] = (float)dr_scene.p[i] * (1.0f / 127.0f);
    for (i = 0; i < NV_ALL; i++)
        vc[i].on = 0;
    if (bytes < need)
        return -1;
    sintab = (float *)a;
    a += SIN_N * 4u;
    for (i = 0; i < SIN_N; i++)
        sintab[i] = fm_sin_turns((float)i / (float)SIN_N);
    lines = (int16_t *)a;
    a += NV_ALL * LINE_LEN * 2u;
    for (i = 0; i < NV_ALL * LINE_LEN; i++)
        lines[i] = 0;
    mixl = (float *)a;
    a += MAXBLOCK * 4u;
    mixr = (float *)a;
    a += MAXBLOCK * 4u;
    mixl2 = (float *)a;
    a += MAXBLOCK * 4u;
    mixr2 = (float *)a;
    a += MAXBLOCK * 4u;
    hall_in_l = (float *)a;
    a += MAXBLOCK * 4u;
    hall_in_r = (float *)a;
    a += MAXBLOCK * 4u;
    hall_out_l = (float *)a;
    a += MAXBLOCK * 4u;
    hall_out_r = (float *)a;
    a += MAXBLOCK * 4u;
    for (i = 0; i < MAXBLOCK; i++)
        hall_in_l[i] = hall_in_r[i] = hall_out_l[i] = hall_out_r[i] = 0.0f;
    hall_n = 0;
    a = (uint8_t *)(((uintptr_t)a + 7u) & ~(uintptr_t)7u);
    fx_init(a);
    have_arena = 1;
    return 0;
}

/* ------------------------------------------------------------------ render --- */
void dr_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    blk_t *b = &g_blk;
    fx_params_t fx;
    uint32_t i;
    float gain = (float)master_q12 * (1.0f / 4096.0f) * 4350000.0f * volume, space;   /* the limiter's ceiling
                                          * at -6 dBFS, not -0.1: Kyle's USB recordings redlined in Live, and the
                                          * headphones were loud a third of the way up */
    int on_two = 0;
    if (n > MAXBLOCK)
        n = MAXBLOCK;
    if (load_pending)
        load_bodies();
    drain_queue();
    if (ovl_cool)
        ovl_cool--;
    else if (voices_max < NV && ++calm_blocks > 1200u) {   /* calm for ~7 s: a body back */
        voices_max++;
        calm_blocks = 0;
    }
    if (morph_pos < 1.0f) {              /* the morph: linear between the scene before and the one loaded */
        morph_pos += morph_step * (float)n / (float)CTL;
        if (morph_pos > 1.0f)
            morph_pos = 1.0f;
        for (i = 1; i < P_COUNT; i++)
            pcur[i] = pstart[i] + (ptarget[i] - pstart[i]) * morph_pos;
    }
    /* the block's settings, from the values heard */
    b->model = dr_scene.p[P_MODEL] < M_COUNT ? dr_scene.p[P_MODEL] : 0u;
    b->mass = p01(P_MASS);
    b->drift = p01(P_DRIFT);
    b->shape = p01(P_SHAPE);
    b->sub = p01(P_SUB);
    b->air = p01(P_AIR);
    b->swell_k = (float)CTL / (dr_time_s((uint32_t)(p01(P_SWELL) * 127.0f)) * FS);
    b->fade_m = fm_expf(-(float)CTL / (dr_time_s((uint32_t)(p01(P_FADE) * 127.0f)) * FS) * 5.0f);
    b->press_amt = p01(P_PRESS);
    b->glide_k = 1.0f - fm_expf(-(float)CTL / (glide_s(p01(P_GLIDE)) * FS) * 4.0f);
    b->tide_inc = 0.02f * fm_exp2f(p01(P_TIDE_RATE) * 4.64f) * ((float)CTL / FS);
    b->tide_depth = p01(P_TIDE);
    b->wob_depth = p01(P_WOBBLE);
    b->wob_inc = (0.1f + 6.0f * p01(P_WOBBLE) * p01(P_WOBBLE)) * ((float)CTL / FS) * (0.4f + 1.6f * p01(P_DRIFT));
    b->walk_depth = p01(P_WALK) * (0.2f + 1.6f * p01(P_DRIFT));
    b->width = p01(P_WIDTH);
    b->bend = bend;
    b->wheel = wheel;
    b->chan_press = chan_press;
    b->breath_inc = 0.07f * ((float)CTL / FS);
    b->hinc = (0.03f + 0.25f * p01(P_TIDE_RATE)) * ((float)CTL / FS);
    b->attack_k = (float)CTL / (dr_attack_s((uint32_t)(p01(P_LD_ATTACK) * 127.0f)) * FS);
    b->rel_m = fm_expf(-(float)CTL / (dr_release_s((uint32_t)(p01(P_LD_RELEASE) * 127.0f)) * FS) * 5.0f);
    b->lead_level = p01(P_LD_LEVEL) * fm_sqrtf(p01(P_LD_LEVEL)) * 2.0f;   /* 75: 2 dB over a body (the bed's trim); full: 9 dB over */
    b->lift = (float)(uint32_t)(p01(P_LD_LIFT) * 24.99f);                 /* whole semitones, up to two octaves */
    b->spread = p01(P_SPREAD);
    b->cross = p01(P_CROSS);
    b->twalk_depth = p01(P_TWALK);
    b->sway = p01(P_SWAY);
    b->sway_inc = b->sway > 0.0f ? (1.0f / 50.0f + b->sway * (1.0f / 4.0f)) * ((float)CTL / FS) : 0.0f;   /* a swing in 50 s to 3 s */
    b->n = n;
    handover(b->model, n);
    for (i = 0; i < NV_ALL; i++) {       /* a voice in another model than the scene's restarts, but a body fading away
                                          * fades in its own, and a body being handed over is the hand-over's */
        voice_t *v = &vc[i];
        if (!v->on || v->model == b->model || v->restart)
            continue;
        if (i < NV ? v->fading || v->xf_out || xf_on : !v->held)
            continue;
        v->restart = 1;
    }
    for (i = NV; i < NV_ALL; i++)        /* a held lead follows LIFT as it turns */
        if (vc[i].on && vc[i].held)
            vc[i].target = (float)vc[i].home + b->lift;
    orbit_tick(n);
    space = p01(P_SPACE);
    fx.ens_mode = dr_scene.p[P_ENS] < ENS_COUNT ? dr_scene.p[P_ENS] : 0u;   /* this block's effect settings */
    fx.ens_mix = p01(P_ENS_MIX);
    {
        uint32_t pt = (uint32_t)(p01(P_DLY_TIME) * 127.0f + 0.5f);
        int32_t dv = dr_dly_div(pt);
        fx.dly_synced = dv >= 0;
        if (dv >= 0) {                   /* in time: the MIDI clock followed, else the runner's RATE */
            float bpm = dr_clock_bpm(), t;
            if (bpm <= 0.0f)
                bpm = dr_arp_bpm(dr_scene.p[P_ARP_RATE]);
            t = DLY_DIV_BEATS[dv] * 60.0f / bpm;
            while (t > DR_DLY_MAX_S)     /* too long for the line at this tempo: half of it, still in time */
                t *= 0.5f;
            fx.dly_time_s = t < 0.05f ? 0.05f : t;
        } else {
            fx.dly_time_s = dr_dly_time_s(pt);
        }
    }
    fx.dly_fdbk = p01(P_DLY_FDBK);
    fx.dly_diff = p01(P_DLY_DIFF);
    fx.dly_colour = p01(P_DLY_COLOUR);
    fx.dly_type = dr_scene.p[P_DLY_TYPE] < DT_COUNT ? dr_scene.p[P_DLY_TYPE] : DT_PING;   /* a kind, not morphed */
    fx.dly_wow = p01(P_DLY_WOW);
    fx.dly_chop = p01(P_DLY_CHOP);
    {                                    /* DLY MIX is the delay's level; SPACE only pushes it a little further. Above
                                          * 60 % it climbs harder, so full brings the first echo about as loud as the
                                          * note (9 dB over the old top); below, as it always was (every factory scene) */
        float mix = p01(P_DLY_MIX), over = mix > 0.6f ? mix - 0.6f : 0.0f;
        fx.dly_send = fm_sqrtf(mix) * (0.6f + 0.4f * space) * (1.0f + 12.0f * over * over);
    }
    fx.rev_decay = p01(P_REV_SIZE);
    fx.rev_damp = p01(P_REV_DAMP);
    fx.rev_send = fm_sqrtf(space) * 0.9f;
    fx.shimmer = p01(P_SHIMMER);
    fx.freeze = frozen;
    fx.bypass = fx_bypass & (FX_BYP_ENS | FX_BYP_DLY | FX_BYP_REV | FX_BYP_SHIM);
    {                                    /* a delay or a hall switched off rings out (its send closed) and rests
                                          * before it is skipped; a frozen hall is cut, as it would hold for ever */
        uint32_t run = fx_running();
        if ((fx.bypass & FX_BYP_DLY) && (run & 2u)) {
            fx.bypass &= ~FX_BYP_DLY;
            fx.dly_send = 0.0f;
        }
        if ((fx.bypass & FX_BYP_REV) && (run & 4u) && !frozen) {
            fx.bypass = (fx.bypass & ~FX_BYP_REV) | FX_BYP_SHIM;   /* and no new octave up into the tail */
            fx.rev_send = 0.0f;
        }
    }
    g_fxp = fx;
    job_n = n;
    /* the hall runs a block behind the bus (its wet is a pre-delay anyway): on the second core beside two bodies
     * while this core does the other six and the rest, or here when there is no second core */
#if FM1B_CPU2
    if (plat_cpu2_on()) {
        plat_cpu2_sync();
        on_two = plat_cpu2_run(half_job, 0) == 1;
    }
#endif
    zero_mix(mixl, mixr, n);
    render_voices(on_two ? NV_CORE2 : 0u, NV, n, mixl, mixr);
    if (!on_two) {
        render_voices(NV, NV_ALL, n, mixl, mixr);
        if (have_arena)
            fx_hall(hall_in_l, hall_in_r, hall_out_l, hall_out_r, hall_n, &g_fxp);
    }
#if FM1B_CPU2
    if (on_two) {
        if (plat_cpu2_wait() != 0) {     /* given up: this core does that half too; the hall misses this block */
            zero_mix(mixl2, mixr2, n);
            render_voices(0, NV_CORE2, n, mixl2, mixr2);
            render_voices(NV, NV_ALL, n, mixl2, mixr2);
            for (i = 0; i < n; i++)
                hall_out_l[i] = hall_out_r[i] = 0.0f;
        } else {
            plat_cpu2_sync();
        }
        for (i = 0; i < n; i++) {
            mixl[i] += mixl2[i];
            mixr[i] += mixr2[i];
        }
    }
#endif
    noise_render(b, n, mixl, mixr);
    pk_bus = 0.0f;
    for (i = 0; i < n; i++) {            /* the bus, kept within +-1 before the effects */
        float a;
        mixl[i] = softclip(mixl[i] * 0.36f);   /* 3 dB under the old bus: room for the effects before the limiter */
        mixr[i] = softclip(mixr[i] * 0.36f);
        a = mixl[i] < 0.0f ? -mixl[i] : mixl[i];
        pk_bus = a > pk_bus ? a : pk_bus;
    }
    if (have_arena) {
        fx_pre(mixl, mixr, n, &fx);      /* the ensemble and the delay */
        for (i = 0; i < n; i++) {        /* this bus goes to the hall next block; last block's wet joins it now */
            hall_in_l[i] = mixl[i];
            hall_in_r[i] = mixr[i];
            mixl[i] += hall_out_l[i];
            mixr[i] += hall_out_r[i];
        }
        hall_n = n;
        fx_post(mixl, mixr, n);          /* the limiter */
    }
    pk_out = 0.0f;
    for (i = 0; i < n; i++) {
        float a = mixl[i] < 0.0f ? -mixl[i] : mixl[i];
        pk_out = a > pk_out ? a : pk_out;
        gain_now += (gain - gain_now) * 0.004f;    /* ~5 ms: turning MASTER does not click */
        lr[2 * i] = (int32_t)(mixl[i] * gain_now);
        lr[2 * i + 1] = (int32_t)(mixr[i] * gain_now);
    }
}

/* ------------------------------------------------------------------ for the screen --- */
uint32_t dr_bodies(void)
{
    uint32_t i, c = 0;
    for (i = 0; i < NV; i++)
        c += vc[i].on != 0;
    return c;
}
int dr_body(uint32_t i, dr_body_t *b)
{
    const voice_t *v;
    if (i >= NV)
        return 0;
    v = &vc[i];
    b->on = v->on;
    b->fading = v->fading;
    b->latched = v->latched;
    b->home = v->home;
    b->pitch = v->pitch;
    b->target = v->target;
    b->level = v->on ? v->env * v->vel * v->ed_gain : 0.0f;
    if (b->level > 1.0f)
        b->level = 1.0f;
    b->spark = v->spark;
    b->lvl = v->ed_lvl;
    b->tone = v->ed_tone;
    b->tune = v->ed_tune;
    b->pinned = v->pinned;
    return v->on;
}
uint32_t dr_leads(void)
{
    uint32_t i, c = 0;
    for (i = NV; i < NV_ALL; i++)
        c += vc[i].on != 0;
    return c;
}
int dr_lead_body(uint32_t i, dr_body_t *b)
{
    const voice_t *v;
    if (i >= NL)
        return 0;
    v = &vc[NV + i];
    b->on = v->on;
    b->fading = (uint8_t)(v->on && !v->held);
    b->latched = 0;
    b->home = v->home;
    b->pitch = v->pitch;
    b->level = v->on ? v->env * v->vel : 0.0f;
    b->spark = 0.0f;
    b->lvl = b->tone = b->tune = 0;
    b->pinned = 0;
    return v->on;
}
int32_t dr_newest_note(void)
{
    uint32_t i;
    for (i = 0; i < NV; i++)
        if (vc[i].on && !vc[i].fading && vc[i].home == (uint32_t)newest_note)
            return newest_note;
    return -1;
}
int dr_sounding(uint32_t note) { return find_body(note & 127u, 0) != 0; }
float dr_out_level(void) { return pk_out; }
float dr_bus_level(void) { return pk_bus; }
