/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann
 * Copyright (C) 2026 Charles Vestal (the panel and Bluetooth handling from fm1-base's template) */
/* DEEP FIELD's screen and controls. The screen mirrors the panel: the header names the scene and shows the three
 * states as pills named for their buttons (SEL LATCH, PLAY ORBIT, SEQ FRZ); the stage below is a view of the bodies
 * over a strip of the 27 keys, each body a beam rising from the key that struck it (or the MOUTH, ORBITS, NEBULA
 * and SCOPE skins); the footer is the page band (the button that opened the page, its name, the SELECT arrows) and
 * the four knobs in one row under the four real ones. Any knob or button shows its name and value large over the
 * view for a moment.
 * Three rules. A page button always lands on the same page: HOME the forces, EDIT BODY, ENV ENVELOPE, LFO MOTION,
 * FX the effects (ENSEMBLE, DELAY, REVERB, one page each in the order of the FX keys), ARP ORBIT, GLO GLOBAL; SELECT
 * moves to the page before or after (DELAY, REVERB and MIDI are a step away),
 * and a page stays until a button or SELECT moves it. A lit button is a mode: SEL is LATCH (on: a key strikes a
 * body that stays, and pressed again lets it go; off: a body sounds while its key is held; held a second: every
 * body let go), PLAY runs ORBIT, SEQ freezes the reverb, the page's button lights. FX held: F3 G3 A3 B3 switch the
 * effects (saved with the scene). PRESET scrolls the scenes and the one it stops on morphs in; ALGO picks the
 * model; SAVE saves a scene (PRESET picks the slot, SAVE again saves, HOME cancels); REC is Bluetooth. */
#include "art.h"                         /* the model glyphs and the page icons */
enum { PG_HOME, PG_BODY, PG_NOISE, PG_NOISE2, PG_ENV, PG_LEAD, PG_MOTION, PG_MOTION2, PG_FX_ENS, PG_FX_DLY, PG_FX_DLY2, PG_FX_REV, PG_ORBIT, PG_ORBIT2,
       PG_GLOBAL, PG_MIDI, PG_USB, PG_COUNT };
enum { PID_SCALE = P_COUNT, PID_ROOT, PID_VIEW, PID_MORPH, PID_MIDI_IN, PID_MIDI_OUT, PID_LOCAL, PID_VEL, PID_SPEAKER,
       PID_USB_LEVEL, PID_CLOCK_OUT, PID_NONE, PID_COUNT };
enum { K_PCT, K_MODEL, K_TIME, K_GLIDE, K_ORBIT, K_ENS, K_DLYTIME, K_SCALE, K_ROOT, K_VIEW, K_MORPH, K_CHIN, K_CHOUT,
       K_ONOFF, K_HZ, K_STEPS, K_ATTACK, K_RELEASE, K_SEMI, K_OMODE, K_DIST, K_BODIES, K_BPM, K_ORDER, K_OCTS,
       K_TEXTURE, K_SPEAKER, K_USBLVL, K_PROG, K_DTYPE, K_RHYTHM, K_DIV, K_NONE };
typedef struct {
    const char *title, *button;
    uint8_t p[4];
} page_t;
static const page_t PAGES[PG_COUNT] = {
    {"HOME", "HOME", {P_MASS, P_DRIFT, P_SPACE, P_TIDE}},
    {"BODY", "EDIT", {P_MODEL, P_SHAPE, P_SUB, P_AIR}},
    {"NOISE", "EDIT", {P_NZ_LEVEL, P_NZ_COLOUR, P_NZ_WIND, P_NZ_BAND}},
    {"NOISE 2", "EDIT", {P_NZ_TEXTURE, P_NZ_GUST, P_NZ_DUCK, P_NZ_WIDTH}},
    {"ENVELOPE", "ENV", {P_SWELL, P_FADE, P_PRESS, P_GLIDE}},
    {"LEAD", "ENV", {P_LD_ATTACK, P_LD_RELEASE, P_LD_LIFT, P_LD_LEVEL}},   /* the keys with LATCH off */
    {"MOTION", "LFO", {P_TIDE_RATE, P_WOBBLE, P_WALK, P_WIDTH}},
    {"MOTION 2", "LFO", {P_SPREAD, P_CROSS, P_TWALK, P_SWAY}},   /* the bodies apart from each other */
    {"ENSEMBLE", "FX", {P_ENS, P_ENS_MIX, PID_NONE, PID_NONE}},   /* the effects, in the order of the FX keys */
    {"DELAY", "FX", {P_DLY_TYPE, P_DLY_TIME, P_DLY_FDBK, P_DLY_MIX}},
    {"DELAY 2", "FX", {P_DLY_COLOUR, P_DLY_DIFF, P_DLY_WOW, P_DLY_CHOP}},
    {"REVERB", "FX", {P_REV_SIZE, P_REV_DAMP, P_SHIMMER, P_SPACE}},
    {"ORBIT", "ARP", {P_ORB_MODE, P_ORB_PERIOD, P_ORB_CHANCE, P_ORB_RANGE}},   /* knobs 2-4 by MODE: page_param */
    {"ORBIT 2", "ARP", {P_GRAVITY, P_ARP_OCT, P_ARP_RHYTHM, P_ARP_DIV}},   /* RHYTHM and DIV: the runner's */
    {"GLOBAL", "GLO", {PID_SCALE, PID_ROOT, PID_VIEW, PID_MORPH}},
    {"MIDI", "GLO", {PID_MIDI_IN, PID_MIDI_OUT, PID_LOCAL, PID_VEL}},
    {"USB", "GLO", {PID_SPEAKER, PID_USB_LEVEL, PID_CLOCK_OUT, PID_NONE}},   /* recording and clock over the cable */
};
typedef struct {
    const char *label;
    uint8_t kind;
} pmeta_t;
static const pmeta_t PM[PID_COUNT] = {
    [P_MODEL] = {"MODEL", K_MODEL},     [P_MASS] = {"MASS", K_HZ},          [P_DRIFT] = {"DRIFT", K_PCT},
    [P_SPACE] = {"SPACE", K_PCT},       [P_TIDE] = {"TIDE", K_PCT},         [P_SHAPE] = {"SHAPE", K_PCT},
    [P_SUB] = {"SUB", K_PCT},           [P_AIR] = {"AIR", K_PCT},           [P_SWELL] = {"SWELL", K_TIME},
    [P_FADE] = {"FADE", K_TIME},        [P_PRESS] = {"PRESS", K_PCT},       [P_GLIDE] = {"GLIDE", K_GLIDE},
    [P_TIDE_RATE] = {"TIDE RATE", K_PCT}, [P_WOBBLE] = {"WOBBLE", K_PCT},   [P_WALK] = {"WALK", K_PCT},
    [P_WIDTH] = {"WIDTH", K_PCT},       [P_REV_SIZE] = {"REV SIZE", K_PCT}, [P_REV_DAMP] = {"REV DAMP", K_PCT},
    [P_SHIMMER] = {"SHIMMER", K_PCT},   [P_DLY_MIX] = {"DLY MIX", K_PCT},   [P_DLY_TIME] = {"DLY TIME", K_DLYTIME},
    [P_DLY_FDBK] = {"DLY FDBK", K_PCT}, [P_ENS] = {"ENSEMBLE", K_ENS},      [P_ENS_MIX] = {"ENS MIX", K_PCT},
    [P_DLY_DIFF] = {"DIFFUSE", K_PCT}, [P_DLY_COLOUR] = {"COLOUR", K_PCT}, [P_DLY_TYPE] = {"TYPE", K_DTYPE},
    [P_DLY_WOW] = {"WOW", K_PCT},      [P_DLY_CHOP] = {"CHOP", K_PCT},
    [P_ORB_PERIOD] = {"EVERY", K_ORBIT}, [P_ORB_CHANCE] = {"CHANCE", K_PCT}, [P_ORB_RANGE] = {"STEPS UP TO", K_STEPS},
    [P_GRAVITY] = {"HOME PULL", K_PCT},  [P_NZ_LEVEL] = {"LEVEL", K_PCT},    [P_NZ_COLOUR] = {"COLOUR", K_PCT},
    [P_NZ_WIND] = {"WIND", K_PCT},      [P_NZ_BAND] = {"BAND", K_PCT},
    [P_LD_ATTACK] = {"ATTACK", K_ATTACK}, [P_LD_RELEASE] = {"RELEASE", K_RELEASE}, [P_LD_LIFT] = {"LIFT", K_SEMI},
    [P_LD_LEVEL] = {"LEVEL", K_PCT},    [P_ORB_MODE] = {"MODE", K_OMODE},   [P_ORB_DIST] = {"DISTANCE", K_DIST},
    [P_ORB_BODIES] = {"BODIES", K_BODIES}, [P_ARP_RATE] = {"RATE", K_BPM},  [P_ARP_GATE] = {"GATE", K_PCT},
    [P_ARP_ORDER] = {"ORDER", K_ORDER}, [P_ARP_OCT] = {"OCTAVES", K_OCTS},
    [P_NZ_TEXTURE] = {"TEXTURE", K_TEXTURE}, [P_NZ_GUST] = {"GUST", K_PCT}, [P_NZ_DUCK] = {"DUCK", K_PCT},
    [P_NZ_WIDTH] = {"WIDTH", K_PCT}, [P_ORB_PROG] = {"CHORDS", K_PROG},
    [P_SPREAD] = {"SPREAD", K_PCT}, [P_CROSS] = {"CROSS", K_PCT}, [P_TWALK] = {"TIMBRE", K_PCT}, [P_SWAY] = {"SWAY", K_PCT},
    [P_ECHO_DECAY] = {"DECAY", K_PCT}, [P_ECHO_JUMP] = {"JUMPS", K_PCT}, [P_ORB_ROLL] = {"ROLL", K_PCT},
    [P_ARP_RHYTHM] = {"RHYTHM", K_RHYTHM}, [P_ARP_DIV] = {"DIV", K_DIV},
    [PID_SCALE] = {"SCALE", K_SCALE},   [PID_ROOT] = {"ROOT", K_ROOT},
    [PID_VIEW] = {"VIEW", K_VIEW},      [PID_MORPH] = {"MORPH", K_MORPH},   [PID_MIDI_IN] = {"IN CH", K_CHIN},
    [PID_MIDI_OUT] = {"OUT CH", K_CHOUT}, [PID_LOCAL] = {"LOCAL", K_ONOFF}, [PID_VEL] = {"VELOCITY", K_ONOFF},
    [PID_SPEAKER] = {"SPEAKER", K_SPEAKER}, [PID_USB_LEVEL] = {"USB LEVEL", K_USBLVL}, [PID_CLOCK_OUT] = {"CLOCK OUT", K_ONOFF},
    [PID_NONE] = {"", K_NONE},
};
static const char *const ENS_NAME[ENS_COUNT] = {"OFF", "CHOR I", "CHOR II", "ENSEMBLE"};
static const char *const NOTE_NAME[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *const BTN_NAME[NB] = {"FX", "SEL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ",
                                         "PLAY", "REC", "OCT-", "OCT+"};

/* the screen: a header of 46 (the model's glyph, the scene, the status, then the pills at fixed places), the view
 * of 84 over the keys strip of 36, the footer of 74 (the page band, the four knobs in a row). PHOSPHOR TUBE: light
 * means sound. Only the bodies give off light; the interface around them stays unlit, and each colour has one
 * meaning: sun is latched (everywhere), white your hand (a lead, an open body), cyan where you are (never in the
 * view), teal an effect on, orange only what needs attention */
#define HEAD_H 46
#define SKY_W 240
#define SKY_H 84
#define SKY_Y 46
#define KEYS_Y 130
#define KEYS_H 36
#define FOOT_Y 166
#define FOOT_H 74
#define HUD_MS 1300u
#define HUE_STAR 0
#define HUE_SUN 1
#define HUE_CYAN 2
#define HUE_MAGENTA 3
#define HUE_VIOLET 4
#define HUE_GREEN 5
#define HUE_BLUE 6
#define HUE_EMBER 7
#define NSTARS 40u
static uint8_t phos[SKY_W * SKY_H] PLAT_POOL;
static uint16_t lut[256];
/* the eight hues as ramps, keyed at levels 31 26 20 13 7 2: the pure hue at 26, white-hot above, sliding toward
 * plum and navy as they dim, every level rising from the sky at 0 and none below it (the old fade from black drew
 * the faintest levels darker than the sky: a black moat round every glow). Body green moved to lime, away from the
 * FX teal; blue moved away from violet */
static const uint8_t RAMP[8][6][3] = {
    {{255, 255, 255}, {216, 224, 248}, {152, 168, 208}, {88, 104, 144}, {40, 52, 88}, {16, 24, 48}},      /* star */
    {{255, 244, 216}, {248, 196, 96}, {224, 136, 64}, {160, 72, 56}, {88, 32, 64}, {32, 12, 40}},         /* sun */
    {{232, 252, 255}, {64, 216, 255}, {40, 152, 208}, {24, 96, 144}, {8, 48, 88}, {0, 24, 48}},           /* cyan */
    {{255, 232, 248}, {255, 88, 200}, {192, 48, 152}, {128, 24, 112}, {64, 16, 72}, {24, 8, 40}},         /* magenta */
    {{240, 232, 255}, {176, 112, 255}, {112, 80, 208}, {72, 48, 152}, {40, 28, 96}, {16, 12, 48}},       /* violet */
    {{244, 255, 224}, {168, 240, 96}, {112, 192, 64}, {64, 120, 48}, {32, 64, 32}, {12, 28, 20}},         /* lime */
    {{224, 236, 255}, {72, 120, 255}, {56, 88, 208}, {32, 52, 160}, {16, 28, 96}, {8, 12, 48}},           /* blue */
    {{255, 240, 224}, {255, 64, 80}, {200, 48, 64}, {136, 28, 56}, {72, 16, 48}, {24, 8, 32}},            /* ember */
};
static const uint8_t BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};   /* ordered dither */
#define C_SKY RGB(0, 4, 16)              /* the sky: every glow rises from it (on the panel it reads as black) */
#define C_HORIZON RGB(8, 16, 48)         /* the view's dithered horizon, so the window reads on a black panel */
#define C_BAND RGB(10, 22, 40)           /* the page band */
#define C_RULE RGB(26, 44, 70)           /* rules, empty tracks */
#define C_BRIGHT RGB(240, 248, 255)      /* values; your hand */
#define C_LABEL RGB(150, 160, 175)       /* labels */
#define C_OFF RGB(104, 128, 160)         /* an unlit pill or letter */
#define C_TEXT RGB(120, 215, 255)        /* where you are: the page, bar fills; never in the view */
#define C_MODE RGB(40, 150, 200)         /* a mode on: a filled pill */
#define C_FX_ON RGB(40, 210, 160)        /* an effect on */
#define C_LATCH RGB(248, 196, 96)        /* latched, everywhere: the sun hue */
#define C_WARN RGB(255, 150, 70)         /* needs attention: unsaved, LOAD, a failed save */
#define C_LEAD RGB(240, 248, 255)        /* a lead's key: your hand */
#define C_LEAD_OFF RGB(112, 124, 148)
#define C_KW_IN RGB(58, 70, 96)          /* the keys as dark glass, lit only by their bodies: white in the scale */
#define C_KW_OFF RGB(26, 33, 48)
#define C_KB_IN RGB(42, 53, 80)
#define C_KB_OFF RGB(10, 14, 22)
#define C_STRIP_TOP RGB(24, 48, 80)      /* the strip's top line, where the beams stand */
static const uint8_t WHITE_K[16] = PLAT_WHITE_KEYS;
static const uint8_t BLACK_K[11] = PLAT_BLACK_KEYS;
static uint8_t key_x[NKEYS];             /* each key's centre on the strip */
static uint8_t key_white[NKEYS];         /* its white index + 1; 0 a black key */

static volatile uint32_t midi_last, midi_last_port, midi_count;   /* the last message in (shown), and how many */
static volatile int32_t pc_pending = -1;     /* a program change from MIDI, loaded by the main loop */
static volatile uint8_t overload_seen;

static struct {
    uint8_t page, dialog;                /* the page shown; the save dialog is up */
    uint8_t save_to;                     /* the dialog's user slot 0..15 */
    char hud[2][24];                     /* the name and value shown large over the sky */
    uint32_t hud_until;
    uint32_t keys, btn, dkeys;           /* held at the last input (dkeys: the plain matrix, the FX layer's) */
    uint8_t vel[NKEYS], prs[NKEYS];
    uint8_t keynote[NKEYS];              /* the note a key struck */
    uint32_t keynotes;                   /* bit k: key k struck a note this press */
    uint32_t keylead;                    /* bit k: it struck a lead (LATCH off), let go at its release */
    uint8_t looking;                     /* Bluetooth MIDI devices: REC pressed, the scan's list up (lib/btdev) */
    uint32_t rec_t, fx_t, fx_up, sel_t, sel_down, glo_t;   /* GLO held: the crossfade layer (a key's pressure blends toward the next scene) */
    uint8_t glo_xf;                      /* a key was leaned on during this hold */
    uint8_t glo_amt;                     /* the blend last set (0..127) */
    uint32_t glo_kt;                     /* a key without pressure (the top row) held under GLO: when its climb began */
    uint8_t fx_used, sel_used;
    uint8_t sel_chord;                   /* SEL held while PRESET turned: the scene lands with its chord as SEL lets go */
    float mist;                          /* the noise mist's drift, in dither cells */
    uint16_t hud_id;                     /* the knob whose value the HUD shows (its bar under it); 0xFFFF none */
    uint32_t enc_t[NE];                  /* each knob's last detent: one within 40 ms of it is a fast turn */
    uint32_t bk_down, bk_open;           /* a latched body's key held (LATCH on): counting to a third of a second (it
                                          * blinks), then open: the four knobs are that body's own (several: all of them) */
    uint32_t bk_t[NKEYS];
    int8_t bval[NKEYS][BE_COUNT];        /* an open body's values as the knobs left them (the engine's lag a block) */
    uint8_t shift;                       /* TRANSPOSE: SEL held and a key touched; the keys move the latched bodies */
    uint32_t keys_sig;                   /* what the keys strip showed last */
    uint8_t redraw_keys;
    uint8_t oct_lit, oct_mid;            /* the OCT buttons: lit at two octaves, half-lit at one (bit 0 OCT-, 1 OCT+) */
    uint8_t nz_last;                     /* the noise LEVEL before C4 switched it off, to come back to */
    uint32_t stat_t;                     /* the last load line in the log, and what it said */
    uint8_t stat_cpu, stat_bodies;
    uint32_t stat_under;
    uint32_t play_down, orbit_flash_t;   /* PLAY pressed (a tap runs ORBIT, a hold opens its page); the pill's flash */
    uint8_t play_used, orbit_eighth;     /* the countdown shown in the pill, in eighths */
    uint32_t arp_steps, arp_step_t;      /* the runner's steps seen, and when the last came (PLAY winks) */
    uint32_t lead_seen, lead_rel;        /* bit i: lead i was there last frame; was releasing */
    uint32_t lead_t[DR_NL];              /* when each lead was struck: its key winks off for 40 ms */
    uint8_t lead_home[DR_NL];
    int32_t sel_slot;                    /* SELECT's candidate while it turns; -1 none */
    uint32_t sky_t, foot_t, head_t;      /* when the sky, the footer and the header were last drawn */
    uint8_t redraw_head, redraw_foot;
    uint32_t frame, orbit_moves;
    float ang[DR_NV];                    /* each body's place on its orbit, turns */
    uint8_t eat[DR_NV], snack[DR_NV];    /* MOUTH: a pill going in (1), inside (2), coming back out (3); a snack
                                          * comes back, a fading body's pill does not */
    float eat_t[DR_NV];                  /* how far along */
    float px[DR_NV], py[DR_NV];          /* each pill's place on the sky */
    float spot_x[DR_NV], spot_y[DR_NV];  /* where it rests */
    uint8_t pill_live[DR_NV];            /* it was on the sky last frame (else the hand throws it in) */
    uint8_t hand_throw;                  /* the body the bottle is tipping out, + 1; 0 none */
    float hand_t;                        /* the pop, 0..1 */
    uint8_t cap;                         /* the cap: 0 shut, 1 half, 2 open */
    uint32_t cap_t;                      /* when the last pill came out: the cap shuts a moment after */
    float chomp;                         /* the mouth closing on a pill, 0 none, else 0..1 */
    uint32_t crumbs;                     /* frames of crumbs after a chomp */
    float sweep;                         /* the scope's radar */
    uint32_t rng;
    struct { uint16_t x, y; uint8_t z, hue; } star[NSTARS];
} ui;

static int usb_recording(void);          /* app.c: a computer takes the USB audio input */
static uint32_t usb_underruns(void);     /* app.c: the USB audio ring ran dry, so far */
static void text_cat(char *d, const char *s)
{
    while (*d)
        d++;
    while (*s)
        *d++ = *s++;
    *d = 0;
}
static void int_text(int32_t v, char *b)     /* signed, no padding */
{
    char t[12];
    int n = 0;
    uint32_t u = (uint32_t)(v < 0 ? -v : v);
    do
        t[n++] = (char)('0' + u % 10u);
    while ((u /= 10u) && n < 11);
    if (v < 0)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}
static void dec1_text(float f, const char *unit, char *b)   /* one decimal: 1.2S */
{
    int32_t d = (int32_t)(f * 10.0f + 0.5f);
    char u[4];
    int_text(d / 10, b);
    text_cat(b, ".");
    int_text(d % 10, u);
    text_cat(b, u);
    text_cat(b, unit);
}
static void note_text(int32_t note, char *b)
{
    char o[8];
    b[0] = 0;
    text_cat(b, NOTE_NAME[note % 12]);
    int_text(note / 12 - 1, o);
    text_cat(b, o);
}
static uint32_t hash32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}

