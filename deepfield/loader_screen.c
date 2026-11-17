/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann
 * Copyright (C) 2026 Charles Vestal: the panel's start, the glyphs and the text, from felucca-bsp's ldr_screen.c */
/* DEEP FIELD's update screen, in place of the update loader's own (fm1.toml: loader_screen). The same five calls
 * and the same promises as felucca-bsp's loader/ldr_screen.c, which this is built in place of: everything in RAM,
 * nothing sent to the panel unless something changed, the progress only ever growing.
 *
 * The progress is the vortex drawing itself. Three arms of dots spiral in from the rim to the eye (loader_swirl.h,
 * made by tools/gen_loader_swirl.py): all 192 stand there as ghosts from the start, and the install lights them in
 * order, violet at the rim through blue and cyan to white at the eye, the newest dot of each arm white-hot. The
 * percentage stands in the eye. Before the host starts, a glimmer runs in along the ghosts (the loader waits,
 * alive). A host silent for 2 s mid-install: the lit dots dim where they are and the lines turn amber, "USB LOST",
 * "RECONNECT THE CABLE", "DO NOT POWER OFF"; its next frame lights them again. At the end a white ring closes round
 * the eye: "DONE", "RESTARTING". */
#ifndef LDR_SCREEN_TEST
#include "fm1_lcd_hw.h"
#endif
#include "loader_swirl.h"

#define SCR_BG 0x0022u                              /* the sky, #000410 */
#define SCR_FG 0xCF9Fu                              /* #c8f0ff */
#define SCR_AMBER 0xFCA8u                           /* #ff9646: the cable is out */
#define SCR_WHITE 0xFFFFu
#define SCR_TOP_Y 5u
#define SCR_SMALL_Y 205u
#define SCR_POWER_Y 223u
#define SCR_PC_Y (SWIRL_CY - 7u)
#define SCR_LOST_POLLS 4000u                        /* 2 s of loader.c's 0.5 ms USB polls without a frame */
#define SCR_SWEEP_POLLS 160u                        /* the glimmer's step, 80 ms of polls, before the install */

static uint16_t scr_strip[240u * 14u];              /* one line of text, byte-swapped (the panel takes MSB first) */
static uint8_t scr_small[8];
static uint32_t scr_shown;                          /* dots lit */
static uint32_t scr_pc;                             /* the percentage drawn */
static uint32_t scr_quiet;                          /* polls since the host's last frame */
static uint32_t scr_wait, scr_sweep;                /* polls waiting; the glimmer's dot + 1 (0: none) */
static uint16_t scr_fg = SCR_FG;
static uint8_t scr_on, scr_armed;                   /* armed: an install has started */
static uint8_t scr_state;                           /* 0 updating, 1 USB lost, 2 done */

