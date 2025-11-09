![A screenshot of the running application](example.png)

![Evidence showing the same picture on the multicast receiver that is on the real RevSpace LedBanner](ledbanner-compare.jpg)

# LedBanner Multicast Receiver

A small C/SDL3 application that visualizes an 80x8 RGB565 [LED banner](https://revspace.nl/LedBanner) from UDP multicast frames. Works when connected to the [RevSpace](https://revspace.nl/) WiFi.

## What it does

- Opens an SDL3 window representing an 80x8 LED matrix (scaled for visibility).
- Listens on a multicast group for raw frames:
  - Default: 239.0.0.1:1565
  - Format: 80x8 pixels, RGB565, 2 bytes per pixel, 1280 bytes per frame.
- For each valid frame:
  - Decodes RGB565 to RGB.
  - Renders the pixels.
  - Logs simple stats (bytes, FPS, kB/s).

## Code structure

- [`config.h`](config.h:1)
  - Dimensions, multicast defaults, `AppConfig`, `DEFAULT_APPCONFIG`.
- [`display.h`](display.h:1) / [`display.c`](display.c:1)
  - SDL init and `draw_pixels_from_buffer(...)`.
- [`events.h`](events.h:1) / [`events.c`](events.c:1)
  - `handle_sdl_events(...)` (QUIT / ESC).
- [`multicast.h`](multicast.h:1) / [`multicast.c`](multicast.c:1)
  - `setup_multicast_socket(...)` for joining the multicast group.
- [`stats.h`](stats.h:1) / [`stats.c`](stats.c:1)
  - `StatsState`, logging of FPS / kB/s.
- [`main.c`](main.c:1)
  - Wires everything together:
    - init config + SDL
    - init multicast
    - event + receive + render loop
    - cleanup.

## Build

Requires:

- C compiler (POSIX-like environment).
- SDL3 development files.
- `pkg-config` (recommended).

Build and clean:

```sh
make
make clean
```

Resulting binary: `led80x8`.

## Run

```sh
./led80x8
```

- Expects 1280-byte RGB565 frames on the configured multicast address.
- Close window or press ESC to exit.

## Local multicast test sender

- [`gol_sender.c`](gol_sender.c:1) is a small demo sender that generates a Conway's Game of Life animation on an 80x8 grid.
- It packs the grid into RGB565 frames (monochrome-style) and sends them via UDP to the same multicast address/port as the receiver.
- Useful for testing the receiver without needing the real LedBanner infrastructure.


```sh
./gol_sender
```

![](gol_sender_in_action.png)

# Disclaimer

This repo contains code written with the help of AI tools ([Roo Code](https://github.com/RooCodeInc/Roo-Code) running an AI's from [openrouter.ai](https://openrouter.ai/)), plus a fair amount of human stubbornness, debugging and cleanup.

- Some of it is smart, some of it is cursed. Treat every line as untrusted until proven otherwise.
- No guarantees about correctness, security, performance, or not accidentally summoning undefined behavior demons.
- I've put effort into shaping this into something usable, but you're still expected to read, test, break, and audit the code yourself—especially before using it in anything serious, exposed, or questionable.
- Some snippets may resemble third-party material. Make sure you respect licenses and applicable laws.

Short version: enjoy, hack responsibly, and if it breaks, you get to keep all the pieces.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