/* ------------------------------------------------------------------ the HUD --- */
static void ui_say(const char *a, const char *b)
{
    uint32_t i;
    char t[48];
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.hud[0][i] = a[i];
    ui.hud[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.hud[1][i] = b[i];
    ui.hud[1][i] = 0;
    ui.hud_until = plat_ms() + HUD_MS;
    ui.hud_id = 0xFFFFu;                 /* a message: no bar (a knob sets its own after this) */
    ui.sky_t = 0;                        /* drawn at the next frame */
    if (ann_on()) {
        t[0] = 0;
        text_cat(t, ui.hud[0]);
        if (ui.hud[1][0]) {
            text_cat(t, " ");
            text_cat(t, ui.hud[1]);
        }
        ann_say(t, 0, ANN_NOW);
    }
}

/* ------------------------------------------------------------------ values --- */
static const char *const OMODE_NAME[OM_COUNT] = {"WANDER", "CHORD", "SCATTER", "ARP", "CYCLE", "ECHO", "PULSE"};
static const char *const PROG_NAME[DR_PROGS] = {"I IV V", "I VI IV V", "I V VI IV", "I VI III VII", "I IV I V",
                                                "I II V", "I III VI IV", "I VII IV"};
static const char *const ORDER_NAME[AO_COUNT] = {"UP", "DOWN", "UP DOWN", "RANDOM", "PLAYED"};
static const char *const DTYPE_NAME[DT_COUNT] = {"PING", "TAPE", "HEADS", "REVERSE", "RISE", "FALL", "GRAINS"};
static uint32_t page_param(uint32_t pg, uint32_t k)   /* ORBIT: knobs 2-4 are the mode's */
{
    if (pg == PG_ORBIT && k) {
        static const uint8_t BY_MODE[OM_COUNT][3] = {
            {P_ORB_PERIOD, P_ORB_CHANCE, P_ORB_RANGE},   /* WANDER */
            {P_ORB_PERIOD, P_ORB_CHANCE, P_ORB_DIST},    /* CHORD */
            {P_ORB_PERIOD, P_ORB_BODIES, P_ORB_RANGE},   /* SCATTER */
            {P_ARP_RATE, P_ARP_GATE, P_ARP_ORDER},       /* ARP */
            {P_ORB_PERIOD, P_ORB_CHANCE, P_ORB_PROG},    /* CYCLE */
            {P_ORB_PERIOD, P_ECHO_DECAY, P_ECHO_JUMP},   /* ECHO */
            {P_ORB_PERIOD, P_ORB_CHANCE, P_ORB_ROLL},    /* PULSE */
        };
        return BY_MODE[dr_orbit_mode()][k - 1u];
    }
    return PAGES[pg].p[k];
}
static uint32_t value_of(uint32_t id)
{
    switch (id) {
    case PID_SCALE: return set.scale;
    case PID_ROOT: return set.root;
    case PID_VIEW: return set.view;
    case PID_MORPH: return set.morph;
    case PID_MIDI_IN: return set.midi_in;
    case PID_MIDI_OUT: return set.midi_out;
    case PID_LOCAL: return set.local;
    case PID_VEL: return set.vel;
    case PID_SPEAKER: return set.speaker;
    case PID_USB_LEVEL: return set.usb_level;
    case PID_CLOCK_OUT: return set.clock_out;
    case PID_NONE: return 0;
    default: return dr_scene.p[id];
    }
}
static uint32_t value_max(uint32_t id)
{
    switch (PM[id].kind) {
    case K_MODEL: return M_COUNT - 1u;
    case K_OMODE: return OM_COUNT - 1u;
    case K_PROG: return DR_PROGS - 1u;
    case K_SPEAKER: return SPK_OFF;
    case K_USBLVL: return 6u;
    case K_ORDER: return AO_COUNT - 1u;
    case K_OCTS: return 2u;
    case K_ENS: return ENS_COUNT - 1u;
    case K_DTYPE: return DT_COUNT - 1u;
    case K_RHYTHM: return DR_RHYTHMS - 1u;
    case K_DIV: return DR_DIVS - 1u;
    case K_SCALE: return SC_COUNT - 1u;
    case K_ROOT: return 11u;
    case K_VIEW: return VIEW_COUNT - 1u;
    case K_MORPH: return 30u;
    case K_CHIN: return 16u;
    case K_CHOUT: return 16u;
    case K_ONOFF: return 1u;
    default: return 127u;
    }
}
static void ms_text(float s, char *b)   /* below a second in ms, else seconds to a tenth */
{
    if (s < 1.0f) {
        int_text((int32_t)(s * 1000.0f + 0.5f), b);
        text_cat(b, "MS");
    } else
        dec1_text(s, "S", b);
}
static uint32_t dx_voice_of(uint32_t v) { return (uint32_t)((float)v * (1.0f / 127.0f) * ((float)DR_DX_VOICES - 0.01f)); }
static void value_text(uint32_t id, char *b)
{
    uint32_t v = value_of(id);
    b[0] = 0;
    if (id == P_SHAPE && dr_scene.p[P_MODEL] == M_DX) {   /* DX: SHAPE is the voice */
        text_cat(b, DR_DX_NAME[dx_voice_of(v)]);
        return;
    }
    if (id == P_SHAPE && dr_scene.p[P_MODEL] == M_SCAN) {   /* SCAN: the table it is in, and how far through */
        uint32_t pos = v * 47u / 127u;
        char n[4];
        text_cat(b, DR_SCAN_NAME[pos / 16u]);
        text_cat(b, " ");
        int_text((int32_t)(pos % 16u) + 1, n);
        text_cat(b, n);
        return;
    }
    switch (PM[id].kind) {
    case K_PCT:
        int_text((int32_t)(v * 100u / 127u), b);
        text_cat(b, "%");
        break;
    case K_HZ:
        if (dr_scene.p[P_MODEL] == M_VOID) {
            float hz = dr_hz(v);
            if (hz >= 1000.0f)
                dec1_text(hz / 1000.0f, "K", b);
            else {
                int_text((int32_t)hz, b);
                text_cat(b, "HZ");
            }
        } else {
            int_text((int32_t)(v * 100u / 127u), b);
            text_cat(b, "%");
        }
        break;
    case K_MODEL: text_cat(b, MODEL_NAME[v % M_COUNT]); break;
    case K_TIME: ms_text(dr_time_s(v), b); break;
    case K_GLIDE: ms_text(0.02f * fm_exp2f((float)v * (8.6f / 127.0f)), b); break;
    case K_ORBIT: {                      /* tenths below 10 s; ECHO never under a second */
        float sec = dr_orbit_s(v);
        if (dr_orbit_mode() == OM_ECHO && sec < 1.0f)
            sec = 1.0f;
        if (sec < 9.95f)
            dec1_text(sec, "S", b);
        else {
            int_text((int32_t)(sec + 0.5f), b);
            text_cat(b, "S");
        }
        break;
    }
    case K_RHYTHM: text_cat(b, DR_RHYTHM_NAME[v % DR_RHYTHMS]); break;
    case K_DIV: text_cat(b, DR_DIV_NAME[v % DR_DIVS]); break;
    case K_ENS: text_cat(b, ENS_NAME[v % ENS_COUNT]); break;
    case K_DTYPE: text_cat(b, DTYPE_NAME[v % DT_COUNT]); break;
    case K_DLYTIME:
        if (dr_dly_div(v) >= 0) {
            text_cat(b, DR_DLY_DIV_NAME[dr_dly_div(v)]);
        } else {
            int_text((int32_t)(dr_dly_time_s(v) * 1000.0f), b);
            text_cat(b, "MS");
        }
        break;
    case K_SCALE: text_cat(b, SCALE_NAME[v % SC_COUNT]); break;
    case K_ROOT: text_cat(b, NOTE_NAME[v % 12u]); break;
    case K_VIEW: text_cat(b, VIEW_NAME[v % VIEW_COUNT]); break;
    case K_MORPH:
        if (!v)
            text_cat(b, "NOW");
        else {
            int_text((int32_t)v, b);
            text_cat(b, "S");
        }
        break;
    case K_CHIN:
        if (!v)
            text_cat(b, "OMNI");
        else
            int_text((int32_t)v, b);
        break;
    case K_CHOUT: int_text((int32_t)v, b); break;
    case K_ONOFF: text_cat(b, v ? "ON" : "OFF"); break;
    case K_STEPS: int_text((int32_t)(1u + v * 4u / 128u), b); break;   /* the engine's 1 .. 4 scale steps */
    case K_ATTACK:
    case K_RELEASE: {
        float s = id == P_LD_ATTACK ? dr_attack_s(v) : dr_release_s(v);
        if (s < 1.0f) {
            int_text((int32_t)(s * 1000.0f + 0.5f), b);
            text_cat(b, "MS");
        } else
            dec1_text(s, "S", b);
        break;
    }
    case K_OMODE: text_cat(b, OMODE_NAME[v % OM_COUNT]); break;
    case K_PROG: text_cat(b, PROG_NAME[v % DR_PROGS]); break;
    case K_SPEAKER: text_cat(b, v == SPK_ON ? "ON" : v == SPK_AUTO ? "AUTO" : "OFF"); break;
    case K_USBLVL:                       /* 0 .. -18 dB, in 3 dB steps */
        if (!v)
            text_cat(b, "0 DB");
        else {
            b[0] = '-';
            b[1] = 0;
            int_text((int32_t)(v * 3u), b + 1);
            text_cat(b, " DB");
        }
        break;
    case K_TEXTURE: {                    /* hiss to a quarter, rain to a half, crackle beyond; the figure too */
        int_text((int32_t)(v * 100u / 127u), b);
        text_cat(b, v < 32u ? "% HISS" : v < 64u ? "% RAIN" : "% CRACKLE");
        break;
    }
    case K_ORDER: text_cat(b, ORDER_NAME[v % AO_COUNT]); break;
    case K_OCTS:
        int_text((int32_t)(v % 3u) + 1, b);
        text_cat(b, v % 3u ? " OCTS" : " OCT");
        break;
    case K_DIST:                         /* the engine's 1 .. 3 scale degrees */
        int_text((int32_t)(1u + v * 3u / 128u), b);
        text_cat(b, v * 3u / 128u ? " STEPS" : " STEP");
        break;
    case K_BODIES:                       /* 1 .. all (the count depends on what is latched) */
        if (v >= 120u)
            text_cat(b, "ALL");
        else {
            int_text((int32_t)(1u + v * 7u / 127u), b);
            text_cat(b, " OF 8");
        }
        break;
    case K_BPM:
        if (dr_clock_ext()) {            /* following MIDI clock: the tempo heard, not the knob's */
            text_cat(b, "EXT ");
            int_text((int32_t)(dr_clock_bpm() + 0.5f), b);
        } else {
            int_text((int32_t)(dr_arp_bpm(v) + 0.5f), b);
            text_cat(b, " BPM");
        }
        break;
    case K_SEMI: {                       /* whole semitones up, as the engine rounds LIFT */
        uint32_t st = (uint32_t)((float)v * (1.0f / 127.0f) * 24.99f);
        if (!st)
            text_cat(b, "0");
        else {
            b[0] = '+';
            b[1] = 0;
            int_text((int32_t)st, b + 1);
            text_cat(b, st % 12u ? " ST" : st == 12u ? " OCT" : " OCTS");
        }
        break;
    }
    default: break;
    }
}

static int orbit_quick(void)             /* EVERY under half a second: where ORBIT goes is not shown (it would only flicker) */
{
    uint32_t m = dr_orbit_mode();
    return m != OM_ARP && m != OM_ECHO && dr_orbit_s(dr_scene.p[P_ORB_PERIOD]) < 0.5f;
}
static void midi_settings_apply(void);
static void value_set(uint32_t id, uint32_t v)
{
    uint32_t i;
    switch (id) {
    case PID_SCALE:
        set.scale = (uint8_t)v;
        scale_apply();
        set_dirty = 1;
        break;
    case PID_ROOT:
        set.root = (uint8_t)v;
        scale_apply();
        set_dirty = 1;
        break;
    case PID_VIEW:
        set.view = (uint8_t)v;
        set_dirty = 1;
        for (i = 0; i < sizeof phos; i++)
            phos[i] = 0;
        break;
    case PID_MORPH: set.morph = (uint8_t)v; set_dirty = 1; break;
    case PID_MIDI_IN: set.midi_in = (uint8_t)v; set_dirty = 1; break;
    case PID_MIDI_OUT:
        set.midi_out = (uint8_t)v;
        set_dirty = 1;
        midi_settings_apply();
        break;
    case PID_LOCAL:
        set.local = (uint8_t)v;
        set_dirty = 1;
        if (!set.local)
            dr_all_off();
        break;
    case PID_VEL: set.vel = (uint8_t)v; set_dirty = 1; break;
    case PID_USB_LEVEL:
        set.usb_level = (uint8_t)v;
        set_dirty = 1;
        break;
    case PID_CLOCK_OUT:
        set.clock_out = (uint8_t)v;
        set_dirty = 1;
        dr_clock_out(set.clock_out);
        break;
    case PID_SPEAKER:                    /* ON: the speaker and USB both at MASTER; else USB at full level, the speaker
                                          * at MASTER (AUTO: silent while a computer records; OFF: silent) */
        set.speaker = (uint8_t)v;
        set_dirty = 1;
        plat_usb_audio_full(set.speaker != SPK_ON);
        ui.redraw_head = 1;
        break;
    case PID_NONE: break;
    default:
        dr_param(id, v);
        scene_dirty = 1;
        break;
    }
}

/* a 0..127 knob turned slowly: 3 a detent (43 a sweep), 2 where a step is heard (VOID's cutoff, a free delay time,
 * ATTACK), 1 for RATE (a BPM a detent); turned fast (a detent within 40 ms of the last): 8, 16 a sweep */
static int32_t knob_slow(uint32_t id, int32_t v)
{
    if (id == P_ARP_RATE)
        return 1;
    if (id == P_LD_ATTACK || (id == P_MASS && dr_scene.p[P_MODEL] == M_VOID) || (id == P_DLY_TIME && v < 100))
        return 2;
    return 3;
}
/* the knobs with a few values over 0..127 move one shown value a detent at any speed: the new raw value, else -1 */
static int32_t knob_shown(uint32_t id, int32_t v, int32_t e)
{
    int32_t i, n;
    uint32_t kind = PM[id].kind;
    if (id == P_SHAPE && dr_scene.p[P_MODEL] == M_DX) {   /* DX's eight voices */
        i = clamp((int32_t)dx_voice_of((uint32_t)v) + e, 0, (int32_t)DR_DX_VOICES - 1);
        return (i * 12700 + 798) / 799;
    }
    switch (kind) {
    case K_SEMI: i = (int32_t)((float)v * (24.99f / 127.0f)); n = 25; break;   /* LIFT: a semitone */
    case K_STEPS: i = v * 4 / 128; n = 4; break;
    case K_DIST: i = v * 3 / 128; n = 3; break;
    case K_BODIES: i = v >= 120 ? 7 : v * 7 / 127; n = 8; break;   /* 1 .. 7 OF 8, ALL */
    case K_DLYTIME:                      /* the divisions in time: one a detent; below the first, the free times */
        if (v < 100)
            return -1;
        i = (v - 100) / 4 + e;
        return i < 0 ? 99 : 100 + 4 * (i > (int32_t)DR_DLY_NDIV - 1 ? (int32_t)DR_DLY_NDIV - 1 : i);
    default: return -1;
    }
    i = clamp(i + e, 0, n - 1);
    switch (kind) {
    case K_SEMI: return (i * 12700 + 2498) / 2499;
    case K_STEPS: return i * 32;
    case K_DIST: return (i * 128 + 2) / 3;
    default: return i == 7 ? 127 : (i * 127 + 6) / 7;
    }
}
static const char *param_label(uint32_t id)   /* SHAPE is the voice on DX and the table on SCAN */
{
    if (id == P_SHAPE && dr_scene.p[P_MODEL] == M_DX)
        return "VOICE";
    if (id == P_SHAPE && dr_scene.p[P_MODEL] == M_SCAN)
        return "TABLE";
    return PM[id].label;
}
static void knob(uint32_t id, int32_t e, int fast)
{
    int32_t v = (int32_t)value_of(id), mx = (int32_t)value_max(id), lo = PM[id].kind == K_CHOUT ? 1 : 0;
    char t[16];
    if (id == PID_NONE)
        return;
    if (mx == 127) {
        int32_t sh = knob_shown(id, v, e);
        int exact = 0;
#ifdef PLAT_HOST
        exact = plat_host_exact;         /* the simulator's "spin": the raw values, a step each */
#endif
        if (exact)
            v += e;
        else if (sh >= 0)
            v = sh;
        else {
            int32_t nv = v + e * (fast ? 8 : knob_slow(id, v));
            if (PM[id].kind == K_DLYTIME && v < 100 && nv >= 100)   /* from the free times into the first division */
                nv = 100;
            v = nv;
        }
    } else
        v += e;
    v = clamp(v, lo, mx);
    if (v == (int32_t)value_of(id))
        return;
    value_set(id, (uint32_t)v);
    value_text(id, t);
    ui_say(param_label(id), t);
    ui.hud_id = (uint16_t)id;
    ui.redraw_foot = 1;
}

/* ------------------------------------------------------------------ scenes --- */
static void slot_title(uint32_t slot, char *t)   /* "F3 GLASS ORBIT" */
{
    char n[4];
    slot_name(slot, n);
    t[0] = 0;
    text_cat(t, n);
    text_cat(t, " ");
    text_cat(t, slot_scene(slot)->name);
}
static void latch_set(uint32_t on, int say);
/* a scene is a sound: landing on one morphs every sound value over MORPH, each body crossing into its model, while
 * the chord, LATCH, ORBIT, FREEZE, the octave and the scale stay as they are; while ORBIT runs, its page stays too (a
 * lit button is never repainted). chord (SEL + PRESET): the whole scene, its chord in today's ROOT replacing the
 * bodies, LATCH on, its ORBIT values; a scene without a chord leaves the bodies alone */
static const uint8_t ORB_P[] = {P_ORB_MODE, P_ORB_PERIOD, P_ORB_CHANCE, P_ORB_RANGE, P_GRAVITY, P_ORB_DIST, P_ORB_BODIES,
                                P_ARP_RATE, P_ARP_GATE, P_ARP_ORDER, P_ARP_OCT, P_ORB_PROG, P_ECHO_DECAY, P_ECHO_JUMP,
                                P_ORB_ROLL, P_ARP_RHYTHM, P_ARP_DIV};
static void scene_go(uint32_t slot, int say, int chord)
{
    char t[24];
    scene_t sc;
    uint32_t i;
    set.scene = (uint8_t)(slot % NSLOTS);
    set_dirty = 1;
    sc = *slot_scene(set.scene);
    chord = chord && sc.vel[0];
    if (dr_orbit_on() && !chord)
        for (i = 0; i < sizeof ORB_P; i++)
            sc.p[ORB_P[i]] = dr_scene.p[ORB_P[i]];
    if (chord && sc.root) {              /* stored on its own root: moved to ROOT, the nearer way */
        int32_t d = ((int32_t)set.root - (int32_t)(sc.root - 1u) + 12) % 12;
        if (d > 6)
            d -= 12;
        for (i = 0; i < DR_NV; i++)
            if (sc.vel[i])
                sc.note[i] = (uint8_t)clamp((int32_t)sc.note[i] + d, 12, 108);
    }
    dr_load(&sc, (float)set.morph, chord);
    dr_fx_bypass(sc.fx_off);
    if (chord)                           /* its bodies come in latched: LATCH on, so the LED says what the keys do */
        latch_set(1, 0);
    scene_dirty = 0;
    slot_title(set.scene, t);
    if (say)
        ui_say(chord ? "SCENE + CHORD" : set.morph ? "MORPH" : "SCENE", t);
    ui.redraw_head = ui.redraw_foot = 1;
}

static void save_confirm(void)
{
    char s[24];
    scene_t *dst = &bank.u[ui.save_to % NUSER];
    const char *keep = (set.scene >= NFACTORY && set.scene - NFACTORY == ui.save_to) ? dst->name : dr_scene.name;
    char name[SCENE_NAME];
    uint32_t i;
    for (i = 0; i < SCENE_NAME; i++)
        name[i] = keep[i];
    scene_capture(dst);
    for (i = 0; i < SCENE_NAME; i++)
        dst->name[i] = name[i];
    dr_scene.name[0] = 0;
    text_cat(dr_scene.name, name);
    set.scene = (uint8_t)(NFACTORY + ui.save_to);
    set_dirty = 1;
    ui.dialog = 0;
    slot_title(set.scene, s);
    if (project_save() == 0) {
        scene_dirty = 0;
        ui_say("SAVED", s);
    } else
        ui_say("SAVE", "FAILED");        /* still dirty: SAVE stays lit */
    ui.redraw_head = ui.redraw_foot = 1;
}

/* ------------------------------------------------------------------ the keys --- */
static void midi_note_out(uint32_t note, uint32_t vel, int on)
{
    uint32_t ch = (uint32_t)set.midi_out - 1u;
    if (on)
        plat_midi_send(0x09u | (0x90u | ch) << 8 | (note & 127u) << 16 | (vel & 127u) << 24, TO_USB | TO_BLE);
    else
        plat_midi_send(0x08u | (0x80u | ch) << 8 | (note & 127u) << 16, TO_USB | TO_BLE);
}
static uint32_t key_note(uint32_t k)     /* the key's own note: the scale glows and bounds ORBIT, it never moves a key */
{
    return (uint32_t)clamp(53 + (int32_t)k + 12 * set.octave, 0, 127);
}
static void key_on(uint32_t k, uint32_t vel)
{
    uint32_t note = key_note(k);
    if (!set.vel)
        vel = 100;
    ui.keynote[k] = (uint8_t)note;
    ui.keynotes |= 1u << k;
    ui.prs[k] = 0;
    ui.vel[k] = (uint8_t)vel;
    if (set.local) {                     /* LATCH on: a latched body pressed again is let go (its note off comes from
                                          * the engine); off: a lead over the bed, whatever is latched */
        if (set.latch) {
            if (!dr_body_latched(note))
                midi_note_out(note, vel, 1);
            dr_note_latched(note, vel);
        } else {
            midi_note_out(note, vel, 1);
            dr_lead(note, vel, 1);
            ui.keylead |= 1u << k;
        }
    } else {
        midi_note_out(note, vel, 1);
    }
}
static void key_off(uint32_t k)
{
    uint32_t note = ui.keynote[k], lead = ui.keylead >> k & 1u;
    ui.keynotes &= ~(1u << k);
    ui.keylead &= ~(1u << k);
    if (set.local) {                     /* the engine sends the note off (a latched body keeps it until let go) */
        if (lead)
            dr_lead(note, 0, 0);
        else
            dr_note(note, 0, 0);
    } else {
        midi_note_out(note, 0, 0);
    }
}
/* ------------------------------------------------------------------ a body's own values --- */
/* LATCH on, a latched body's key held a third of a second (or a knob turned while it is held): the four knobs are
 * that body's LEVEL, TONE, TUNE and ORBIT pin (several keys held: all of them, each from where it is); a tap still
 * lets the body go, at the key's release. Out over MIDI as NRPN: MSB 1..4 the value, LSB the body's note, data
 * entry the value (LEVEL 0 muted, 1..31 = -24..+6 dB; TONE, TUNE 64 + the value; ORBIT 0 moves, 127 pinned) */
static const char *const BE_LABEL[BE_COUNT] = {"LEVEL", "TONE", "TUNE", "ORBIT"};
static void body_value_text(uint32_t what, int32_t x, char *b)
{
    b[0] = 0;
    switch (what) {
    case BE_LEVEL:
        if (x == DR_LVL_MUTE) {
            text_cat(b, "MUTE");
            return;
        }
        if (x > 0)
            text_cat(b, "+");
        int_text(x, b + (x > 0));
        text_cat(b, " DB");
        break;
    case BE_TONE:
    case BE_TUNE:
        if (x > 0)
            text_cat(b, "+");
        int_text(x, b + (x > 0));
        text_cat(b, what == BE_TONE ? "%" : " CT");
        break;
    default: text_cat(b, x ? "PINNED" : "MOVES"); break;
    }
}
static uint32_t body_code(uint32_t what, int32_t x)   /* a value as NRPN data */
{
    switch (what) {
    case BE_LEVEL: return x == DR_LVL_MUTE ? 0u : (uint32_t)(x + 25);
    case BE_TONE:
    case BE_TUNE: return (uint32_t)(x + 64);
    default: return x ? 127u : 0u;
    }
}
static void midi_cc_out(uint32_t cc, uint32_t v)
{
    plat_midi_send(0x0Bu | (0xB0u | ((uint32_t)set.midi_out - 1u)) << 8 | (cc & 127u) << 16 | (v & 127u) << 24,
                   TO_USB | TO_BLE);
}
static void body_open(uint32_t bits)     /* the keys' bodies open: their values read once, the knobs move them on */
{
    uint32_t k, w;
    bits &= ~ui.bk_open;
    for (k = 0; k < NKEYS; k++)
        if (bits >> k & 1u)
            for (w = 0; w < BE_COUNT; w++)
                ui.bval[k][w] = (int8_t)dr_body_value(ui.keynote[k], w);
    if (bits) {
        ui.bk_open |= bits;
        ui.redraw_foot = 1;
    }
}
static void body_knob(uint32_t what, int32_t e, int fast)
{
    uint32_t k, said = 0;
    for (k = 0; k < NKEYS; k++) {
        int32_t x, was;
        char a[24], v[16];
        if (!(ui.bk_open >> k & 1u))
            continue;
        x = was = ui.bval[k][what];
        if (what == BE_PIN)
            x = e > 0;
        else if (what == BE_LEVEL)       /* a dB a detent, 3 fast; below -24 dB, muted */
            x = clamp(x + e * (fast ? 3 : 1), DR_LVL_MUTE, 6);
        else                             /* TONE 2 % a detent, TUNE a cent; 8 fast */
            x = clamp(x + e * (fast ? 8 : what == BE_TONE ? 2 : 1), -50, 50);
        if (x == was)
            continue;
        ui.bval[k][what] = (int8_t)x;
        dr_body_edit(ui.keynote[k], what, x);
        midi_cc_out(99, 1u + what);
        midi_cc_out(98, ui.keynote[k]);
        midi_cc_out(6, body_code(what, x));
        scene_dirty = 1;
        if (!said) {
            note_text((int32_t)ui.keynote[k], a);
            text_cat(a, " ");
            text_cat(a, BE_LABEL[what]);
            body_value_text(what, x, v);
            ui_say(a, v);
            said = 1;
        }
    }
    ui.redraw_foot = 1;
}

static void orbit_toggle(void)
{
    char t[24], v[12];
    dr_orbit(!dr_orbit_on());
    if (dr_orbit_on()) {
        uint32_t mode = dr_orbit_mode();
        t[0] = 0;
        text_cat(t, OMODE_NAME[mode]);
        text_cat(t, " ");
        if (mode == OM_ARP) {
            value_text(P_ARP_RATE, v);
            text_cat(t, v);
        } else {
            text_cat(t, "EVERY ");
            value_text(P_ORB_PERIOD, v);
            text_cat(t, v);
        }
        ui_say("ORBIT ON", t);
    } else
        ui_say("ORBIT", "OFF");
    ui.redraw_head = 1;
}
static void all_let_go(void)
{
    dr_all_off();
    ui.shift = 0;
    ui.redraw_head = 1;
    ui_say("EVERY BODY", "LET GO");
}
static void transpose_key(uint32_t k)    /* TRANSPOSE: the lowest latched body to this key, the rest with it */
{
    char t[8];
    if (!dr_latched()) {
        ui_say("TRANSPOSE", "LATCH FIRST");
        return;
    }
    dr_transpose_to(key_note(k));
    note_text((int32_t)key_note(k), t);
    ui_say("TRANSPOSE", t);
}
static void latch_set(uint32_t on, int say)   /* SEL: the LATCH mode. On, a key strikes a body that stays; off, the
                                               * keys play leads over the bed, which stays as it is */
{
    if (set.latch != on) {
        set.latch = (uint8_t)on;
        set_dirty = 1;
    }
    if (say) {
        if (on)
            ui_say("LATCH", "ON");
        else
            ui_say("LATCH OFF", "KEYS PLAY");
    }
    ui.redraw_head = 1;
}

/* ------------------------------------------------------------------ the FX layer --- */
#define NFXK 5u
static const uint8_t FXKEY[NFXK] = {0, 2, 4, 6, 7};   /* F3 G3 A3 B3 C4: ensemble, delay, reverb, shimmer, noise */
static const char *const FXNAME[NFXK] = {"ENSEMBLE", "DELAY", "REVERB", "SHIMMER", "NOISE"};
static const char *const FXSHORT[NFXK] = {"ENS", "DLY", "REV", "SHM", "NOISE"};
static const char *const FXLETTER[NFXK] = {"E", "D", "R", "S", "N"};
static uint32_t fx_keys_lit(void)
{
    uint32_t a = dr_fx_active(), m = 0, i;
    for (i = 0; i < NFXK; i++)
        if (a & (1u << i))
            m |= 1u << FXKEY[i];
    return m;
}
static void fx_key(uint32_t k)
{
    uint32_t i, b = dr_fx_bypass_get();
    if (k == FXKEY[4]) {                 /* the noise layer: its LEVEL is its switch; from zero, a third */
        uint32_t lv = dr_scene.p[P_NZ_LEVEL];
        char t[16];
        ui.fx_used = 1;
        if (lv) {
            ui.nz_last = (uint8_t)lv;
            value_set(P_NZ_LEVEL, 0);
            ui_say("NOISE", "OFF");
        } else {
            uint32_t saved = slot_scene(set.scene)->p[P_NZ_LEVEL];
            value_set(P_NZ_LEVEL, ui.nz_last ? ui.nz_last : saved ? saved : 44u);
            value_text(P_NZ_LEVEL, t);
            ui_say("NOISE", t);
        }
        ui.redraw_foot = 1;
        return;
    }
    for (i = 0; i < 4u; i++)
        if (FXKEY[i] == k) {
            uint32_t on = dr_fx_active() >> i & 1u;
            ui.fx_used = 1;
            if (on) {
                b |= 1u << i;
            } else {                     /* a stage the scene has at zero comes in at a usable amount */
                b &= ~(1u << i);
                if (i == 0u) {
                    if (!dr_scene.p[P_ENS])
                        value_set(P_ENS, ENS_CHORUS1);
                    if (!dr_scene.p[P_ENS_MIX])
                        value_set(P_ENS_MIX, 40u);
                } else if (i == 1u) {
                    if (!dr_scene.p[P_DLY_MIX])
                        value_set(P_DLY_MIX, 35u);
                } else {                 /* the shimmer sounds through the hall, so it brings the hall too */
                    if (i == 3u) {
                        b &= ~DR_FX_REV;
                        if (!dr_scene.p[P_SHIMMER])
                            value_set(P_SHIMMER, 30u);
                    }
                    if (!dr_scene.p[P_SPACE])
                        value_set(P_SPACE, 45u);
                }
            }
            dr_fx_bypass(b);
            dr_scene.fx_off = (uint8_t)b;
            scene_dirty = 1;
            ui.redraw_foot = 1;
            ui_say(FXNAME[i], on ? "OFF" : "ON");
            return;
        }
}

/* ------------------------------------------------------------------ Bluetooth --- */
int df_ble_off(void) { return set.ble_off; }
int df_shift(void) { return ui.shift; }
static void ble_toggle(void)
{
    set.ble_off = (uint8_t)!set.ble_off;
    set_dirty = 1;
    plat_ble_enable(!set.ble_off);
    ui_say("BLUETOOTH", set.ble_off ? "OFF" : "ON");
}
static void ble_device_say(void)
{
    char name[21];
    if (btdev_found(btdev_pick(), name, 0))
        ui_say("DEVICE", name);
    else
        ui_say("BLE", "SCANNING");
}
static void ble_device_button(void)
{
    int ev;
    if (!btdev_busy() && !ui.looking) {
        ui.looking = 1;
        ui_say("BLE", "SCANNING");
        return;
    }
    ev = btdev_save();
    ui.looking = 0;
    ui_say(ev == BTDEV_CONNECTING ? btdev_word(ev) : "BLE", ev == BTDEV_CONNECTING ? btdev_name() : btdev_word(ev));
}

/* ------------------------------------------------------------------ buttons --- */
static const uint8_t PAGE_BTN[PG_COUNT] = {B_HOME, B_EDIT, B_EDIT, B_EDIT, B_ENV, B_ENV, B_LFO, B_LFO, B_FX, B_FX, B_FX, B_FX, B_ARP, B_ARP,
                                           B_GLO, B_GLO, B_GLO};
static void page_to(uint32_t pg)
{
    ui.page = (uint8_t)pg;
    ui.redraw_foot = 1;
}
static void octave_step(int32_t d)
{
    int32_t o = clamp(set.octave + d, -2, 2);
    char t[4];
    if (o == set.octave)
        return;
    set.octave = (int8_t)o;
    set_dirty = 1;
    t[0] = o < 0 ? '-' : '+';
    t[1] = (char)('0' + (o < 0 ? -o : o));
    t[2] = 0;
    ui_say("OCTAVE", t);
    ui.redraw_head = 1;
}
static void button(uint32_t k, uint32_t now)
{
    if (ui.dialog) {                     /* the save dialog: SAVE saves, anything else backs out */
        if (k == B_SAVE)
            save_confirm();
        else if (k == B_REC && FM1B_BLE)
            ui.rec_t = now;
        else {
            ui.dialog = 0;
            ui.redraw_foot = 1;
            ui_say("SAVE", "CANCELLED");
        }
        return;
    }
    switch (k) {
    case B_HOME: page_to(PG_HOME); break;
    case B_EDIT: page_to(PG_BODY); break;
    case B_ENV: page_to(PG_ENV); break;
    case B_LFO: page_to(PG_MOTION); break;
    case B_FX:
        ui.fx_t = now;
        ui.fx_used = 0;
        break;
    case B_ARP: page_to(PG_ORBIT); break;
    case B_GLO:
        page_to(PG_GLOBAL);
        ui.glo_t = now;
        ui.glo_xf = 0;
        ui.glo_amt = 0;
        ui.glo_kt = 0;
        break;
    case B_SEL:
        ui.sel_down = now;
        ui.sel_used = 0;
        break;
    case B_PLAY:
        ui.play_down = now;
        ui.play_used = 0;
        break;
    case B_SEQ:
        dr_freeze(!dr_frozen());
        ui_say("FREEZE", dr_frozen() ? "ON" : "OFF");
        ui.redraw_head = 1;
        break;
    case B_SAVE:
        ui.dialog = 1;
        ui.save_to = (uint8_t)(set.scene >= NFACTORY ? set.scene - NFACTORY : 0u);
        ui.redraw_foot = 1;
        ui_say("SAVE TO", "PRESS SAVE AGAIN");
        break;
    case B_OCTDN: octave_step(-1); break;
    case B_OCTUP: octave_step(1); break;
    case B_REC:
        if (FM1B_BLE)
            ui.rec_t = now;
        else
            ui_say("BUTTON", BTN_NAME[k]);
        break;
    default:
        break;
    }
}

static void input(void)
{
    uint32_t keys, btn = plat_buttons(), k, fx_layer = btn >> B_FX & 1u, dkeys = plat_keys(), now = plat_ms();
    uint32_t lit_keys = 0, mid_keys = 0, glow_keys = 0, wink_keys = 0, lit_btn, mid_btn = 0, i, play_led = 0;
    int32_t e;
    dr_body_t b;
    touch_poll();
    keys = touch_keys();
    if (fx_layer && ui.fx_t) {           /* the layer: the plain matrix, not the sensed rows */
        for (k = 0; k < NFXK; k++) {
            uint32_t bit = 1u << FXKEY[k];
            if ((dkeys & bit) && !(ui.dkeys & bit))
                fx_key(FXKEY[k]);
        }
    }
    ui.dkeys = dkeys;
    for (k = 0; k < NKEYS; k++) {
        uint32_t bit = 1u << k;
        if ((keys & bit) && !(ui.keys & bit)) {
            if (fx_layer || ui.fx_t || now - ui.fx_up < 120u || ui.glo_t)
                continue;
            if (ui.sel_down && (btn >> B_SEL & 1u) && !ui.shift) {   /* SEL held, a key: TRANSPOSE */
                ui.sel_used = 1;
                ui.shift = 1;
                ui.redraw_head = 1;
            }
            if (ui.shift) {
                transpose_key(k);
                continue;
            }
            if (set.local && set.latch && dr_body_latched(key_note(k))) {   /* a latched body's key: held, its own
                                                                              * values; a tap lets it go at the release */
                ui.bk_down |= bit;
                ui.bk_t[k] = now;
                ui.keynote[k] = (uint8_t)key_note(k);
                continue;
            }
            key_on(k, touch_vel(k));
        } else if (!(keys & bit) && (ui.keys & bit)) {
            if (ui.bk_down & bit) {
                if (!(ui.bk_open & bit))
                    dr_note_latched(ui.keynote[k], 100);   /* a tap: let go (the engine sends the note off) */
                ui.bk_down &= ~bit;
                if (ui.bk_open & bit) {
                    ui.bk_open &= ~bit;
                    ui.redraw_foot = 1;  /* back to the page it was on */
                }
            } else if (ui.keynotes & bit)
                key_off(k);
        } else if ((keys & bit) && (ui.keynotes & bit) && touch_pressure(k) != ui.prs[k]) {
            ui.prs[k] = touch_pressure(k);
            if (set.local)
                dr_pressure(ui.keynote[k], ui.prs[k]);
            plat_midi_send(0x0Au | (0xA0u | ((uint32_t)set.midi_out - 1u)) << 8 | (uint32_t)ui.keynote[k] << 16
                           | (uint32_t)ui.prs[k] << 24, TO_USB | TO_BLE);
        }
    }
    for (k = 0; k < NKEYS; k++)          /* a body's key held a third of a second: open */
        if ((ui.bk_down & ~ui.bk_open) >> k & 1u && now - ui.bk_t[k] >= 333u)
            body_open(1u << k);
    for (k = 0; k < NB; k++) {
        uint32_t bit = 1u << k;
        if ((btn & bit) && !(ui.btn & bit))
            button(k, now);
    }
    if (ui.fx_t) {                       /* FX: a tap opens the effects pages; held, the keys switched effects */
        if (btn >> B_FX & 1u) {
            if (now - ui.fx_t > 600u && !ui.fx_used)
                ui.fx_used = 1;          /* held long with nothing switched: the layer only, no page */
        } else {
            if (!ui.fx_used)
                page_to(PG_FX_ENS);
            ui.fx_t = 0;
            ui.fx_up = now;
        }
    }
    if (ui.glo_t) {                      /* GLO held: lean on a key and its pressure blends what is heard toward the
                                          * next scene (the bar in the header); let go and the blend stays */
        if (btn >> B_GLO & 1u) {
            if (now - ui.glo_t >= 300u) {
                uint32_t prs = 0, next = (set.scene + 1u) % NSLOTS, plain = 0;
                for (k = 0; k < NKEYS; k++)
                    if (dkeys >> k & 1u) {
                        if (!touch_sensed(k))
                            plain = 1;
                        else if (touch_pressure(k) > prs)
                            prs = touch_pressure(k);
                    }
                if (plain) {             /* the top row reads no pressure: held, the blend climbs from where it
                                          * is to full in two seconds */
                    uint32_t held;
                    if (!ui.glo_kt)
                        ui.glo_kt = now - (uint32_t)ui.glo_amt * 2000u / 127u;
                    held = now - ui.glo_kt;
                    if (held > 2000u)
                        held = 2000u;
                    if (held * 127u / 2000u > prs)
                        prs = held * 127u / 2000u;
                } else {
                    ui.glo_kt = 0;
                }
                if (dkeys) {
                    char t[24], v[8];
                    slot_title(next, t);
                    dr_morph_hold(slot_scene(next), (float)prs * (1.0f / 127.0f));
                    ui.glo_xf = 1;
                    ui.glo_amt = (uint8_t)prs;
                    int_text((int32_t)(prs * 100u / 127u), v);
                    text_cat(v, "%");
                    ui_say(t, v);
                } else if (!ui.glo_xf && now - ui.glo_t < 400u) {
                    ui_say("CROSSFADE", "LEAN ON A KEY");
                }
            }
        } else {
            if (ui.glo_xf) {
                dr_morph_hold_end();
                ui_say("CROSSFADE", "HELD");
            }
            ui.glo_t = 0;
            ui.redraw_head = 1;
        }
    }
    if (ui.play_down) {                  /* PLAY: a tap runs or stops ORBIT; held, its page */
        if (btn >> B_PLAY & 1u) {
            if (now - ui.play_down >= 600u && !ui.play_used) {
                ui.play_used = 1;
                page_to(PG_ORBIT);
                ui_say("ARP", "ORBIT");
            }
        } else {
            if (!ui.play_used)
                orbit_toggle();
            ui.play_down = 0;
        }
    }
    if (ui.sel_down) {                   /* SEL: a tap switches LATCH; held a second, every body is let go */
        if (btn >> B_SEL & 1u) {
            if (now - ui.sel_down >= 1000u && !ui.sel_used) {
                ui.sel_used = 1;
                all_let_go();
            }
        } else {
            if (ui.sel_chord) {          /* PRESET turned while SEL was held: the candidate lands with its chord */
                ui.sel_chord = 0;
                if (ui.sel_slot >= 0) {
                    int32_t slot = ui.sel_slot;
                    ui.sel_slot = -1;
                    scene_go((uint32_t)slot, 1, 1);
                }
            }
            if (!ui.sel_used) {
                if (ui.shift) {          /* out of TRANSPOSE: the keys strike bodies again */
                    ui.shift = 0;
                    latch_set(1, 1);
                } else {
                    latch_set(!set.latch, 1);
                }
            }
            ui.sel_down = 0;
        }
    }
    if (FM1B_BLE && ui.rec_t) {
        if (btn >> B_REC & 1u) {
            if (now - ui.rec_t >= 1000u) {
                ui.rec_t = 0;
                ble_toggle();
            }
        } else {
            ui.rec_t = 0;
            if (FM1B_BLE_CENTRAL && !set.ble_off)
                ble_device_button();
            else
                ui_say("BUTTON", "REC");
        }
    }
    for (k = 0; k < NE; k++) {
        int fast;
        if ((e = plat_enc((int)k)) == 0)
            continue;
        fast = e > 1 || e < -1 || now - ui.enc_t[k] < 40u;   /* two detents in a read, or one hard on the last */
        ui.enc_t[k] = now;
        if (ui.dialog) {
            if (k == EN_PRESET || k == EN_SELECT || k == EN_K1) {   /* any of them picks the slot */
                char t[24];
                ui.save_to = (uint8_t)clamp((int32_t)ui.save_to + e, 0, (int32_t)NUSER - 1);
                slot_title(NFACTORY + ui.save_to, t);
                ui_say("SAVE TO", t);
                ui.redraw_foot = 1;
            }
            continue;
        }
        if (k == EN_ALGO && FM1B_BLE_CENTRAL && ui.looking) {
            btdev_choose(btdev_pick() + e);
            ble_device_say();
        } else if (k == EN_PRESET) {     /* scroll: the candidate shows, a bar filling under it; it loads once PRESET
                                          * rests 0.7 s. SEL held: it lands with its chord when SEL is let go */
            char t[24];
            int32_t from = ui.sel_slot >= 0 ? ui.sel_slot : (int32_t)set.scene;
            ui.sel_slot = clamp(from + e, 0, (int32_t)NSLOTS - 1);
            ui.sel_t = now;
            if (ui.sel_down && (btn >> B_SEL & 1u)) {
                ui.sel_chord = 1;
                ui.sel_used = 1;         /* SEL's tap and its let-go are not this */
            }
            slot_title((uint32_t)ui.sel_slot, t);
            ui_say(ui.sel_chord ? "SCENE + CHORD" : "SCENE", t);
        } else if (k == EN_ALGO) {
            knob(P_MODEL, e, 0);
        } else if (k == EN_SELECT) {     /* the page before or after */
            uint32_t pg = (uint32_t)clamp((int32_t)ui.page + e, 0, PG_COUNT - 1);
            if (pg != ui.page) {
                page_to(pg);
                ui_say(PAGES[pg].button, PAGES[pg].title);
            }
        } else if (ui.bk_down) {         /* a body's key held: the knobs are its own (open at once) */
            body_open(ui.bk_down);
            body_knob(k - EN_K1, e, fast);
        } else {
            knob(page_param(ui.page, k - EN_K1), e, fast);
        }
    }
    if (ui.sel_slot >= 0 && !ui.sel_chord && now - ui.sel_t >= 700u) {
        int32_t slot = ui.sel_slot;
        ui.sel_slot = -1;
        if (slot != (int32_t)set.scene)
            scene_go((uint32_t)slot, 1, 0);
    }
    ui.keys = keys;
    ui.btn = btn;
    /* the lights. Keys, three levels, each one meaning: LIT is what your fingers address now (a body under LATCH
     * on, a lead while held), MID is the machine's (a fading body, a moved body's home key, the bed under LATCH
     * off, a lead releasing, a body off the strip at the end key), GLOW is the scale. ORBIT's destination sits at
     * mid and blinks through the last eighth of the countdown; a lead's strike winks its key off for 60 ms, so a
     * runner's step or an accent on a lit key is seen. Buttons: the page's button lit (mid under the FX layer),
     * SEL LATCH, SEQ FREEZE, SAVE with something to save (blinking in its dialog), PLAY in the mode's colour (green
     * WANDER, orange CHORD, orange SCATTER winking at each move, red ARP winking at each step), OCT- / OCT+ mid one
     * octave out and lit two; a held button blinks from 300 ms until its hold fires */
    for (i = 0; i < DR_NV; i++)
        if (dr_body(i, &b)) {
            int32_t kk = (int32_t)(b.pitch + 0.5f) - 53 - 12 * set.octave, kh = (int32_t)b.home - 53 - 12 * set.octave;
            if (kk < 0 || kk >= (int32_t)NKEYS) {   /* off the strip: the end key, mid */
                mid_keys |= 1u << (kk < 0 ? 0u : NKEYS - 1u);
            } else if (b.fading || (!set.latch && b.latched)) {
                mid_keys |= 1u << kk;
            } else {
                lit_keys |= 1u << kk;
            }
            if (kh != kk && kh >= 0 && kh < (int32_t)NKEYS)
                mid_keys |= 1u << kh;
        }
    for (i = 0; i < DR_NV; i++) {        /* where ORBIT is taking a body: mid, blinking as the move nears */
        uint32_t to;
        if (!orbit_quick() && dr_orbit_pending(i, &to)) {
            int32_t kk = (int32_t)to - 53 - 12 * set.octave;
            if (kk >= 0 && kk < (int32_t)NKEYS) {
                if (dr_orbit_next() >= 0.875f && (now / 125u) & 1u)
                    lit_keys |= 1u << kk;
                else
                    mid_keys |= 1u << kk;
            }
        }
    }
    for (i = 0; i < DR_NL; i++) {        /* a lead: a wink at its strike, lit while held, mid as it releases */
        uint32_t here = dr_lead_body(i, &b) != 0;
        int32_t kk = (int32_t)b.home - 53 - 12 * set.octave;
        if (here && (!(ui.lead_seen >> i & 1u) || ui.lead_home[i] != b.home || (ui.lead_rel >> i & 1u && !b.fading))) {
            ui.lead_t[i] = now;          /* struck (or struck again while releasing) */
            ui.lead_home[i] = b.home;
        }
        ui.lead_seen = (ui.lead_seen & ~(1u << i)) | here << i;
        ui.lead_rel = (ui.lead_rel & ~(1u << i)) | (uint32_t)(here && b.fading) << i;
        if (!here || kk < 0 || kk >= (int32_t)NKEYS)
            continue;
        if (now - ui.lead_t[i] < 60u)
            wink_keys |= 1u << kk;
        else if (b.fading)
            mid_keys |= 1u << kk;
        else
            lit_keys |= 1u << kk;
    }
    if (set.scale != SC_CHROMA)
        for (k = 0; k < NKEYS; k++) {
            uint32_t note = (uint32_t)clamp(53 + (int32_t)k + 12 * set.octave, 0, 127);
            if (dr_snap(note) == note)
                glow_keys |= 1u << k;
        }
    for (k = 0; k < NKEYS; k++)          /* a body's key counting to open: it blinks */
        if ((ui.bk_down & ~ui.bk_open) >> k & 1u) {
            if ((now / 100u) & 1u)
                wink_keys |= 1u << k;
            else
                lit_keys |= 1u << k;
        }
    lit_keys &= ~wink_keys;
    mid_keys &= ~(lit_keys | wink_keys);
    glow_keys &= ~(lit_keys | mid_keys | wink_keys);
    if (fx_layer) {
        lit_keys = fx_keys_lit();
        mid_keys = glow_keys = 0;
    }
    ui.oct_lit = (uint8_t)(set.octave >= 2 ? 2u : set.octave <= -2 ? 1u : 0u);
    ui.oct_mid = (uint8_t)(set.octave == 1 ? 2u : set.octave == -1 ? 1u : 0u);
    if (dr_orbit_on()) {
        uint32_t m = dr_orbit_mode(), steps = dr_arp_steps();
        play_led = (m == OM_ARP ? 0u : LED_PLAY_GREEN) | (m == OM_WANDER ? 0u : 1u << B_PLAY);
        if (steps != ui.arp_steps) {
            ui.arp_steps = steps;
            ui.arp_step_t = now;
        }
        if ((m == OM_SCATTER && now - ui.orbit_flash_t < 150u) || (m == OM_ARP && now - ui.arp_step_t < 60u))
            play_led = 0;
    }
    lit_btn = btn | (set.latch || ui.shift ? 1u << B_SEL : 0u) | (dr_frozen() ? 1u << B_SEQ : 0u) | play_led
            | (uint32_t)ui.oct_lit << B_OCTDN;
    if ((now / 150u) & 1u) {             /* a hold under way blinks until it fires, over the mode's light too */
        if (ui.sel_down && !ui.sel_used && now - ui.sel_down >= 300u)
            lit_btn &= ~(1u << B_SEL);
        if (ui.play_down && !ui.play_used && now - ui.play_down >= 300u)
            lit_btn &= ~(1u << B_PLAY | LED_PLAY_GREEN);
        if (ui.rec_t && now - ui.rec_t >= 300u)
            lit_btn &= ~(1u << B_REC);
    }
    if (ui.dialog)
        lit_btn |= ((now / 250u) & 1u) ? 1u << B_SAVE : 0u;
    else {
        lit_btn |= (set_dirty || scene_dirty) ? 1u << B_SAVE : 0u;
        if (fx_layer)
            mid_btn |= 1u << PAGE_BTN[ui.page];
        else
            lit_btn |= 1u << PAGE_BTN[ui.page];
    }
    plat_lights(lit_btn, lit_keys, (1u << NB) - 1u, glow_keys, 1);
    plat_lights_mid(mid_btn | (uint32_t)ui.oct_mid << B_OCTDN, mid_keys);
}
static void ui_input_only(void) { input(); }

/* ------------------------------------------------------------------ the phosphor --- */
static inline void ph_dot(int32_t x, int32_t y, uint32_t hue, uint32_t lvl)
{
    uint8_t *p;
    if ((uint32_t)x >= (uint32_t)SKY_W || (uint32_t)y >= (uint32_t)SKY_H)
        return;
    p = &phos[y * SKY_W + x];
    if ((*p & 31u) < lvl)
        *p = (uint8_t)(hue << 5 | lvl);
}
static void ph_disc(int32_t cx, int32_t cy, int32_t r, uint32_t hue, uint32_t lvl)
{
    int32_t dx, dy, r2 = r * r + 1;
    for (dy = -r; dy <= r; dy++)
        for (dx = -r; dx <= r; dx++) {
            int32_t d2 = dx * dx + dy * dy;
            if (d2 <= r2) {
                uint32_t l = (uint32_t)((int32_t)lvl * (r2 - d2 / 2) / r2);
                if (l > 31u)
                    l = 31u;
                ph_dot(cx + dx, cy + dy, hue, l);
            }
        }
}
static void ph_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t hue, uint32_t lvl)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int32_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy, guard = 600;
    while (guard--) {
        int32_t e2 = 2 * err;
        ph_dot(x0, y0, hue, lvl);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}
