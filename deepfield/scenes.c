/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann */
/* What DEEP FIELD saves: the settings (octave, scale, latch, the view, MIDI, Bluetooth) and the bank of user
 * scenes, each a store object; the factory scenes built at boot from an INIT scene and a list of changes each.
 * Slots 0..7 are F1..F8, 8..23 are U1..U16. A scene is the whole instrument: every parameter and the bodies that
 * were latched when it was saved. */
#define SET_MAGIC 0x34464544u            /* "DEF4": the settings record (SPEAKER); DEF3 is read and brought up to date,
                                          * another magic loads defaults */
#define SET_MAGIC_V3 0x33464544u
enum { SPK_ON, SPK_AUTO, SPK_OFF };      /* SPEAKER: with the USB audio input; AUTO: silent while a computer records */
typedef struct {
    uint32_t magic;
    int8_t octave;                       /* -2 .. +2 */
    uint8_t ble_off;                     /* Bluetooth switched off (REC held); 0, the default: on */
    uint8_t scene;                       /* the slot in use */
    uint8_t scale;                       /* SC_* */
    uint8_t root;                        /* 0..11, C = 0 */
    uint8_t vel;                         /* 1: a key's velocity is its body's weight; 0: every body the same */
    uint8_t view;                        /* once the VIEW (MOUTH, ORBITS...): only BEAMS now, kept 0 for the layout */
    uint8_t morph;                       /* seconds a scene change takes, 0..30 */
    uint8_t midi_in;                     /* 0 omni, 1..16 */
    uint8_t midi_out;                    /* 1..16 */
    uint8_t local;                       /* 1: the keys play the engine; 0: MIDI out only */
    uint8_t latch;                       /* LATCH (SEL): 1, the keys toggle bodies that stay; 0, momentary */
    uint8_t speaker;                     /* SPK_*: the speaker and headphones while recording over USB */
    uint8_t usb_level;                   /* the recording's level while a computer records: 0 .. 6 = 0 .. -18 dB by 3 */
    uint8_t clock_out;                   /* MIDI clock out while the runner runs from RATE */
    uint8_t rsv;
} settings_t;
#define SET_SIZE_V3 16u
static settings_t set;
static uint8_t set_dirty, scene_dirty;   /* changed since the last save: SAVE is lit */

#define BANK_MAGIC 0x354E4353u           /* "SCN5": each body's own values, the pins and the chord's ROOT; EVERY from
                                          * 0.1 s */
#define BANK_MAGIC_V4 0x344E4353u        /* "SCN4" (the LEAD page), "SCN3" (the noise layer) and "SCN2": read and
                                          * brought up to date */
#define BANK_MAGIC_V3 0x334E4353u
#define BANK_MAGIC_V2 0x324E4353u
#define P_COUNT_V3 32u
#define P_COUNT_V2 28u
#define NFACTORY 16u
#define NUSER 16u
#define NSLOTS (NFACTORY + NUSER)
typedef struct {
    uint32_t magic;
    uint8_t count, ssize, rsv[2];
    scene_t u[NUSER];
} bank_t;
/* a scene as an older bank stored it: the same layout with fewer parameters (name, p[old], notes, vels, fx_off,
 * three spare bytes) and no per-body values after them */
#define OLD_SCENE_SIZE(np) (SCENE_NAME + (np) + 2u * DR_NV + 4u)
static bank_t bank PLAT_POOL;
static scene_t factory[NFACTORY] PLAT_POOL;

enum { SC_CHROMA, SC_MAJOR, SC_MINOR, SC_DORIAN, SC_PHRYGIAN, SC_LYDIAN, SC_MIXO, SC_PENTA_MAJ, SC_PENTA_MIN,
       SC_WHOLE, SC_HARM_MIN, SC_COUNT };
static const char *const SCALE_NAME[SC_COUNT] = {"CHROMATIC", "MAJOR", "MINOR", "DORIAN", "PHRYGIAN", "LYDIAN",
                                                 "MIXOLYDIAN", "MAJ PENTA", "MIN PENTA", "WHOLE TONE", "HARM MINOR"};
#define SC(...) scale_mask_of((const uint8_t[]){__VA_ARGS__, 255})
static uint32_t scale_mask_of(const uint8_t *d)
{
    uint32_t m = 0;
    for (; *d != 255u; d++)
        m |= 1u << (*d % 12u);
    return m;
}
static uint32_t scale_mask(uint32_t sc)
{
    switch (sc) {
    case SC_MAJOR: return SC(0, 2, 4, 5, 7, 9, 11);
    case SC_MINOR: return SC(0, 2, 3, 5, 7, 8, 10);
    case SC_DORIAN: return SC(0, 2, 3, 5, 7, 9, 10);
    case SC_PHRYGIAN: return SC(0, 1, 3, 5, 7, 8, 10);
    case SC_LYDIAN: return SC(0, 2, 4, 6, 7, 9, 11);
    case SC_MIXO: return SC(0, 2, 4, 5, 7, 9, 10);
    case SC_PENTA_MAJ: return SC(0, 2, 4, 7, 9);
    case SC_PENTA_MIN: return SC(0, 3, 5, 7, 10);
    case SC_WHOLE: return SC(0, 2, 4, 6, 8, 10);
    case SC_HARM_MIN: return SC(0, 2, 3, 5, 7, 8, 11);
    default: return 0xFFFu;
    }
}
static void scale_apply(void) { dr_scale(scale_mask(set.scale), set.root); }

