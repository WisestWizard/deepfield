/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann */
/* DEEP FIELD on fm1-base: the app's unity build (compiled into fm1-base's, after the platform) and the glue between
 * fm1-base's contract (app.h) and the parts: what is saved (scenes.c), the screen and controls (ui.c) and the
 * engine (engine/drone.c, its own -O2 unit). */
#include "drone.h"
#include "fx.h"                            /* fx_lim_min, for the simulator's limiter check */
static volatile uint8_t midi_edited;     /* a CC changed a scene parameter: the footer redraws */
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

/* the CC map (render context): 1 wheel, 7 volume, 64 sustain, 16-19 the four forces, 20 NOISE LEVEL, 74 MASS, 71 SHAPE, 72 FADE,
 * 73 SWELL, 91 SPACE, 93 ENS MIX, 94 DLY MIX, 120 all sound off, 121 reset controllers, 123 all notes off */
static void midi_param(uint32_t id, uint32_t v)
{
    dr_param(id, v & 127u);
    scene_dirty = 1;
    midi_edited = 1;
}
static void midi_cc(uint32_t cc, uint32_t v)
{
    switch (cc) {
    case 1: dr_wheel(v); break;
    case 7: dr_volume(v); break;
    case 64: dr_sustain(v); break;
    case 16: midi_param(P_MASS, v); break;
    case 17: midi_param(P_DRIFT, v); break;
    case 18: midi_param(P_SPACE, v); break;
    case 19: midi_param(P_TIDE, v); break;
    case 20: midi_param(P_NZ_LEVEL, v); break;
    case 74: midi_param(P_MASS, v); break;
    case 71: midi_param(P_SHAPE, v); break;
    case 72: midi_param(P_FADE, v); break;
    case 73: midi_param(P_SWELL, v); break;
    case 91: midi_param(P_SPACE, v); break;
    case 93: midi_param(P_ENS_MIX, v); break;
    case 94: midi_param(P_DLY_MIX, v); break;
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
                dr_clock();
            else if (full == 0xFAu || full == 0xFBu)
                dr_clock_start();
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
            if (set.latch)
                dr_note_now(d1, d2, d2 != 0u);
            else
                dr_lead_now(d1, d2, d2 != 0u);
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
uint32_t df_gain(uint32_t m)             /* the speaker and headphones (audio interrupt): MASTER squared, or nothing */
{
    if (set.speaker == SPK_OFF || (set.speaker == SPK_AUTO && usb_recording()))
        return 0;
    return m * m >> 12;
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
    dr_load(slot_scene(set.scene), 0.0f);    /* the scene's bodies swell in from power-on */
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
void app_frame(void)
{
    CRUMB(1);
    ui_frame();
    CRUMB(0);
}
void app_input_only(void) { ui_input_only(); }
void app_say(const char *a, const char *b) { ui_say(a, b); }

#ifdef PLAT_HOST
/* the simulator's checks: octave, note (the newest body's, -1 none), bodies, scene, page, dialog, dirty, latch (bodies latched), latchmode, velon,
 * orbit, freeze, model, scale, view, morphing, fxbypass, fxactive, sounding (dr_sounding of the next arg: "sounding60"),
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
    else if (!strcmp(what, "view"))
        *got = set.view;
    else if (!strcmp(what, "morphing"))
        *got = dr_morphing();
    else if (!strcmp(what, "fxbypass"))
        *got = (int)dr_fx_bypass_get();
    else if (!strcmp(what, "fxactive"))
        *got = (int)dr_fx_active();
    else if (!strcmp(what, "shift"))         /* TRANSPOSE: the keys move the latched bodies */
        *got = df_shift();
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
#endif