static void ph_ellipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint32_t hue, uint32_t lvl, uint32_t every)
{
    uint32_t i, n = (uint32_t)(rx + ry) * 2u;
    if (n < 24u)
        n = 24u;
    for (i = 0; i < n; i += every) {
        float t = (float)i / (float)n;
        ph_dot(cx + (int32_t)((float)rx * fm_sin_turns(t + 0.25f)), cy + (int32_t)((float)ry * fm_sin_turns(t)), hue, lvl);
    }
}
static void ph_compose(uint32_t shift, uint32_t dim)   /* the phosphor onto the canvas, each pixel faded a step for
                                                        * the next frame (dim: the HUD's words go over it, and every
                                                        * level drops to two fifths, so the beams stay in view) */
{
    uint32_t i, x, y;
    uint16_t hz = swap16(C_HORIZON);
    for (i = 0; i < (uint32_t)(SKY_W * SKY_H); i++) {
        uint32_t v = phos[i], l = v & 31u, d = (l >> shift) + 1u;
        cv_px[i] = lut[(v & 0xE0u) | (dim ? (l * 2u + 2u) / 5u : l)];
        if (l)
            phos[i] = (uint8_t)((v & 0xE0u) | (l > d ? l - d : 0u));
    }
    for (y = SKY_H - 6u; y < (uint32_t)SKY_H; y++)   /* the horizon: the bottom six rows lifted, dithered, more lower down */
        for (x = 0; x < (uint32_t)SKY_W; x++) {
            i = y * SKY_W + x;
            if (!(phos[i] & 31u) && BAYER[y & 3u][x & 3u] < (y - (SKY_H - 6u)) * 5u / 2u)
                cv_px[i] = hz;
        }
}
static void lut_build(void)              /* each hue's 32 levels from its ramp's six keys, the sky at 0 */
{
    static const uint8_t KL[7] = {31, 26, 20, 13, 7, 2, 0};
    static const uint8_t SKY3[3] = {0, 4, 16};
    uint32_t h, l, i, c;
    for (h = 0; h < 8u; h++)
        for (l = 0; l < 32u; l++) {
            float t, v[3];
            for (i = 0; !(l <= KL[i] && l >= KL[i + 1u]); i++)
                ;
            t = (float)(KL[i] - l) / (float)(KL[i] - KL[i + 1u]);
            for (c = 0; c < 3u; c++) {
                float a = RAMP[h][i][c], b = i + 1u < 6u ? RAMP[h][i + 1u][c] : SKY3[c];
                v[c] = a + (b - a) * t + 0.5f;
            }
            lut[h << 5 | l] = swap16(RGB((uint32_t)v[0], (uint32_t)v[1], (uint32_t)v[2]));
        }
}
static inline uint16_t hue_px(uint32_t hue, uint32_t lvl) { return lut[(hue & 7u) << 5 | (lvl > 31u ? 31u : lvl)]; }   /* a
                                         * ramp's colour as a canvas pixel */
