/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann */
/* DEEP FIELD on fm1-base: the app's unity build (compiled into fm1-base's, after the platform) and the glue between
 * fm1-base's contract (app.h) and the parts: what is saved (scenes.c), the screen and controls (ui.c) and the
 * engine (engine/drone.c, its own -O2 unit). */
#include "drone.h"
#include "fx.h"                            /* fx_lim_min, for the simulator's limiter check */
static volatile uint8_t midi_edited;     /* a CC changed a scene parameter: the footer redraws */
#define MCMD_N 16u                       /* the switches and triggers that came in as CCs, for the main loop (ui.c) */
static volatile uint8_t mcmd_cc[MCMD_N], mcmd_v[MCMD_N];
static volatile uint32_t mcmd_w, mcmd_r;
#define CRUMB(n) (crumb.frame = (n))
static struct { uint32_t magic, frame, render, frames; } crumb __attribute__((section(".noinit")));
#include "scenes.c"
#include "ui.c"

static uint32_t dr_arena[DR_ARENA_BYTES / 4u] PLAT_POOL __attribute__((aligned(32)));   /* cache lines: the second core's bodies */

/* breadcrumbs for a watchdog reset (the main loop stuck 8 s): where the frame was, and whether the render was
 * in progress, in RAM a reset leaves alone; the next boot puts them in the log (fm1_remote.py log) */
#define CRUMB_MAGIC 0x44464331u
static void crumb_log(void)
{
    char t[48];
    if (crumb.magic == CRUMB_MAGIC && (crumb.frame || crumb.render)) {
        uint32_t i;
        t[0] = 0;
        text_cat(t, "deepfield last frame step ");
        for (i = 0; t[i]; i++)
            ;
        int_text((int32_t)crumb.frame, t + i);
        text_cat(t, crumb.render ? " render busy" : " render idle");
        plat_log(t);
    }
    crumb.magic = CRUMB_MAGIC;
    crumb.frame = crumb.render = crumb.frames = 0;
}
static void t_crumb_init(void) { crumb_log(); }

/* the CC map (render context): every value has a CC, in and out (ui.c CC_OF; the table in the manual), plus 1
 * wheel, 7 volume, 64 sustain, 74 and 91 as TIMBRE and REVERB on the way in, 120 all sound off, 121 reset
 * controllers, 123 all notes off; the switches and triggers (CC_LATCH, CC_GEN, CC_FREEZE, CC_FX_SW.., CC_NOISE_SW,
 * CC_AS_SAVED, CC_GEN_NOW; SCALE and ROOT) are queued for the main loop (midi_cmd_apply), as the buttons would do
 * them; NRPN (99 MSB, 98 LSB, 6 data): a body's own values, MSB 1 LEVEL (0 muted, 1..31 = -24..+6 dB), 2 TONE and
 * 3 TUNE (64 + the value), 4 LOCK (64 up), LSB the note the body was struck at. A CC in is playback, not editing:
 * it never lights SAVE */
static uint8_t nrpn_msb, nrpn_lsb;
static void midi_param(uint32_t id, uint32_t v)
{
    dr_param(id, v & 127u);
    midi_edited = 1;
}
static void midi_cmd_put(uint32_t cc, uint32_t v)
{
    uint32_t w = mcmd_w;
    if (w - mcmd_r >= MCMD_N)
        return;
    mcmd_cc[w % MCMD_N] = (uint8_t)cc;
    mcmd_v[w % MCMD_N] = (uint8_t)v;
    mcmd_w = w + 1u;
}
static void midi_cc(uint32_t cc, uint32_t v)
{
    int32_t id;
    switch (cc) {
    case 1: dr_wheel(v); return;
    case 7: dr_volume(v); return;
    case 64: dr_sustain(v); return;
    case 99: nrpn_msb = (uint8_t)(v & 127u); return;
    case 98: nrpn_lsb = (uint8_t)(v & 127u); return;
    case 6:
        if (nrpn_msb >= 1u && nrpn_msb <= BE_COUNT) {
            uint32_t w = nrpn_msb - 1u;
            int32_t x = w == BE_LEVEL ? (v ? (int32_t)v - 25 : DR_LVL_MUTE) : w == BE_PIN ? v >= 64u : (int32_t)v - 64;
            dr_body_edit_now(nrpn_lsb, w, x);
        }
        return;
    case 2:
    case 3:
    case CC_NOISE_SW:
    case CC_AS_SAVED:
    case CC_GEN_NOW:
    case CC_LATCH:
    case CC_GEN:
    case CC_FREEZE:
    case CC_FX_SW:
    case CC_FX_SW + 1u:
    case CC_FX_SW + 2u:
    case CC_FX_SW + 3u:
        midi_cmd_put(cc, v);
        return;
    default: break;
    }
    if ((id = cc_param(cc)) >= 0 && id < (int32_t)P_COUNT) {
        midi_param((uint32_t)id, cc_in_val((uint32_t)id, v));
        return;
    }
    switch (cc) {
    case 120: dr_panic(); break;
    case 121:
        dr_wheel(0);
        dr_bend(8192);
        dr_channel_pressure(0);
        dr_sustain(0);
        dr_volume(127);
        break;
    case 123: dr_all_off(); break;
    default: break;
    }
}