static const char *const MODEL_NAME[M_COUNT] = {"ADDITIVE", "SUPERSAW", "FM", "BOWED", "FORMANT", "BELL", "SHEPARD", "WAVEFOLD",
                                                "WAVETABLE", "FM 4-OP", "SYNC", "TAPE", "RESONATOR"};

static void scene_name_set(scene_t *s, const char *name)
{
    uint32_t i;
    for (i = 0; i < SCENE_NAME - 1u && name[i]; i++)
        s->name[i] = name[i];
    for (; i < SCENE_NAME; i++)
        s->name[i] = 0;
}

static void scene_init(scene_t *s, const char *name)
{
    static const uint8_t INIT[P_COUNT] = {
        [P_MODEL] = M_SOLAR, [P_MASS] = 50, [P_DRIFT] = 35, [P_SPACE] = 45, [P_TIDE] = 25,
        [P_SHAPE] = 64, [P_SUB] = 30, [P_AIR] = 20,
        [P_SWELL] = 25, [P_FADE] = 75, [P_PRESS] = 50, [P_GLIDE] = 40,   /* GLIDE 40: 0.13 s */
        [P_TIDE_RATE] = 40, [P_WOBBLE] = 20, [P_WALK] = 30, [P_WIDTH] = 70,
        [P_REV_SIZE] = 70, [P_REV_DAMP] = 40, [P_SHIMMER] = 0, [P_DLY_MIX] = 0,
        [P_DLY_TIME] = 60, [P_DLY_FDBK] = 45, [P_ENS] = 0, [P_ENS_MIX] = 40,
        [P_ORB_PERIOD] = 78, [P_ORB_CHANCE] = 70, [P_ORB_RANGE] = 40, [P_GRAVITY] = 30,   /* EVERY 78: 5 s */
        [P_NZ_LEVEL] = 0, [P_NZ_COLOUR] = 50, [P_NZ_WIND] = 40, [P_NZ_BAND] = 30,
        [P_LD_ATTACK] = 30, [P_LD_RELEASE] = 55, [P_LD_LIFT] = 0, [P_LD_LEVEL] = 75,
        [P_ORB_MODE] = OM_WANDER, [P_ORB_DIST] = 40, [P_ORB_BODIES] = 60,
        [P_ARP_RATE] = 51, [P_ARP_GATE] = 60, [P_ARP_ORDER] = AO_UP, [P_ARP_OCT] = 1,
        [P_NZ_TEXTURE] = 0, [P_NZ_GUST] = 0, [P_NZ_DUCK] = 30, [P_NZ_WIDTH] = 70,
        [P_ORB_PROG] = 0,
        [P_SPREAD] = 30, [P_CROSS] = 0, [P_TWALK] = 25, [P_SWAY] = 15,
        [P_ECHO_DECAY] = 50, [P_ECHO_JUMP] = 25, [P_ORB_ROLL] = 50,
        [P_DLY_DIFF] = 25, [P_DLY_COLOUR] = 50,
        [P_DLY_TYPE] = DT_PING, [P_DLY_WOW] = 40, [P_DLY_CHOP] = 0,   /* WOW 40: the wander the delay always had */
        [P_ARP_RHYTHM] = 0, [P_ARP_DIV] = 2,                          /* every step a note, an eighth: as before */
    };
    uint32_t i;
    for (i = 0; i < P_COUNT; i++)
        s->p[i] = INIT[i];
    for (i = 0; i < DR_NV; i++) {
        s->note[i] = s->vel[i] = 0;
        s->lvl[i] = s->tone[i] = s->tune[i] = 0;
    }
    s->fx_off = 0;
    s->pins = s->root = s->rsv = 0;
    scene_name_set(s, name);
}

typedef struct { uint8_t id, v; } change_t;
#define END {255, 0}
static void scene_make(scene_t *s, const char *name, const change_t *c, const uint8_t *notes, uint32_t vel)
{
    uint32_t i;
    scene_init(s, name);
    for (; c->id != 255u; c++)
        s->p[c->id] = c->v;
    for (i = 0; i < DR_NV && notes[i]; i++) {
        s->note[i] = notes[i];
        s->vel[i] = (uint8_t)vel;
    }
    s->root = 1;                         /* every factory chord stands on C, and follows ROOT when it is recalled */
}