static void stars_init(void)
{
    uint32_t i;
    ui.rng = 0xC0FFEE11u;
    for (i = 0; i < NSTARS; i++) {
        ui.rng = hash32(ui.rng + i);
        ui.star[i].x = (uint16_t)(ui.rng % (SKY_W * 256u));
        ui.star[i].y = (uint16_t)((ui.rng >> 12) % ((SKY_H - 8u) * 256u));   /* above the horizon */
        ui.star[i].z = (uint8_t)(1u + (ui.rng >> 24) % 3u);
        ui.star[i].hue = HUE_STAR;       /* stars are the sky's: no body colour among them */
    }
}
static void stars_draw(float drift)      /* the stars drift at DRIFT's pace and each twinkles on its own slow sine (3
                                          * to 8 s), the brightest flashing a cross at its peak; FREEZE holds them */
{
    uint32_t i, still = dr_frozen();
    for (i = 0; i < NSTARS; i++) {
        uint32_t z = ui.star[i].z, hsh = hash32(i * 2654435761u + 7u), lvl;
        float per = 75.0f + (float)(hsh % 126u), s = still ? 0.0f : fm_sin_turns((float)ui.frame / per + (float)(hsh >> 8 & 255u) * (1.0f / 256.0f));
        int32_t v = still ? 0 : (int32_t)(z * 9u) * (int32_t)(40.0f + 220.0f * drift) / 64;
        int32_t x = (int32_t)ui.star[i].x - v, y;
        if (x < 0)
            x += SKY_W * 256;
        ui.star[i].x = (uint16_t)x;
        x >>= 8;
        y = ui.star[i].y >> 8;
        lvl = (uint32_t)(6.0f + (float)z * 3.0f + (4.0f + (float)z * 2.0f) * s);
        ph_dot(x, y, HUE_STAR, lvl);
        if (z == 3u && s > 0.93f) {      /* a glint */
            ph_dot(x - 1, y, HUE_STAR, 18u);
            ph_dot(x + 1, y, HUE_STAR, 18u);
            ph_dot(x, y - 1, HUE_STAR, 18u);
            ph_dot(x, y + 1, HUE_STAR, 18u);
            ph_dot(x, y, HUE_STAR, 31u);
        }
    }
}
static void freeze_frame(void)           /* FREEZE: four ice brackets in the corners (and the stars, the mist and the
                                          * breathing hold still) */
{
    int32_t k;
    for (k = 0; k < 12; k++) {
        ph_dot(k, 0, HUE_STAR, 24u);
        ph_dot(k, 1, HUE_STAR, 24u);
        ph_dot(0, k, HUE_STAR, 24u);
        ph_dot(1, k, HUE_STAR, 24u);
        ph_dot(SKY_W - 1 - k, 0, HUE_STAR, 24u);
        ph_dot(SKY_W - 1 - k, 1, HUE_STAR, 24u);
        ph_dot(SKY_W - 1, k, HUE_STAR, 24u);
        ph_dot(SKY_W - 2, k, HUE_STAR, 24u);
        ph_dot(k, SKY_H - 1, HUE_STAR, 24u);
        ph_dot(k, SKY_H - 2, HUE_STAR, 24u);
        ph_dot(0, SKY_H - 1 - k, HUE_STAR, 24u);
        ph_dot(1, SKY_H - 1 - k, HUE_STAR, 24u);
        ph_dot(SKY_W - 1 - k, SKY_H - 1, HUE_STAR, 24u);
        ph_dot(SKY_W - 1 - k, SKY_H - 2, HUE_STAR, 24u);
        ph_dot(SKY_W - 1, SKY_H - 1 - k, HUE_STAR, 24u);
        ph_dot(SKY_W - 2, SKY_H - 1 - k, HUE_STAR, 24u);
    }
}
static uint32_t body_hue(uint32_t note) { return 2u + (note % 12u) * 5u % 6u; }
#define CX 120
#define CY (SKY_H / 2)

