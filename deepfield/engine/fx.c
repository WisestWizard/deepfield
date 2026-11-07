/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann */
#include <stdint.h>
#include "fastmath.h"
#include "drone.h"
#include "fx.h"

#define FS DR_FS
#define ENS_LEN FX_ENS_LEN
#define DLY_LEN FX_DLY_LEN
#define SHIM_LEN FX_SHIM_LEN
#define SHIM_W 2048.0f               /* the shimmer's grain: 46 ms, two heads half a grain apart */
#define TWO_PI 6.28318531f

/* The hall: a feedback delay network of eight lines (45 to 131 ms) through a Hadamard mix, stereo in and out, with
 * a 30 ms pre-delay (one block of it from running a block behind the bus), a diffusing allpass a side, a high-pass on the way in (the sub stays dry), damping in each
 * line and a slow wander on four of them. REV SIZE is the decay time (0.6 to 8 s); freeze holds the network; the
 * shimmer feeds its own octave back in. */
#define RV_PRE 1080u                     /* 24.5 ms here, 30 ms heard: the hall runs a block (256) behind the bus */
#define RV_MOD 24u                       /* room past each line's length for the wander */
static const uint32_t RV_AP[4] = {199u, 431u, 173u, 388u};
static const uint32_t RV_L[8] = {1979u, 2371u, 2811u, 3323u, 3863u, 4493u, 5107u, 5801u};
#define RV_TOTAL (2u * RV_PRE + 199u + 431u + 173u + 388u \
                  + 1979u + 2371u + 2811u + 3323u + 3863u + 4493u + 5107u + 5801u + 8u * RV_MOD)

typedef struct { int16_t *buf; uint32_t len, pos; } line_t;
typedef char fx_rv_check[RV_TOTAL == FX_RV_TOTAL ? 1 : -1];   /* fx.h's FX_BYTES counts the same lines */

static float *ens_l, *ens_r;
static uint32_t ens_pos;
static float ens_ph1, ens_ph2;

/* The delay: one 16-bit buffer at half the rate (the repeats are filtered well below 11 kHz anyway, and the same
 * memory holds twice the time), used by its TYPE in one of two ways. Two legs (the left 0.80 s, then the right
 * 1.11 s): the input into the left, each leg's repeat crossing into the other with the feedback, the right read
 * 1.382 times the left so the sides never coincide (as long as the left when TIME is in time) - PING, and RISE and
 * FALL, which read each leg through two crossfading heads an octave up or down. Or one line of the whole 1.91 s:
 * TAPE (one head, darker and harder each pass, wandering more), HEADS (three heads at a third, two thirds and the
 * whole time, left centre right), REVERSE (two heads reading back across windows of the delay time), GRAINS (short
 * windowed pieces from up to 1.9 s back, one every delay time, scattered left and right). Every write passes two
 * allpasses (DIFFUSE), a low-pass and high-pass set by COLOUR and a soft clip. WOW moves every head's distance
 * (wow and flutter); CHOP gates the input in slices of the delay time. A change of TYPE fades the repeats out over
 * 12 ms, empties the buffer and starts the new type there. */
#define FSH (FS * 0.5f)
#define DLY_LEN_L 17664u                 /* the left leg: 0.80 s at the half rate */
#define DLY_LEN_R (DLY_LEN - DLY_LEN_L)  /* the right: 1.11 s */
#define DLY_RATIO 1.382f
#define DLY_WIDTH 0.7f                   /* the two legs' spread: 0 mono, 1 hard left and right */
#define DLY_AP0 157u                     /* 7 and 11 ms on the left, 5 and 13 on the right */
#define DLY_AP1 241u
#define DLY_AP2 113u
#define DLY_AP3 285u
#define DLY_AP_SUM 398u                  /* a leg's allpasses delay it this much: its read is that much shorter */
static const uint32_t DLY_AP[4] = {DLY_AP0, DLY_AP1, DLY_AP2, DLY_AP3};
typedef char fx_ap_check[DLY_AP0 + DLY_AP1 + DLY_AP2 + DLY_AP3 == FX_DLY_AP
                         && DLY_AP0 + DLY_AP1 == DLY_AP_SUM && DLY_AP2 + DLY_AP3 == DLY_AP_SUM
                         && DLY_LEN_R > DLY_LEN_L ? 1 : -1];
#define DLY_SHIFT_G 1536u               /* RISE and FALL: a head's grain, 70 ms at the half rate */
#define DLY_WOB_MAX 420.0f               /* the most WOW moves a head back, in half-rate samples (TAPE's at full) */
#define DLY_NGR 6u                       /* GRAINS: pieces sounding at once */
static int16_t *dly;
static line_t dly_ap[4];
static uint32_t dly_pos_l, dly_pos_r, dly_pos_w, dly_type_cur;
static float dly_read, dly_ratio = DLY_RATIO, dly_lp[2], dly_hp_x[2], dly_hp_y[2], wow_ph, flut_ph;
static float dly_dn1, dly_dn2, dly_ol, dly_or, dly_ul, dly_ur;   /* down to the half rate, and back up */
static float dly_fade = 1.0f, chop_ph, chop_g = 1.0f, sh_ph, rev_ph, gr_left;
static float on_fast, on_slow, on_hold;
static uint32_t rev_lr;                  /* REVERSE: the side this swell leans to */  /* HEADS: a note's onset opens its slicing gate, so a played note echoes whole */
typedef struct { float d, pos, len, gl, gr, rate; uint32_t on; } grain_t;   /* rate: how far its head moves a sample
                                                                            * against the line (0 unison, 1 an octave
                                                                            * up, -0.5 down, 0.5 a fifth up) */
static grain_t grn[DLY_NGR];
static const float DLY_OG[DT_COUNT] = {1.0f, 0.5f, 1.16f, 1.35f, 1.55f, 1.4f, 1.15f};   /* each TYPE's wet, level
                                         * with PING's on a held chord (measured: the drone and a played key) */
static uint32_t gr_rng = 0x9E3779B9u;

