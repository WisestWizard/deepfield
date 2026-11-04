/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann */
/* DEEP FIELD's engine: eight drone bodies, each a sustained voice that swells in and fades out, in one of four
 * models, through the effects (fx.c). Its own -O2 units: float on the FPU, no platform calls but the second
 * core's, memory from an arena the app hands over at init. The app owns the live scene (dr_scene) and hands
 * note and controller events in. */
#pragma once
#include <stdint.h>

#define DR_FS 44117.6f                   /* the codec's real rate: 192 MHz / 17 / 256 */
#define DR_CTL 32u                       /* samples per control tick */
#define DR_NV 8u                         /* bodies */
#define DR_NL 4u                         /* leads: the keys with LATCH off, the scene's model struck short */
#define DR_ARENA_BYTES (230u * 1024u)    /* what dr_init wants from the app's pool: lines, tables, mix and hall buffers */

enum { M_SOLAR, M_VOID, M_GLASS, M_STRING, M_CHOIR, M_BELL, M_SHEPARD, M_FOLD, M_COUNT };
enum { ENS_OFF, ENS_CHORUS1, ENS_CHORUS2, ENS_ENSEMBLE, ENS_COUNT };
enum {
    P_MODEL, P_MASS, P_DRIFT, P_SPACE, P_TIDE,                    /* the model, and the four forces (HOME) */
    P_SHAPE, P_SUB, P_AIR,                                        /* BODY */
    P_SWELL, P_FADE, P_PRESS, P_GLIDE,                            /* ENVELOPE */
    P_TIDE_RATE, P_WOBBLE, P_WALK, P_WIDTH,                       /* MOTION */
    P_REV_SIZE, P_REV_DAMP, P_SHIMMER, P_DLY_MIX,                 /* SPACE 1 */
    P_DLY_TIME, P_DLY_FDBK, P_ENS, P_ENS_MIX,                     /* SPACE 2 */
    P_ORB_PERIOD, P_ORB_CHANCE, P_ORB_RANGE, P_GRAVITY,           /* ORBIT */
    P_NZ_LEVEL, P_NZ_COLOUR, P_NZ_WIND, P_NZ_BAND,                /* NOISE: the layer under the bodies */
    P_LD_ATTACK, P_LD_RELEASE, P_LD_LIFT, P_LD_LEVEL,             /* LEAD: the keys with LATCH off */
    P_ORB_MODE, P_ORB_DIST, P_ORB_BODIES,                         /* ORBIT: the mode, CHORD's distance, SCATTER's count */
    P_ARP_RATE, P_ARP_GATE, P_ARP_ORDER, P_ARP_OCT,               /* ARP: the runner's rate, gate, order, octaves */
    P_NZ_TEXTURE, P_NZ_GUST, P_NZ_DUCK, P_NZ_WIDTH,               /* NOISE 2: its texture, weather, ducking, width */
    P_COUNT
};
enum { OM_WANDER, OM_CHORD, OM_SCATTER, OM_ARP, OM_COUNT };        /* P_ORB_MODE, stored as the index */
enum { AO_UP, AO_DOWN, AO_UPDOWN, AO_RANDOM, AO_PLAYED, AO_COUNT };   /* P_ARP_ORDER, the index; P_ARP_OCT: 0..2 = 1..3 octaves */

#define SCENE_NAME 12u
typedef struct {
    char name[SCENE_NAME];
    uint8_t p[P_COUNT];                  /* every parameter 0..127 */
    uint8_t note[DR_NV], vel[DR_NV];     /* the bodies latched when it was saved (vel 0: none) */
    uint8_t fx_off;                      /* the effect switches off (DR_FX_* bits), saved with the scene */
    uint8_t rsv[3];
} scene_t;

extern scene_t dr_scene;                 /* the live scene: the UI writes it, the render reads it */

int dr_init(void *arena, uint32_t bytes);           /* 0 = ok, -1 = arena too small */
void dr_param(uint32_t id, uint32_t v);             /* a knob: the scene's value, heard at once (no morph) */
void dr_load(const scene_t *s, float seconds);      /* a scene: its values morph in over seconds, its bodies swell
                                                     * in and the others fade (0 s: at once) */
int dr_morphing(void);
float dr_morph_pos(void);                            /* 0..1 */
/* the keys (main loop, queued): on strikes a body that sounds while the key is held, or lets a latched body at
 * that note go; off lets an unlatched body go. The engine sends the note offs of the keys' bodies (dr_note_out) */
void dr_note(uint32_t note, uint32_t vel, int on);
void dr_note_latched(uint32_t note, uint32_t vel);  /* LATCH on: strikes a body that stays; at a latched body's note,
                                                     * lets it go */
void dr_unlatch(uint32_t note);                     /* LATCH off while its key is held: the body sounds while held */
void dr_note_now(uint32_t note, uint32_t vel, int on);   /* MIDI, from the render: held while on, let go at off */
/* LATCH off: a key (or a MIDI note) plays a lead, the scene's model with the LEAD envelope (ATTACK up, held, then
 * RELEASE), LIFT semitones up, never latched, never moved by ORBIT; the bed of latched bodies is left alone */