/* ---------------------------------------------------------------- pixel art --- */
/* The MOUTH view is drawn as sprites at 2x (120 x 40 game pixels on the 240 x 80 strip), authored as rows of
 * characters: ' ' clear, k outline, d dark, g grey, w white, s/S/t skin, r/R/q red, p pink, m the mouth's dark,
 * y/Y the sleeve, b/c/o/v/G/M colours; A, B and H are a pill's two halves and highlight, given per pill. */
typedef struct {
    uint8_t w, h;
    const char *const *rows;
} sprite_t;
static uint16_t spal[128];
static void spal_build(void)
{
    static const struct { char c; uint8_t r, g, b; } P[] = {
        {'k', 10, 8, 14}, {'d', 58, 56, 70}, {'g', 128, 126, 140}, {'w', 250, 250, 255},
        {'s', 235, 172, 122}, {'S', 255, 214, 176}, {'t', 178, 108, 70}, {'r', 222, 30, 42}, {'R', 255, 96, 108},
        {'q', 128, 10, 22}, {'p', 255, 124, 164}, {'m', 36, 0, 12}, {'y', 240, 196, 50}, {'Y', 170, 120, 20},
        {'b', 80, 120, 255}, {'c', 90, 222, 255}, {'o', 255, 150, 60}, {'v', 170, 110, 255}, {'G', 90, 240, 150},
        {'M', 255, 90, 210}, {'n', 20, 28, 60},
    };
    uint32_t i;
    for (i = 0; i < 128u; i++)
        spal[i] = 0;
    for (i = 0; i < sizeof P / sizeof P[0]; i++)
        spal[(uint8_t)P[i].c] = swap16(RGB(P[i].r, P[i].g, P[i].b));
}
static inline void px2(int32_t x, int32_t y, uint16_t c)   /* a game pixel: 2 x 2 on the canvas, clipped */
{
    int32_t dx, dy;
    for (dy = 0; dy < 2; dy++)
        for (dx = 0; dx < 2; dx++) {
            int32_t px = x + dx, py = y + dy;
            if ((uint32_t)px < cv_w && (uint32_t)py < cv_h)
                cv_px[(uint32_t)py * cv_w + (uint32_t)px] = c;
        }
}
/* a sprite at canvas x, y; rot 1 turns it a quarter turn; a, b, h: the colours for A, B and H */
static void spr(int32_t x, int32_t y, const sprite_t *s, int rot, uint16_t a, uint16_t b, uint16_t h)
{
    uint32_t r, c, W = rot ? s->h : s->w, H = rot ? s->w : s->h;
    for (r = 0; r < H; r++)
        for (c = 0; c < W; c++) {
            /* a quarter turn: output (r, c) comes from source row c, column w-1-r */
            const char *row = s->rows[rot ? c : r];
            uint32_t sc = rot ? (uint32_t)(s->w - 1u - r) : c, len = 0;
            char ch;
            uint16_t col;
            while (row[len])
                len++;
            ch = sc < len ? row[sc] : ' ';
            if (ch == ' ')
                continue;
            col = ch == 'A' ? a : ch == 'B' ? b : ch == 'H' ? h : spal[(uint8_t)ch & 127u];
            if (!col)
                continue;
            px2(x + (int32_t)c * 2, y + (int32_t)r * 2, col);
        }
}
static const char *const BOTTLE[] = {
    "  kkkkkkkkkkkk  ",
    " kooooooooooook ",
    " koOOOOOOOOOOok ",
    " kooooooooooook ",
    " kookkkkkkkkkook",
    " kokwwwwwwwwwkok",
    " kokwkkkwwkwkkok",
    " kokwkwkwwwkwkok",
    " kokwkkkwwkwkkok",
    " kokwkwkwwwwwkok",
    " kokwkwwkwwwwkok",
    " kokwwwwwwwwwkok",
    " kookkkkkkkkkook",
    " kooooooooooook ",
    " koOOOOOOOOOOok ",
    " kooooooooooook ",
    " kooooooooooook ",
    "  kkkkkkkkkkkk  ",
};
static const sprite_t SPR_BOTTLE = {16, 18, BOTTLE};
static const char *const CAP_SHUT[] = {
    " kkkkkkkkkkkkkk ",
    "kwwwwwwwwwwwwwwk",
    "kwggwwggwwggwwgk",
    "kwwwwwwwwwwwwwwk",
    " kkkkkkkkkkkkkk ",
};
static const char *const CAP_HALF[] = {
    "  kk            ",
    " kwwk           ",
    "kwggwk          ",
    "kwwwwwk         ",
    " kwwggwk        ",
    "  kwwwwwk       ",
    "   kwwggwk      ",
    "    kwwwwwk     ",
    "     kkkkkkk    ",
};
static const char *const CAP_OPEN[] = {
    " kkkkk          ",
    "kwwwwk          ",
    "kwggwk          ",
    "kwwwwk          ",
    "kwggwk          ",
    "kwwwwk          ",
    "kwggwk          ",
    "kwwwwk          ",
    "kwggwk          ",
    "kwwwwk          ",
    "kwggwk          ",
    "kwwwwk          ",
    "kwggwk          ",
    "kwwwwk          ",
    "kkkkkk          ",
};
static const sprite_t SPR_CAP[3] = {{16, 5, CAP_SHUT}, {16, 9, CAP_HALF}, {16, 15, CAP_OPEN}};
static const char *const MOUTH_OPEN[] = {
    "           kkkkkkkkkkk      ",
    "        kkkRRRRRRRRRRRkkk   ",
    "      kkRRrrrrrrrrrrrrrRRkk ",
    "    kkRrrrrrrrrrrrrrrrrrrrRk",
    "   kRrrrrkkkkkkkkkkkkkkrrrrk",
    "  kRrrrkwwwwkwwwwkwwwwkkrrrk",
    "  krrrkwwwwwkwwwwwkwwwwwkrrk",
    "  krrkwwwwwwkwwwwwwkwwwwwkrk",
    "  krkkkkkkkkkkkkkkkkkkkkkkrk",
    "  krkmmmmmmmmmmmmmmmmmmmmkrk",
    "  krkmmmmmmmmmmmmmmmmmmmmkrk",
    "  krkmmmmmmmmmmmmmmmmmmmmkrk",
    "  krkmmmmmmmmmmmmmmmmmmmmkrk",
    "  krkmmmmmmkkppppppkkmmmmkrk",
    "  krkmmmmmkppppppppppkmmmkrk",
    "  krkkkkkkkkkkkkkkkkkkkkkkrk",
    "  krrkwwwwwkwwwwwkwwwwwkrrrk",
    "  kqrrkwwwwkwwwwkwwwwkrrrqk ",
    "   kqrrkkkkkkkkkkkkkkrrrqk  ",
    "    kqqrrrrrrrrrrrrrrrrqk   ",
    "     kkqqrrrrrrrrrrrqqkk    ",
    "       kkkqqqqqqqqqkkk      ",
    "          kkkkkkkkk         ",
};
static const char *const MOUTH_HALF[] = {
    "                            ",
    "                            ",
    "                            ",
    "           kkkkkkkkkkk      ",
    "        kkkRRRRRRRRRRRkkk   ",
    "      kkRRrrrrrrrrrrrrrRRkk ",
    "    kkRrrrrkkkkkkkkkkkrrrrRk",
    "   kRrrrkwwwwkwwwwkwwwwkrrrk",
    "  kRrrrkwwwwwkwwwwwkwwwwkrrk",
    "  krrkkkkkkkkkkkkkkkkkkkkkrk",
    "  krkmmmmmmmmmmmmmmmmmmmmkrk",
    "  krkmmmmmmmkkppppppkkmmmkrk",
    "  krkkkkkkkkkkkkkkkkkkkkkkrk",
    "  krrkwwwwwkwwwwwkwwwwwkrrrk",
    "  kqrrkwwwwkwwwwkwwwwkrrrqk ",
    "   kqrrkkkkkkkkkkkkkkrrrqk  ",
    "    kqqrrrrrrrrrrrrrrrrqk   ",
    "     kkqqrrrrrrrrrrrqqkk    ",
    "       kkkqqqqqqqqqkkk      ",
    "          kkkkkkkkk         ",
};
static const char *const MOUTH_SHUT[] = {
    "                            ",
    "                            ",
    "                            ",
    "                            ",
    "                            ",
    "                            ",
    "           kkkkkkkkkkk      ",
    "        kkkRRRRRRRRRRRkkk   ",
    "      kkRRrrrrrrrrrrrrrRRkk ",
    "    kkRrrrrrrrrrrrrrrrrrrrRk",
    "   kRrrrrrrrrrrrrrrrrrrrrrrk",
    "  kkkkkkkkkkkkkkkkkkkkkkkkkk",
    "  kqrrrrrrrrrrrrrrrrrrrrrrqk",
    "   kqqrrrrrrrrrrrrrrrrrrqqk ",
    "    kkqqrrrrrrrrrrrrrrqqkk  ",
    "      kkkqqqqqqqqqqqqkkk    ",
    "         kkkkkkkkkkk        ",
};
static const sprite_t SPR_MOUTH[3] = {{28, 23, MOUTH_OPEN}, {28, 20, MOUTH_HALF}, {28, 17, MOUTH_SHUT}};
static const char *const PILL[] = {
    "  kkkkkkkkkkkk  ",
    " kAHHHHHkBBBBBk ",
    "kAAHHHHHkBBBBBBk",
    "kAAAAAAAkBBBBBBk",
    "kAAAAAAAkBBBBBBk",
    " kAAAAAAkBBBBBk ",
    "  kkkkkkkkkkkk  ",
};
static const sprite_t SPR_PILL = {16, 7, PILL};
static const char *const PLANET[] = {
    "      kkkk      ",
    "    kkbbbbkk    ",
    "   kbbccbbbbk   ",
    "  kbbbbbbcbbbk  ",
    "kkkkkkkkkkkkkkkk",
    "kggggggggggggggk",
    " kkkkkkkkkkkkkk ",
    "  kbbbbbbbbbbk  ",
    "   kbbbbbbbbk   ",
    "    kkbbbbkk    ",
    "      kkkk      ",
};
static const sprite_t SPR_PLANET = {16, 11, PLANET};

#define BOTTLE_X 10
#define BOTTLE_Y (SKY_H - 42)       /* the bottle stands at the bottom left; its cap sits on its neck */
#define MOUTHS_X 180
#define MOUTHS_Y (CY - 23)
static uint16_t body_px(uint32_t note, int light)   /* a pill's colour: its body's hue, as the beam and the key */
{
    return hue_px(body_hue(note), light ? 29u : 26u);
}
static void sky_mouth(void)
{
    uint32_t i, frame_mouth, now = plat_ms();
    float out = dr_out_level(), tip_x = BOTTLE_X + 16, tip_y = BOTTLE_Y - 6;
    dr_body_t b;
    int eating = 0, released = 0;
    uint16_t cw = swap16(C_BRIGHT), cb = swap16(RGB(120, 150, 255));
    for (i = 0; i < NSTARS; i++) {       /* chunky stars, a few of them blue, twinkling */
        uint32_t tw = hash32(ui.frame / 6u + i * 131u) & 7u;
        int32_t x = (int32_t)(ui.star[i].x >> 8) & ~1, y = (int32_t)(ui.star[i].y >> 8) & ~1;
        if (tw < 6u)
            px2(x, y, ui.star[i].hue == HUE_BLUE ? cb : cw);
        else if (tw == 6u)
            cv_pset(x, y, C_BRIGHT);
    }
    spr(118, SKY_H - 24, &SPR_PLANET, 0, 0, 0, 0);
    /* the bottle: a new body pops its cap (shut, half, open), a pill tumbles out of the neck; the cap shuts
     * again a moment after the last one */
    if (!ui.hand_throw)
        for (i = 0; i < DR_NV; i++)
            if (ui.eat[i] == 4u && ui.pill_live[i]) {
                ui.hand_throw = (uint8_t)(i + 1u);
                ui.hand_t = ui.cap == 2u ? 0.3f : 0.0f;
                break;
            }
    if (ui.hand_throw) {
        float t = ui.hand_t, was = t;
        t += 0.07f;
        ui.hand_t = t;
        ui.cap = t < 0.12f ? 0u : t < 0.25f ? 1u : 2u;
        if (was < 0.4f && t >= 0.4f)
            released = 1;
        if (t >= 0.7f) {
            ui.hand_throw = 0;
            ui.cap_t = now;
        }
    } else if (ui.cap && now - ui.cap_t > 900u) {
        ui.cap = (uint8_t)(ui.cap == 2u && now - ui.cap_t < 1050u ? 1u : 0u);
    }
    spr(BOTTLE_X, BOTTLE_Y, &SPR_BOTTLE, 0, 0, 0, 0);
    spr(BOTTLE_X, BOTTLE_Y - (int32_t)SPR_CAP[ui.cap].h * 2 + (ui.cap ? 2 : 0), &SPR_CAP[ui.cap], 0, 0, 0, 0);
    if (released) {                      /* the pill comes out of the neck */
        i = (uint32_t)ui.hand_throw - 1u;
        ui.eat[i] = 5;
        ui.eat_t[i] = 0.0f;
        ui.px[i] = tip_x;
        ui.py[i] = tip_y;
    }
    for (i = 0; i < DR_NV; i++)
        if (ui.eat[i] == 1u || ui.eat[i] == 2u)
            eating = 1;
    if (ui.chomp > 0.0f) {               /* closing on a pill, then open again */
        ui.chomp += 0.1f;
        if (ui.chomp >= 1.0f)
            ui.chomp = 0.0f;
    }
    frame_mouth = out > 0.12f ? 0u : 1u;
    if (ui.chomp > 0.0f)
        frame_mouth = ui.chomp < 0.2f ? 1u : ui.chomp < 0.6f ? 2u : ui.chomp < 0.8f ? 1u : 0u;
    spr(MOUTHS_X, MOUTHS_Y + (int32_t)(2u * (SPR_MOUTH[0].h - SPR_MOUTH[frame_mouth].h)) / 2, &SPR_MOUTH[frame_mouth], 0, 0, 0, 0);
    if (ui.crumbs) {                     /* crumbs fly for a moment after a chomp */
        uint32_t k;
        ui.crumbs--;
        for (k = 0; k < 4u; k++) {
            uint32_t h = hash32(ui.frame * 31u + k * 977u);
            px2((MOUTHS_X - 12 + (int32_t)(h % 30u)) & ~1, (CY - 14 + (int32_t)((h >> 8) % 28u)) & ~1, cw);
        }
    }
    if (!eating && (hash32(ui.frame * 2654435761u) % 220u) == 0u) {   /* now and then, a snack: a pill that comes back */
        uint32_t cand[DR_NV], n = 0;
        for (i = 0; i < DR_NV; i++)
            if (dr_body(i, &b) && !b.fading && !ui.eat[i])
                cand[n++] = i;
        if (n) {
            i = cand[hash32(ui.frame * 7919u) % n];
            ui.eat[i] = 1;
            ui.snack[i] = 1;
            ui.eat_t[i] = 0.0f;
        }
    }
    {                                    /* each pill's place in the air: over its key, high notes high; one that
                                          * would sit on another goes a row up or down, a few tries at most (an
                                          * unbounded retry here was the 2026-11-01 watchdog reset) */
        uint32_t j, tries;
        for (i = 0; i < DR_NV; i++) {
            if (!dr_body(i, &b)) {
                ui.spot_y[i] = -100.0f;
                continue;
            }
            {                            /* over the key that struck it; a MIDI note past the keys, in a column */
                int32_t kk = (int32_t)b.home - 53 - 12 * set.octave;
                ui.spot_x[i] = kk >= 0 && kk < (int32_t)NKEYS ? (float)key_x[kk] : 58.0f + 36.0f * (float)(i % 4u);
            }
            ui.spot_y[i] = (float)(SKY_H - 14) - fm_clampf((b.pitch - 36.0f) * (1.0f / 60.0f), 0.0f, 1.0f) * (float)(SKY_H - 30);
            tries = 0;
            for (j = 0; j < i; j++)
                if (ui.spot_y[j] > -50.0f && fm_fabsf(ui.spot_x[i] - ui.spot_x[j]) < 34.0f
                    && fm_fabsf(ui.spot_y[i] - ui.spot_y[j]) < 16.0f) {
                    ui.spot_y[i] = ui.spot_y[j] + (ui.spot_y[j] > (float)(SKY_H - 24) ? -16.0f : 16.0f);
                    if (++tries < 8u)
                        j = 0;           /* and check again from the start, up to eight times */
                }
        }
    }
    for (i = 0; i < DR_NV; i++) {
        float sx, sy, tx, ty, k = 0.12f, d;
        int rot = 0;
        uint16_t ca, cb2, ch;
        if (!dr_body(i, &b)) {
            ui.eat[i] = 0;
            ui.pill_live[i] = 0;
            continue;
        }
        sx = ui.spot_x[i];
        sy = ui.spot_y[i] + 2.0f * fm_sin_turns((float)ui.frame * 0.006f + 0.13f * (float)i);
        if (!ui.pill_live[i]) {          /* new: it waits in the hand for its throw */
            ui.pill_live[i] = 1;
            ui.eat[i] = 4;
            ui.eat_t[i] = 0.0f;
        }
        if (b.fading && ui.eat[i] != 1u && ui.eat[i] != 2u && ui.eat[i] != 4u) {   /* let go: the mouth takes it */
            ui.eat[i] = 1;
            ui.snack[i] = 0;
            ui.eat_t[i] = 0.0f;
        }
        tx = sx;
        ty = sy;
        switch (ui.eat[i]) {
        case 1:                          /* flying to the mouth */
            tx = (float)MOUTHS_X + 28.0f;
            ty = (float)CY + 2.0f;
            k = 0.16f;
            ui.eat_t[i] += 0.05f;
            rot = ((uint32_t)(ui.eat_t[i] * 8.0f) & 1u) != 0u;
            d = fm_fabsf(ui.px[i] - tx) + fm_fabsf(ui.py[i] - ty);
            if (d < 6.0f) {
                ui.eat[i] = 2;
                ui.eat_t[i] = 0.0f;
                ui.chomp = 0.05f;
                ui.crumbs = 12;
            }
            break;
        case 2:                          /* inside; a snack comes back out */
            ui.eat_t[i] += 0.05f;
            if (ui.eat_t[i] >= 1.0f) {
                if (ui.snack[i]) {
                    ui.eat[i] = 3;
                    ui.eat_t[i] = 0.0f;
                } else {
                    ui.eat_t[i] = 1.0f;
                }
            }
            continue;
        case 3:                          /* spat back out to its place */
            ui.eat_t[i] += 0.04f;
            rot = ((uint32_t)(ui.eat_t[i] * 6.0f) & 1u) != 0u;
            if (ui.eat_t[i] >= 1.0f)
                ui.eat[i] = 0;
            break;
        case 4:                          /* in the hand, not yet thrown */
            continue;
        case 5: {                        /* out of the bottle: up, then an arc to its place, tumbling */
            float e;
            ui.eat_t[i] += 0.05f;
            e = ui.eat_t[i] > 1.0f ? 1.0f : ui.eat_t[i];
            rot = ((uint32_t)(e * 6.0f) & 1u) != 0u;
            ui.px[i] += (tx - ui.px[i]) * (0.04f + 0.3f * e * e);
            ui.py[i] += (ty - 22.0f * fm_sin_turns(e * 0.5f) - ui.py[i]) * (0.12f + 0.3f * e);
            if (ui.eat_t[i] >= 1.0f)
                ui.eat[i] = 0;
            k = 0.0f;
            break;
        }
        default:
            break;
        }
        ui.px[i] += (tx - ui.px[i]) * k;
        ui.py[i] += (ty - ui.py[i]) * k;
        ca = body_px(b.home, 0);
        cb2 = swap16(RGB(255, 255, 255));   /* the other half, white */
        ch = body_px(b.home, 1);
        if (b.fading) {
            ca = swap16(RGB(110, 110, 120));
            cb2 = swap16(RGB(160, 160, 170));
            ch = swap16(RGB(190, 190, 200));
        }
        spal['k'] = b.latched ? swap16(C_LATCH) : swap16(RGB(10, 8, 14));   /* latched: a sun outline */
        spr(((int32_t)ui.px[i] - (rot ? 7 : 16)) & ~1, ((int32_t)ui.py[i] - (rot ? 16 : 7)) & ~1, &SPR_PILL, rot, ca, cb2, ch);
        spal['k'] = swap16(RGB(10, 8, 14));
    }
    if (dr_frozen()) {                   /* ice: a dotted frame of blue blocks */
        int32_t x, y;
        for (x = 0; x < SKY_W; x += 8)
            px2(x, 0, cb), px2(x + 2, SKY_H - 2, cb);
        for (y = 0; y < SKY_H; y += 8)
            px2(0, y, cb), px2(SKY_W - 2, y + 2, cb);
    }
}