/* 5x7 glyphs, a row a byte (bit 4 = left) */
static const struct { char c; uint8_t rows[7]; } SCR_FONT[] = {
    { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
    { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
    { '3', { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E } },
    { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
    { '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
    { '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
    { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
    { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
    { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },
    { '%', { 0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03 } },
    { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
    { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
    { 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
    { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
    { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
    { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
    { 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
    { 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
    { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
    { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
    { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
    { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    { 'W', { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A } },
};

static void scr_cmd(uint8_t c)
{
    fm1_lcd_send_cmd(c);
    fm1_lcd_wait();
    fm1_lcd_deselect();
}

static void scr_data(const void *p, uint32_t n)     /* n > 0 */
{
    fm1_lcd_send_data(p, n);
    fm1_lcd_wait();
    fm1_lcd_deselect();
}

static void scr_window(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
    scr_small[0] = (uint8_t)(x0 >> 8); scr_small[1] = (uint8_t)x0;
    scr_small[2] = (uint8_t)(x1 >> 8); scr_small[3] = (uint8_t)x1;
    scr_cmd(0x2A);
    scr_data(scr_small, 4);
    scr_small[4] = (uint8_t)(y0 >> 8); scr_small[5] = (uint8_t)y0;
    scr_small[6] = (uint8_t)(y1 >> 8); scr_small[7] = (uint8_t)y1;
    scr_cmd(0x2B);
    scr_data(scr_small + 4, 4);
    scr_cmd(0x2C);
}

static uint16_t scr_swap(uint16_t c) { return (uint16_t)((c >> 8) | (c << 8)); }

static void scr_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, n = w * h, k = sizeof scr_strip / sizeof scr_strip[0];
    for (i = 0; i < k && i < n; i++)
        scr_strip[i] = scr_swap(c);
    scr_window(x, y, x + w - 1u, y + h - 1u);
    for (; n; n -= k) {
        if (k > n)
            k = n;
        scr_data(scr_strip, k * 2u);
    }
}

/* one line at scale 2, centred in a box of cols characters (or its own length if longer), the box cleared: a
 * shorter line put over a longer one of the same box leaves nothing behind. 19 characters fit (226 px) */
static void scr_text(uint32_t y, const char *s, uint32_t cols)
{
    uint32_t len = 0, i, r, b, sx, sy, x0, w, pad;
    while (s[len])
        len++;
    if (cols < len)
        cols = len;
    if (cols > 19u)
        return;
    w = cols * 12u - 2u;
    x0 = (240u - w) / 2u;
    pad = (cols - len) * 6u;
    for (i = 0; i < w * 14u; i++)
        scr_strip[i] = scr_swap(SCR_BG);
    for (i = 0; i < len; i++) {
        const uint8_t *g = 0;
        uint32_t f;
        for (f = 0; f < sizeof SCR_FONT / sizeof SCR_FONT[0]; f++)
            if (SCR_FONT[f].c == s[i])
                g = SCR_FONT[f].rows;
        if (!g)
            continue;                               /* a space */
        for (r = 0; r < 7u; r++)
            for (b = 0; b < 5u; b++)
                if (g[r] & (0x10u >> b))
                    for (sy = 0; sy < 2u; sy++)
                        for (sx = 0; sx < 2u; sx++)
                            scr_strip[(r * 2u + sy) * w + pad + (i * 6u + b) * 2u + sx] = scr_swap(scr_fg);
    }
    scr_window(x0, y, x0 + w - 1u, y + 13u);
    scr_data(scr_strip, w * 14u * 2u);
}

static void scr_lines(const char *top, const char *small, uint32_t warn)
{
    scr_text(SCR_TOP_Y, top, 8);                    /* "UPDATING", "USB LOST", "DONE" */
    scr_text(SCR_SMALL_Y, small, 19);               /* "DO NOT UNPLUG", "RECONNECT THE CABLE", "RESTARTING" */
    scr_text(SCR_POWER_Y, warn ? "DO NOT POWER OFF" : "", 19);
}

static void scr_percent(uint32_t pc)                /* "0%" .. "100%", in the eye */
{
    char t[5], *p = t;
    if (pc >= 100u)
        *p++ = '1';
    if (pc >= 10u)
        *p++ = (char)('0' + pc / 10u % 10u);
    *p++ = (char)('0' + pc % 10u);
    *p++ = '%';
    *p = 0;
    scr_text(SCR_PC_Y, t, 4);
}

static void scr_dot(uint32_t k, const uint16_t *ramp)   /* dot k in its colour of a ramp (0: white-hot) */
{
    uint32_t s = SWIRL_SC[k] >> 4;
    scr_fill(SWIRL_X[k], SWIRL_Y[k], s, s, ramp ? ramp[SWIRL_SC[k] & 15u] : SCR_WHITE);
}

static void scr_arms(void)                          /* every lit dot again, as the state has them: dimmed while the
                                                     * cable is out, else in colour with each arm's newest white */
{
    uint32_t k;
    for (k = 0; k < scr_shown; k++)
        scr_dot(k, scr_state == 1u ? SWIRL_DIM : k + SWIRL_ARMS >= scr_shown && scr_state != 2u ? 0 : SWIRL_LIT);
}

/* the waiting glimmer: one dot of each arm brighter, a step further in each time, the ones behind ghosts again */
static void scr_sweep_step(void)
{
    uint32_t a;
    if (scr_sweep)
        for (a = 0; a < SWIRL_ARMS; a++)
            scr_dot(scr_sweep - 1u + a, SWIRL_GHOST);
    scr_sweep = scr_sweep && scr_sweep + SWIRL_ARMS < SWIRL_N ? scr_sweep + SWIRL_ARMS : 1u;
    for (a = 0; a < SWIRL_ARMS; a++)
        scr_dot(scr_sweep - 1u + a, SWIRL_WAIT);
}

static void ldr_screen_init(void)
{
    uint32_t k;
    fm1_lcd_hw_init();                              /* pins, backlight on, SPI1 */
    fm1_delay_ms(20);
    scr_cmd(0x01);                                  /* SWRESET */
    fm1_delay_ms(150);
    scr_cmd(0x11);                                  /* SLPOUT */
    fm1_delay_ms(120);
    scr_cmd(0x3A);                                  /* COLMOD: RGB565 */
    scr_small[0] = 0x55;
    scr_data(scr_small, 1);
    scr_cmd(0x36);                                  /* MADCTL: top-left origin, RGB order */
    scr_small[0] = 0x00;
    scr_data(scr_small, 1);
    scr_cmd(0x21);                                  /* INVON: IPS panel */
    scr_cmd(0x13);                                  /* NORON */
    scr_fill(0, 0, 240, 240, SCR_BG);
    scr_lines("UPDATING", "DO NOT UNPLUG", 1);
    for (k = 0; k < SWIRL_N; k++)
        scr_dot(k, SWIRL_GHOST);
    scr_percent(0);
    scr_cmd(0x29);                                  /* DISPON */
    scr_on = 1;
}

/* the dots and the percentage only grow: a resumed install starts below what is shown and keeps it */
static void ldr_screen_progress(uint32_t done, uint32_t total)
{
    uint32_t n, pc;
    if (!scr_on || !total)
        return;
    if (!scr_armed && scr_sweep) {                  /* the install starts: the glimmer goes */
        for (n = 0; n < SWIRL_ARMS; n++)
            scr_dot(scr_sweep - 1u + n, SWIRL_GHOST);
        scr_sweep = 0;
    }
    scr_armed = 1;
    n = done >= total ? SWIRL_N : done / (total / SWIRL_N + 1u);   /* (no product: total is up to 2 MiB, and the
                                                     * last dots wait for the end either way) */
    pc = done >= total ? 100u : done / (total / 100u + 1u);
    for (; scr_shown < n; scr_shown++) {            /* the new dot white-hot, the one it follows on its arm cooled */
        scr_dot(scr_shown, scr_state == 1u ? SWIRL_DIM : 0);
        if (scr_shown >= SWIRL_ARMS && scr_state != 1u)
            scr_dot(scr_shown - SWIRL_ARMS, SWIRL_LIT);
    }
    if (pc > scr_pc) {
        scr_percent(pc);
        scr_pc = pc;
    }
}

/* a frame from the host (cheap: called for each one) */
static void ldr_screen_heard(void)
{
    scr_quiet = 0;
    if (scr_state == 1u) {
        scr_state = 0;
        scr_fg = SCR_FG;
        scr_lines("UPDATING", "DO NOT UNPLUG", 1);
        scr_percent(scr_pc);
        scr_arms();
    }
}

/* one USB poll (0.5 ms) without a frame: before the install the glimmer steps; an install whose host stays silent
 * 2 s is shown lost, once */
static void ldr_screen_tick(void)
{
    if (!scr_on)
        return;
    if (!scr_armed) {
        if (++scr_wait >= SCR_SWEEP_POLLS) {
            scr_wait = 0;
            scr_sweep_step();
        }
        return;
    }
    if (scr_state || ++scr_quiet < SCR_LOST_POLLS)
        return;
    scr_state = 1;
    scr_fg = SCR_AMBER;
    scr_lines("USB LOST", "RECONNECT THE CABLE", 1);
    scr_percent(scr_pc);
    scr_arms();
}

/* written and verified: the app starts next */
static void ldr_screen_done(void)
{
    uint32_t k;
    if (!scr_on)
        return;
    if (scr_state == 1u)
        ldr_screen_heard();
    ldr_screen_progress(1, 1);
    scr_state = 2;
    scr_arms();
    for (k = 0; k < SWIRL_RING; k++)
        scr_fill(SWIRL_RX[k], SWIRL_RY[k], 3, 3, SCR_WHITE);
    scr_lines("DONE", "RESTARTING", 0);
}
