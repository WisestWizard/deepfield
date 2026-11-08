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
#define DR_ARENA_BYTES (235u * 1024u)    /* what dr_init wants from the app's pool: lines, tables, mix and hall buffers */

enum { M_SOLAR, M_VOID, M_GLASS, M_STRING, M_CHOIR, M_BELL, M_SHEPARD, M_FOLD, M_SCAN, M_DX, M_TEAR, M_TAPE, M_RESON,
       M_COUNT };
#define DR_DX_VOICES 8u                  /* DX: SHAPE picks one of eight four-operator voices */
extern const char *const DR_DX_NAME[DR_DX_VOICES];
extern const char *const DR_SCAN_NAME[3];   /* SCAN: the three tables SHAPE sweeps through */
extern const char *const DR_RES_NAME[4];    /* RESONATOR: the materials SHAPE runs through */
float dr_tear_ratio(float shape);           /* TEAR: SHAPE (0..1) as the sync's ratio to the note, 1 to 8 */
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
    P_ORB_PROG,                                                   /* CYCLE: the progression it walks */
    P_SPREAD, P_CROSS, P_TWALK, P_SWAY,                           /* MOTION 2: the bodies apart, coupled, drifting in timbre, swaying */
    P_ECHO_DECAY, P_ECHO_JUMP, P_ORB_ROLL,                        /* ECHO's decay and octave jumps; PULSE's roll */
    P_DLY_DIFF,                                                   /* the delay's repeats smeared into a cloud */
    P_DLY_COLOUR,                                                 /* the delay's repeats dark and full to bright and thin */
    P_DLY_TYPE, P_DLY_WOW, P_DLY_CHOP,                            /* the delay's kind, its tape's wander, its input in slices */
    P_ARP_RHYTHM, P_ARP_DIV,                                      /* ARP: the runner's pattern of hits and rests, its step */
    P_COUNT
};
enum { DT_PING, DT_TAPE, DT_HEADS, DT_REVERSE, DT_RISE, DT_FALL, DT_GRAINS, DT_COUNT };   /* P_DLY_TYPE, the index */
enum { OM_WANDER, OM_CHORD, OM_SCATTER, OM_ARP, OM_CYCLE, OM_ECHO, OM_PULSE, OM_COUNT };   /* P_ORB_MODE, the index */
enum { AO_UP, AO_DOWN, AO_UPDOWN, AO_RANDOM, AO_PLAYED, AO_COUNT };   /* P_ARP_ORDER, the index; P_ARP_OCT: 0..2 = 1..3 octaves */
#define DR_RHYTHMS 8u                    /* P_ARP_RHYTHM, the index: 8/8 5/8 3/8 7/12 5/12 4/9 7/16 5/16 */
#define DR_DIVS 9u                       /* P_ARP_DIV, the index: 1/16 1/16T 1/8 1/8T 1/8D 1/4 1/4T 1/2 1 BAR */
extern const char *const DR_RHYTHM_NAME[DR_RHYTHMS];
extern const char *const DR_DIV_NAME[DR_DIVS];

#define SCENE_NAME 12u
#define DR_LVL_MUTE (-25)               /* a body's LEVEL: -24 .. +6 dB, or muted (still latched, still played by ARP) */
typedef struct {
    char name[SCENE_NAME];
    uint8_t p[P_COUNT];                  /* every parameter 0..127 */
    uint8_t note[DR_NV], vel[DR_NV];     /* the bodies latched when it was saved (vel 0: none) */
    uint8_t fx_off;                      /* the effect switches off (DR_FX_* bits), saved with the scene */
    uint8_t pins;                        /* bit k: body k is pinned (ORBIT and the arp leave it alone) */
    uint8_t root;                        /* the ROOT its chord stands in, + 1 (0: the chord as it is, never moved) */
    uint8_t rsv;
    int8_t lvl[DR_NV], tone[DR_NV], tune[DR_NV];   /* each body's own LEVEL (dB), TONE (MASS, %) and TUNE (cents) */
} scene_t;

extern scene_t dr_scene;                 /* the live scene: the UI writes it, the render reads it */

int dr_init(void *arena, uint32_t bytes);           /* 0 = ok, -1 = arena too small */
void dr_param(uint32_t id, uint32_t v);             /* a knob: the scene's value, heard at once (no morph) */
/* a scene: its values morph in over seconds (0: at once), and each body sounding crosses into its model over them;
 * chord: its bodies too, struck with their own values, the latched ones not in it let go (else the bodies stay) */
void dr_load(const scene_t *s, float seconds, int chord);
int dr_morphing(void);
float dr_morph_pos(void);                            /* 0..1 */
float dr_heard(uint32_t id);                         /* a parameter as heard now, 0..127 (a morph lies between scenes) */
/* the keys (main loop, queued): on strikes a body that sounds while the key is held, or lets a latched body at
 * that note go; off lets an unlatched body go. The engine sends the note offs of the keys' bodies (dr_note_out) */
void dr_note(uint32_t note, uint32_t vel, int on);
void dr_note_latched(uint32_t note, uint32_t vel);  /* LATCH on: strikes a body that stays; at a latched body's note,
                                                     * lets it go; a latched body ORBIT moved away from this note is
                                                     * left where it sounds (it is struck there from now on) */