static line_t rv_pre[2], rv_ap[4], rv_fd[8];
static float rv_hp_l, rv_hp_r, rv_lp[8], rv_mod_ph;
static int16_t *shim;                    /* the plate's output, read back an octave up (two crossfading heads) */
static uint32_t shim_pos;
static float shim_ph, shim_lp, shim_last, frz_g = 1.0f;   /* frozen, the cloud comes up to the level the dry had */

static float lim_gain = 1.0f, lim_min = 1.0f;
#define LIM_LA 64u                       /* the limiter's lookahead, 1.5 ms: a power of two */
static float lim_dl[LIM_LA], lim_dr[LIM_LA], lim_tgt = 1.0f;
static uint32_t lim_pos;
float fx_lim_min(void)
{
    float m = lim_min;
    lim_min = 1.0f;
    return m;
}
static uint32_t ens_tail, dly_tail, rev_tail;   /* blocks a stage still runs once nothing passes through it: with its
                                                 * input and output silent that long, it holds only zeros and rests */
static uint32_t dly_live, rv_live;       /* the stage's lines had something in them last block */
static float pk_ens, pk_dly, pk_rev;      /* each stage's output peak, decaying: a diagnostic (fx_peaks) */

static float block_peak(const float *l, const float *r, uint32_t n)
{
    float pk = 0.0f;
    uint32_t i;
    for (i = 0; i < n; i++) {
        float a = l[i] < 0.0f ? -l[i] : l[i], b = r[i] < 0.0f ? -r[i] : r[i];
        pk = a > pk ? a : pk;
        pk = b > pk ? b : pk;
    }
    return pk;
}
uint32_t fx_running(void) { return (ens_tail ? 1u : 0u) | (dly_tail ? 2u : 0u) | (rev_tail ? 4u : 0u); }
void fx_peaks(float *ens, float *dly, float *rev)
{
    *ens = pk_ens;
    *dly = pk_dly;
    *rev = pk_rev;
}

uint32_t fx_bytes(void) { return FX_BYTES; }

static inline float tri(float ph) { return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph; }

/* a 16-bit delay line: write, read at an integer or fractional distance back */
static inline void line_put(line_t *d, float v)
{
    float c = v * 32767.0f;
    d->buf[d->pos] = (int16_t)(c > 32767.0f ? 32767.0f : c < -32767.0f ? -32767.0f : c);
    d->pos = d->pos + 1u == d->len ? 0u : d->pos + 1u;
}
static inline float line_at(const line_t *d, uint32_t back)   /* back >= 1, before this step's write */
{
    uint32_t i = d->pos + d->len - back;
    if (i >= d->len)
        i -= d->len;
    return (float)d->buf[i] * (1.0f / 32767.0f);
}
static inline float line_at_f(const line_t *d, float back)
{
    uint32_t b = (uint32_t)back;
    float f = back - (float)b, a = line_at(d, b), c = line_at(d, b + 1u);
    return a + (c - a) * f;
}
/* a Schroeder allpass on a line of length d->len: w[n] = x + g w[n-L], y = w[n-L] - g w[n]. (The first version
 * returned w[n-L] - g x, which is not an allpass: it peaks at up to 1/(1-g) and made the plate run away.) */
static inline float allpass(line_t *d, float x, float g)
{
    float v = line_at(d, d->len - 1u), w = x + g * v;
    line_put(d, w);
    return v - g * w;
}
static inline float allpass_mod(line_t *d, float x, float g, float back)
{
    float v = line_at_f(d, back), w = x + g * v;
    line_put(d, w);
    return v - g * w;
}

void fx_init(void *arena)
{
    uint8_t *a = (uint8_t *)arena;
    line_t *lines[14];
    uint32_t i, k;
    ens_l = (float *)a;
    a += ENS_LEN * sizeof(float);
    ens_r = (float *)a;
    a += ENS_LEN * sizeof(float);
    dly = (int16_t *)a;
    a += DLY_LEN * 2u;
    for (i = 0; i < 4u; i++) {
        dly_ap[i].buf = (int16_t *)a;
        dly_ap[i].len = DLY_AP[i];
        dly_ap[i].pos = 0;
        a += DLY_AP[i] * 2u;
        for (k = 0; k < DLY_AP[i]; k++)
            dly_ap[i].buf[k] = 0;
    }
    shim = (int16_t *)a;
    a += SHIM_LEN * 2u;
    for (i = 0; i < SHIM_LEN; i++)
        shim[i] = 0;
    shim_pos = 0;
    shim_ph = shim_lp = shim_last = 0.0f;
    lines[0] = &rv_pre[0];
    lines[1] = &rv_pre[1];
    for (i = 0; i < 4u; i++)
        lines[2u + i] = &rv_ap[i];
    for (i = 0; i < 8u; i++)
        lines[6u + i] = &rv_fd[i];
    for (i = 0; i < 14u; i++) {
        line_t *d = lines[i];
        d->buf = (int16_t *)a;
        d->len = i < 2u ? RV_PRE : i < 6u ? RV_AP[i - 2u] : RV_L[i - 6u] + RV_MOD;
        d->pos = 0;
        a += d->len * 2u;
        for (k = 0; k < d->len; k++)
            d->buf[k] = 0;
    }
    for (i = 0; i < 8u; i++)
        rv_lp[i] = 0.0f;
    for (i = 0; i < ENS_LEN; i++)
        ens_l[i] = ens_r[i] = 0.0f;
    for (i = 0; i < DLY_LEN; i++)
        dly[i] = 0;
    ens_pos = dly_pos_l = dly_pos_r = 0;
    ens_tail = dly_tail = rev_tail = 0;
    dly_live = rv_live = 0;
    ens_ph1 = ens_ph2 = wow_ph = flut_ph = rv_mod_ph = 0.0f;
    dly_read = 2205.0f;
    dly_ratio = DLY_RATIO;
    dly_type_cur = DT_PING;
    dly_pos_w = 0;
    dly_fade = chop_g = 1.0f;
    chop_ph = sh_ph = rev_ph = gr_left = on_fast = on_slow = on_hold = 0.0f;
    rev_lr = 0;
    for (i = 0; i < DLY_NGR; i++)
        grn[i].on = 0;
    dly_lp[0] = dly_lp[1] = dly_hp_x[0] = dly_hp_x[1] = dly_hp_y[0] = dly_hp_y[1] = rv_hp_l = rv_hp_r = 0.0f;
    dly_dn1 = dly_dn2 = dly_ol = dly_or = dly_ul = dly_ur = 0.0f;
    lim_gain = lim_tgt = 1.0f;
    for (i = 0; i < LIM_LA; i++)
        lim_dl[i] = lim_dr[i] = 0.0f;
    lim_pos = 0;
}