static void midi_in(void)                    /* audio interrupt: the packets that came in, straight to the engine */
{
    uint32_t pkt;
    int port;
    while (plat_midi_in(&pkt, &port)) {
        uint32_t st = (pkt >> 8) & 0xF0u, d1 = (pkt >> 16) & 0x7Fu, d2 = pkt >> 24 & 0x7Fu;
        uint32_t full = (pkt >> 8) & 0xFFu;
        if (full >= 0xF8u) {             /* real time: the clock for the runner and ORBIT */
            if (full == 0xF8u)
                dr_clock(port);
            else if (full == 0xFAu || full == 0xFBu)
                dr_clock_start(port);
            else if (full == 0xFCu)
                dr_clock_stop();
            continue;
        }
        midi_last = pkt;
        midi_last_port = (uint32_t)port;
        midi_count++;
        if (st >= 0xF0u)
            continue;
        if (set.midi_in && (full & 0x0Fu) != (uint32_t)set.midi_in - 1u)   /* not our channel */
            continue;
        switch (st) {
        case 0x90u:                      /* a MIDI note is a key: LATCH on holds a body, off plays a lead */
            if (!d2) {                   /* a note off sent as a note on at nothing (much gear does): both, as 0x80
                                          * is, so LATCH switched between the note on and the note off cannot leave
                                          * the note sounding with nothing left to stop it */
                dr_note_now(d1, 0, 0);
                dr_lead_now(d1, 0, 0);
            } else if (set.latch)
                dr_note_now(d1, d2, 1);
            else
                dr_lead_now(d1, d2, 1);
            break;
        case 0x80u:
            dr_note_now(d1, 0, 0);
            dr_lead_now(d1, 0, 0);
            break;
        case 0xA0u: dr_pressure(d1, d2); break;
        case 0xB0u: midi_cc(d1, d2); break;
        case 0xD0u: dr_channel_pressure(d1); break;
        case 0xC0u: pc_pending = (int32_t)d1; break;
        case 0xE0u: dr_bend(d1 | d2 << 7); break;
        default: break;
        }
    }
}

static void midi_settings_apply(void) {}

/* the bank over SysEx (main loop; the manual's section 10): F0 7D 44 46 cmd .. F7 ("DF").
 *   01 ss        a request: slot ss (0..15 F1..F16, 16..31 U1..U16) comes back as a 02 frame; 40 every user slot,
 *                41 every slot, one frame each
 *   02 ss 06 d.. a preset for slot ss: the scene (118 bytes, SCN6's layout, 06 says so; an 05 frame is taken and
 *                its velocities brought up to the engine's law) packed seven bytes to eight
 *                (the high bits first, then the seven bytes with them stripped). In: written to user slot ss (a
 *                factory slot is refused) and the bank saved once the frames stop; out: the answer to 01.
 * A sound heard now is left alone: the slot's next recall plays what came in. A .syx from a computer is the same
 * frames one after another (tools/fm1_bank.py dumps and loads one). */
