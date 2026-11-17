/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann
 * Copyright (C) 2026 hdavid: the panel's model, from felucca-bsp's tests/ldr_screen_test.c */
/* DEEP FIELD's update screen (loader_screen.c) on a model of the panel: what it is sent lands in a 240x240 frame.
 * Checked: nothing sent when nothing changes; the lit dots only grow, and all of them are lit at the end; a silent
 * host dims them once and its next frame lights them; the lines stay off the vortex. With a directory as its
 * argument, the states are written there as PPMs.
 *   cc -std=c99 -Wall -Wextra -I deepfield deepfield/tests/loader_screen_test.c -o /tmp/lst && /tmp/lst [DIR] */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static uint16_t frame[240][240];
static uint32_t win[4], wx, wy, sent, outside;
static uint8_t cmd;

static void fm1_lcd_hw_init(void) {}
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
static void fm1_lcd_wait(void) {}
static void fm1_lcd_deselect(void) {}
static void fm1_lcd_send_cmd(uint8_t c)
{
    cmd = c;
    sent++;
    if (c == 0x2C) {
        wx = win[0];
        wy = win[2];
    }
}
static void fm1_lcd_send_data(const void *p, uint32_t n)
{
    const uint8_t *b = p;
    uint32_t i;
    sent += n;
    if (cmd == 0x2A || cmd == 0x2B) {
        uint32_t *w = win + (cmd == 0x2B ? 2 : 0);
        w[0] = (uint32_t)b[0] << 8 | b[1];
        w[1] = (uint32_t)b[2] << 8 | b[3];
        if (w[0] > w[1] || w[1] > 239u)
            outside++;
        return;
    }
    if (cmd != 0x2C)
        return;
    for (i = 0; i + 1 < n; i += 2) {
        if (wx < 240 && wy < 240)
            frame[wy][wx] = (uint16_t)(b[i] << 8 | b[i + 1]);
        else
            outside++;
        if (++wx > win[1]) {
            wx = win[0];
            wy++;
        }
    }
}

#define LDR_SCREEN_TEST 1
#include "../loader_screen.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); \
                                       printf("\n"); } } while (0)
static const char *dir;
static void shot(const char *name)
{
    char path[512];
    FILE *f;
    uint32_t x, y;
    if (!dir)
        return;
    snprintf(path, sizeof path, "%s/%s.ppm", dir, name);
    f = fopen(path, "wb");
    if (!f)
        return;
    fprintf(f, "P6 240 240 255\n");
    for (y = 0; y < 240; y++)
        for (x = 0; x < 240; x++) {
            uint16_t c = frame[y][x];
            fputc((c >> 11) << 3, f);
            fputc((c >> 5 & 63) << 2, f);
            fputc((c & 31) << 3, f);
        }
    fclose(f);
}
static uint32_t count(uint16_t c, uint32_t y0, uint32_t y1)
{
    uint32_t x, y, n = 0;
    for (y = y0; y < y1; y++)
        for (x = 0; x < 240; x++)
            n += frame[y][x] == c;
    return n;
}
static uint32_t dots_as(const uint16_t *ramp)       /* dots whose first pixel is that ramp's colour for them */
{
    uint32_t k, n = 0;
    for (k = 0; k < SWIRL_N; k++)
        n += frame[SWIRL_Y[k]][SWIRL_X[k]] == (ramp ? ramp[SWIRL_SC[k] & 15u] : SCR_WHITE);
    return n;
}