static void sky_orbits(void)
{
    uint32_t i, lvl;
    float out = dr_out_level();
    dr_body_t b;
    int32_t r;
    stars_draw((float)dr_scene.p[P_DRIFT] * (1.0f / 127.0f));
    r = 4 + (int32_t)(out * 4.0f);
    ph_disc(CX, CY, r, HUE_SUN, 31);
    ph_ellipse(CX, CY, r + 2, r + 2, HUE_SUN, 11u, 1);
    ph_ellipse(CX, CY, r + 4 + (int32_t)(ui.frame % 3u), r + 4 + (int32_t)(ui.frame % 3u), HUE_SUN, 5u, 3);
    for (i = 0; i < DR_NV; i++) {
        float rr, px, py, spd;
        int32_t rx, ry, pr;
        if (!dr_body(i, &b))
            continue;
        rr = 13.0f + (b.pitch - 36.0f) * (25.0f / 60.0f);
        rr = fm_clampf(rr, 9.0f, 38.0f);
        spd = (0.0022f + 0.06f / rr) * (0.55f + 0.9f * b.spark);
        ui.ang[i] += spd;
        if (ui.ang[i] >= 1.0f)
            ui.ang[i] -= 1.0f;
        rx = (int32_t)(rr * 1.7f);
        ry = (int32_t)(rr * 0.75f);
        ph_ellipse(CX, CY, rx, ry, body_hue(b.home), b.fading ? 4u : 7u, 2u);
        px = (float)CX + rr * 1.7f * fm_sin_turns(ui.ang[i] + 0.25f);
        py = (float)CY + rr * 0.75f * fm_sin_turns(ui.ang[i]);
        lvl = (uint32_t)(12.0f + 19.0f * b.level);
        if (b.fading && lvl > 18u)
            lvl = 18u;
        pr = 1 + (int32_t)(3.0f * b.level);
        ph_disc((int32_t)px, (int32_t)py, pr + 2, body_hue(b.home), lvl / 3u);   /* a halo */
        ph_disc((int32_t)px, (int32_t)py, pr, body_hue(b.home), lvl);
        if (b.latched)
            ph_ellipse((int32_t)px, (int32_t)py, pr + 3, pr + 3, HUE_SUN, 20u, 2u);   /* a ring: latched, the sun's */
    }
    if (dr_frozen())
        freeze_frame();
}

static void sky_nebula(void)
{
    static float col[SKY_W];
    static uint8_t chue[SKY_W];
    uint32_t i, h, x;
    dr_body_t b;
    float amp[8], t = (float)ui.frame * 0.0025f;
    stars_draw((float)dr_scene.p[P_DRIFT] * (1.0f / 127.0f) * 0.5f);
    for (x = 0; x < (uint32_t)SKY_W; x++)
        col[x] *= 0.8f;
    for (i = 0; i < DR_NV; i++) {
        float f0;
        if (!dr_body(i, &b) || !dr_harmonics(i, amp))
            continue;
        f0 = 440.0f * fm_exp2f((b.pitch - 69.0f) * (1.0f / 12.0f));
        for (h = 0; h < 8u; h++) {
            float f = f0 * (float)(h + 1u), v = amp[h] * b.level * 2.4f, fx;
            int32_t xc, d;
            if (v < 0.02f || f > 15000.0f)
                continue;
            if (v > 1.0f)
                v = 1.0f;
            fx = fm_log2f(f * (1.0f / 30.0f)) * (240.0f / 9.0f);
            xc = (int32_t)fx;
            for (d = -9; d <= 9; d++) {
                int32_t xx = xc + d;
                float w = v * (1.0f - (float)(d * d) * (1.0f / 100.0f));
                if ((uint32_t)xx < (uint32_t)SKY_W && w > col[xx]) {
                    col[xx] = w;
                    chue[xx] = (uint8_t)body_hue(b.home);
                }
            }
        }
    }
    for (x = 0; x < (uint32_t)SKY_W; x++) {
        float I = col[x], yc, hh;
        int32_t y, y0, y1;
        if (I < 0.03f)
            continue;
        yc = (float)CY + 9.0f * fm_sin_turns((float)x * 0.0028f + t) + 4.0f * fm_sin_turns((float)x * 0.011f - t * 1.3f);
        hh = 4.0f + 32.0f * I;
        y0 = (int32_t)(yc - hh);
        y1 = (int32_t)(yc + hh);
        for (y = y0; y <= y1; y++) {
            float k = 1.0f - fm_fabsf((float)y - yc) / hh;
            uint32_t lvl = (uint32_t)(31.0f * I * k * k);
            if (lvl > 1u)
                ph_dot((int32_t)x, y, chue[x], lvl);
        }
    }
    for (i = 0; i < DR_NV; i++)          /* each body's fundamental: a mark under the band */
        if (dr_body(i, &b)) {
            float f0 = 440.0f * fm_exp2f((b.pitch - 69.0f) * (1.0f / 12.0f));
            int32_t xc = (int32_t)(fm_log2f(f0 * (1.0f / 30.0f)) * (240.0f / 9.0f));
            ph_line(xc, SKY_H - 7, xc, SKY_H - 2, body_hue(b.home), b.fading ? 10u : 22u);
        }
    if (dr_frozen())
        freeze_frame();
}

static void sky_scope(void)
{
    const int16_t *l, *r;
    uint32_t n, i;
    int32_t px = 0, py = 0;
    dr_body_t b;
    float sx, sy;
    dr_scope(&l, &r, &n);
    ph_ellipse(CX, CY, 38, 38, HUE_CYAN, 9u, 2u);
    ph_ellipse(CX, CY, 25, 25, HUE_CYAN, 6u, 3u);
    ph_ellipse(CX, CY, 12, 12, HUE_CYAN, 6u, 3u);
    ph_line(CX - 38, CY, CX + 38, CY, HUE_CYAN, 5u);
    ph_line(CX, 2, CX, SKY_H - 2, HUE_CYAN, 5u);
    ui.sweep += 0.012f;
    if (ui.sweep >= 1.0f)
        ui.sweep -= 1.0f;
    sx = fm_sin_turns(ui.sweep + 0.25f);
    sy = fm_sin_turns(ui.sweep);
    ph_line(CX, CY, CX + (int32_t)(38.0f * sx), CY + (int32_t)(38.0f * sy), HUE_CYAN, 22u);
    for (i = 0; i < DR_NV; i++) {        /* each body a blip: its pitch class round the dial, its octave the radius */
        float a, rr, d;
        uint32_t lvl;
        if (!dr_body(i, &b))
            continue;
        a = (float)(b.home % 12u) * (1.0f / 12.0f);
        rr = 7.0f + fm_clampf((b.pitch - 24.0f) * (1.0f / 12.0f), 0.0f, 7.0f) * 4.5f;
        d = fm_fabsf(a - ui.sweep);
        if (d > 0.5f)
            d = 1.0f - d;
        lvl = 8u + (uint32_t)(14.0f * b.level) + (d < 0.06f ? 9u : 0u);
        ph_disc(CX + (int32_t)(rr * fm_sin_turns(a + 0.25f)), CY + (int32_t)(rr * fm_sin_turns(a)),
                1 + (int32_t)(1.5f * b.level), body_hue(b.home), lvl > 31u ? 31u : lvl);
    }
    for (i = 0; i < n; i += 2u) {        /* the output, left against right: the phosphor keeps the figure */
        int32_t x = CX + ((int32_t)l[i] * 36) / 32768, y = CY - ((int32_t)r[i] * 36) / 32768;
        if (i)
            ph_line(px, py, x, y, HUE_GREEN, 28u);
        px = x;
        py = y;
    }
    if (dr_frozen())
        freeze_frame();
}

/* ------------------------------------------------------------------ the beams --- */
static void keys_geom(void)
{
    uint32_t w, b;
    for (w = 0; w < 16u; w++) {
        key_x[WHITE_K[w]] = (uint8_t)(15u * w + 7u);
        key_white[WHITE_K[w]] = (uint8_t)(w + 1u);
    }
    for (b = 0; b < 11u; b++) {          /* a black key sits on the boundary after the white key below it */
        uint32_t k = BLACK_K[b];
        for (w = 0; w < 16u; w++)
            if (WHITE_K[w] == k - 1u)
                key_x[k] = (uint8_t)(15u * (w + 1u));
        key_white[k] = 0;
    }
}
static float beam_x(float pitch, int *off)   /* where a pitch stands over the strip; off: past its ends */
{
    float kk = pitch - 53.0f - 12.0f * (float)set.octave, f;
    int32_t k0;
    *off = 0;
    if (kk < -0.5f) {
        *off = 1;
        return 3.0f;
    }
    if (kk > 26.5f) {
        *off = 1;
        return 236.0f;
    }
    if (kk < 0.0f)
        return (float)key_x[0];
    if (kk >= 26.0f)
        return (float)key_x[26];
    k0 = (int32_t)kk;
    f = kk - (float)k0;
    return (float)key_x[k0] + f * ((float)key_x[k0 + 1] - (float)key_x[k0]);
}
/* a beam over its key: a 7-pixel core rising from level 20 at the top to the key's pure hue at its foot through the
 * dither, its breathing moving it, edges at 13, a dithered halo into the sky, a white-hot head three rows deep that
 * marks the level, and a white-hot foot where it stands on its key; f scales it (fading, or behind an open body) */
static void beam(int32_t x, int32_t top, uint32_t hue, float f, float breath, int thin)
{
    int32_t y, dx, w = thin ? 1 : 3;
    for (y = top; y < SKY_H; y++) {
        float frac = (float)(y - top) / (float)(SKY_H - top > 1 ? SKY_H - 1 - top : 1), core = 20.0f + 6.0f * frac + breath;
        uint32_t lo = (uint32_t)core, t = (uint32_t)((core - (float)lo) * 16.0f);
        for (dx = -w; dx <= w; dx++)
            ph_dot(x + dx, y, hue, (uint32_t)((float)(BAYER[y & 3][(x + dx) & 3] < t ? lo + 1u : lo) * f));
        if (thin)
            continue;
        for (dx = 4; dx <= 5; dx++) {
            ph_dot(x - dx, y, hue, (uint32_t)(13.0f * f));
            ph_dot(x + dx, y, hue, (uint32_t)(13.0f * f));
        }
        if (BAYER[y & 3][(x - 6) & 3] < 8u)
            ph_dot(x - 6, y, hue, (uint32_t)(7.0f * f));
        if (BAYER[y & 3][(x + 6) & 3] < 8u)
            ph_dot(x + 6, y, hue, (uint32_t)(7.0f * f));
    }
    for (dx = -w; dx <= w; dx++) {       /* the head, its top corners cut */
        if (dx != -w && dx != w)
            ph_dot(x + dx, top, hue, (uint32_t)(31.0f * f));
        ph_dot(x + dx, top + 1, hue, (uint32_t)(31.0f * f));
        ph_dot(x + dx, top + 2, hue, (uint32_t)(31.0f * f));
        if (dx > -3 && dx < 3)
            ph_dot(x + dx, SKY_H - 1, hue, (uint32_t)(31.0f * f));
    }
}
static void sky_beams(void)                  /* each body a beam over its key: its level the height, a sun spark when
                                              * latched, a sun bar at its foot when pinned, white frames when open;
                                              * an ORBIT move slides it and marks where it was; the noise a mist */
{
    uint32_t i, k, to, now = plat_ms(), still = dr_frozen(), quick = orbit_quick();
    int32_t arp = dr_arp_note();
    dr_body_t b;
    float nlv, nnote, ncol, nband, frac = dr_orbit_next();
    stars_draw((float)dr_scene.p[P_DRIFT] * (1.0f / 127.0f) * 0.4f);
    dr_noise(&nlv, &nnote, &ncol, &nband);
    if (!still)                          /* the mist's dither drifts with WIND */
        ui.mist += 0.05f + 0.3f * (float)dr_scene.p[P_NZ_WIND] * (1.0f / 127.0f);
    if (nlv > 0.004f) {                  /* the noise: a mound of dithered mist where its band sits, warm when brown,
                                          * pale when white, feathered at the top, lifting with each gust */
        int off, hw = 8 + (int32_t)((1.0f - nband) * 40.0f), dx, y, sc = (int32_t)ui.mist;
        int32_t x = (int32_t)(beam_x(nnote, &off) + 0.5f);
        float a = fm_sqrtf(nlv), peak = 5.0f + 8.0f * a, hgt = 8.0f + a * (float)SKY_H * 0.4f;
        uint32_t hue = ncol < 0.5f ? HUE_SUN : HUE_STAR;
        for (dx = -hw; dx <= hw; dx++) {
            float kk = 1.0f - fm_fabsf((float)dx) / (float)(hw + 1);
            int32_t top = SKY_H - 1 - (int32_t)(hgt * kk);
            for (y = top; y < SKY_H; y++) {
                float feather = (float)(y - top) * (1.0f / 6.0f);
                if (BAYER[y & 3][(x + dx + sc) & 3] < 8u)
                    ph_dot(x + dx, y, hue, (uint32_t)(peak * kk * (feather > 1.0f ? 1.0f : feather)));
            }
        }
    }
    for (i = 0; i < DR_NV; i++) {
        int off, open = 0;
        int32_t x, h, top, y, dx;
        uint32_t hue;
        float f;
        if (!dr_body(i, &b))
            continue;
        for (k = 0; k < NKEYS; k++)
            if (ui.bk_open >> k & 1u && ui.keynote[k] == b.home && !b.fading)
                open = 1;
        x = (int32_t)(beam_x(b.pitch, &off) + 0.5f);
        hue = body_hue(b.home);
        h = (int32_t)(b.level * (float)(SKY_H - 10));
        if (h < 3)
            h = 3;
        top = SKY_H - 1 - h;
        f = (b.fading ? 0.5f : 1.0f) * (ui.bk_open && !open ? 0.5f : 1.0f);
        beam(x, top, hue, f, still ? 0.0f : (b.spark - 0.5f) * 3.0f, off);
        if (b.latched && !b.fading) {    /* latched: a sun spark over the head */
            ph_dot(x, top - 5, HUE_SUN, (uint32_t)(31.0f * f));
            ph_dot(x - 1, top - 4, HUE_SUN, (uint32_t)(31.0f * f));
            ph_dot(x + 1, top - 4, HUE_SUN, (uint32_t)(31.0f * f));
            ph_dot(x, top - 3, HUE_SUN, (uint32_t)(31.0f * f));
            ph_dot(x, top - 4, HUE_STAR, (uint32_t)(31.0f * f));
        }
        if (b.pinned && !b.fading)       /* pinned: a sun bar across its foot */
            for (dx = -7; dx <= 7; dx++) {
                ph_dot(x + dx, SKY_H - 1, HUE_SUN, (uint32_t)(31.0f * f));
                ph_dot(x + dx, SKY_H - 2, HUE_SUN, (uint32_t)(26.0f * f));
            }
        if (open)                        /* open: white frames, your hand */
            for (y = top - 7; y < SKY_H; y++) {
                ph_dot(x - 8, y, HUE_STAR, 31u);
                ph_dot(x + 8, y, HUE_STAR, 31u);
            }
        if (arp >= 0 && (int32_t)(b.pitch + 0.5f) == arp && now - ui.arp_step_t < 80u) {   /* an ARP step on its
                                                                                        * note: the head flares */
            for (dx = -2; dx <= 2; dx++) {
                ph_dot(x + dx, top + 1, HUE_STAR, 31u);
                ph_dot(x, top + 1 + dx, HUE_STAR, 31u);
            }
        }
        if (fm_fabsf(b.pitch - (float)b.home) > 0.05f) {   /* moved: a mark where it was struck */
            int32_t hx = (int32_t)(beam_x((float)b.home, &off) + 0.5f);
            for (y = SKY_H - 4; y < SKY_H; y++)
                ph_dot(hx, y, hue, 24u);
        }
        if (!quick && dr_orbit_pending(i, &to)) {   /* where ORBIT is taking it: a dithered ghost of the beam at the
                                                    * destination, and a line reaching out from its head as the
                                                    * countdown fills */
            int32_t tx = (int32_t)(beam_x((float)to, &off) + 0.5f), ty = top < 7 ? 7 : top;
            int32_t ex = x + (int32_t)((float)(tx - x) * frac);
            for (y = ty; y < SKY_H; y++)
                for (dx = -2; dx <= 2; dx++)
                    if ((y + dx) & 1)
                        ph_dot(tx + dx, y, hue, 13u);
            ph_line(x, ty - 5, ex, ty - 5, hue, 26u);
            ph_line(x, ty - 6, ex, ty - 6, hue, 18u);
            ph_dot(ex, ty - 7, hue, 31u);
            ph_dot(ex, ty - 4, hue, 31u);
        }
    }
    for (i = 0; i < DR_NL; i++) {        /* a lead: a thin white beam over its key (your hand), its edges its note's
                                          * hue, half as bright as it releases */
        int off;
        int32_t x, h, y;
        float f;
        if (!dr_lead_body(i, &b))
            continue;
        x = (int32_t)(beam_x((float)b.home, &off) + 0.5f);
        h = (int32_t)(b.level * (float)(SKY_H - 10));
        if (h < 3)
            h = 3;
        f = b.fading ? 0.5f : 1.0f;
        for (y = SKY_H - 1 - h; y < SKY_H; y++) {
            ph_dot(x, y, HUE_STAR, (uint32_t)((y < SKY_H - h + 2 ? 31.0f : 26.0f) * f));
            ph_dot(x - 1, y, body_hue(b.home), (uint32_t)(20.0f * f));
            ph_dot(x + 1, y, body_hue(b.home), (uint32_t)(20.0f * f));
        }
    }
    if (still)
        freeze_frame();
}