void dr_lead(uint32_t note, uint32_t vel, int on);  /* the keys (main loop, queued) */
void dr_lead_now(uint32_t note, uint32_t vel, int on);   /* MIDI, from the render */
uint32_t dr_leads(void);                            /* leads sounding */
float dr_attack_s(uint32_t p);
float dr_release_s(uint32_t p);
void dr_latch_hold(void);                           /* SEL: the bodies sounding from the keys are latched (they stay
                                                     * when their keys are released); none to latch: every latched
                                                     * body is let go */
uint32_t dr_latched(void);                          /* bodies latched */
int dr_body_latched(uint32_t note);                 /* the body struck at this note is latched */
void dr_pressure(uint32_t note, uint32_t v);        /* poly aftertouch, 0..127: brighter, more wobble */
void dr_channel_pressure(uint32_t v);
void dr_wheel(uint32_t v);                          /* CC 1: wobble */
void dr_bend(uint32_t bend14);                      /* +-2 semitones */
void dr_sustain(uint32_t on);                       /* CC 64: MIDI notes stay after their note off */
void dr_volume(uint32_t cc);                        /* CC 7 */
void dr_all_off(void);                              /* every body fades */
void dr_panic(void);                                /* silence now */
void dr_scale(uint32_t mask12, uint32_t root);      /* the scale ORBIT moves within (bit n: semitone n from root) */
uint32_t dr_snap(uint32_t note);                    /* the scale note at or below (the keys use it) */
void dr_orbit(int on);
int dr_orbit_on(void);
float dr_orbit_next(void);                          /* how far to the next move, 0..1 (0 while off); ARP: through
                                                     * its bar of eight steps */
uint32_t dr_orbit_mode(void);                       /* OM_*, from the scene */
int dr_orbit_pending(uint32_t i, uint32_t *to);     /* body i has a move planned: 1 and its destination note */
int32_t dr_arp_note(void);                          /* ARP: the note of the step sounding now, -1 none */
int dr_arp_running(void);                           /* ARP runs and has notes (its own, or remembered) */
uint32_t dr_arp_steps(void);                        /* ARP: steps so far */
float dr_arp_bpm(uint32_t p);                       /* RATE as BPM (a step is an eighth) */
void dr_freeze(int on);                             /* the reverb holds what it has, for as long as it is on */
int dr_frozen(void);
#define DR_FX_ENS 1u
#define DR_FX_DLY 2u
#define DR_FX_REV 4u
#define DR_FX_SHIM 8u
#define DR_FX_NOISE 16u                  /* the noise layer is on (its LEVEL above zero); not a bypass */
void dr_fx_bypass(uint32_t bits);                   /* live, not the scene's */
uint32_t dr_fx_bypass_get(void);
uint32_t dr_fx_active(void);                        /* on in the scene and not bypassed (and DR_FX_NOISE) */
/* the noise layer for the screen: how loud now (0..1), where its band sits (a MIDI note), its colour and band */
void dr_noise(float *level, float *note, float *colour, float *band);
float dr_noise_gust(void);                          /* the gust's envelope now, 0..1 */
uint32_t dr_noise_gusts(void);                      /* gusts so far */
void dr_note_out(void (*fn)(uint32_t note, uint32_t vel, int on));   /* the keys' note offs and ORBIT's moves, for MIDI out
                                                                      * (render context) */
uint32_t dr_orbit_moved(uint32_t *from, uint32_t *to);   /* moves so far, and the last one's notes */
void dr_overload(void);                             /* the render ran long: one body fewer until a restart */
void dr_render(int32_t *lr, uint32_t n, uint32_t master_q12);

/* for the screen */
typedef struct {
    uint8_t on;                          /* sounding (swelling, held or fading) */
    uint8_t fading;
    uint8_t latched;
    uint8_t home;                        /* the note it was struck at */
    float pitch;                         /* where it is now, MIDI note (ORBIT and glide move it) */
    float level;                         /* 0..1, what is heard */
    float spark;                         /* its breathing, 0..1 */
} dr_body_t;
uint32_t dr_bodies(void);                /* bodies sounding */
int dr_body(uint32_t i, dr_body_t *b);   /* body i (0..7): 0 = silent */
int dr_lead_body(uint32_t i, dr_body_t *b);   /* lead i (0..DR_NL-1): 0 = silent; home its key's note, pitch with LIFT */
int32_t dr_newest_note(void);            /* the newest body's note, -1 none */
int dr_sounding(uint32_t note);          /* a body struck at this note is sounding and not fading */
float dr_out_level(void);                /* the output's peak, last block, 0..1 */
float dr_bus_level(void);
void dr_scope(const int16_t **l, const int16_t **r, uint32_t *n);   /* the last block's output, for a scope */
uint32_t dr_harmonics(uint32_t i, float *amp);     /* body i's 8 harmonic levels (SOLAR; the others approximated) */
float dr_time_s(uint32_t p);             /* the UI shows the engine's own mappings */
float dr_orbit_s(uint32_t p);
float dr_dly_time_s(uint32_t p);
float dr_hz(uint32_t p);                 /* VOID's cutoff for a MASS value */
