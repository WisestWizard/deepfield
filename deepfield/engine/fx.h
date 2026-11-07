/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann */
/* The master effects, in order: ensemble (Juno-style chorus, two LFOs, antiphase left and right), tape delay
 * (one line, wow and flutter, dark saturated feedback), a hall (an eight-line feedback delay network, 16-bit
 * lines, pre-delay) with a shimmer (its output an octave up, fed back in) and a freeze (the network holds what it
 * has), a peak limiter. Stereo float
 * in place; memory from the arena. From Pocket Poly's fx.c. The delay runs at half the rate on one buffer of 1.91 s,
 * used by its TYPE as two legs (0.80 s left, 1.11 s right: PING, RISE, FALL) or as one line (TAPE, HEADS, REVERSE,
 * GRAINS); two allpasses a leg smear the repeats by DIFFUSE, COLOUR sets the feedback's low-pass and high-pass, WOW
 * the tape's wander, CHOP lets the input in only in slices of the delay time. */
#pragma once
#include <stdint.h>

typedef struct {
    uint32_t ens_mode;                   /* ENS_* */
    float ens_mix;                       /* 0..1 */
    uint32_t dly_type;                   /* DT_* */
    uint32_t dly_synced;                 /* TIME is a division of the beat: PING's right leg as long as the left */
    float dly_time_s;                    /* 0.05 .. 0.8: the left leg (PING's right is 1.382 of it free); GRAINS: the
                                          * time between grains */
    float dly_fdbk;                      /* 0..1 */
    float dly_send;                      /* 0..1 */
    float dly_diff;                      /* 0..1: the repeats smeared into a cloud */
    float dly_colour;                    /* 0..1: dark and full to bright and thin */
    float dly_wow;                       /* 0..1: the tape's wander, clean to seasick */
    float dly_chop;                      /* 0..1: everything in, to a tenth of each delay time */
    float rev_decay;                     /* 0..1: the decay time, 0.6 to 8 s */
    float rev_damp;                      /* 0..1 */
    float rev_send;                      /* 0..1 */
    float shimmer;                       /* 0..1: the octave-up feedback into the hall */
    uint32_t freeze;                     /* the hall holds: no input, no decay, no damping */
    uint32_t bypass;                     /* FX_BYP_*: a stage skipped, its lines left as they are */
} fx_params_t;

#define FX_BYP_ENS 1u
#define FX_BYP_DLY 2u
#define FX_BYP_REV 4u
#define FX_BYP_SHIM 8u

#define FX_ENS_LEN 512u                  /* 11.6 ms a channel, float */
#define FX_DLY_LEN 42240u                /* two legs, 0.80 s and 1.11 s at half the rate, int16 */
#define FX_DLY_AP 796u                   /* the legs' four allpasses, int16 */
#define FX_RV_TOTAL 33291u               /* the hall's fourteen lines, int16 (fx.c checks the sum) */
#define FX_SHIM_LEN 4096u                /* the shimmer's window, int16 */
#define FX_BYTES (FX_ENS_LEN * 2u * 4u + FX_DLY_LEN * 2u + FX_DLY_AP * 2u + FX_RV_TOTAL * 2u + FX_SHIM_LEN * 2u + 64u)

uint32_t fx_bytes(void);                 /* the arena the effects need (FX_BYTES) */
void fx_init(void *arena);
void fx_pre(float *l, float *r, uint32_t n, const fx_params_t *p);    /* the ensemble and the delay, in place */
void fx_hall(const float *l, const float *r, float *outl, float *outr, uint32_t n, const fx_params_t *p);
                                         /* the hall's wet of this block of input, written to outl/outr; it may run
                                          * on the second core, a block behind the bus */
void fx_post(float *l, float *r, uint32_t n);   /* the limiter */
void fx_peaks(float *ens, float *dly, float *rev);   /* each stage's output peak in the last block it ran */
uint32_t fx_running(void);               /* the stages not at rest: 1 ensemble, 2 delay, 4 reverb */
float fx_lim_min(void);                 /* the limiter's deepest gain since the last call, 0..1 (1: idle) */