static void factory_build(void)
{
    /* F1 FIRST LIGHT: ADDITIVE, a warm major triad; the unit boots into it. WALK. */
    static const change_t FIRST_LIGHT[] = {{P_MASS, 48}, {P_DRIFT, 38}, {P_SPACE, 58}, {P_TIDE, 35}, {P_SUB, 35},
                                           {P_SWELL, 20}, {P_FADE, 82}, {P_WIDTH, 80}, {P_SPREAD, 45}, {P_CROSS, 10},
                                           {P_TWALK, 35}, {P_SWAY, 25}, END};
    static const uint8_t FIRST_LIGHT_N[] = {48, 55, 64, 0};
    /* F2 VOID TIDE: SUPERSAW, an octave-fifth with a ninth, low and breathing under a brown wind. WALK, slow. */
    static const change_t VOID_TIDE[] = {{P_MODEL, M_VOID}, {P_MASS, 48}, {P_DRIFT, 50}, {P_SPACE, 70}, {P_TIDE, 60},
                                         {P_SHAPE, 60}, {P_SUB, 70}, {P_SWELL, 25}, {P_FADE, 85}, {P_TIDE_RATE, 30},
                                         {P_WALK, 45}, {P_REV_SIZE, 85}, {P_REV_DAMP, 55}, {P_ENS, ENS_ENSEMBLE},
                                         {P_ENS_MIX, 55}, {P_ORB_PERIOD, 91}, {P_ORB_CHANCE, 60}, {P_ORB_RANGE, 30},
                                         {P_SPREAD, 55}, {P_TWALK, 40}, {P_SWAY, 30}, {P_NZ_LEVEL, 14}, {P_NZ_COLOUR, 20},
                                         {P_NZ_WIND, 30}, {P_NZ_BAND, 10}, {P_NZ_TEXTURE, 10}, {P_NZ_GUST, 40},
                                         {P_NZ_DUCK, 50}, {P_NZ_WIDTH, 110}, END};
    static const uint8_t VOID_TIDE_N[] = {36, 43, 48, 62, 0};
    /* F3 GLASS ORBIT: FM, octave-fifths; WALK every 3.5 s and OCT UP echoes climbing out of it. Made for GEN. */
    static const change_t GLASS_ORBIT[] = {{P_MODEL, M_GLASS}, {P_MASS, 42}, {P_DRIFT, 45}, {P_SPACE, 62}, {P_TIDE, 40},
                                           {P_SHAPE, 50}, {P_SUB, 20}, {P_SWELL, 30}, {P_FADE, 80}, {P_GLIDE, 50},
                                           {P_WOBBLE, 25}, {P_SHIMMER, 30}, {P_DLY_TYPE, DT_RISE}, {P_DLY_MIX, 40},
                                           {P_DLY_TIME, 114}, {P_DLY_FDBK, 50}, {P_DLY_DIFF, 30}, {P_DLY_COLOUR, 70},
                                           {P_ORB_PERIOD, 71}, {P_ORB_CHANCE, 90}, {P_SPREAD, 50}, {P_TWALK, 30},
                                           {P_SWAY, 40}, END};
    static const uint8_t GLASS_ORBIT_N[] = {48, 55, 60, 67, 0};
    /* F4 BOWED SKY: BOWED, a Lydian voicing, wide; SHIFT moves the whole chord; REVERSE echoes breathe in. */
    static const change_t BOWED_SKY[] = {{P_MODEL, M_STRING}, {P_MASS, 55}, {P_DRIFT, 40}, {P_SPACE, 65}, {P_TIDE, 30},
                                         {P_AIR, 40}, {P_SUB, 25}, {P_SWELL, 30}, {P_FADE, 85}, {P_WOBBLE, 30},
                                         {P_WIDTH, 90}, {P_REV_SIZE, 80}, {P_ENS, ENS_CHORUS2}, {P_ENS_MIX, 35},
                                         {P_DLY_TYPE, DT_REVERSE}, {P_DLY_MIX, 40}, {P_DLY_TIME, 85}, {P_DLY_FDBK, 40},
                                         {P_DLY_DIFF, 30}, {P_DLY_COLOUR, 55}, {P_ORB_MODE, OM_CHORD}, {P_ORB_PERIOD, 87},
                                         {P_ORB_CHANCE, 80}, {P_ORB_DIST, 50}, {P_GRAVITY, 40}, {P_TWALK, 30}, END};
    static const uint8_t BOWED_SKY_N[] = {48, 55, 66, 71, 0};
    /* F5 PULSAR: SYNC at X2.0 with ring mod, a 7sus4 stack, fast tremolo, dry; ARP 5/8 at 120 BPM over it, a
     * gated PING PONG in 1/8D. */
    static const change_t PULSAR[] = {{P_MODEL, M_TEAR}, {P_MASS, 60}, {P_DRIFT, 45}, {P_SPACE, 25}, {P_TIDE, 70},
                                      {P_SHAPE, 48}, {P_SUB, 30}, {P_AIR, 30}, {P_SWELL, 20}, {P_FADE, 70},
                                      {P_TIDE_RATE, 90}, {P_WOBBLE, 20}, {P_WIDTH, 80}, {P_REV_SIZE, 55},
                                      {P_DLY_TYPE, DT_PING}, {P_DLY_MIX, 45}, {P_DLY_TIME, 114}, {P_DLY_FDBK, 50},
                                      {P_DLY_DIFF, 10}, {P_DLY_COLOUR, 70}, {P_DLY_CHOP, 60}, {P_ORB_MODE, OM_ARP},
                                      {P_ARP_RATE, 51}, {P_ARP_GATE, 40}, {P_ARP_ORDER, AO_UPDOWN}, {P_ARP_OCT, 1},
                                      {P_ARP_RHYTHM, 1}, {P_ARP_DIV, 2}, {P_LD_ATTACK, 10}, {P_LD_RELEASE, 50},
                                      {P_LD_LEVEL, 90}, {P_SPREAD, 50}, {P_CROSS, 20}, {P_TWALK, 20}, {P_SWAY, 20}, END};
    static const uint8_t PULSAR_N[] = {48, 55, 58, 65, 0};
    /* F6 NEBULA: ADDITIVE, hollow, a five-note cluster in very slow swells, drowned in reverb and shimmer, a
     * GRANULAR cloud and a wind bed; RANDOM scatters the voices every 10 s. */
    static const change_t NEBULA[] = {{P_SHAPE, 22}, {P_MASS, 55}, {P_DRIFT, 70}, {P_SPACE, 88}, {P_TIDE, 45},
                                      {P_SUB, 25}, {P_SWELL, 95}, {P_FADE, 100}, {P_TIDE_RATE, 25}, {P_WALK, 60},
                                      {P_WOBBLE, 15}, {P_WIDTH, 100}, {P_REV_SIZE, 95}, {P_REV_DAMP, 30},
                                      {P_SHIMMER, 45}, {P_DLY_TYPE, DT_GRAINS}, {P_DLY_MIX, 35}, {P_DLY_TIME, 70},
                                      {P_DLY_FDBK, 50}, {P_DLY_DIFF, 50}, {P_DLY_COLOUR, 45}, {P_ORB_MODE, OM_SCATTER},
                                      {P_ORB_PERIOD, 91}, {P_ORB_BODIES, 70}, {P_ORB_RANGE, 50}, {P_SPREAD, 70},
                                      {P_TWALK, 50}, {P_SWAY, 45}, {P_NZ_LEVEL, 18}, {P_NZ_COLOUR, 35},
                                      {P_NZ_WIND, 45}, {P_NZ_BAND, 15}, {P_NZ_TEXTURE, 10}, {P_NZ_GUST, 50},
                                      {P_NZ_DUCK, 20}, {P_NZ_WIDTH, 110}, END};
    static const uint8_t NEBULA_N[] = {60, 62, 64, 65, 67, 0};
    /* F7 DEEP VOID: TAPE, one low C on a worn organ loop with sub and hiss, a TAPE echo and a quiet crackle; LOOP
     * brings the leads back an octave up. */
    static const change_t DEEP_VOID[] = {{P_MODEL, M_TAPE}, {P_MASS, 45}, {P_DRIFT, 60}, {P_SPACE, 50}, {P_TIDE, 20},
                                         {P_SHAPE, 15}, {P_SUB, 100}, {P_AIR, 35}, {P_SWELL, 35}, {P_FADE, 80},
                                         {P_WALK, 20}, {P_WIDTH, 50}, {P_REV_SIZE, 75}, {P_REV_DAMP, 70},
                                         {P_DLY_TYPE, DT_TAPE}, {P_DLY_MIX, 50}, {P_DLY_TIME, 99}, {P_DLY_FDBK, 70},
                                         {P_DLY_DIFF, 20}, {P_DLY_COLOUR, 30}, {P_DLY_WOW, 70}, {P_ORB_MODE, OM_ECHO},
                                         {P_ORB_PERIOD, 68}, {P_ECHO_DECAY, 40}, {P_ECHO_JUMP, 50}, {P_LD_ATTACK, 40},
                                         {P_LD_RELEASE, 90}, {P_LD_LIFT, 61}, {P_LD_LEVEL, 80}, {P_SPREAD, 20},
                                         {P_TWALK, 40}, {P_SWAY, 10}, {P_NZ_LEVEL, 16}, {P_NZ_COLOUR, 20},
                                         {P_NZ_WIND, 30}, {P_NZ_BAND, 10}, {P_NZ_TEXTURE, 90}, {P_NZ_GUST, 25},
                                         {P_NZ_DUCK, 30}, {P_NZ_WIDTH, 80}, END};
    static const uint8_t DEEP_VOID_N[] = {36, 0};
    /* F8 STARLIGHT: WAVETABLE in the GLASS table with unison, a high sus2, wide; STRUM every 5 s; PING PONG in 1/4. */
    static const change_t STARLIGHT[] = {{P_MODEL, M_SCAN}, {P_MASS, 60}, {P_DRIFT, 45}, {P_SPACE, 60}, {P_TIDE, 40},
                                         {P_SHAPE, 96}, {P_SUB, 10}, {P_AIR, 45}, {P_SWELL, 25}, {P_FADE, 78},
                                         {P_TIDE_RATE, 50}, {P_WIDTH, 100}, {P_REV_SIZE, 72}, {P_REV_DAMP, 25},
                                         {P_ENS, ENS_CHORUS1}, {P_ENS_MIX, 30}, {P_DLY_TYPE, DT_PING}, {P_DLY_MIX, 35},
                                         {P_DLY_TIME, 116}, {P_DLY_FDBK, 45}, {P_DLY_DIFF, 20}, {P_DLY_COLOUR, 75},
                                         {P_ORB_MODE, OM_PULSE}, {P_ORB_PERIOD, 78}, {P_ORB_CHANCE, 80}, {P_ORB_ROLL, 60},
                                         {P_SPREAD, 70}, {P_TWALK, 40}, {P_SWAY, 50}, END};
    static const uint8_t STARLIGHT_N[] = {60, 67, 74, 79, 0};
    /* F9 CATHEDRAL: FORMANT, a quartal stack on an open vowel in a big hall; PROG walks I VI IV V every 8 s. */
    static const change_t CATHEDRAL[] = {{P_MODEL, M_CHOIR}, {P_MASS, 32}, {P_DRIFT, 35}, {P_SPACE, 60}, {P_TIDE, 30},
                                         {P_SHAPE, 20}, {P_SUB, 20}, {P_AIR, 25}, {P_SWELL, 45}, {P_FADE, 90},
                                         {P_TIDE_RATE, 30}, {P_WOBBLE, 15}, {P_WIDTH, 85}, {P_REV_SIZE, 88},
                                         {P_REV_DAMP, 35}, {P_ORB_MODE, OM_CYCLE}, {P_ORB_PERIOD, 87}, {P_ORB_CHANCE, 127},
                                         {P_ORB_PROG, 1}, {P_SPREAD, 55}, {P_CROSS, 10}, {P_TWALK, 35}, {P_SWAY, 30}, END};
    static const uint8_t CATHEDRAL_N[] = {48, 53, 58, 63, 0};
    /* F10 RAIN CLOCK: FM 4-OP E.PIANO, a minor ninth, dry; ARP 5/8 at 92 BPM in a random order, a gated MULTI TAP
     * in 1/4, under rain. */
    static const change_t RAIN_CLOCK[] = {{P_MODEL, M_DX}, {P_MASS, 55}, {P_DRIFT, 35}, {P_SPACE, 22}, {P_TIDE, 25},
                                          {P_SHAPE, 72}, {P_SUB, 20}, {P_AIR, 25}, {P_SWELL, 30}, {P_FADE, 80},
                                          {P_WIDTH, 80}, {P_REV_SIZE, 60}, {P_REV_DAMP, 50}, {P_DLY_TYPE, DT_HEADS},
                                          {P_DLY_MIX, 45}, {P_DLY_TIME, 116}, {P_DLY_FDBK, 40}, {P_DLY_DIFF, 10},
                                          {P_DLY_COLOUR, 65}, {P_DLY_CHOP, 50}, {P_ORB_MODE, OM_ARP}, {P_ARP_RATE, 33},
                                          {P_ARP_GATE, 50}, {P_ARP_ORDER, AO_RANDOM}, {P_ARP_OCT, 1}, {P_ARP_RHYTHM, 1},
                                          {P_ARP_DIV, 2}, {P_LD_ATTACK, 5}, {P_LD_RELEASE, 70}, {P_LD_LEVEL, 90},
                                          {P_SPREAD, 45}, {P_TWALK, 20}, {P_SWAY, 25}, {P_NZ_LEVEL, 15}, {P_NZ_COLOUR, 70},
                                          {P_NZ_WIND, 50}, {P_NZ_BAND, 20}, {P_NZ_TEXTURE, 45}, {P_NZ_GUST, 30},
                                          {P_NZ_DUCK, 60}, {P_NZ_WIDTH, 100}, END};
    static const uint8_t RAIN_CLOCK_N[] = {48, 58, 63, 67, 74, 0};
    /* F11 GAMELAN: BELL at bell ratios, a pentatonic voicing, dry; RANDOM every 2 s; a gated PING PONG in 1/8. */
    static const change_t GAMELAN[] = {{P_MODEL, M_BELL}, {P_MASS, 55}, {P_DRIFT, 40}, {P_SPACE, 28}, {P_TIDE, 55},
                                       {P_SHAPE, 85}, {P_SUB, 15}, {P_AIR, 20}, {P_SWELL, 15}, {P_FADE, 75},
                                       {P_TIDE_RATE, 55}, {P_WOBBLE, 10}, {P_WIDTH, 90}, {P_REV_SIZE, 70},
                                       {P_DLY_TYPE, DT_PING}, {P_DLY_MIX, 40}, {P_DLY_TIME, 104}, {P_DLY_FDBK, 40},
                                       {P_DLY_DIFF, 0}, {P_DLY_COLOUR, 75}, {P_DLY_CHOP, 70}, {P_ORB_MODE, OM_SCATTER},
                                       {P_ORB_PERIOD, 60}, {P_ORB_BODIES, 50}, {P_ORB_RANGE, 60}, {P_SPREAD, 65},
                                       {P_TWALK, 15}, {P_SWAY, 20}, END};
    static const uint8_t GAMELAN_N[] = {60, 62, 67, 69, 72, 0};
    /* F12 BRONZE BOWL: RESONATOR between BOWL and BRONZE, stacked fifths, slow swells and shimmer; STRUM every 4 s
     * dips each voice in turn; a GRANULAR glitter. */
    static const change_t BRONZE_BOWL[] = {{P_MODEL, M_RESON}, {P_MASS, 55}, {P_DRIFT, 40}, {P_SPACE, 65}, {P_TIDE, 30},
                                           {P_SHAPE, 100}, {P_SUB, 15}, {P_AIR, 20}, {P_SWELL, 50}, {P_FADE, 90},
                                           {P_TIDE_RATE, 35}, {P_WOBBLE, 8}, {P_WIDTH, 90}, {P_REV_SIZE, 85},
                                           {P_REV_DAMP, 20}, {P_SHIMMER, 20}, {P_DLY_TYPE, DT_GRAINS}, {P_DLY_MIX, 40},
                                           {P_DLY_TIME, 60}, {P_DLY_FDBK, 45}, {P_DLY_DIFF, 40}, {P_DLY_COLOUR, 70},
                                           {P_ORB_MODE, OM_PULSE}, {P_ORB_PERIOD, 73}, {P_ORB_CHANCE, 90}, {P_ORB_ROLL, 70},
                                           {P_SPREAD, 60}, {P_TWALK, 30}, {P_SWAY, 35}, END};
    static const uint8_t BRONZE_BOWL_N[] = {48, 55, 62, 69, 0};
    /* F13 ASCENT: SHEPARD rising over an octave-fifth; SHIFT every 10 s; OCT UP echoes climb with it. */
    static const change_t ASCENT[] = {{P_MODEL, M_SHEPARD}, {P_MASS, 50}, {P_DRIFT, 30}, {P_SPACE, 65}, {P_TIDE, 20},
                                      {P_SHAPE, 72}, {P_SUB, 20}, {P_AIR, 15}, {P_SWELL, 40}, {P_FADE, 85},
                                      {P_WOBBLE, 10}, {P_WIDTH, 80}, {P_REV_SIZE, 85}, {P_REV_DAMP, 40},
                                      {P_DLY_TYPE, DT_RISE}, {P_DLY_MIX, 35}, {P_DLY_TIME, 90}, {P_DLY_FDBK, 45},
                                      {P_DLY_DIFF, 20}, {P_DLY_COLOUR, 70}, {P_ORB_MODE, OM_CHORD}, {P_ORB_PERIOD, 91},
                                      {P_ORB_CHANCE, 70}, {P_ORB_DIST, 40}, {P_GRAVITY, 30}, {P_SPREAD, 40},
                                      {P_TWALK, 25}, {P_SWAY, 50}, END};
    static const uint8_t ASCENT_N[] = {48, 55, 60, 0};
    /* F14 DESCENT: SHEPARD falling over a low open fifth, dark; PROG walks I VII IV every 13 s; OCT DOWN echoes sink
     * under it, a low wind. */
    static const change_t DESCENT[] = {{P_MODEL, M_SHEPARD}, {P_MASS, 35}, {P_DRIFT, 45}, {P_SPACE, 70}, {P_TIDE, 35},
                                       {P_SHAPE, 30}, {P_SUB, 45}, {P_AIR, 25}, {P_SWELL, 50}, {P_FADE, 90},
                                       {P_TIDE_RATE, 25}, {P_WOBBLE, 15}, {P_WIDTH, 75}, {P_REV_SIZE, 90},
                                       {P_REV_DAMP, 55}, {P_DLY_TYPE, DT_FALL}, {P_DLY_MIX, 45}, {P_DLY_TIME, 99},
                                       {P_DLY_FDBK, 55}, {P_DLY_DIFF, 60}, {P_DLY_COLOUR, 35}, {P_ORB_MODE, OM_CYCLE},
                                       {P_ORB_PERIOD, 95}, {P_ORB_CHANCE, 127}, {P_ORB_PROG, 7}, {P_SPREAD, 35},
                                       {P_TWALK, 30}, {P_SWAY, 20}, {P_NZ_LEVEL, 16}, {P_NZ_COLOUR, 10}, {P_NZ_WIND, 25},
                                       {P_NZ_BAND, 5}, {P_NZ_TEXTURE, 5}, {P_NZ_GUST, 65}, {P_NZ_DUCK, 30},
                                       {P_NZ_WIDTH, 90}, END};
    static const uint8_t DESCENT_N[] = {36, 43, 55, 0};
    /* F15 FOLDED SUN: WAVEFOLD, a 6/9 chord with ensemble, bright; LOOP brings the leads back every 3 s; a TAPE echo
     * in 1/4. */
    static const change_t FOLDED_SUN[] = {{P_MODEL, M_FOLD}, {P_MASS, 60}, {P_DRIFT, 45}, {P_SPACE, 55}, {P_TIDE, 45},
                                          {P_SHAPE, 55}, {P_SUB, 35}, {P_AIR, 10}, {P_SWELL, 25}, {P_FADE, 80},
                                          {P_TIDE_RATE, 40}, {P_WOBBLE, 20}, {P_WIDTH, 80}, {P_REV_SIZE, 75},
                                          {P_ENS, ENS_ENSEMBLE}, {P_ENS_MIX, 45}, {P_DLY_TYPE, DT_TAPE}, {P_DLY_MIX, 35},
                                          {P_DLY_TIME, 116}, {P_DLY_FDBK, 45}, {P_DLY_DIFF, 20}, {P_DLY_COLOUR, 60},
                                          {P_DLY_WOW, 55}, {P_ORB_MODE, OM_ECHO}, {P_ORB_PERIOD, 68}, {P_ECHO_DECAY, 50},
                                          {P_ECHO_JUMP, 35}, {P_LD_ATTACK, 20}, {P_LD_RELEASE, 70}, {P_LD_LEVEL, 85},
                                          {P_CROSS, 30}, {P_SPREAD, 50}, {P_TWALK, 30}, {P_SWAY, 30}, END};
    static const uint8_t FOLDED_SUN_N[] = {48, 55, 64, 69, 74, 0};
    scene_make(&factory[0], "FIRST LIGHT", FIRST_LIGHT, FIRST_LIGHT_N, 100);
    scene_make(&factory[1], "VOID TIDE", VOID_TIDE, VOID_TIDE_N, 127);
    scene_make(&factory[2], "GLASS ORBIT", GLASS_ORBIT, GLASS_ORBIT_N, 96);
    scene_make(&factory[3], "BOWED SKY", BOWED_SKY, BOWED_SKY_N, 84);
    scene_make(&factory[4], "PULSAR", PULSAR, PULSAR_N, 100);
    scene_make(&factory[5], "NEBULA", NEBULA, NEBULA_N, 64);
    scene_make(&factory[6], "DEEP VOID", DEEP_VOID, DEEP_VOID_N, 127);
    scene_make(&factory[7], "STARLIGHT", STARLIGHT, STARLIGHT_N, 92);
    scene_make(&factory[8], "CATHEDRAL", CATHEDRAL, CATHEDRAL_N, 84);
    scene_make(&factory[9], "RAIN CLOCK", RAIN_CLOCK, RAIN_CLOCK_N, 86);
    scene_make(&factory[10], "GAMELAN", GAMELAN, GAMELAN_N, 100);
    scene_make(&factory[11], "BRONZE BOWL", BRONZE_BOWL, BRONZE_BOWL_N, 84);
    scene_make(&factory[12], "ASCENT", ASCENT, ASCENT_N, 100);
    scene_make(&factory[13], "DESCENT", DESCENT, DESCENT_N, 100);
    scene_make(&factory[14], "FOLDED SUN", FOLDED_SUN, FOLDED_SUN_N, 100);
    scene_init(&factory[15], "INIT");    /* F16: the INIT scene as it is, no chord */
}