static inline float dly_tap(const int16_t *buf, uint32_t len, uint32_t pos, float back, int32_t *live)
{                                        /* a leg read a fraction back (kept within the leg) */
    uint32_t b, i0, i1;
    back = back < 1.0f ? 1.0f : back > (float)(len - 2u) ? (float)(len - 2u) : back;
    b = (uint32_t)back;
    i0 = pos + len - b;
    if (i0 >= len)
        i0 -= len;
    i1 = i0 == 0u ? len - 1u : i0 - 1u;
    *live |= buf[i0] | buf[i1];
    return ((float)buf[i0] + ((float)buf[i1] - (float)buf[i0]) * (back - (float)b)) * (1.0f / 32767.0f);
}
static inline void dly_put(int16_t *buf, uint32_t pos, float c, int32_t *live)
{
    c *= 32767.0f;
    buf[pos] = (int16_t)(c > 32767.0f ? 32767.0f : c < -32767.0f ? -32767.0f : c);
    *live |= buf[pos];
}
static inline float dly_leg(uint32_t s, float c, float g, float lpc, float hpa, float drive)
{                                        /* what goes into a leg: smeared, filtered by COLOUR, softer */
    float y;
    c = allpass(&dly_ap[2u * s], c, g);
    c = allpass(&dly_ap[2u * s + 1u], c, g);
    dly_lp[s] += (c - dly_lp[s]) * lpc;
    c = dly_lp[s] * drive;                          /* a cubic clip, harder with the drive, the same gain below it */
    c = c > 1.5f ? 1.5f : c < -1.5f ? -1.5f : c;
    c = (c - c * c * c * (4.0f / 27.0f)) * (1.02f / drive);
    y = hpa * (dly_hp_y[s] + c - dly_hp_x[s]);      /* the low end thinned, so the repeats sit off the drone */
    dly_hp_x[s] = c;
    dly_hp_y[s] = y;
    return y;
}

static void dly_clear(void)              /* a new TYPE: the buffer, the smearing, the filters and the heads from zero */
{
    uint32_t i, k;
    for (i = 0; i < DLY_LEN; i++)
        dly[i] = 0;
    for (i = 0; i < 4u; i++) {
        for (k = 0; k < dly_ap[i].len; k++)
            dly_ap[i].buf[k] = 0;
        dly_ap[i].pos = 0;
    }
    for (i = 0; i < 2u; i++)
        dly_lp[i] = dly_hp_x[i] = dly_hp_y[i] = 0.0f;
    for (i = 0; i < DLY_NGR; i++)
        grn[i].on = 0;
    dly_pos_l = dly_pos_r = dly_pos_w = 0;
    sh_ph = rev_ph = gr_left = 0.0f;
}
static inline float gr_rand(void)        /* 0..1 */
{
    gr_rng ^= gr_rng << 13;
    gr_rng ^= gr_rng >> 17;
    gr_rng ^= gr_rng << 5;
    return (float)(gr_rng >> 8) * (1.0f / 16777216.0f);
}
static inline float tri01(float w) { return 1.0f - fm_fabsf(2.0f * w - 1.0f); }   /* 0 at the ends, 1 in the middle */
static inline float dly_shift(const int16_t *buf, uint32_t len, uint32_t pos, float base, float up, int32_t *live)
{                                        /* a leg read an octave up (or down) by two heads half a grain apart: the
                                          * distance falls a grain over a grain (up) or rises half of one (down) */
    float w1 = sh_ph, w2 = sh_ph + 0.5f, g1, d1, d2;
    if (w2 >= 1.0f)
        w2 -= 1.0f;
    g1 = tri01(w1);
    d1 = base + (up > 0.0f ? (float)DLY_SHIFT_G * (1.0f - w1) : 0.5f * (float)DLY_SHIFT_G * w1);
    d2 = base + (up > 0.0f ? (float)DLY_SHIFT_G * (1.0f - w2) : 0.5f * (float)DLY_SHIFT_G * w2);
    return g1 * dly_tap(buf, len, pos, d1, live) + (1.0f - g1) * dly_tap(buf, len, pos, d2, live);
}