#define DFX_HEAD 6u                       /* 7D 44 46 cmd slot ver */
#define DFX_GROUPS ((sizeof(scene_t) + 6u) / 7u)
#define DFX_BODY (DFX_GROUPS * 8u)
static uint8_t dfx_frame[2u + DFX_HEAD + DFX_BODY];
static uint32_t dfx_pending;              /* bit s: slot s is to be sent (one a frame, so the panel stays live) */
static void dfx_pack(const scene_t *s, uint8_t *d)
{
    const uint8_t *b = (const uint8_t *)s;
    uint32_t g, k;
    for (g = 0; g < DFX_GROUPS; g++) {
        uint8_t hi = 0;
        for (k = 0; k < 7u; k++) {
            uint32_t i = g * 7u + k, v = i < sizeof(scene_t) ? b[i] : 0u;
            hi |= (uint8_t)((v >> 7) << k);
            d[1u + k] = (uint8_t)(v & 127u);
        }
        d[0] = hi;
        d += 8;
    }
}
static void dfx_unpack(const uint8_t *d, scene_t *s)
{
    uint8_t *b = (uint8_t *)s;
    uint32_t g, k;
    for (g = 0; g < DFX_GROUPS; g++) {
        for (k = 0; k < 7u; k++) {
            uint32_t i = g * 7u + k;
            if (i < sizeof(scene_t))
                b[i] = (uint8_t)(d[1u + k] | (d[0] >> k & 1u) << 7);
        }
        d += 8;
    }
}
static uint32_t dfx_build(uint32_t slot)  /* a 02 frame for a slot into dfx_frame: its length */
{
    dfx_frame[0] = 0xF0;
    dfx_frame[1] = 0x7D;
    dfx_frame[2] = 0x44;
    dfx_frame[3] = 0x46;
    dfx_frame[4] = 0x02;
    dfx_frame[5] = (uint8_t)(slot % NSLOTS);
    dfx_frame[6] = 6;
    dfx_pack(slot_scene(slot % NSLOTS), dfx_frame + 1u + DFX_HEAD);
    dfx_frame[1u + DFX_HEAD + DFX_BODY] = 0xF7;
    return 2u + DFX_HEAD + DFX_BODY;
}
static int dfx_take(const uint8_t *p, uint32_t n)   /* a 02 frame in: the preset into its user slot; 1 taken */
{
    uint32_t slot = p[4];
    scene_t sc;
    if (n < DFX_HEAD + DFX_BODY || (p[5] != 6u && p[5] != 5u) || slot < NFACTORY || slot >= NSLOTS)
        return 0;
    dfx_unpack(p + DFX_HEAD, &sc);
    sc.name[SCENE_NAME - 1u] = 0;
    if (p[5] == 5u)                       /* a .syx from before the velocity law changed: its chord sounds as it did */
        vel_v5_to_v6(&sc);
    bank.u[slot - NFACTORY] = sc;
    bank_check(slot - NFACTORY);
    ui.bank_save_t = plat_ms();
    {
        char t[24];
        slot_title(slot, t);
        ui_say("PRESET IN", t);
    }
    return 1;
}
int app_sysex(const uint8_t *p, uint32_t n)
{
    if (n < 5u || p[0] != 0x7Du || p[1] != 0x44u || p[2] != 0x46u)
        return 0;
    if (p[3] == 0x01u) {
        uint32_t s = p[4];
        if (s < NSLOTS)
            dfx_pending |= 1u << s;
        else if (s == 0x40u)
            dfx_pending |= 0xFFFF0000u;
        else if (s == 0x41u)
            dfx_pending = 0xFFFFFFFFu;
        return 1;
    }
    if (p[3] == 0x02u)
        return dfx_take(p, n);
    return 0;
}
static void dfx_step(void)                /* main loop: one pending slot out a frame */
{
    uint32_t s;
    if (!dfx_pending)
        return;
    for (s = 0; s < NSLOTS; s++)
        if (dfx_pending >> s & 1u) {
            dfx_pending &= ~(1u << s);
            plat_sysex_send(dfx_frame, dfx_build(s));
            return;
        }
}

/* the USB audio input: whether a computer records now. usb.c keeps the stream's state in this unity unit (alt 1
 * and the host reading); plat.h has no word for it yet, so it is read here. The host simulator: a script flag. */
#ifdef PLAT_HOST
static int host_usb_rec;
static int usb_recording(void) { return host_usb_rec; }
static uint32_t usb_underruns(void) { return 0; }
#elif FELUCCA_UAC
static int usb_recording(void) { return uac.feed != 0; }
static uint32_t usb_underruns(void) { return uac.underruns; }   /* the ring ran dry: a frame repeated, a crackle in
                                                                 * the recording (not on the speaker) */