static void settings_defaults(void)
{
    set.magic = SET_MAGIC;
    set.octave = 0;
    set.ble_off = 0;
    set.scene = 0;
    set.scale = SC_MAJOR;
    set.root = 0;
    set.vel = 0;
    set.view = 0;
    set.morph = 4;
    set.midi_in = 0;
    set.midi_out = 1;
    set.local = 1;
    set.latch = 1;
    set.speaker = SPK_AUTO;
    set.usb_level = 2;
    set.clock_out = 0;
    set.rsv = 0;
}

static void bank_defaults(void)
{
    uint32_t i;
    char n[SCENE_NAME] = "USER 1";
    bank.magic = BANK_MAGIC;
    bank.count = NUSER;
    bank.ssize = (uint8_t)sizeof(scene_t);
    bank.rsv[0] = bank.rsv[1] = 0;
    for (i = 0; i < NUSER; i++) {
        n[5] = (char)('1' + i % 9u);
        n[6] = 0;
        if (i >= 9u) {
            n[5] = '1';
            n[6] = (char)('0' + (i - 9u));
            n[7] = 0;
        }
        scene_init(&bank.u[i], n);
    }
}

static const scene_t *slot_scene(uint32_t slot)
{
    return slot < NFACTORY ? &factory[slot] : &bank.u[(slot - NFACTORY) % NUSER];
}
static void slot_name(uint32_t slot, char *t)   /* "F3", "U12" */
{
    uint32_t n = slot < NFACTORY ? slot + 1u : slot - NFACTORY + 1u;
    t[0] = slot < NFACTORY ? 'F' : 'U';
    if (n >= 10u) {
        t[1] = (char)('0' + n / 10u);
        t[2] = (char)('0' + n % 10u);
        t[3] = 0;
    } else {
        t[1] = (char)('0' + n);
        t[2] = 0;
    }
}