void fx_pre(float *l, float *r, uint32_t n, const fx_params_t *p)   /* the ensemble and the delay, in place */
{
    uint32_t i;
    float bus_pk = block_peak(l, r, n);  /* what the next stage gets: silence into a silent stage lets it rest */
    /* ensemble */
    {
        uint32_t ens_on = !(p->bypass & FX_BYP_ENS) && p->ens_mode != ENS_OFF && p->ens_mix > 0.001f;
        if (bus_pk > 0.0f && ens_on)
            ens_tail = ENS_LEN / n + 2u; /* its line holds the last 11.6 ms */
        else if (ens_tail)
            ens_tail--;
    }
    if (ens_tail) {
        float inc1 = 0.513f / FS, inc2 = 0.863f / FS, depth, base = 0.0035f * FS, wet = 0.8f * fm_sqrtf(p->ens_mix),
              dry = 1.0f - 0.4f * p->ens_mix;   /* heard from the first turn: the wet rises as a square root */
        depth = (p->ens_mode == ENS_CHORUS1 ? 0.0012f : p->ens_mode == ENS_CHORUS2 ? 0.0018f : 0.0015f) * FS;
        for (i = 0; i < n; i++) {
            float m, dl, dr, wl, wr;
            uint32_t b0;
            ens_ph1 += inc1;
            if (ens_ph1 >= 1.0f)
                ens_ph1 -= 1.0f;
            ens_ph2 += inc2;
            if (ens_ph2 >= 1.0f)
                ens_ph2 -= 1.0f;
            m = p->ens_mode == ENS_CHORUS1 ? tri(ens_ph1) : p->ens_mode == ENS_CHORUS2 ? tri(ens_ph2)
                                                                                       : 0.5f * (tri(ens_ph1) + tri(ens_ph2));
            ens_l[ens_pos] = l[i];
            ens_r[ens_pos] = r[i];
            dl = base + depth * m;
            dr = base - depth * m;
            b0 = (uint32_t)dl;
            {
                uint32_t i0 = (ens_pos + ENS_LEN - b0) & (ENS_LEN - 1u), i1 = (i0 + ENS_LEN - 1u) & (ENS_LEN - 1u);
                float f = dl - (float)b0;
                wl = ens_l[i0] + (ens_l[i1] - ens_l[i0]) * f;
            }
            b0 = (uint32_t)dr;
            {
                uint32_t i0 = (ens_pos + ENS_LEN - b0) & (ENS_LEN - 1u), i1 = (i0 + ENS_LEN - 1u) & (ENS_LEN - 1u);
                float f = dr - (float)b0;
                wr = ens_r[i0] + (ens_r[i1] - ens_r[i0]) * f;
            }
            ens_pos = (ens_pos + 1u) & (ENS_LEN - 1u);
            l[i] = l[i] * dry + wl * wet;
            r[i] = r[i] * dry + wr * wet;
        }
        pk_ens = block_peak(l, r, n);
        bus_pk = pk_ens;
    } else {
        float inc1 = 0.513f / FS, inc2 = 0.863f / FS;
        for (i = 0; i < n; i++) {        /* at rest or off: its LFOs keep time, sample by sample, and the line empties
                                          * (switched on again, it does not replay what passed when it went off) */
            ens_ph1 += inc1;
            if (ens_ph1 >= 1.0f)
                ens_ph1 -= 1.0f;
            ens_ph2 += inc2;
            if (ens_ph2 >= 1.0f)
                ens_ph2 -= 1.0f;
            ens_l[ens_pos] = ens_r[ens_pos] = 0.0f;
            ens_pos = (ens_pos + 1u) & (ENS_LEN - 1u);
        }
        pk_ens = 0.0f;
    }
    /* the delay: mono in (through CHOP's gate), down to the half rate, its TYPE's heads at a wandering distance,
     * written back filtered and soft, back up to the full rate */
    {
        uint32_t want = p->dly_type < DT_COUNT ? p->dly_type : DT_PING;
        if ((p->dly_send > 0.001f && bus_pk > 0.0f) || dly_live)
            dly_tail = 2u * DLY_LEN / n + 2u;   /* the whole buffer: nothing in or out that long, and it is all zeros */
        else if (dly_tail)
            dly_tail--;
        if (want != dly_type_cur && (!dly_tail || (p->bypass & FX_BYP_DLY))) {   /* silent or skipped: at once */
            dly_clear();
            dly_type_cur = want;
            dly_fade = 1.0f;
        }
    }
    {
        uint32_t ty = dly_type_cur, two = ty == DT_PING || ty == DT_RISE || ty == DT_FALL;
        float wa = 400.0f * p->dly_wow * p->dly_wow, wfl = 14.1f * fm_sqrtf(p->dly_wow);   /* WOW: the wow's and the
                                         * flutter's reach back, in samples; 40 of 127 is the 3 and 12 cents of before */
        float tmax, tgt = p->dly_time_s * FSH - (float)DLY_AP_SUM;
        if (ty == DT_TAPE) {             /* the worn tape sways far enough to be heard against a held chord: at WOW's
                                          * 40 about 22 cents either way, at full 57; it reaches back further, which
                                          * its one long line has room for */
            wa = 600.0f * (0.12f + p->dly_wow);
            wfl *= 1.5f;
        }
        tmax = two ? (float)(DLY_LEN_L - 64u) - DLY_WOB_MAX - (ty == DT_PING ? 0.0f : (float)DLY_SHIFT_G)
                   : (float)(DLY_LEN - 64u) - DLY_WOB_MAX;
        if (ty == DT_REVERSE)            /* its heads reach back twice the window */
            tmax = 0.5f * ((float)(DLY_LEN - 64u) - DLY_WOB_MAX) - (float)DLY_AP_SUM;
        if (tgt > tmax)
            tgt = tmax;
        if (tgt < 32.0f)
            tgt = 32.0f;
        if (dly_tail && !(p->bypass & FX_BYP_DLY)) {
            float slew = 0.0008f, fb = 0.95f * fm_sqrtf(p->dly_fdbk), send = p->dly_send, g = 0.6f * p->dly_diff,
                  maxr = (float)(DLY_LEN_R - 64u) - DLY_WOB_MAX - (ty == DT_PING ? 0.0f : (float)DLY_SHIFT_G),
                  col = p->dly_colour, ratio_t = p->dly_synced ? 1.0f : DLY_RATIO,
                  lpc = 1.0f - fm_expf(-TWO_PI * (1500.0f + 7500.0f * col * col) / FSH),   /* 1.5 to 9 kHz */
                  hpa = fm_expf(-TWO_PI * (30.0f + 170.0f * col) / FSH),                   /* 30 to 200 Hz */
                  drive = 1.2f, per = p->dly_time_s * FSH, chop_inc, duty = 1.0f - 0.9f * p->dly_chop,
                  chop_mk = 1.0f / fm_sqrtf(duty),   /* the slices as loud on average as the whole: louder each */
                  sh_inc = (ty == DT_RISE ? 1.0f : 0.5f) / (float)DLY_SHIFT_G, up = ty == DT_RISE ? 1.0f : -1.0f;
                                         /* the feedback as a square root: repeats from the first turn, 0.95 at full */
            float wa0, wa1, wf0, wf1, dwa, dwf;
            uint32_t h = n / 2u, want = p->dly_type < DT_COUNT ? p->dly_type : DT_PING;
            int32_t live = 0;
            if (ty == DT_TAPE) {         /* darker and harder each time round */
                lpc *= 0.4f;
                drive = 2.0f;
            }
            if (per < 32.0f)
                per = 32.0f;
            chop_inc = 1.0f / per;
            wa0 = wa * 0.5f * (1.0f + fm_sin_turns(wow_ph));   /* wow and flutter: the ends of the block, a line
                                                                 * between; one-sided, so a head never reads ahead */
            wf0 = wfl * 0.5f * (1.0f + fm_sin_turns(flut_ph));
            wow_ph += 0.35f * (float)n / FS;
            if (wow_ph >= 1.0f)
                wow_ph -= 1.0f;
            flut_ph += 6.3f * (float)n / FS;
            if (flut_ph >= 1.0f)
                flut_ph -= 1.0f;
            wa1 = wa * 0.5f * (1.0f + fm_sin_turns(wow_ph));
            wf1 = wfl * 0.5f * (1.0f + fm_sin_turns(flut_ph));
            dwa = (wa1 - wa0) / (float)h;
            dwf = (wf1 - wf0) / (float)h;
            for (i = 0; i < n; i += 2u) {
                float in, outl, outr, c, m, x0, x1, wob;
                x0 = (l[i] + r[i]) * 0.5f * send;           /* two one-poles at 7 kHz, then every other sample */
                dly_dn1 += (x0 - dly_dn1) * 0.63f;
                dly_dn2 += (dly_dn1 - dly_dn2) * 0.63f;
                x1 = (l[i + 1u] + r[i + 1u]) * 0.5f * send;
                dly_dn1 += (x1 - dly_dn1) * 0.63f;
                dly_dn2 += (dly_dn1 - dly_dn2) * 0.63f;
                chop_ph += chop_inc;                        /* CHOP: the input let in for the first part of each
                                                             * delay time, its edges 2 ms soft */
                if (chop_ph >= 1.0f)
                    chop_ph -= 1.0f;
                chop_g += ((chop_ph < duty ? 1.0f : 0.0f) - chop_g) * 0.02f;
                in = dly_dn2 * chop_g * chop_mk;
                if (ty == DT_HEADS) {                       /* a slice in at the start of each DLY TIME, the rest at a
                                                             * quarter: the three heads make a held chord a triplet,
                                                             * left, centre, right; a note's onset opens it whole */
                    float a = fm_fabsf(dly_dn2), gate;
                    on_fast += (a - on_fast) * 0.02f;
                    on_slow += (a - on_slow) * 0.0008f;
                    if (on_fast > 1.8f * on_slow + 0.002f)
                        on_hold = 0.12f * FSH;
                    gate = on_hold > 0.0f ? 1.0f : chop_ph < 0.3f ? 1.0f : 0.25f;
                    if (on_hold > 0.0f)
                        on_hold -= 1.0f;
                    in *= gate;
                }
                dly_read += (tgt - dly_read) * slew;       /* a time change bends the pitch, like tape */
                dly_ratio += (ratio_t - dly_ratio) * slew;
                wa0 += dwa;
                wf0 += dwf;
                wob = wa0 + wf0;
                switch (ty) {
                default:
                case DT_PING:
                case DT_RISE:
                case DT_FALL: {
                    float backl = dly_read + wob, backr = dly_read * dly_ratio + wob;
                    if (backr > maxr)
                        backr = maxr;
                    if (ty == DT_PING) {
                        outl = dly_tap(dly, DLY_LEN_L, dly_pos_l, backl, &live);
                        outr = dly_tap(dly + DLY_LEN_L, DLY_LEN_R, dly_pos_r, backr, &live);
                    } else {
                        outl = dly_shift(dly, DLY_LEN_L, dly_pos_l, backl, up, &live);
                        outr = dly_shift(dly + DLY_LEN_L, DLY_LEN_R, dly_pos_r, backr, up, &live);
                        sh_ph += sh_inc;
                        if (sh_ph >= 1.0f)
                            sh_ph -= 1.0f;
                    }
                    c = dly_leg(0, in + outr * fb, g, lpc, hpa, drive);   /* the left leg: the input, and the right's
                                                                           * repeat back */
                    dly_put(dly, dly_pos_l, c, &live);
                    c = dly_leg(1, outl * fb, g, lpc, hpa, drive);        /* the right leg: the left's repeat crossing */
                    dly_put(dly + DLY_LEN_L, dly_pos_r, c, &live);
                    dly_pos_l = dly_pos_l + 1u == DLY_LEN_L ? 0u : dly_pos_l + 1u;
                    dly_pos_r = dly_pos_r + 1u == DLY_LEN_R ? 0u : dly_pos_r + 1u;
                    break;
                }
                case DT_TAPE: {
                    float o = dly_tap(dly, DLY_LEN, dly_pos_w, dly_read + wob, &live);
                    c = dly_leg(0, in + o * fb, g, lpc, hpa, drive);
                    outl = outr = o;
                    break;
                }
                case DT_HEADS: {                            /* a third, two thirds and the whole time back */
                    float t3 = (dly_read + (float)DLY_AP_SUM) * (1.0f / 3.0f), d1 = t3 - (float)DLY_AP_SUM,
                          d2 = 2.0f * t3 - (float)DLY_AP_SUM, h1, h2, h3;
                    d1 = d1 < 16.0f ? 16.0f : d1;
                    d2 = d2 < 16.0f ? 16.0f : d2;
                    h1 = dly_tap(dly, DLY_LEN, dly_pos_w, d1 + wob, &live);
                    h2 = dly_tap(dly, DLY_LEN, dly_pos_w, d2 + wob, &live);
                    h3 = dly_tap(dly, DLY_LEN, dly_pos_w, dly_read + wob, &live);
                    c = dly_leg(0, in + h3 * fb, g, lpc, hpa, drive);   /* the last head is the feedback */
                    outl = 0.6f * h1 + 0.4f * h2 + 0.15f * h3;
                    outr = 0.15f * h1 + 0.4f * h2 + 0.6f * h3;
                    break;
                }
                case DT_REVERSE: {                          /* one head reading back across the last window, twice as
                                                             * fast as the line runs: each window the last DLY TIME
                                                             * backwards, swelling from a quarter to full and cut in
                                                             * 4 % of it, so a held chord breathes in, window by window */
                    float win = dly_read + (float)DLY_AP_SUM, w1 = rev_ph, env, s1;
                    env = 0.25f + 0.75f * w1 * w1;
                    if (w1 > 0.96f)
                        env *= (1.0f - w1) * 25.0f;
                    if (w1 < 0.01f)
                        env *= w1 * 100.0f;
                    s1 = env * dly_tap(dly, DLY_LEN, dly_pos_w, 2.0f + 2.0f * win * w1 + wob, &live);
                    rev_ph += 1.0f / win;
                    if (rev_ph >= 1.0f) {
                        rev_ph -= 1.0f;
                        rev_lr = !rev_lr;               /* each swell from the other side */
                    }
                    c = dly_leg(0, in + s1 * fb, g, lpc, hpa, drive);
                    outl = rev_lr ? 0.8f * s1 : 0.45f * s1;
                    outr = rev_lr ? 0.45f * s1 : 0.8f * s1;
                    break;
                }
                case DT_GRAINS: {                           /* a piece every delay time, from anywhere up to 1.9 s
                                                             * back (more often the recent), 1.6 delay times long
                                                             * (40 to 300 ms), its own place left to right */
                    uint32_t k;
                    float sum = 0.0f;
                    outl = outr = 0.0f;
                    gr_left -= 1.0f;
                    if (gr_left <= 0.0f) {          /* a piece every half DLY TIME, 40 to 150 ms long, from anywhere
                                                     * in the last 1.8 s, at the note, an octave up or down, or a fifth
                                                     * up, its own place left to right: a held chord glitters, notes
                                                     * come back scattered */
                        grain_t *gv = &grn[0];
                        float per = dly_read + (float)DLY_AP_SUM, len = 0.8f * per, pan, q = gr_rand(), pr = gr_rand(),
                              lo, hi;
                        gr_left += 0.5f * per < 880.0f ? 880.0f : 0.5f * per;
                        for (k = 0; k < DLY_NGR; k++)   /* a free one, else the furthest through */
                            if (!grn[k].on || (gv->on && grn[k].pos / grn[k].len > gv->pos / gv->len))
                                gv = &grn[k];
                        len = len < 880.0f ? 880.0f : len > 3300.0f ? 3300.0f : len;
                        gv->rate = pr < 0.35f ? 0.0f : pr < 0.6f ? 1.0f : pr < 0.8f ? -0.5f : 0.5f;
                        lo = 1765.0f + (gv->rate > 0.0f ? gv->rate * len : 0.0f);   /* room for its head to run */
                        hi = (float)(DLY_LEN - 64u) - DLY_WOB_MAX - (gv->rate < 0.0f ? -gv->rate * len : 0.0f);
                        gv->on = 1;
                        gv->pos = 0.0f;
                        gv->len = len;
                        gv->d = lo + q * (hi - lo);
                        pan = 0.9f * (2.0f * gr_rand() - 1.0f);
                        gv->gl = fm_sqrtf(0.5f * (1.0f - pan));
                        gv->gr = fm_sqrtf(0.5f * (1.0f + pan));
                    }
                    for (k = 0; k < DLY_NGR; k++) {
                        grain_t *gv = &grn[k];
                        float t, w, v;
                        if (!gv->on)
                            continue;
                        t = tri01(gv->pos / gv->len);
                        w = t * t * (3.0f - 2.0f * t);   /* a smooth window */
                        v = w * dly_tap(dly, DLY_LEN, dly_pos_w, gv->d + wob, &live);
                        outl += v * gv->gl;
                        outr += v * gv->gr;
                        sum += v;
                        gv->d -= gv->rate;              /* the head runs against the line: the piece's pitch */
                        gv->pos += 1.0f;
                        if (gv->pos >= gv->len)
                            gv->on = 0;
                    }
                    c = dly_leg(0, in + sum * fb * 0.45f, g, lpc, hpa, drive);   /* less back in: pieces an octave
                                                                                     * up would climb */
                    break;
                }
                }
                if (!two) {                                 /* the one line: its write, and on */
                    dly_put(dly, dly_pos_w, c, &live);
                    dly_pos_w = dly_pos_w + 1u == DLY_LEN ? 0u : dly_pos_w + 1u;
                }
                m = (outl + outr) * 0.5f;
                outl = (m + (outl - m) * DLY_WIDTH) * dly_fade * DLY_OG[ty];
                outr = (m + (outr - m) * DLY_WIDTH) * dly_fade * DLY_OG[ty];
                if (want != ty) {                           /* a new TYPE: out over 12 ms, then a fresh start */
                    dly_fade -= 1.0f / 256.0f;
                    if (dly_fade <= 0.0f) {
                        dly_clear();
                        dly_type_cur = ty = want;
                        two = ty == DT_PING || ty == DT_RISE || ty == DT_FALL;
                        dly_fade = 1.0f;
                        sh_inc = (ty == DT_RISE ? 1.0f : 0.5f) / (float)DLY_SHIFT_G;
                        up = ty == DT_RISE ? 1.0f : -1.0f;
                    }
                }
                x0 = (dly_ol + outl) * 0.5f;                /* back up: the midpoint, then the sample, smoothed */
                dly_ul += (x0 - dly_ul) * 0.75f;
                l[i] += dly_ul;
                dly_ul += (outl - dly_ul) * 0.75f;
                l[i + 1u] += dly_ul;
                x1 = (dly_or + outr) * 0.5f;
                dly_ur += (x1 - dly_ur) * 0.75f;
                r[i] += dly_ur;
                dly_ur += (outr - dly_ur) * 0.75f;
                r[i + 1u] += dly_ur;
                dly_ol = outl;
                dly_or = outr;
            }
            dly_live = live != 0;
            for (i = 0; i < 2u; i++) {
                dly_lp[i] = fm_flush(dly_lp[i]);
                dly_hp_x[i] = fm_flush(dly_hp_x[i]);
                dly_hp_y[i] = fm_flush(dly_hp_y[i]);
            }
            dly_dn1 = fm_flush(dly_dn1);
            dly_dn2 = fm_flush(dly_dn2);
            dly_ul = fm_flush(dly_ul);
            dly_ur = fm_flush(dly_ur);
            pk_dly = block_peak(l, r, n);
            bus_pk = pk_dly;
        } else {
            float ratio_t = p->dly_synced ? 1.0f : DLY_RATIO;
            for (i = 0; i < n; i += 2u) {  /* at rest: the time still settles */
                dly_read += (tgt - dly_read) * 0.0008f;
                dly_ratio += (ratio_t - dly_ratio) * 0.0008f;
            }
            wow_ph += 0.35f * (float)n / FS;   /* wow and flutter keep their phase */
            if (wow_ph >= 1.0f)
                wow_ph -= 1.0f;
            flut_ph += 6.3f * (float)n / FS;
            if (flut_ph >= 1.0f)
                flut_ph -= 1.0f;
            pk_dly = 0.0f;
            dly_live = 0;
        }
    }
}