#else
static int usb_recording(void) { return 0; }
static uint32_t usb_underruns(void) { return 0; }
#endif
uint32_t df_gain(uint32_t m)             /* the speaker and headphones (audio interrupt): MASTER cubed, or nothing;
                                          * cubed, a third of the way up is -29 dB (squared it was -19, loud on
                                          * headphones) and the top end still reaches full */
{
    if (set.speaker == SPK_OFF || (set.speaker == SPK_AUTO && usb_recording()))
        return 0;
    return (m * m >> 12) * m >> 12;
}

/* the keys' note offs and ORBIT's moves, out over MIDI (render context) */
static void orbit_note_out(uint32_t note, uint32_t vel, int on) { midi_note_out(note, vel, on); }
static void realtime_out(uint32_t st) { plat_midi_send(0x0Fu | st << 8, TO_USB | TO_BLE); }

void app_overload(void)
{
    dr_overload();
    ui_overload();
}

/* ---- the contract */
void app_init(void)
{
    t_crumb_init();
    project_load();
    dr_scene = *slot_scene(set.scene);
    dr_init(dr_arena, sizeof dr_arena);
#if FM1B_CPU2
    plat_cpu2_set(1);                        /* the second core renders half the bodies; one core if it fails */
#endif
    scale_apply();
    plat_usb_audio_full(set.speaker != SPK_ON);
    dr_note_out(orbit_note_out);
    dr_realtime_out(realtime_out);
    dr_clock_out(set.clock_out);
    dr_load(slot_scene(set.scene), 0.0f, 0);   /* the scene's sound, in silence: nothing plays until a key does */
    dr_fx_bypass(slot_scene(set.scene)->fx_off);   /* and its effect switches */
    scene_dirty = 0;
}
static const uint16_t USB_TRIM_Q12[7] = {4096, 2896, 2048, 1448, 1024, 724, 512};   /* 0 .. -18 dB by 3 */
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    crumb.render = 1;
    midi_in();
    if (set.speaker != SPK_ON && usb_recording())   /* the recording at USB LEVEL (the speaker is silent or at MASTER
                                                     * after it, so it costs the room nothing) */
        master_q12 = USB_TRIM_Q12[set.usb_level < 7u ? set.usb_level : 2u];
    dr_render(lr, n, master_q12);
    crumb.render = 0;
}
void app_panic(void) { dr_panic(); }
void app_ui_init(void) { ui_init(); }
void app_splash(void)                        /* before the engine is up: the vortex's first frame, which ui_init plays on */
{
    lut_build();
    boot_frame(0.0f);
}
void app_frame(void)
{
    CRUMB(1);
    ui_frame();
    CRUMB(9);
    dfx_step();
    CRUMB(0);
}
void app_input_only(void) { ui_input_only(); }
void app_say(const char *a, const char *b) { ui_say(a, b); }