static void bank_check(uint32_t i);
static void project_load(void)
{
    uint32_t i;
    factory_build();
    {
        int got = plat_store_load(OBJ_SETTINGS, &set, sizeof set);
        if (got == (int)SET_SIZE_V3 && set.magic == SET_MAGIC_V3) {   /* DEF3: the same, without SPEAKER */
            set.magic = SET_MAGIC;
            set.speaker = SPK_AUTO;
            set.usb_level = 2;
            set.clock_out = 0;
            set.rsv = 0;
            got = (int)sizeof set;
        }
        if (got != (int)sizeof set || set.magic != SET_MAGIC
            || set.octave < -2 || set.octave > 2 || set.scene >= NSLOTS || set.scale >= SC_COUNT || set.root >= 12u
            || set.vel > 1u || set.morph > 30u || set.midi_in > 16u || set.midi_out < 1u
            || set.midi_out > 16u || set.latch > 1u || set.speaker > SPK_OFF || set.usb_level > 6u || set.clock_out > 1u)
            settings_defaults();
        set.view = 0;                    /* a view saved by an older build: BEAMS, the one left */
    }
    {
        int got = plat_store_load(OBJ_SCENES, &bank, sizeof bank);
        uint32_t oldp = (bank.magic == BANK_MAGIC_V4 || bank.magic == BANK_MAGIC_V3 || bank.magic == BANK_MAGIC_V2)
                        && bank.ssize < sizeof(scene_t) && bank.ssize > OLD_SCENE_SIZE(0) ? bank.ssize - OLD_SCENE_SIZE(0) : 0u;   /* an older
                         * record of any of our magics: its size says how many parameters it held */
        if (oldp && got == (int)(8u + NUSER * OLD_SCENE_SIZE(oldp)) && bank.count == NUSER
            && bank.ssize == OLD_SCENE_SIZE(oldp)) {   /* an older bank: the same scenes, a few bytes short each;
                                                        * unpacked from the end, so nothing is overwritten before it
                                                        * is read; the new values take the INIT scene's */
            uint8_t *raw = (uint8_t *)&bank + 8u;
            uint32_t osz = OLD_SCENE_SIZE(oldp);
            int32_t k;
            for (k = (int32_t)NUSER - 1; k >= 0; k--) {
                uint8_t o[sizeof(scene_t)];
                scene_t *d = &bank.u[k];
                uint32_t j;
                char name[SCENE_NAME];
                for (j = 0; j < osz; j++)
                    o[j] = raw[(uint32_t)k * osz + j];
                for (j = 0; j < SCENE_NAME; j++)
                    name[j] = (char)o[j];
                name[SCENE_NAME - 1u] = 0;
                scene_init(d, name);
                for (j = 0; j < oldp; j++)
                    d->p[j] = o[SCENE_NAME + j];
                for (j = 0; j < DR_NV; j++) {
                    d->note[j] = o[SCENE_NAME + oldp + j];
                    d->vel[j] = o[SCENE_NAME + oldp + DR_NV + j];
                }
                d->fx_off = o[SCENE_NAME + oldp + 2u * DR_NV];
                d->p[P_ORB_PERIOD] = (uint8_t)((d->p[P_ORB_PERIOD] * 6393u + 457100u + 5000u) / 10000u);   /* EVERY
                                         * from 0.1 s: the same seconds on the new curve */
            }
            bank.magic = BANK_MAGIC;
            bank.ssize = (uint8_t)sizeof(scene_t);
            got = (int)sizeof bank;
        }
        if (got != (int)sizeof bank || bank.magic != BANK_MAGIC || bank.count != NUSER || bank.ssize != sizeof(scene_t))
            bank_defaults();
    }
    for (i = 0; i < NUSER; i++)
        bank_check(i);
    set_dirty = scene_dirty = 0;
}
static void bank_check(uint32_t i)       /* user scene i's values within range, whatever the flash (or a SysEx frame) held */
{
    uint32_t k;
    bank.u[i].name[SCENE_NAME - 1u] = 0;
    for (k = 0; k < P_COUNT; k++)
        bank.u[i].p[k] &= 127u;
    if (bank.u[i].p[P_MODEL] >= M_COUNT)
        bank.u[i].p[P_MODEL] = 0;
    if (bank.u[i].p[P_ORB_MODE] >= OM_COUNT)
        bank.u[i].p[P_ORB_MODE] = 0;
    if (bank.u[i].p[P_ARP_ORDER] >= AO_COUNT)
        bank.u[i].p[P_ARP_ORDER] = 0;
    if (bank.u[i].p[P_ARP_OCT] >= 3u)
        bank.u[i].p[P_ARP_OCT] = 0;
    if (bank.u[i].p[P_ORB_PROG] >= DR_PROGS)
        bank.u[i].p[P_ORB_PROG] = 0;
    if (bank.u[i].p[P_DLY_TYPE] >= DT_COUNT)
        bank.u[i].p[P_DLY_TYPE] = DT_PING;
    if (bank.u[i].p[P_ARP_RHYTHM] >= DR_RHYTHMS)
        bank.u[i].p[P_ARP_RHYTHM] = 0;
    if (bank.u[i].p[P_ARP_DIV] >= DR_DIVS)
        bank.u[i].p[P_ARP_DIV] = 2;
    for (k = 0; k < DR_NV; k++) {
        bank.u[i].note[k] &= 127u;
        bank.u[i].vel[k] &= 127u;
        bank.u[i].lvl[k] = (int8_t)clamp(bank.u[i].lvl[k], DR_LVL_MUTE, 6);
        bank.u[i].tone[k] = (int8_t)clamp(bank.u[i].tone[k], -50, 50);
        bank.u[i].tune[k] = (int8_t)clamp(bank.u[i].tune[k], -50, 50);
    }
    bank.u[i].fx_off &= 15u;
    if (bank.u[i].root > 12u)
        bank.u[i].root = 0;
}