/* the hall's wet for a block of input, written to outl/outr (the dry is not in it); its own function so the second
 * core can run it a block behind the bus */
void fx_hall(const float *l, const float *r, float *outl_b, float *outr_b, uint32_t n, const fx_params_t *p)
{
    uint32_t i;
    float bus_pk = block_peak(l, r, n);
    if ((p->rev_send > 0.001f && bus_pk > 0.0f) || rv_live || p->freeze)
        rev_tail = 80u;                  /* past the longest line (23 blocks): nothing in or out that long, all zeros */
    else if (rev_tail)
        rev_tail--;
    if (rev_tail && !(p->bypass & FX_BYP_REV)) {
        float live = 0.0f, g[8], gm = 0.0f;
        float rt60 = 0.6f + 7.4f * p->rev_decay * p->rev_decay, dampc = 1.0f - (0.03f + 0.55f * p->rev_damp),
              send = p->rev_send * 0.2f, hpc = 0.017f, s8 = 0.35355339f, out_g;
        float shim_g = 0.0f, shim_inc = 1.0f / SHIM_W, shim_lpc = 0.25f, frz_t = p->freeze ? 3.5f : 1.0f, wander[4];
        uint32_t shim_on = p->shimmer > 0.001f && !(p->bypass & FX_BYP_SHIM) && !p->freeze, k;
        rv_mod_ph += 0.45f * (float)n / FS;   /* the wander, once a block: at 0.45 Hz it moves a tenth of a sample
                                              * in a block, nothing a per-sample sine would add */
        if (rv_mod_ph >= 1.0f)
            rv_mod_ph -= 1.0f;
        for (k = 0; k < 4u; k++)
            wander[k] = 6.0f * fm_sin_turns(rv_mod_ph + 0.25f * (float)k);
        for (k = 0; k < 8u; k++) {       /* each line's gain for the same decay time, whatever its length */
            g[k] = fm_expf(-6.908f * (float)RV_L[k] / (rt60 * FS));
            gm += g[k] * 0.125f;
        }
        if (p->freeze) {                 /* frozen: the network rings on its own, nothing in, nothing lost to damping */
            for (k = 0; k < 8u; k++)
                g[k] = 0.9998f;
            gm = 0.9998f;
            dampc = 1.0f;
            send = 0.0f;
        }
        out_g = 0.8f + 4.0f * (1.0f - gm);   /* the lines' sum: 6 dB under the dry at F1's SPACE, 3 under at full (measured) */
        if (shim_on) {                   /* the octave up fed back, less the longer the hall already rings; the loop
                                          * stays bounded up to the cap (measured: a let-go chord dies away at every size) */
            shim_g = p->shimmer * 0.6f * (1.0f - gm);
            if (shim_g > 0.3f)
                shim_g = 0.3f;
        }
        for (i = 0; i < n; i++) {
            float xl = l[i] * send, xr = r[i] * send, al, ar, v[8], y[8], outl, outr, sh = 0.0f;
            if (shim_on) {               /* the last output an octave up: two heads a half grain apart, triangle windows */
                float w1 = shim_ph, w2 = shim_ph + 0.5f, d1, d2, g1, g2, sv;
                uint32_t b, i0, i1;
                if (w2 >= 1.0f)
                    w2 -= 1.0f;
                d1 = 1.0f + SHIM_W * (1.0f - w1);
                d2 = 1.0f + SHIM_W * (1.0f - w2);
                g1 = 1.0f - fm_fabsf(2.0f * w1 - 1.0f);
                g2 = 1.0f - g1;
                b = (uint32_t)d1;
                i0 = (shim_pos + SHIM_LEN - b) & (SHIM_LEN - 1u);
                i1 = (i0 + SHIM_LEN - 1u) & (SHIM_LEN - 1u);
                sv = g1 * ((float)shim[i0] + ((float)shim[i1] - (float)shim[i0]) * (d1 - (float)b));
                b = (uint32_t)d2;
                i0 = (shim_pos + SHIM_LEN - b) & (SHIM_LEN - 1u);
                i1 = (i0 + SHIM_LEN - 1u) & (SHIM_LEN - 1u);
                sv += g2 * ((float)shim[i0] + ((float)shim[i1] - (float)shim[i0]) * (d2 - (float)b));
                sv *= 1.0f / 32767.0f;
                shim_lp += (sv - shim_lp) * shim_lpc;       /* softened: the octaves pile up without glare */
                sh = shim_lp * shim_g;
                shim_ph += shim_inc;
                if (shim_ph >= 1.0f)
                    shim_ph -= 1.0f;
            }
            rv_hp_l += (xl - rv_hp_l) * hpc;   /* the sub stays dry: a high-pass near 120 Hz on the way in */
            rv_hp_r += (xr - rv_hp_r) * hpc;
            xl -= rv_hp_l;
            xr -= rv_hp_r;
            line_put(&rv_pre[0], xl);
            line_put(&rv_pre[1], xr);
            al = line_at(&rv_pre[0], RV_PRE - 1u);
            ar = line_at(&rv_pre[1], RV_PRE - 1u);
            al = allpass(&rv_ap[0], al, 0.65f);   /* one diffusing allpass a side: the network does the rest */
            ar = allpass(&rv_ap[2], ar, 0.65f);
            al += sh;
            ar += sh;
            for (k = 0; k < 8u; k++) {   /* the lines out, damped and decayed; the odd four wander a little */
                v[k] = (k & 1u) ? line_at_f(&rv_fd[k], (float)RV_L[k] + wander[k >> 1]) : line_at(&rv_fd[k], RV_L[k]);
                rv_lp[k] += (v[k] - rv_lp[k]) * dampc;
                v[k] = rv_lp[k] * g[k];
            }
            {                            /* the Hadamard mix, 8 x 8, scaled to keep the energy */
                float a0 = v[0] + v[1], a1 = v[0] - v[1], a2 = v[2] + v[3], a3 = v[2] - v[3];
                float a4 = v[4] + v[5], a5 = v[4] - v[5], a6 = v[6] + v[7], a7 = v[6] - v[7];
                float b0 = a0 + a2, b1 = a1 + a3, b2 = a0 - a2, b3 = a1 - a3;
                float b4 = a4 + a6, b5 = a5 + a7, b6 = a4 - a6, b7 = a5 - a7;
                y[0] = (b0 + b4) * s8;
                y[1] = (b1 + b5) * s8;
                y[2] = (b2 + b6) * s8;
                y[3] = (b3 + b7) * s8;
                y[4] = (b0 - b4) * s8;
                y[5] = (b1 - b5) * s8;
                y[6] = (b2 - b6) * s8;
                y[7] = (b3 - b7) * s8;
            }
            line_put(&rv_fd[0], y[0] + al);
            line_put(&rv_fd[1], y[1] + ar);
            line_put(&rv_fd[2], y[2] - al);
            line_put(&rv_fd[3], y[3] - ar);
            line_put(&rv_fd[4], y[4] + al);
            line_put(&rv_fd[5], y[5] + ar);
            line_put(&rv_fd[6], y[6] - al);
            line_put(&rv_fd[7], y[7] - ar);
            outl = out_g * (v[0] - v[2] + v[4] - v[6]);
            outr = out_g * (v[1] - v[3] + v[5] - v[7]);
            frz_g += (frz_t - frz_g) * 0.0002f;
            outl *= frz_g;
            outr *= frz_g;
            live += al * al + outl * outl + outr * outr;   /* anything at all, in or out: still ringing */
            {                            /* the output into the shimmer's line, whether it reads it or not */
                float c = (outl + outr) * 0.175f * 32767.0f;
                shim[shim_pos] = (int16_t)(c > 32767.0f ? 32767.0f : c < -32767.0f ? -32767.0f : c);
                shim_pos = (shim_pos + 1u) & (SHIM_LEN - 1u);
            }
            if (shim_on) {               /* and the octave up heard straight away, outside the loop */
                float so = shim_lp * p->shimmer * 6.0f;
                outl += so;
                outr += so;
            }
            outl_b[i] = outl;
            outr_b[i] = outr;
        }
        for (k = 0; k < 8u; k++)
            rv_lp[k] = fm_flush(rv_lp[k]);
        rv_hp_l = fm_flush(rv_hp_l);
        rv_hp_r = fm_flush(rv_hp_r);
        shim_lp = fm_flush(shim_lp);
        rv_live = live > 0.0f;
        pk_rev = block_peak(outl_b, outr_b, n);
    } else {
        rv_mod_ph += 0.45f * (float)n / FS;   /* at rest, the wander keeps its phase */
        if (rv_mod_ph >= 1.0f)
            rv_mod_ph -= 1.0f;
        for (i = 0; i < n; i++)
            outl_b[i] = outr_b[i] = 0.0f;
        pk_rev = 0.0f;
        rv_live = 0;
    }
}

