/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Kyle McCann
 * Copyright (C) 2026 Charles Vestal (fm1-base's template, which this started from) */
/* DEEP FIELD: the app's half of the contract with fm1-base (device/unity.c, host/plat_host.c). */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "DEEP FIELD"
#define APP_SPLASH_RGB RGB(90, 200, 255)
#define APP_PALETTE 2u                   /* cyan: the platform's own screens (crash, update) */
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_CANVAS_PX (240u * 86u)       /* the largest strip, the footer, in one transfer */
#define APP_TOUCH                        /* velocity and pressure from the keys (lib/touch.h) */
#define APP_BTDEV                        /* Bluetooth MIDI devices (lib/btdev.h) */
#define APP_BLE_AT_BOOT (!df_ble_off())  /* Bluetooth at power-on: as the player left it (REC held) */
#define APP_OVERLOAD                     /* the render over 90 % of its time: the engine sheds a body */
#define APP_GAIN(m) df_gain(m)           /* the speaker's gain from MASTER: none while a computer records over USB and
                                          * SPEAKER is AUTO (the USB audio input then gets the render at full level) */
uint32_t df_gain(uint32_t m);
int df_ble_off(void);
int df_shift(void);                      /* TRANSPOSE is on */
void app_overload(void);

/* the store: the settings, the panel calibration (the platform's), Bluetooth's address and pairing, the scenes;
 * each an A/B pair of 4 KiB sectors in the data area (0xFC000 is Felucca's settings pair) */
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_BLE, OBJ_SCENES, OBJ_NOBJ };
#define APP_STORE_MAP {0xFC000u, 0xDC000u, 0xDE000u, 0xDA000u}

/* the contract: the platform calls these */
void app_init(void);                     /* boot: the engine, then the saved settings and scenes (or defaults) */
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);   /* audio interrupt: n stereo frames, 24-bit */
void app_panic(void);                    /* silence now (before an update rewrites the flash) */
void app_ui_init(void);                  /* the first screen, after the splash */
void app_frame(void);                    /* the UI: input and a redraw, about 60 times a second */
void app_input_only(void);               /* between frames: input only */
void app_say(const char *a, const char *b);   /* a short message on the screen and to a screen reader */

/* the host simulator's checks (host/plat_host.c "expect"): see app.c */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