/* ------------------------------------------------------------------ values, drawn --- */
/* one bar per kind of value: segments filling for a continuous value (the last one white-hot), a cell per option
 * for a list (the one chosen lit), two cells for a switch, the hits and rests for RHYTHM, a bar from the centre for a
 * value either way (a body's TONE and TUNE) */
enum { BAR_NONE, BAR_FILL, BAR_LIST, BAR_SWITCH, BAR_RHYTHM, BAR_CENTRE };
static uint32_t bar_kind(uint32_t id, uint32_t *n, uint32_t *sel, float *frac)
{
    uint32_t v, mx;
    *n = *sel = 0;
    *frac = 0.0f;
    if (id == PID_NONE)
        return BAR_NONE;
    v = value_of(id);
    mx = value_max(id);
    *frac = mx ? (float)v / (float)mx : 0.0f;
    if (id == P_SHAPE && dr_scene.p[P_MODEL] == M_DX) {
        *n = DR_DX_VOICES;
        *sel = dx_voice_of(v);
        return BAR_LIST;
    }
    switch (PM[id].kind) {
    case K_ONOFF: *sel = v; return BAR_SWITCH;
    case K_RHYTHM: *sel = v; return BAR_RHYTHM;
    case K_STEPS: *n = 4; *sel = v * 4u / 128u; return BAR_LIST;
    case K_DIST: *n = 3; *sel = v * 3u / 128u; return BAR_LIST;
    case K_BODIES: *n = 8; *sel = v >= 120u ? 7u : v * 7u / 127u; return BAR_LIST;
    case K_DLYTIME:
        if (v >= 100u) {
            *n = DR_DLY_NDIV;
            *sel = (v - 100u) / 4u;
            return BAR_LIST;
        }
        *frac = (float)v / 99.0f;
        return BAR_FILL;
    default: break;
    }
    if (mx < 127u && mx + 1u <= 14u) {
        *n = mx + 1u;
        *sel = v;
        return BAR_LIST;
    }
    return BAR_FILL;
}
static void draw_bar(int32_t x, int32_t y, int32_t w, uint32_t kind, uint32_t n, uint32_t sel, float frac)
{
    int32_t i;
    switch (kind) {
    case BAR_FILL: {
        int32_t segs = w / 4, lit = (int32_t)(frac * (float)segs + 0.5f);
        if (lit == 0 && frac > 0.0f)
            lit = 1;
        for (i = 0; i < segs; i++)
            cv_rect(x + 4 * i, y, 3, 5, i < lit ? (i == lit - 1 ? C_BRIGHT : C_TEXT) : C_RULE);
        break;
    }
    case BAR_LIST:
        for (i = 0; i < (int32_t)n; i++) {
            int32_t x0 = x + i * w / (int32_t)n, x1 = x + (i + 1) * w / (int32_t)n - (n > 8u ? 1 : 2);
            cv_rect(x0, y, x1 - x0, 5, (uint32_t)i == sel ? C_TEXT : C_RULE);
        }
        break;
    case BAR_SWITCH:
        cv_rect(x, y, w / 2 - 1, 5, sel ? C_RULE : C_LABEL);
        cv_rect(x + w / 2 + 1, y, w - w / 2 - 1, 5, sel ? C_TEXT : C_RULE);
        break;
    case BAR_RHYTHM: {                   /* "5/8": five hits spread over eight steps, as the engine spreads them */
        const char *r = DR_RHYTHM_NAME[sel % DR_RHYTHMS];
        uint32_t k = (uint32_t)(r[0] - '0'), len = (uint32_t)(r[2] - '0'), j;
        if (r[3] >= '0' && r[3] <= '9')
            len = len * 10u + (uint32_t)(r[3] - '0');
        for (j = 0; j < len; j++) {
            int32_t x0 = x + (int32_t)j * w / (int32_t)len, x1 = x + ((int32_t)j + 1) * w / (int32_t)len - 1;
            if ((j * k) % len < k)
                cv_rect(x0, y, x1 - x0, 5, C_TEXT);
            else
                cv_rect(x0, y + 4, x1 - x0, 1, C_RULE);
        }
        break;
    }
    case BAR_CENTRE: {                   /* frac 0.5 is the centre */
        int32_t c = x + w / 2, v = x + (int32_t)(frac * (float)w + 0.5f);
        cv_rect(x, y, w, 5, C_RULE);
        cv_rect(v < c ? v : c, y, (v < c ? c - v : v - c) > 1 ? (v < c ? c - v : v - c) : 2, 5, C_TEXT);
        cv_rect(c - 1, y - 2, 2, 9, C_LABEL);
        break;
    }
    default: break;
    }
}
static int value_split(const char *t, char *num, char *unit)   /* "334MS": "334" and "MS"; a word: 0 */
{
    uint32_t i = 1, j = 0;
    num[0] = unit[0] = 0;
    if (!((t[0] >= '0' && t[0] <= '9') || ((t[0] == '+' || t[0] == '-') && t[1] >= '0' && t[1] <= '9')))
        return 0;
    num[0] = t[0];
    while (t[i] && i < 15u && ((t[i] >= '0' && t[i] <= '9') || t[i] == '.' || t[i] == '/')) {
        num[i] = t[i];
        i++;
    }
    num[i] = 0;
    while (t[i] == ' ')
        i++;
    while (t[i] && j < 15u)
        unit[j++] = t[i++];
    unit[j] = 0;
    return 1;
}
static void cell_value(int32_t x, int32_t y, const char *t)   /* a number large with its unit small, on one
                                                              * baseline, so the value never changes size with its
                                                              * unit; a word in S (XS when wider than the cell) */
{
    char num[16], unit[16];
    if (value_split(t, num, unit)) {
        const felucca_font_t *f = text_w(&FONT_M, num) + 1 + text_w(&FONT_T, unit) <= 58 ? &FONT_M : &FONT_S;
        int32_t nx = cv_text(x, f == &FONT_M ? y : y + 6, f, num, C_BRIGHT);
        cv_text(nx + 1, y + 10, &FONT_T, unit, C_LABEL);
    } else {
        const felucca_font_t *f = text_w(&FONT_S, t) <= 56 ? &FONT_S : &FONT_XS;
        cv_text(x, f == &FONT_S ? y + 6 : y + 8, f, t, C_BRIGHT);
    }
}

static void browse_draw(uint32_t now)    /* PRESET turning: the scene's neighbours, its name, the 32 slots as a row of
                                          * stars (F then U, the candidate tall and white, the scene playing in cyan),
                                          * and the line filling until it lands; SEL held: + CHORD */
{
    char t[24], u[28];
    int32_t s = ui.sel_slot, i;
    const felucca_font_t *f;
    if (s > 0) {
        slot_title((uint32_t)s - 1u, t);
        u[0] = 0;
        text_cat(u, "< ");
        text_cat(u, t);
        cv_text(4, 3, &FONT_XS, u, C_LABEL);
    }
    if (s + 1 < (int32_t)NSLOTS) {
        slot_title((uint32_t)s + 1u, t);
        text_cat(t, " >");
        cv_text(236 - text_w(&FONT_XS, t), 3, &FONT_XS, t, C_LABEL);
    }
    slot_title((uint32_t)s, t);
    f = text_w(&FONT_L, t) <= 232 ? &FONT_L : &FONT_M;
    cv_text((SKY_W - text_w(f, t)) / 2, f == &FONT_L ? 20 : 24, f, t, C_BRIGHT);
    if (ui.sel_chord)
        cv_text((SKY_W - text_w(&FONT_XS, "+ CHORD")) / 2, 52, &FONT_XS, "+ CHORD", C_LATCH);
    for (i = 0; i < (int32_t)NSLOTS; i++) {
        int32_t x = 25 + i * 6 + (i >= (int32_t)NFACTORY ? 4 : 0);
        if (i == s)
            cv_rect(x, 60, 2, 10, C_BRIGHT);
        else if (i == (int32_t)set.scene)
            cv_rect(x, 62, 2, 8, C_TEXT);
        else
            cv_rect(x, 65, 2, 5, i < (int32_t)NFACTORY ? C_OFF : C_RULE);
    }
    cv_text(25, 71, &FONT_T, "F1", C_LABEL);
    cv_text(25 + (int32_t)NFACTORY * 6 + 4, 71, &FONT_T, "U1", C_LABEL);
    if (!ui.sel_chord) {
        uint32_t el = now - ui.sel_t;
        cv_rect(20, SKY_H - 2, 200, 2, C_RULE);
        cv_rect(20, SKY_H - 2, (int32_t)((el > 700u ? 700u : el) * 200u / 700u), 2, C_TEXT);
    }
}

static void sky_draw(void)
{
    uint32_t now = plat_ms(), hud = now < ui.hud_until, i, fx_layer = ui.btn >> B_FX & 1u;
    ui.frame++;
    if (set.view == VIEW_MOUTH) {        /* pixel art, drawn straight onto the canvas */
        CRUMB(9);
        cv_begin(SKY_W, SKY_H, C_SKY);
        for (i = 0; i < NSTARS; i++) {   /* the stars drift */
            int32_t x = (int32_t)ui.star[i].x - (int32_t)(ui.star[i].z * 6u);
            if (x < 0)
                x += SKY_W * 256;
            ui.star[i].x = (uint16_t)x;
        }
        CRUMB(10);
        sky_mouth();
        CRUMB(11);
    } else {
        switch (set.view) {
        case VIEW_ORBITS: sky_orbits(); break;
        case VIEW_NEBULA: sky_nebula(); break;
        case VIEW_SCOPE: sky_scope(); break;
        default: sky_beams(); break;
        }
        cv_begin(SKY_W, SKY_H, C_SKY);
        ph_compose(set.view == VIEW_SCOPE ? 2u : set.view == VIEW_NEBULA || set.view == VIEW_BEAMS ? 3u : 5u, hud);
    }
    if (fx_layer) {                      /* FX held: the four switches large, in the order of their keys */
        uint32_t act = dr_fx_active();
        const char *t = "FX HELD   F3 G3 A3 B3 C4 SWITCH";
        cv_rect(0, 0, SKY_W, SKY_H, C_SKY);
        cv_text((SKY_W - text_w(&FONT_T, t)) / 2, 2, &FONT_T, t, C_LABEL);
        for (i = 0; i < NFXK; i++) {
            int on = act >> i & 1u, x = 2 + 48 * (int32_t)i;
            char v[16];
            if (i == 4u && on)
                value_text(P_NZ_LEVEL, v);
            else {
                v[0] = 0;
                text_cat(v, on ? "ON" : "OFF");
            }
            cv_text(x, 20, &FONT_S, FXSHORT[i], on ? C_FX_ON : C_OFF);
            cv_text(x, 40, &FONT_M, v, on ? C_BRIGHT : C_OFF);
            cv_rect(x, 68, 44, 3, on ? C_FX_ON : C_RULE);
        }
        hud = 0;
    }
    CRUMB(12);
    if (hud) {                           /* the HUD: the name small, the value as large as fits, a knob's bar under it */
        if (set.view == VIEW_MOUTH)
            cv_rect(0, 0, SKY_W, SKY_H, C_BLACK);
        if (ui.sel_slot >= 0) {
            browse_draw(now);
        } else if (ui.hud[1][0]) {
            char num[16], unit[16];
            int32_t w = text_w(&FONT_XS, ui.hud[0]);
            cv_text((SKY_W - w) / 2, 6, &FONT_XS, ui.hud[0], C_LABEL);
            if (value_split(ui.hud[1], num, unit) && text_w(&FONT_L, num) + 3 + text_w(&FONT_S, unit) <= 232) {
                int32_t nw = text_w(&FONT_L, num), x0 = (SKY_W - nw - 3 - text_w(&FONT_S, unit)) / 2;
                cv_text(x0, 24, &FONT_L, num, C_BRIGHT);
                cv_text(x0 + nw + 3, 38, &FONT_S, unit, C_LABEL);
            } else {
                const felucca_font_t *f = text_w(&FONT_L, ui.hud[1]) <= 232 ? &FONT_L : &FONT_M;
                cv_text((SKY_W - text_w(f, ui.hud[1])) / 2, f == &FONT_L ? 24 : 28, f, ui.hud[1], C_BRIGHT);
            }
            if (ui.hud_id < PID_COUNT) {
                uint32_t n, sel, kind;
                float frac;
                kind = bar_kind(ui.hud_id, &n, &sel, &frac);
                if (kind != BAR_NONE)
                    draw_bar(40, 70, 160, kind, n, sel, frac);
            }
        } else {
            cv_text((SKY_W - text_w(&FONT_L, ui.hud[0])) / 2, 24, &FONT_L, ui.hud[0], C_BRIGHT);
        }
    }
    CRUMB(13);
    cv_blit(0, SKY_Y);
}

/* ------------------------------------------------------------------ the keys strip --- */
static uint16_t body_rgb(uint32_t note, int fading)   /* a body's colour for a flat fill: its beam's */
{
    return swap16(hue_px(body_hue(note), fading ? 10u : 26u));
}
static uint32_t keys_sig(void)           /* what the strip would show: drawn again only when it changes */
{
    uint32_t h = 0x811C9DC5u, i;
    dr_body_t b;
    for (i = 0; i < DR_NV; i++)
        h = hash32(h ^ (dr_body(i, &b) ? (uint32_t)b.home | (uint32_t)b.fading << 8 | (uint32_t)b.latched << 9
                                           | (uint32_t)(b.pitch + 0.5f) << 10 : 0xFFFFu));
    for (i = 0; i < DR_NL; i++)
        h = hash32(h ^ (dr_lead_body(i, &b) ? (uint32_t)b.home | (uint32_t)b.fading << 8 | 1u << 20 : 0xFFFEu));
    for (i = 0; i < DR_NV; i++) {
        uint32_t to;
        h = hash32(h ^ (dr_orbit_pending(i, &to) ? to | 1u << 21 : 0xFFFDu));
    }
    return hash32(h ^ (uint32_t)(set.octave + 2) ^ (uint32_t)set.scale << 4 ^ (uint32_t)set.root << 8
                  ^ (ui.btn >> B_FX & 1u) << 12 ^ dr_fx_active() << 13 ^ ui.bk_open * 2654435761u
                  ^ (uint32_t)orbit_quick() << 19);
}
/* a key lit from its beam: the ramp from hi at the top to lo at the bottom through the dither */
static void key_lit(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t hue, uint32_t hi, uint32_t lo)
{
    int32_t yy, xx;
    for (yy = 0; yy < h; yy++) {
        float lv = (float)hi - (float)(hi - lo) * (float)yy / (float)(h > 1 ? h - 1 : 1);
        uint32_t l0 = (uint32_t)lv, t = (uint32_t)((lv - (float)l0) * 16.0f);
        for (xx = 0; xx < w; xx++)
            cv_px[(uint32_t)(y + yy) * cv_w + (uint32_t)(x + xx)] = hue_px(hue, BAYER[yy & 3][xx & 3] < t ? l0 + 1u : l0);
    }
}
static void key_frame(int32_t x, int32_t y, int32_t w, int32_t h)   /* a 2 px white frame inside a key: the body
                                                                     * open under your finger */
{
    cv_rect(x, y, w, 2, C_BRIGHT);
    cv_rect(x, y + h - 2, w, 2, C_BRIGHT);
    cv_rect(x, y, 2, h, C_BRIGHT);
    cv_rect(x + w - 2, y, 2, h, C_BRIGHT);
}
/* the 27 keys as dark glass, lit only by sound: a sounding body's key in its beam's hue, bright at the top and
 * dimming down the key (dim while it fades), a sun cap when latched; a moved body's home key and ORBIT's destination
 * in the body's dim hue; a lead's key white; the scale a little lighter glass; an open body's key framed white */
