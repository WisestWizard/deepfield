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

#define FS DR_FS
#define CTL DR_CTL
#define NV DR_NV
#define MAXBLOCK 256u
#define SIN_N 4096u
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
static uint32_t clk_now, clk_last, clk_period, clk_count, clk_ext, clk_out, orbit_clk_left, orbit_clk_per;
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
static int16_t *scope_l, *scope_r;
static int have_arena;
static float pk_bus, pk_out;
static uint32_t ovl_cool, calm_blocks;
static int32_t newest_note = -1;

static struct { uint8_t note, vel, on, kind; } q[NQ];   /* main loop -> render; kind 1: SEL's latch / release,
                                                         * 2: a latched strike, 3: unlatch, 4: every body let go,
                                                         * 5: a lead, 6: the latched bodies transposed */
static volatile uint32_t q_head, q_tail;
static scene_t load_scene;               /* a scene loaded: its bodies are struck and let go by the render */
static volatile uint32_t load_pending;

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
float dr_orbit_s(uint32_t p) { return fm_exp2f((float)(p & 127u) * (5.9f / 127.0f)); }              /* 1 .. 60 s */
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
static inline float vmass(const voice_t *v, const blk_t *b)   /* a voice's MASS: the force, pressure, a lead's boost */
{
    return clamp01(b->mass + v->press * b->press_amt * 0.6f + v->mboost + v->sp_mass * b->spread * 0.15f
                   + v->twalk * b->twalk_depth * 0.25f);
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

static voice_t *find_body(uint32_t note, int fading_too)
{
    uint32_t i;
    voice_t *best = 0;
    for (i = 0; i < NV; i++)
        if (vc[i].on && vc[i].home == note && (fading_too || !vc[i].fading))
            if (!best || vc[i].age > best->age)
                best = &vc[i];
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
            let_go(v);
            return;
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
    if (dr_orbit_mode() == OM_ARP) {     /* through the bar: eight steps */
        if (clk_ext)
            return (float)(clk_count % 96u) * (1.0f / 96.0f);
        return arp_step ? ((float)(arp_count % 8u) + 1.0f - (float)arp_left / (float)arp_step) * 0.125f : 0.0f;
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
}
void dr_load(const scene_t *s, float seconds)
{
    uint32_t i;
    dr_scene = *s;
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
    load_pending = 0;                    /* down while the copy is torn, up once it has landed */
    __asm__ volatile("" ::: "memory");
    load_scene = *s;
    __asm__ volatile("" ::: "memory");
    load_pending = 1;
}
static void load_bodies(void)            /* render: the scene's bodies swell in; the latched ones not in it fade */
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
        if (s->vel[k] && !find_body(s->note[k], 0)) {
            voice_t *v;
            note_event(s->note[k], s->vel[k], 1, 1, 0);
            if ((v = find_body(s->note[k], 0)) != 0)
                v->latched = 1;
        }
}
int dr_morphing(void) { return morph_pos < 1.0f; }
static uint8_t hold_on;
void dr_morph_hold(const scene_t *to, float pos)
{
    uint32_t i;
    if (!hold_on) {                      /* from what is heard now */
        for (i = 0; i < P_COUNT; i++)
            pstart[i] = pcur[i];
        hold_on = 1;
    }
    for (i = 0; i < P_COUNT; i++)
        ptarget[i] = (float)(to->p[i] & 127u) * (1.0f / 127.0f);
    pstart[P_MODEL] = ptarget[P_MODEL] = pcur[P_MODEL];   /* the model does not blend */
    morph_step = 0.0f;
    morph_pos = fm_clampf(pos, 0.0f, 0.999f);
}
void dr_morph_hold_end(void) { hold_on = 0; }
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
    cents = v->walk * b->walk_depth * 40.0f + v->sp_det * b->spread * 14.0f;   /* SPREAD: up to 14 cents apart */
    v->twalk += frand(&v->rng) * 0.003f - v->twalk * 0.0006f;   /* the timbre's own slow walk */
    v->twalk = fm_clampf(v->twalk, -1.0f, 1.0f);
    v->sway_ph += b->sway_inc * (0.8f + 0.4f * v->sp_tide);
    if (v->sway_ph >= 1.0f)
        v->sway_ph -= 1.0f;
    v->pan_live = fm_clampf(v->pan + v->sp_pan * b->spread * 0.5f + tsin(v->sway_ph) * b->sway * 0.7f, -1.0f, 1.0f);
    if (b->cross > 0.0f && !v->lead) {  /* CROSS: the partner's swell bends this body (bodies in pairs) */
        const voice_t *q = &vc[(v - vc) ^ 1u];
        if (q->on && !q->lead)
            cents += (q->lvl_s - v->lvl_s) * b->cross * 180.0f;
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
    lvl = v->env * v->vel * tide * (v->lead ? b->lead_level : 1.0f);
    *lvl_out = lvl;
    v->breath_ph += b->breath_inc;
    if (v->breath_ph >= 1.0f)
        v->breath_ph -= 1.0f;
    /* the model's spectrum this tick: SOLAR plays it, the others show it */
    switch (v->model) {
    case M_SOLAR: {
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
        break;
    }
    case M_VOID: {
        float fc = dr_hz((uint32_t)(mass * 127.0f)), g;
        if (fc < f * 1.2f)
            fc = f * 1.2f;
        g = fc / f;
        for (h = 0; h < 8u; h++) {
            float r = (float)(h + 1u) / g;
            v->amp[h] = (1.0f / (float)(h + 1u)) / (1.0f + r * r);
        }
        break;
    }
    case M_GLASS: {
        float idx = mass * 4.0f, r = GLASS_RATIO[(uint32_t)(b->shape * 7.99f)];
        for (h = 0; h < 8u; h++)
            v->amp[h] = 0.0f;
        v->amp[0] = 1.0f;
        if (r < 8.0f && (uint32_t)(r + 0.5f) >= 2u)
            v->amp[(uint32_t)(r + 0.5f) - 1u] = idx * 0.25f;
        v->amp[1] += idx * 0.15f;
        break;
    }
    case M_CHOIR: {                      /* the harmonics near the vowel's formants */
        float F[3], G[3];
        choir_formants(b->shape, mass, F, G);
        for (h = 0; h < 8u; h++) {
            float fh = f * (float)(h + 1u), a = 0.0f;
            uint32_t k;
            for (k = 0; k < 3u; k++) {
                float d = (fh - F[k]) / (0.12f * F[k] + 40.0f);
                a += G[k] / (1.0f + d * d);
            }
            v->amp[h] = a * 0.8f / (float)(h + 1u) * 2.0f;
        }
        break;
    }
    case M_BELL: {                       /* the four partials, at the nearest harmonic slots */
        for (h = 0; h < 8u; h++)
            v->amp[h] = 0.0f;
        for (h = 0; h < 4u; h++) {
            float r = bell_ratio(h, b->shape);
            uint32_t slot = (uint32_t)(r + 0.5f);
            if (slot >= 1u && slot <= 8u)
                v->amp[slot - 1u] += fm_exp2f(-(2.5f - 2.3f * mass) * LOG2H[h]) * (0.5f + 0.5f * tsin(v->hb[h]));
        }
        break;
    }
    case M_SHEPARD: {                    /* the octaves under the window */
        static const uint8_t SLOT[6] = {0, 0, 0, 1, 3, 7};
        for (h = 0; h < 8u; h++)
            v->amp[h] = 0.0f;
        for (h = 0; h < 6u; h++)
            v->amp[SLOT[h]] += shepard_window(h, v->gp, mass) * 0.5f;
        break;
    }
    default: {
        float d = v->model == M_FOLD ? 0.3f + b->shape * 2.0f : 0.3f + mass * 2.2f;
        for (h = 0; h < 8u; h++)
            v->amp[h] = fm_exp2f(-(2.6f - d) * LOG2H[h]) * (0.6f + 0.4f * b->air);
        break;
    }
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
uint32_t dr_arp_steps(void) { return arp_count; }

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
    for (i = 0; i < NV; i++)
        if (vc[i].on && !vc[i].fading && vc[i].latched && !vc[i].restart)
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
            if ((orbit_rng >> 9) % 100u < grav && cur != (int32_t)v->home) {
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
        if (!v->on || v->fading || !v->latched || v->restart)
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
static void arp_gather(void)             /* the latched notes, low to high, then OCTAVES up; none: keep the last */
{
    uint32_t i, j, n = 0, oct = 1u + (dr_scene.p[P_ARP_OCT] < 3u ? dr_scene.p[P_ARP_OCT] : 2u), notes[NV];
    for (i = 0; i < NV; i++)
        if (vc[i].on && !vc[i].fading && vc[i].latched && !vc[i].restart)
            notes[n++] = (uint32_t)(vc[i].target + 0.5f);
    if (!n)
        return;
    for (i = 1; i < n; i++) {            /* sorted, insertion */
        uint32_t x = notes[i];
        j = i;
        while (j && notes[j - 1u] > x) {
            notes[j] = notes[j - 1u];
            j--;
        }
        notes[j] = x;
    }
    arp_n = 0;
    for (j = 0; j < oct; j++)
        for (i = 0; i < n; i++)
            if (notes[i] + 12u * j <= 108u)
                arp_notes[arp_n++] = notes[i] + 12u * j;
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
    if (clk_ext) {                       /* following: the clock says when (dr_clock), the step is twelve clocks */
        if (!arp_due)
            return;
        arp_due = 0;
        arp_step = clk_period * 12u;
    } else {
        if (arp_left > n) {
            arp_left -= n;
            return;
        }
        arp_step = (uint32_t)(30.0f / dr_arp_bpm(dr_scene.p[P_ARP_RATE]) * FS);   /* an eighth at RATE */
        arp_left = arp_step;
    }
    arp_count++;
    arp_release();
    arp_gather();
    if (!arp_n)
        return;
    {
        uint32_t note;
        switch (order) {
        case AO_DOWN:
            arp_pos = arp_pos ? arp_pos - 1u : arp_n - 1u;
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
        case AO_PLAYED: {                /* in the order the bodies were struck, then the octaves up */
            uint32_t i, cnt = 0, idx[NV], a, b, oct = 1u + (dr_scene.p[P_ARP_OCT] < 3u ? dr_scene.p[P_ARP_OCT] : 2u);
            for (i = 0; i < NV; i++)
                if (vc[i].on && !vc[i].fading && vc[i].latched && !vc[i].restart)
                    idx[cnt++] = i;
            for (a = 1; a < cnt; a++)
                for (b = a; b && vc[idx[b - 1u]].age > vc[idx[b]].age; b--) {
                    uint32_t t = idx[b];
                    idx[b] = idx[b - 1u];
                    idx[b - 1u] = t;
                }
            if (cnt) {
                arp_pos = (arp_pos + 1u) % (cnt * oct);
                note = (uint32_t)(vc[idx[arp_pos % cnt]].target + 0.5f) + 12u * (arp_pos / cnt);
                if (note > 108u)
                    note -= 12u * (arp_pos / cnt);
                goto play;
            }
            arp_pos = (arp_pos + 1u) % arp_n;   /* the bed gone: its memory, low to high */
            break;
        }
        default:
            arp_pos = (arp_pos + 1u) % arp_n;
            break;
        }
        if (arp_pos >= arp_n)
            arp_pos = 0;
        note = arp_notes[arp_pos];
play:
        if (note_out)
            note_out(note, 100, 1);
        lead_machine = 1;
        lead_event(note, 100, 1, 0);
        lead_machine = 0;
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
    arp_n = arp_pos = arp_dir = arp_left = arp_step = arp_count = arp_gate_left = arp_due = 0;
    cyc_pos = 0;
    for (i = 0; i < 8u; i++)
        echo_due[i] = echo_off[i] = 0;
    echo_vel = 1.0f;
    orbit_clk_left = orbit_clk_per = 0;
    if (arp_held >= 0)
        arp_release();
}
static uint32_t orbit_beats(void)        /* EVERY as whole beats at the clock's tempo */
{
    float bpm = dr_clock_bpm(), b = dr_orbit_s(dr_scene.p[P_ORB_PERIOD]) * (bpm > 0.0f ? bpm : 120.0f) / 60.0f + 0.5f;
    return b < 1.0f ? 1u : (uint32_t)b;
}
void dr_clock(void)
{
    uint32_t mode = dr_orbit_mode();
    if (clk_ext && clk_now > clk_last)
        clk_period = clk_period ? (clk_period * 3u + (clk_now - clk_last)) / 4u : clk_now - clk_last;
    clk_last = clk_now;
    clk_ext = 1;
    if (orbit_on && mode == OM_ARP) {
        if (clk_count % 12u == 0u)
            arp_due = 1;
    } else if (orbit_on) {
        if (orbit_clk_left)
            orbit_clk_left--;
        if (!orbit_clk_left) {
            orbit_apply();
            orbit_clk_left = orbit_clk_per = orbit_beats() * 24u;
            orbit_plan();
        }
    }
    clk_count++;
}
void dr_clock_start(void)
{
    clk_count = 0;
    arp_pos = 0;
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
    if (clk_ext)                         /* the clock counts the period (dr_clock) */
        return;
    if (orbit_left > n) {
        orbit_left -= n;
        return;
    }
    orbit_apply();
    {                                    /* the next countdown: the period, give or take a quarter (a half in SCATTER) */
        float j = mode == OM_SCATTER ? 0.5f : 0.25f;
        float per = dr_orbit_s(dr_scene.p[P_ORB_PERIOD]) * (1.0f + j * frand(&orbit_rng));
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
    amp *= 1.0f - duck * 0.75f * nz.duck;
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
#define DR_NEED (SIN_N * 4u + NV_ALL * LINE_LEN * 2u + MAXBLOCK * 8u * 4u + MAXBLOCK * 2u * 2u + FX_BYTES + 64u)
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
    scope_l = (int16_t *)a;
    a += MAXBLOCK * 2u;
    scope_r = (int16_t *)a;
    a += MAXBLOCK * 2u;
    for (i = 0; i < MAXBLOCK; i++)
        scope_l[i] = scope_r[i] = 0;
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
    float gain = (float)master_q12 * (1.0f / 4096.0f) * 8700000.0f * volume, space;
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
    b->lead_level = p01(P_LD_LEVEL) * fm_sqrtf(p01(P_LD_LEVEL)) * 2.0f;   /* 75: level with a body; full: 6 dB over */
    b->lift = (float)(uint32_t)(p01(P_LD_LIFT) * 24.99f);                 /* whole semitones, up to two octaves */
    b->spread = p01(P_SPREAD);
    b->cross = p01(P_CROSS);
    b->twalk_depth = p01(P_TWALK);
    b->sway = p01(P_SWAY);
    b->sway_inc = b->sway > 0.0f ? (1.0f / 50.0f + b->sway * (1.0f / 4.0f)) * ((float)CTL / FS) : 0.0f;   /* a swing in 50 s to 3 s */
    b->n = n;
    for (i = 0; i < NV_ALL; i++)         /* a voice in another model than the scene's restarts */
        if (vc[i].on && vc[i].model != b->model && !vc[i].restart)
            vc[i].restart = 1;
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
    fx.dly_send = fm_sqrtf(p01(P_DLY_MIX)) * (0.6f + 0.4f * space);   /* DLY MIX is the delay's level; SPACE
                                                                        * only pushes it a little further */
    fx.rev_decay = p01(P_REV_SIZE);
    fx.rev_damp = p01(P_REV_DAMP);
    fx.rev_send = fm_sqrtf(space) * 0.9f;
    fx.shimmer = p01(P_SHIMMER);
    fx.freeze = frozen;
    fx.bypass = fx_bypass & (FX_BYP_ENS | FX_BYP_DLY | FX_BYP_REV | FX_BYP_SHIM);
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
        scope_l[i] = (int16_t)(mixl[i] * 32000.0f);
        scope_r[i] = (int16_t)(mixr[i] * 32000.0f);
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
    b->level = v->on ? v->env * v->vel : 0.0f;
    b->spark = v->spark;
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
void dr_scope(const int16_t **l, const int16_t **r, uint32_t *n)
{
    *l = scope_l;
    *r = scope_r;
    *n = MAXBLOCK;
}
uint32_t dr_harmonics(uint32_t i, float *amp)
{
    uint32_t h;
    if (i >= NV || !vc[i].on)
        return 0;
    for (h = 0; h < 8u; h++)
        amp[h] = vc[i].amp[h];
    return 8;
}
