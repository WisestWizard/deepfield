# DEEP FIELD — the device's budget

What the FM-1 can do, what DEEP FIELD uses of it (build of 2026-11-03, image 317 KB), and the rules to keep to
when adding things. Sources: fm1-base's `app.ld`, `main_fm1.c`, `lcd.c`, `storage.c`, `fm1_cpu1.h`; the ELF's
section table; host profiles in `tools/`. The one figure not yet measured on the unit is the audio CPU, which the
build now reports (below).

| Resource | The device | DEEP FIELD today | Rule |
| --- | --- | --- | --- |
| Audio CPU | 360 MHz; a 256-frame half every 5.8 ms per core; `app_overload` at 90 %. The CPU figure is the half's wall time on core 1, which includes waiting for core 2: it reads the longer core | Core 1 (the audio interrupt): bodies 3–8, the noise, the ensemble, the delay and the limiter; core 2: the hall (on the bus of the block before) and bodies 1–2, balanced so both take about the same. Host profile per block: eight bodies 8–11 µs, hall 5–8 µs, delay 1.2 (2026-11-06, the stage alone over its input: PING 1.4, RISE and FALL 1.2, TAPE, HEADS, REVERSE and GRAINS 0.3–0.4, one line written instead of two), ensemble 0.7, limiter 0.5, noise 0.03. Device figure: the header, and the log line `cpu N% bM MODEL sS xK` (bodies, model, scene, xruns) every 10 s on change (`fm1_remote.py log`); `fm1_remote.py screen` speaks it too. Readings, 2026-11-02, Kyle playing: SOLAR ×8 bodies 41 %, BELL ×8 42 %, STRING ×8 47 % (the heaviest), VOID ×4 39 %, and 42 % with no body left but the hall and delay still ringing, 3 % once everything rests. So the active effects are about 40 % of core 1 on their own and eight bodies add only 2 to 7 points, because five of them render on core 2 in parallel: the hall is the budget, not the voices. After the hall moved to core 2 beside two bodies (the same evening): SOLAR ×8 29–31 %, CHOIR ×8 36 %, SHEPARD ×8 33 %, VOID ×4 with its ensemble and wind bed 37 %, no xruns; STRING ×8 not yet read since the move, expected high 30s. Readings 2026-11-03 night, the unit driven by `fm1_remote.py`, every factory scene as shipped with eight bodies latched, ORBIT off: FIRST LIGHT 33, VOID TIDE 35, GLASS ORBIT 33, BOWED SKY 35, PULSAR 33, NEBULA 40, DEEP VOID 39, STARLIGHT 37, CATHEDRAL 35, LOW CHOIR 47 (the heaviest; 44 at five bodies), GAMELAN 36, ICE BELLS 42, ASCENT 36, DESCENT 41, FOLDED SUN 34, TIDE POOL 29; the full load on STARLIGHT (STRING ×8, CROSS and SWAY at 100, noise BAND at 100 so every body breathes, ORBIT in ARP at 120 BPM with CLOCK OUT on) 56–57 %, no xruns, `late 0`: 37 as shipped, CROSS and SWAY add nothing readable, the breath adds 11, the runner 8, the clock 1. Everything rests at 5 % | Keep the worst scene with eight bodies under 70 % with the second core on. The effects are core 1's load (about 40 % while they ring), so a new effect must be cheap or replace one; more bodies are nearly free while core 2 carries them |
| Second core | one job a block, 8 ms timeout, 2.8 KB stack; falls back to one core until power-off | five bodies a block; `vc` and the arena on 32-byte lines | A job must finish well inside a half; share nothing it writes with core 1 in the same cache line |
| RAM (.data + .bss) | 98,304 B | 80,240 B used (build of 2026-11-06: the delay types' grains and the arp's memory on top of 11-04's lead voices, ORBIT plan, noise bands, limiter lookahead and ECHO), 17.6 KB spare; the app's own share is about 7 KB, the rest Bluetooth and the platform's rings | Keep 8 KB spare (the build refuses less) |
| Pool | 344,064 B | 330,064 B used (2026-11-06, the delay types): 14.0 KB spare. Big items: the engine's arena 235 KB (sine table 16, STRING lines 48 for twelve voices, effects lines 165: the delay's one buffer 84 at half the rate, two legs or one line by TYPE, and its four allpasses 1.6), canvas 41, Bluetooth stacks 24.5, phosphor 20 | Keep 8 KB spare. A sampler or granular model does not fit; a new delay line costs 2 bytes a sample |
| Flash (app slot) | 581,564 B | 300,220 B (48 % free): code 291 KB of which Bluetooth about 132 KB, fonts 61 KB | Plenty; a font face costs 4–20 KB |
| Stacks | main 24 KB; interrupts 7.9 KB; core 2 2.8 KB | main about 1.5 KB deep, interrupt about 1 KB, core 2 under 0.6 KB | No large locals in the render or on core 2 (a 1 KB array there is a quarter of the stack) |
| LCD | SPI about 44 MHz measured: 5.5 MB/s, a full screen 21 ms | view 7.3 ms a frame at 25 fps (18 % of the bus), keys strip 3.1 ms and footer 7.5 ms on change, header 3 ms up to once a second; the worst second about 37 % | Draw the view only on frames that sent nothing else (as now); never wait on the DMA inside a knob frame |
| Watchdog | 8 s, fed only at the top of the main loop | the longest ordinary frame about 30 ms; every loop in a frame is bounded (the pill spacing was not, until 2026-11-02) | A loop in the UI needs a bound you can name |
| Storage | 3,840 B an object, A/B sectors, a save silences the audio for tens of ms | settings 16 B, the bank 1,032 B; saved only on SAVE | Never save from a knob; a bigger bank means a second object |
| MIDI | USB 64 packets in and out, Bluetooth 64 a link, the keys' queue 32 | ORBIT and the keys send from the render, which the platform allows | A burst of more than 32 key events in a block is dropped |
| USB audio in | the ring runs dry past about 40 % render load unless `FM1B_UAC_DEEP 1` (4 KB of pool, +5.8 ms latency) | not set | Set it if recording at high load matters |

## How to read the device's CPU

1. Play a scene with eight bodies latched. The header's top right shows bodies and CPU.
2. From a computer: `python3 base/tools/fm1_remote.py log` prints the last lines, among them
   `cpu 54% b8 STRING s3 x0` (CPU, bodies, model, scene, xruns); `fm1_remote.py screen` ends with `BODIES 8. CPU 54%`.
3. `xruns` counts halves that ran late (a click each). Any growth at all is over budget.

## What is heaviest, in order

The hall (eight lines, the shimmer when on), then eight bodies (STRING and CHOIR heaviest, GLASS and FOLD
lightest), then the delay. The noise layer is free. If the unit reads over 70 % on the worst scene, the next
moves are, cheapest first: the hall's shimmer only when SHIMMER is turned up (done), six hall lines instead of
eight (a Hadamard needs four or eight, so this means four), or, done on 2026-11-02, the hall on the second core one block late (the wet
signal is a pre-delay anyway; its pre-delay line is a block shorter to keep 30 ms).