static int project_save(void)            /* the settings and the bank: 0 = ok */
{
    if (plat_store_save(OBJ_SETTINGS, &set, sizeof set))
        return -1;
    if (plat_store_save(OBJ_SCENES, &bank, sizeof bank))
        return -1;
    set_dirty = 0;
    return 0;
}

/* the live scene into a user slot, with the bodies latched now, their own values, and the ROOT they stand in */
static void scene_capture(scene_t *s)
{
    uint32_t i, n = 0;
    dr_body_t b;
    *s = dr_scene;
    for (i = 0; i < DR_NV; i++) {
        s->note[i] = s->vel[i] = 0;
        s->lvl[i] = s->tone[i] = s->tune[i] = 0;
    }
    s->pins = 0;
    for (i = 0; i < DR_NV; i++)
        if (dr_body(i, &b) && !b.fading) {
            float lv = b.lvl == DR_LVL_MUTE || b.level <= 0.0f ? 0.0f : b.level / fm_exp2f((float)b.lvl * (1.0f / 6.0206f));
            s->note[n] = b.home;
            s->vel[n] = (uint8_t)(lv > 0.0f ? clamp((int32_t)(lv * 127.0f), 40, 127) : 100);
            s->lvl[n] = b.lvl;
            s->tone[n] = b.tone;
            s->tune[n] = b.tune;
            s->pins |= (uint8_t)(b.pinned << n);
            n++;
        }
    s->root = (uint8_t)(set.root + 1u);
}