int main(int argc, char **argv)
{
    uint32_t i, s, total = 448u * 1024u + 200u * 1024u, last = 0;
    dir = argc > 1 ? argv[1] : 0;
    for (i = 0; i < SWIRL_N; i++) {                 /* the table: on the panel, between the lines, clear of the eye */
        uint32_t sz = SWIRL_SC[i] >> 4;
        CHECK(sz >= 1 && SWIRL_X[i] + sz <= 240u && SWIRL_Y[i] >= SCR_TOP_Y + 14u && SWIRL_Y[i] + sz <= SCR_SMALL_Y,
              "dot %u at %u,%u size %u", (unsigned)i, SWIRL_X[i], SWIRL_Y[i], (unsigned)sz);
        CHECK(!(SWIRL_X[i] + sz > 97u && SWIRL_X[i] < 143u && SWIRL_Y[i] + sz > SCR_PC_Y && SWIRL_Y[i] < SCR_PC_Y + 14u),
              "dot %u in the percentage's box", (unsigned)i);
    }
    ldr_screen_init();
    CHECK(dots_as(SWIRL_GHOST) == SWIRL_N, "ghosts %u", (unsigned)dots_as(SWIRL_GHOST));
    shot("0-waiting-start");
    for (i = 0; i < 20u * SCR_SWEEP_POLLS; i++)
        ldr_screen_tick();
    CHECK(dots_as(SWIRL_WAIT) == SWIRL_ARMS, "the glimmer: %u dots", (unsigned)dots_as(SWIRL_WAIT));
    shot("1-waiting");
    for (i = 0; i < 400u * SCR_SWEEP_POLLS; i++)    /* many times round: it wraps inside the table */
        ldr_screen_tick();
    CHECK(dots_as(SWIRL_WAIT) == SWIRL_ARMS && !outside, "the glimmer after a long wait");
    ldr_screen_progress(0, 1);
    CHECK(dots_as(SWIRL_GHOST) == SWIRL_N, "the glimmer gone at the install's start");
    for (i = 0; i < total; i += 4096u) {
        ldr_screen_heard();
        ldr_screen_progress(i, total);
        CHECK(scr_shown >= last, "the dots went back");
        last = scr_shown;
        s = sent;
        ldr_screen_progress(i, total);              /* the same again: nothing to send */
        ldr_screen_heard();
        CHECK(sent == s, "sent %u bytes with nothing changed at %u", (unsigned)(sent - s), (unsigned)i);
        if (i == (total / 3u & ~4095u)) {
            CHECK(dots_as(0) == SWIRL_ARMS, "white heads: %u", (unsigned)dots_as(0));
            shot("2-a-third");
            ldr_screen_progress(i / 2u, total);     /* a resumed install below what is shown: kept */
            CHECK(scr_shown == last, "a resume took dots away");
        }
        if (i == (total * 2u / 3u & ~4095u)) {
            uint32_t lit = scr_shown;
            shot("3-two-thirds");
            for (s = 0; s < SCR_LOST_POLLS + 10u; s++)
                ldr_screen_tick();
            CHECK(scr_state == 1u && dots_as(SWIRL_DIM) == lit, "lost: %u of %u dim", (unsigned)dots_as(SWIRL_DIM), (unsigned)lit);
            CHECK(count(SCR_AMBER, 0, 240) > 300u && count(SCR_FG, 0, 240) == 0u, "the lines amber while lost");
            s = sent;
            for (lit = 0; lit < 5000u; lit++)
                ldr_screen_tick();
            CHECK(sent == s, "lost drawn more than once");
            shot("4-usb-lost");
            ldr_screen_heard();
            CHECK(scr_state == 0u && dots_as(SWIRL_DIM) == 0u && count(SCR_AMBER, 0, 240) == 0u, "the host back");
        }
    }
    CHECK(scr_pc <= 99u, "100%% before the end");
    ldr_screen_done();
    CHECK(scr_shown == SWIRL_N && scr_pc == 100u && dots_as(SWIRL_LIT) == SWIRL_N, "done: %u dots, %u%%, %u in colour",
          (unsigned)scr_shown, (unsigned)scr_pc, (unsigned)dots_as(SWIRL_LIT));
    CHECK(count(SCR_WHITE, 70, 150) >= SWIRL_RING * 9u - 20u, "the ring round the eye");
    CHECK(!outside, "%u writes outside the panel", (unsigned)outside);
    shot("5-done");
    printf(fails ? "loader_screen_test: %d FAILED\n" : "loader_screen_test: ok\n", fails);
    return fails != 0;
}