#ifdef PLAT_HOST
/* the simulator's checks: octave, note (the newest body's, -1 none), bodies, scene, page, dialog, dirty, latch (bodies latched), latchmode, velon,
 * orbit, freeze, model, scale, morphing, fxbypass, fxactive, sounding (dr_sounding of the next arg: "sounding60"),
 * pN (parameter N), ch_in, ch_out, local */
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "octave"))
        *got = set.octave;
    else if (!strcmp(what, "note"))
        *got = dr_newest_note();
    else if (!strcmp(what, "bodies"))
        *got = (int)dr_bodies();
    else if (!strcmp(what, "scene"))
        *got = set.scene;
    else if (!strcmp(what, "page"))
        *got = ui.page;
    else if (!strcmp(what, "dialog"))
        *got = ui.dialog;
    else if (!strcmp(what, "dirty"))
        *got = set_dirty || scene_dirty;
    else if (!strcmp(what, "boot"))          /* the vortex still playing */
        *got = ui.boot;
    else if (!strcmp(what, "savelit"))       /* SAVE lit: a sound or a setting changed (not browsing, LATCH, octave) */
        *got = scene_dirty || ui.cfg_dirty;
    else if (!strcmp(what, "latch"))         /* bodies latched */
        *got = (int)dr_latched();
    else if (!strcmp(what, "latchmode"))     /* SEL: the LATCH mode */
        *got = set.latch;
    else if (!strcmp(what, "octlit"))        /* the OCT buttons lit (bit 0 OCT-, 1 OCT+) and half-lit, as bits 0-1 and 2-3 */
        *got = ui.oct_lit | ui.oct_mid << 2;
    else if (!strcmp(what, "velon"))
        *got = set.vel;
    else if (!strcmp(what, "orbit"))
        *got = dr_orbit_on();
    else if (!strcmp(what, "orbitmode"))
        *got = (int)dr_orbit_mode();
    else if (!strcmp(what, "pending")) {     /* bodies with a move planned */
        uint32_t i, to;
        *got = 0;
        for (i = 0; i < DR_NV; i++)
            *got += dr_orbit_pending(i, &to);
    } else if (!strcmp(what, "arpnote"))
        *got = (int)dr_arp_note();
    else if (!strcmp(what, "arprun"))
        *got = dr_arp_running();
    else if (!strcmp(what, "freeze"))
        *got = dr_frozen();
    else if (!strcmp(what, "model"))
        *got = dr_scene.p[P_MODEL];
    else if (!strcmp(what, "scale"))
        *got = set.scale;
    else if (!strcmp(what, "morphing"))
        *got = dr_morphing();
    else if (!strcmp(what, "fxbypass"))
        *got = (int)dr_fx_bypass_get();
    else if (!strcmp(what, "fxactive"))
        *got = (int)dr_fx_active();
    else if (!strcmp(what, "shift"))         /* TRANSPOSE: the keys move the latched bodies */
        *got = df_shift();
    else if (!strcmp(what, "moves")) {      /* ORBIT's moves so far */
        uint32_t f, t;
        *got = (int)dr_orbit_moved(&f, &t);
    } else if (!strcmp(what, "fxrun"))       /* the effect stages still sounding: 1 ensemble, 2 delay, 4 hall */
        *got = (int)fx_running();
    else if (!strcmp(what, "bodyopen")) {    /* bodies open under held keys (their own values on the knobs) */
        uint32_t k;
        *got = 0;
        for (k = 0; k < NKEYS; k++)
            *got += (int)(ui.bk_open >> k & 1u);
    } else if (!strncmp(what, "blvl", 4))    /* "blvl60": the body struck at 60, its LEVEL (dB; -25 muted) */
        *got = (int)dr_body_value((uint32_t)atoi(what + 4), BE_LEVEL);
    else if (!strncmp(what, "btone", 5))
        *got = (int)dr_body_value((uint32_t)atoi(what + 5), BE_TONE);
    else if (!strncmp(what, "btune", 5))
        *got = (int)dr_body_value((uint32_t)atoi(what + 5), BE_TUNE);
    else if (!strncmp(what, "bpin", 4))
        *got = (int)dr_body_value((uint32_t)atoi(what + 4), BE_PIN);
    else if (!strncmp(what, "level", 5)) {   /* "level60": the body struck at 60, its level as heard, in 1/1000 */
        uint32_t i;
        dr_body_t b;
        *got = -1;
        for (i = 0; i < DR_NV; i++)
            if (dr_body(i, &b) && !b.fading && b.home == (uint32_t)atoi(what + 5))
                *got = (int)(b.level * 1000.0f);
    } else if (!strncmp(what, "pitch", 5)) {   /* "pitch60": where the body struck at 60 stands now (its target, rounded) */
        uint32_t i;
        dr_body_t b;
        *got = -1;
        for (i = 0; i < DR_NV; i++)
            if (dr_body(i, &b) && !b.fading && b.home == (uint32_t)atoi(what + 5))
                *got = (int)(b.pitch + 0.5f);
    }
    else if (!strncmp(what, "fxon", 4))      /* one effect heard: fxon0 ensemble .. fxon3 shimmer */
        *got = (int)(dr_fx_active() >> atoi(what + 4) & 1u);
    else if (!strncmp(what, "sounding", 8))
        *got = dr_sounding((uint32_t)atoi(what + 8));
    else if (!strcmp(what, "leads"))         /* leads sounding (held or releasing) */
        *got = (int)dr_leads();
    else if (!strcmp(what, "leadnote")) {    /* the newest held lead's key note, -1 none */
        uint32_t i;
        dr_body_t b;
        *got = -1;
        for (i = 0; i < DR_NL; i++)
            if (dr_lead_body(i, &b) && !b.fading)
                *got = b.home;
    } else if (!strncmp(what, "lead", 4)) {  /* "lead60": a lead at this key note is held */
        uint32_t i;
        dr_body_t b;
        *got = 0;
        for (i = 0; i < DR_NL; i++)
            if (dr_lead_body(i, &b) && !b.fading && b.home == (uint32_t)atoi(what + 4))
                *got = 1;
    }
    else if (!strcmp(what, "gust"))          /* the gust's envelope, in percent */
        *got = (int)(dr_noise_gust() * 100.0f);
    else if (!strcmp(what, "gusts"))         /* gusts so far */
        *got = (int)dr_noise_gusts();
    else if (!strcmp(what, "noise")) {       /* the noise layer is heard now */
        float lv, nt, co, bd;
        dr_noise(&lv, &nt, &co, &bd);
        *got = lv > 0.01f;
    } else if (!strcmp(what, "bus"))         /* the voice bus peak and the output peak, last block, in 1/1000 */
        *got = (int)(dr_bus_level() * 1000.0f);
    else if (!strcmp(what, "out"))
        *got = (int)(dr_out_level() * 1000.0f);
    else if (!strcmp(what, "ch_in"))
        *got = set.midi_in;
    else if (!strcmp(what, "ch_out"))
        *got = set.midi_out;
    else if (!strcmp(what, "local"))
        *got = set.local;
    else if (!strcmp(what, "speaker"))
        *got = set.speaker;
    else if (!strcmp(what, "usblevel"))
        *got = set.usb_level;
    else if (what[0] == 'h' && what[1] >= '0' && what[1] <= '9')   /* "h3": parameter 3 as heard, 0..127 */
        *got = (int)(dr_heard((uint32_t)atoi(what + 1)) + 0.5f);
    else if (!strcmp(what, "pressure")) {   /* the pressure of the keys held, the most */
        uint32_t k;
        *got = 0;
        for (k = 0; k < NKEYS; k++)
            if ((plat_keys() >> k & 1u) && touch_pressure(k) > (uint32_t)*got)
                *got = touch_pressure(k);
    }
    else if (!strcmp(what, "echorun"))
        *got = dr_echo_running();
    else if (!strcmp(what, "cyclepos"))
        *got = (int)dr_orbit_cycle_pos();
    else if (!strcmp(what, "arpsteps"))
        *got = (int)dr_arp_steps();
    else if (!strcmp(what, "clockext"))
        *got = dr_clock_ext();
    else if (!strcmp(what, "clockbpm"))
        *got = (int)(dr_clock_bpm() + 0.5f);
    else if (!strcmp(what, "clockout"))
        *got = set.clock_out;
    else if (!strncmp(what, "usbrec", 6)) {   /* "usbrec1" / "usbrec0": the script says a computer records (or not) */
        host_usb_rec = what[6] == '1';
        *got = host_usb_rec;
    } else if (!strcmp(what, "limiter"))    /* the limiter's deepest gain since the last read, in 1/1000 */
        *got = (int)(fx_lim_min() * 1000.0f);
    else if (!strcmp(what, "gain"))         /* the speaker's gain at MASTER full, 0..4096 */
        *got = (int)df_gain(4096u);
    else if (!strcmp(what, "vel")) {
        uint32_t k;
        *got = 0;
        for (k = 0; k < NKEYS; k++)
            if (ui.keys >> k & 1u)
                *got = ui.vel[k];
    } else if (what[0] == 'p' && what[1] >= '0' && what[1] <= '9')
        *got = dr_scene.p[atoi(what + 1) % P_COUNT];
    else
        return 0;
    return 1;
}
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d)
{
    (void)c;
    (void)d;
    if (!strcmp(cmd, "land")) {          /* "land N": scene N lands with its chord, this millisecond (the renders) */
        scene_go((uint32_t)atoi(a) % NSLOTS, 1, 1);
        return 1;
    }
    if (!strcmp(cmd, "sxloop")) {        /* "sxloop A B": slot A as a SysEx frame, taken into user slot B (the bank's
                                          * dump and load, round trip) */
        uint32_t n = dfx_build((uint32_t)atoi(a));
        dfx_frame[5] = (uint8_t)atoi(b);
        return app_sysex(dfx_frame + 1, n - 2u);
    }
    if (strcmp(cmd, "param") || atoi(a) < 0 || atoi(a) >= (int)P_COUNT)
        return 0;
    value_set((uint32_t)atoi(a), (uint32_t)atoi(b) & 127u);
    return 1;
}
#endif