enum { KS_GLASS, KS_LIT, KS_DIM, KS_LEAD, KS_LEAD_OFF, KS_FX_ON, KS_FX_OFF };
static void keys_draw(void)
{
    uint32_t k, i, fx_layer = ui.btn >> B_FX & 1u, act = dr_fx_active(), quick = orbit_quick();
    uint8_t st[NKEYS], hue[NKEYS], fade[NKEYS], cap[NKEYS], open[NKEYS];
    dr_body_t b;
    cv_begin(240, KEYS_H, C_BLACK);
    for (k = 0; k < NKEYS; k++)
        st[k] = KS_GLASS, hue[k] = fade[k] = cap[k] = open[k] = 0;
    for (i = 0; i < DR_NV; i++) {
        int32_t kk, kh;
        if (!dr_body(i, &b))
            continue;
        kk = (int32_t)(b.pitch + 0.5f) - 53 - 12 * set.octave;
        kh = (int32_t)b.home - 53 - 12 * set.octave;
        if (kh != kk && kh >= 0 && kh < (int32_t)NKEYS && st[kh] == KS_GLASS) {
            st[kh] = KS_DIM;
            hue[kh] = (uint8_t)body_hue(b.home);
        }
        if (kk >= 0 && kk < (int32_t)NKEYS) {
            uint32_t j, o = 0;
            for (j = 0; j < NKEYS; j++)
                if (ui.bk_open >> j & 1u && ui.keynote[j] == b.home && !b.fading)
                    o = 1;
            st[kk] = KS_LIT;
            hue[kk] = (uint8_t)body_hue(b.home);
            fade[kk] = (uint8_t)(b.fading || (ui.bk_open && !o));
            cap[kk] = (uint8_t)(b.latched && !b.fading);
            open[kk] = (uint8_t)o;
        }
    }
    for (i = 0; i < DR_NV && !quick; i++) {
        uint32_t to;
        int32_t kk;
        if (!dr_orbit_pending(i, &to) || !dr_body(i, &b))
            continue;
        kk = (int32_t)to - 53 - 12 * set.octave;
        if (kk >= 0 && kk < (int32_t)NKEYS && st[kk] == KS_GLASS) {
            st[kk] = KS_DIM;
            hue[kk] = (uint8_t)body_hue(b.home);
        }
    }
    for (i = 0; i < DR_NL; i++) {
        int32_t kk;
        if (!dr_lead_body(i, &b))
            continue;
        kk = (int32_t)b.home - 53 - 12 * set.octave;
        if (kk >= 0 && kk < (int32_t)NKEYS)
            st[kk] = b.fading ? KS_LEAD_OFF : KS_LEAD;
    }
    if (fx_layer)
        for (i = 0; i < NFXK; i++) {
            st[FXKEY[i]] = (act >> i & 1u) ? KS_FX_ON : KS_FX_OFF;
            cap[FXKEY[i]] = open[FXKEY[i]] = 0;
        }
    cv_rect(0, 0, 240, 1, C_STRIP_TOP);
    for (i = 0; i < 27u; i++) {
        int32_t x, w, h;
        uint32_t note, in;
        k = i < 16u ? WHITE_K[i] : BLACK_K[i - 16u];   /* the white keys first, the black ones over them */
        x = i < 16u ? 15 * (int32_t)i + 1 : (int32_t)key_x[k] - 4;
        w = i < 16u ? 13 : 9;
        h = i < 16u ? 34 : 20;
        note = (uint32_t)clamp(53 + (int32_t)k + 12 * set.octave, 0, 127);
        in = set.scale == SC_CHROMA || dr_snap(note) == note;
        switch (st[k]) {
        case KS_LIT: key_lit(x, 1, w, h, hue[k], fade[k] ? 13u : 26u, fade[k] ? 6u : 13u); break;
        case KS_DIM: cv_rect(x, 1, w, h, swap16(hue_px(hue[k], 8u))); break;
        case KS_LEAD: cv_rect(x, 1, w, h, C_LEAD); break;
        case KS_LEAD_OFF: cv_rect(x, 1, w, h, C_LEAD_OFF); break;
        case KS_FX_ON: cv_rect(x, 1, w, h, C_FX_ON); break;
        case KS_FX_OFF: cv_rect(x, 1, w, h, C_KW_OFF); break;
        default: cv_rect(x, 1, w, h, key_white[k] ? (in ? C_KW_IN : C_KW_OFF) : (in ? C_KB_IN : C_KB_OFF)); break;
        }
        if (cap[k])
            cv_rect(x, 1, w, 3, C_LATCH);
        if (open[k])
            key_frame(x, 1, w, h);
    }
    cv_blit(0, KEYS_Y);
}

/* ------------------------------------------------------------------ header and footer --- */
static void art_draw(int32_t x, int32_t y, const char *const *rows, uint32_t n, uint32_t hue)   /* one of art.h's
                                                                                                * bitmaps */
{
    static const uint8_t LV[128] = {['1'] = 7, ['2'] = 13, ['3'] = 20, ['4'] = 26, ['W'] = 31};
    uint32_t r, c;
    for (r = 0; r < n; r++)
        for (c = 0; rows[r][c]; c++) {
            uint32_t l = LV[(uint8_t)rows[r][c] & 127u];
            if (l && (uint32_t)(x + (int32_t)c) < cv_w && (uint32_t)(y + (int32_t)r) < cv_h)
                cv_px[(uint32_t)(y + (int32_t)r) * cv_w + (uint32_t)(x + (int32_t)c)] = hue_px(hue, l);
        }
}
/* the three states, named for their buttons, at fixed places: filled while on; ORBIT's countdown a line filling
 * along the foot of its pill, the pill bright as a move lands */
static void pill(int32_t x, int32_t w, const char *s, int on, float frac, int flash)
{
    int32_t tx = x + (w - text_w(&FONT_T, s)) / 2;
    if (on) {
        cv_rect(x, 29, w, 15, flash ? C_BRIGHT : C_MODE);
        cv_text(tx, 30, &FONT_T, s, C_BLACK);
        if (frac > 0.0f && !flash)
            cv_rect(x, 41, (int32_t)((float)w * frac + 0.5f), 3, C_BRIGHT);
    } else {
        cv_rect(x, 29, w, 1, C_RULE);
        cv_rect(x, 43, w, 1, C_RULE);
        cv_rect(x, 29, 1, 15, C_RULE);
        cv_rect(x + w - 1, 29, 1, 15, C_RULE);
        cv_text(tx, 30, &FONT_T, s, C_OFF);
    }
}
static void draw_header(void)
{
    char n[4], t[32], c[8];
    int32_t x;
    uint32_t i, act = dr_fx_active();
    cv_begin(240, HEAD_H, C_BLACK);
    art_draw(3, 6, MODEL_GLYPH[dr_scene.p[P_MODEL] % M_COUNT], ART_GLYPH, HUE_STAR);   /* the model, as a glyph */
    slot_name(set.scene, n);
    x = cv_text(19, 0, &FONT_M, n, C_LABEL);
    x = cv_text(x + 5, 0, &FONT_M, dr_scene.name, C_BRIGHT);
    if (scene_dirty)                     /* edited since it was loaded or saved: a mark, not an orange name */
        cv_rect(x + 4, 13, 5, 5, C_WARN);
    t[0] = 0;                            /* the status, at the right: octave, recording, Bluetooth, bodies and CPU */
    if (set.octave) {
        c[0] = set.octave < 0 ? '-' : '+';
        c[1] = (char)('0' + (set.octave < 0 ? -set.octave : set.octave));
        c[2] = 0;
        text_cat(t, "OCT");
        text_cat(t, c);
        text_cat(t, "  ");
    }
    if (usb_recording())                 /* a computer records over USB (the speaker silent with SPEAKER AUTO) */
        text_cat(t, set.speaker == SPK_AUTO ? "USB MUTE " : "USB ");
    if (FM1B_BLE && plat_ble_connected())
        text_cat(t, "BLE ");
    if (FM1B_BLE_CENTRAL && plat_ble_device() == BLE_DEV_READY)
        text_cat(t, "DEV ");
    int_text((int32_t)dr_bodies(), c);
    text_cat(t, c);
    text_cat(t, "/8 ");
    int_text((int32_t)plat_cpu_pct(), c);
    text_cat(t, c);
    text_cat(t, "%");
    if (overload_seen)
        text_cat(t, " LOAD");
    x = 237 - text_w(&FONT_T, t);
    cv_rect(x - 5, 0, 240 - x + 5, 18, C_BLACK);   /* a long name ends under it */
    cv_text(x, 4, &FONT_T, t, overload_seen ? C_WARN : C_LABEL);
    pill(3, 54, ui.shift ? "SEL TRANS" : "SEL LATCH", set.latch || ui.shift, 0.0f, 0);
    {                                    /* PLAY ORBIT; running in another mode than WANDER, that mode's name */
        static const char *const PILL[OM_COUNT] = {"PLAY ORBIT", "PLAY CHORD", "PLAY SCATTER", "PLAY ARP", "PLAY CYCLE",
                                                   "PLAY ECHO", "PLAY PULSE"};
        pill(61, 69, PILL[dr_orbit_on() ? dr_orbit_mode() : 0u], dr_orbit_on(), (float)ui.orbit_eighth * 0.125f,
             plat_ms() - ui.orbit_flash_t < 300u);
    }
    pill(134, 42, "SEQ FRZ", dr_frozen(), 0.0f, 0);
    for (i = 0; i < NFXK; i++)           /* the effects heard, and the noise: E D R S N */
        cv_text(196 + (int32_t)i * 9, 31, &FONT_T, FXLETTER[i], (act >> i & 1u) ? C_FX_ON : C_OFF);
    if (dr_morphing())                   /* the morph's progress along the header's foot */
        cv_rect(0, HEAD_H - 2, (int32_t)(240.0f * dr_morph_pos()), 2, C_TEXT);
    else
        for (i = 0; i < 16u; i++)        /* a tick over every white key */
            cv_rect(15 * (int32_t)i + 7, HEAD_H - 2, 1, 2, C_RULE);
    cv_blit(0, 0);
}
static void knob_cell(int32_t x, uint32_t id)   /* a knob: its name, its value, its bar by kind */
{
    char t[16];
    uint32_t n, sel, kind;
    float frac;
    if (id == PID_NONE) {                /* nothing on this knob here */
        cv_rect(x, 40, 12, 2, C_RULE);
        return;
    }
    cv_text(x, 17, &FONT_T, param_label(id), C_LABEL);
    value_text(id, t);
    cell_value(x, 28, t);
    kind = bar_kind(id, &n, &sel, &frac);
    draw_bar(x, 60, 56, kind, n, sel, frac);
}
static uint32_t page_icon(uint32_t pg)
{
    switch (PAGE_BTN[pg]) {
    case B_EDIT: return ICON_EDIT;
    case B_ENV: return ICON_ENV;
    case B_LFO: return ICON_LFO;
    case B_FX: return ICON_FX;
    case B_ARP: return ICON_ARP;
    case B_GLO: return ICON_GLO;
    default: return ICON_HOME;
    }
}
static void draw_footer(void)
{
    char t[32];
    uint32_t k;
    cv_begin(240, FOOT_H, C_BLACK);      /* one transfer: the page band and the four knobs */
    if (ui.dialog) {
        char n[24];
        slot_title(NFACTORY + ui.save_to, n);
        cv_text(4, 0, &FONT_S, "SAVE SCENE TO", C_LABEL);
        cv_text(4, 16, &FONT_M, n, C_BRIGHT);
        cv_text(4, 41, &FONT_S, "SAVE SAVES   HOME CANCELS", C_TEXT);
        cv_text(4, 57, &FONT_S, "PRESET PICKS THE SLOT", C_LABEL);
        cv_blit(0, FOOT_Y);
        return;
    }
    if (ui.bk_open) {                    /* a body open: its note and the four knobs that are its own */
        uint32_t kk = 0, n = 0, w;
        char v[16];
        for (k = 0; k < NKEYS; k++)
            if (ui.bk_open >> k & 1u) {
                if (!n)
                    kk = k;
                n++;
            }
        cv_rect(0, 0, 240, 14, body_rgb(ui.keynote[kk], 1));
        t[0] = 0;
        text_cat(t, "BODY ");
        note_text((int32_t)ui.keynote[kk], v);
        text_cat(t, v);
        if (n > 1) {
            text_cat(t, " + ");
            int_text((int32_t)n - 1, v);
            text_cat(t, v);
        }
        cv_text(4, 1, &FONT_T, t, C_BRIGHT);
        cv_text(236 - text_w(&FONT_T, "LET GO: BACK"), 1, &FONT_T, "LET GO: BACK", C_BRIGHT);
        for (w = 0; w < BE_COUNT; w++) {
            int32_t x = 3 + 60 * (int32_t)w, val = ui.bval[kk][w];
            body_value_text(w, val, v);
            cv_text(x, 17, &FONT_T, BE_LABEL[w], C_LABEL);
            cell_value(x, 28, v);
            if (w == BE_LEVEL)
                draw_bar(x, 60, 56, BAR_FILL, 0, 0, (float)(val - DR_LVL_MUTE) / (float)(6 - DR_LVL_MUTE));
            else if (w == BE_PIN)
                draw_bar(x, 60, 56, BAR_SWITCH, 0, (uint32_t)(val != 0), 0.0f);
            else
                draw_bar(x, 60, 56, BAR_CENTRE, 0, 0, (float)(val + 50) * 0.01f);
        }
        cv_blit(0, FOOT_Y);
        return;
    }
    cv_rect(0, 0, 240, 14, C_BAND);      /* the page band: its button's icon, the page, the SELECT arrows */
    art_draw(4, 3, PAGE_ICON[page_icon(ui.page)], ART_ICON, HUE_CYAN);
    t[0] = 0;
    if (ui.page > 0u)
        text_cat(t, "< ");
    text_cat(t, PAGES[ui.page].button);
    if (ui.page != PG_HOME) {
        text_cat(t, "  ");
        text_cat(t, PAGES[ui.page].title);
    }
    if (ui.page < PG_COUNT - 1u)
        text_cat(t, " >");
    cv_text(17, 1, &FONT_T, t, C_TEXT);
    t[0] = 0;
    text_cat(t, MODEL_NAME[dr_scene.p[P_MODEL] % M_COUNT]);
    text_cat(t, "  ");
    text_cat(t, NOTE_NAME[set.root]);
    text_cat(t, " ");
    text_cat(t, SCALE_NAME[set.scale]);
    cv_text(236 - text_w(&FONT_T, t), 1, &FONT_T, t, C_LABEL);
    for (k = 0; k < 4u; k++)
        knob_cell(3 + 60 * (int32_t)k, page_param(ui.page, k));
    cv_blit(0, FOOT_Y);
}

/* a screen reader that has just started listening (or asks) hears the page */
static void speak(void)
{
    char t[120], v[16];
    uint32_t i;
    if (ann_on() && ann_where()) {
        t[0] = 0;
        text_cat(t, APP_NAME ". ");
        text_cat(t, dr_scene.name);
        text_cat(t, ". ");
        text_cat(t, PAGES[ui.page].title);
        for (i = 0; i < 4u; i++) {
            uint32_t id = page_param(ui.page, i);
            if (id == PID_NONE)
                continue;
            text_cat(t, ". ");
            text_cat(t, param_label(id));
            text_cat(t, " ");
            value_text(id, v);
            text_cat(t, v);
        }
        text_cat(t, ". BODIES ");          /* the status too: what the header shows */
        int_text((int32_t)dr_bodies(), v);
        text_cat(t, v);
        text_cat(t, ". CPU ");
        int_text((int32_t)plat_cpu_pct(), v);
        text_cat(t, v);
        text_cat(t, "%");
        ann_say(t, 0, ANN_NOW);
    }
    ann_tick(plat_ms());
}

static void ui_overload(void) { overload_seen = 1; }

static void ui_init(void)
{
    uint32_t i;
    touch_on(1);
    lcd_fill(0, 0, 240, 240, C_BLACK);
    lut_build();
    spal_build();
    stars_init();
    keys_geom();
    ui.keys_sig = 0xFFFFFFFFu;
    for (i = 0; i < sizeof phos; i++)
        phos[i] = 0;
    for (i = 0; i < DR_NV; i++)
        ui.ang[i] = 0.125f * (float)i;
    ui.page = PG_HOME;
    ui.sel_slot = -1;
    ui.redraw_head = ui.redraw_foot = 1;
    ui.sky_t = 0;
}

static void ui_frame(void)
{
    static uint32_t last_ble, last_bodies, last_states, last_dirty, last_cpu;
    uint32_t now = plat_ms(), nb = dr_bodies(), dirty = (uint32_t)(set_dirty | scene_dirty << 1), from, to, moves;
    uint32_t states, sent, sig, cpu = plat_cpu_pct();
    CRUMB(2);
    input();
    CRUMB(3);
    speak();
    CRUMB(4);
    if (FM1B_BLE_CENTRAL) {
        static int found;
        int n = ui.looking ? btdev_count() : 0, ev;
        btdev_page(ui.looking && !set.ble_off);
        if (n > found && btdev_pick() < n)
            ble_device_say();
        found = n;
        while ((ev = btdev_event()) != BTDEV_NONE)
            if (ev == BTDEV_CONNECTED || ev == BTDEV_NO_ANSWER || ev == BTDEV_LOST)
                ui_say("DEVICE", btdev_word(ev));
    }
    CRUMB(5);
    if (pc_pending >= 0) {
        int32_t pc = pc_pending;
        pc_pending = -1;
        scene_go((uint32_t)pc % NSLOTS, 1, 0);   /* a program change: the sound, as PRESET */
    }
    if (midi_edited) {
        midi_edited = 0;
        ui.redraw_foot = 1;
    }
    moves = dr_orbit_moved(&from, &to);
    if (moves != ui.orbit_moves) {       /* ORBIT moved a body: the pill flashes (the beam slides, the light walks) */
        ui.orbit_moves = moves;
        ui.orbit_flash_t = now;
        ui.redraw_head = 1;
    }
    {                                    /* the countdown to the next ORBIT move, in eighths; the flash ending */
        uint8_t e = (uint8_t)(dr_orbit_next() * 8.0f);
        if (e != ui.orbit_eighth) {
            ui.orbit_eighth = e;
            ui.redraw_head = 1;
        }
    }
    states = (uint32_t)set.latch | (uint32_t)dr_orbit_on() << 1 | (uint32_t)dr_frozen() << 2 | dr_orbit_mode() << 10
           | (uint32_t)usb_recording() << 12 | (uint32_t)dr_clock_ext() << 13 | (uint32_t)ui.shift << 14
           | (uint32_t)dr_morphing() << 3 | (uint32_t)overload_seen << 4 | dr_fx_active() << 5
           | (uint32_t)(now - ui.orbit_flash_t < 300u) << 9;
    if (states != last_states || dirty != last_dirty || (dr_morphing() && now - ui.head_t > 100u)) {
        last_states = states;
        last_dirty = dirty;
        ui.redraw_head = 1;
    }
    if ((uint32_t)(plat_ble_connected() | plat_ble_device() << 1) != last_ble) {
        last_ble = (uint32_t)(plat_ble_connected() | plat_ble_device() << 1);
        ui.redraw_head = 1;
    }
    if (now - ui.stat_t >= 10000u
        && (cpu > ui.stat_cpu + 4u || cpu + 4u < ui.stat_cpu || nb != ui.stat_bodies || usb_underruns() != ui.stat_under)) {
        char t[80], v[12];               /* the load, to the log (fm1_remote.py log): a capability probe */
        ui.stat_t = now;
        ui.stat_cpu = (uint8_t)cpu;
        ui.stat_bodies = (uint8_t)nb;
        ui.stat_under = usb_underruns();
        t[0] = 0;                        /* short: the log keeps about 40 characters of a line */
        text_cat(t, "cpu ");
        int_text((int32_t)cpu, v);
        text_cat(t, v);
        text_cat(t, "% b");
        int_text((int32_t)nb, v);
        text_cat(t, v);
        text_cat(t, " ");
        text_cat(t, MODEL_NAME[dr_scene.p[P_MODEL] % M_COUNT]);
        text_cat(t, " s");
        int_text((int32_t)set.scene, v);
        text_cat(t, v);
        text_cat(t, " x");
        int_text((int32_t)(plat_xruns() & 0xFFFFu), v);
        text_cat(t, v);
        if (ui.stat_under) {             /* the USB audio ring's underruns: crackle in a recording */
            text_cat(t, " u");
            int_text((int32_t)(ui.stat_under & 0xFFFFu), v);
            text_cat(t, v);
        }
        plat_log(t);
    }
    if (nb != last_bodies || (cpu != last_cpu && now - ui.head_t > 1000u)) {   /* the status: bodies, CPU */
        last_bodies = nb;
        last_cpu = cpu;
        ui.redraw_head = 1;
    }
    sig = keys_sig();
    if (sig != ui.keys_sig) {
        ui.keys_sig = sig;
        ui.redraw_keys = 1;
    }
    sent = 0;
    if (ui.redraw_head) {
        ui.redraw_head = 0;
        ui.head_t = now;
        CRUMB(6);
        draw_header();
        sent = 1;
    }
    if (ui.redraw_foot) {
        ui.redraw_foot = 0;
        ui.foot_t = now;
        CRUMB(7);
        draw_footer();
        sent = 1;
    }
    if (ui.redraw_keys) {
        ui.redraw_keys = 0;
        CRUMB(14);
        keys_draw();
        sent = 1;
    }
    if (now - ui.sky_t >= 40u && !sent) {   /* the sky, 25 times a second; never behind a wait for the footer's
                                             * transfer (the keys are read between frames) */
        ui.sky_t = now;
        CRUMB(8);
        sky_draw();
    }
}