void fx_post(float *l, float *r, uint32_t n)   /* the limiter: 1.5 ms of lookahead, so the gain ramps down ahead of
                                                * a peak instead of cutting into it (a cut is a click: measured, eight
                                                * GLASS bodies held it 4 dB down with an instant attack), 250 ms
                                                * release, so nothing the effects add can clip */
{
    uint32_t i;
    float rel = 1.0f / (0.25f * FS), thr = 0.95f, ka = 4.0f / (float)LIM_LA;   /* at the peak the gain is 98 % there */
    for (i = 0; i < n; i++) {
        float pk = fm_fabsf(l[i]) > fm_fabsf(r[i]) ? fm_fabsf(l[i]) : fm_fabsf(r[i]);
        float need = pk > thr ? thr / pk : 1.0f, ol = lim_dl[lim_pos], or_ = lim_dr[lim_pos];
        if (need < lim_tgt)
            lim_tgt = need;
        else
            lim_tgt += (1.0f - lim_tgt) * rel;
        if (lim_gain > lim_tgt)
            lim_gain += (lim_tgt - lim_gain) * ka;
        else
            lim_gain = lim_tgt;
        if (lim_gain < lim_min)
            lim_min = lim_gain;
        lim_dl[lim_pos] = l[i];
        lim_dr[lim_pos] = r[i];
        lim_pos = (lim_pos + 1u) & (LIM_LA - 1u);
        l[i] = ol * lim_gain;
        r[i] = or_ * lim_gain;
    }
}