int32_t dr_body_at(uint32_t note);                  /* the latched body sounding at this note (where ORBIT or
                                                     * TRANSPOSE took it): the note it is known by, else -1 */
void dr_let_go(uint32_t home);                      /* the latched body known by this note fades */
void dr_unlatch(uint32_t note);                     /* LATCH off while its key is held: the body sounds while held */
void dr_transpose_to(uint32_t note);                /* the latched bodies moved together, the lowest to this note */
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
/* a latched body's own values, by the note it was struck at: LEVEL (dB, DR_LVL_MUTE), TONE (-50..50 % of MASS),
 * TUNE (-50..50 cents), PIN (1: ORBIT, PULSE and the arp leave it where it is; TRANSPOSE still moves it) */
enum { BE_LEVEL, BE_TONE, BE_TUNE, BE_PIN, BE_COUNT };
void dr_body_edit(uint32_t note, uint32_t what, int32_t v);       /* the keys (main loop, queued) */
void dr_body_edit_now(uint32_t note, uint32_t what, int32_t v);   /* MIDI NRPN, from the render */
int32_t dr_body_value(uint32_t note, uint32_t what);             /* 0 when there is no such body */
void dr_pressure(uint32_t note, uint32_t v);        /* poly aftertouch, 0..127: brighter, more wobble */
#ifdef PLAT_HOST
uint32_t dr_press_at(uint32_t note);               /* the pressure remembered for a note (the simulator) */
#endif
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
void dr_orbit_now(void);                            /* SEL + PLAY (or CC 14): the move coming comes now (none planned:
                                                     * one is chosen, whatever CHANCE says); ARP: the next step now;
                                                     * then the countdown starts over */
int dr_orbit_on(void);
float dr_orbit_next(void);                          /* how far to the next move, 0..1 (0 while off); ARP: through
                                                     * its bar of eight steps */
uint32_t dr_orbit_mode(void);                       /* OM_*, from the scene */
int dr_orbit_pending(uint32_t i, uint32_t *to);     /* body i has a move planned: 1 and its destination note */
#define DR_PROGS 8u
uint32_t dr_orbit_cycle_pos(void);                  /* CYCLE: the progression's step the chord stands on now */
int32_t dr_arp_note(void);                          /* ARP: the note of the step sounding now, -1 none */
int dr_arp_running(void);                           /* ARP runs and has notes (its own, or remembered) */
int dr_echo_running(void);                          /* ECHO runs and holds a phrase */
/* MIDI clock (from the render, app.c): while clocks come, the runner steps at its DIV in clocks and ORBIT counts
 * its period in beats (a quarter and a half beat below one); two seconds without one and the knobs' own time
 * returns. Clock out: while the runner runs from RATE, 24 a beat (and start / stop) through dr_realtime_out */
void dr_clock(int port);                            /* port: the input it came in on (only one is followed) */
void dr_clock_start(int port);
uint32_t dr_orbit_clocks(uint32_t p);               /* EVERY (p) in MIDI clocks at the clock's tempo */
void dr_clock_stop(void);
int dr_clock_ext(void);                             /* following MIDI clock now */
float dr_clock_bpm(void);                           /* the tempo followed (0 none) */
void dr_clock_out(int on);
void dr_realtime_out(void (*fn)(uint32_t status));   /* F8 / FA / FC out (render context) */
uint32_t dr_arp_steps(void);                        /* ARP: notes played so far (the RHYTHM's rests not counted) */
float dr_arp_bpm(uint32_t p);                       /* RATE as BPM (a beat; a step is DIV of it) */
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
    float target;                        /* where it is going (or stands): the key that addresses it */
    float level;                         /* 0..1, what is heard */
    uint8_t vel;                         /* the velocity it was struck at, 1..127 (what a save stores) */
    float spark;                         /* its breathing, 0..1 */
    int8_t lvl, tone, tune;              /* its own values (dr_body_edit) */
    uint8_t pinned;
} dr_body_t;
uint32_t dr_bodies(void);                /* bodies sounding */
int dr_body(uint32_t i, dr_body_t *b);   /* body i (0..7): 0 = silent */
int dr_lead_body(uint32_t i, dr_body_t *b);   /* lead i (0..DR_NL-1): 0 = silent; home its key's note, pitch with LIFT */
int32_t dr_newest_note(void);            /* the newest body's note, -1 none */
int dr_sounding(uint32_t note);          /* a body struck at this note is sounding and not fading */
float dr_out_level(void);                /* the output's peak, last block, 0..1 */
float dr_bus_level(void);
float dr_time_s(uint32_t p);             /* the UI shows the engine's own mappings */
float dr_orbit_s(uint32_t p);           /* EVERY: 0.1 .. 60 s */
float dr_dly_time_s(uint32_t p);                    /* DLY TIME below 100: free, 50 to 800 ms */
int32_t dr_dly_div(uint32_t p);                     /* DLY TIME from 100: a division of the beat (DR_DLY_DIV_NAME), else -1 */
#define DR_DLY_MAX_S 0.8f
#define DR_DLY_NDIV 7u
extern const char *const DR_DLY_DIV_NAME[DR_DLY_NDIV];
float dr_hz(uint32_t p);                 /* VOID's cutoff for a MASS value */
