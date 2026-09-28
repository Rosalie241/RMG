# SoftRDP

A fast and accurate software renderer for the Nintendo 64 Reality Display
Processor.

## Overview

SoftRDP renders the RDP pixel pipeline in software with high accuracy and
performance. The core is written from scratch in portable C17, is threaded and optimized for modern CPUs, and presents through OpenGL.

It builds as a plugin for Project64 and for Mupen64Plus.

SoftRDP is also integrated into ares [in this branch](https://github.com/PixelMechanic0/ares/tree/softrdp-integration).

## Confirmed accuracy

Accuracy is confirmed by passing all paraLLEl-RDP conformance tests
against the pixel-accurate reference angrylion-rdp-plus. Where SoftRDP
departs from that reference, the reference has been proven wrong by a
hardware test. Fixes from CEN64 RDP were ported over to further improve
accuracy.

## 2x internal resolution

SoftRDP can render at twice the resolution on each axis, so every pixel the
console would draw becomes a small block of four. The whole renderer works at
that higher resolution rather than stretching a finished picture, so edges come
out with real extra detail instead of a smoother version of the same steps.

Games still see what they expect. The image the console itself would have drawn
is kept exactly, with the extra detail carried alongside it, so a game that
reads the screen back gets the picture it was written for.

Turning it on is about four times as much work for the CPU, which makes
threading matter more.

## Requirements

* Windows, Linux or macOS
* OpenGL 3.3 is used for presentation
* A little-endian CPU architecture
* AVX2 on x86 and x86-64; non-x86 builds do not require AVX2

## Building

`make` builds the Mupen64Plus plugin with the host compiler. On Windows it also
builds the Project64 plugin, which is a 32-bit DLL by definition and needs a
32-bit MinGW toolchain (`i686-w64-mingw32-gcc`).

```text
make mupen64plus   # build/mupen64plus-video-softrdp.dll, build/linux/...so
make pj64          # build32/softrdp-pj64.dll (Windows only)
make               # everything available on this host
```

## Installation

**Project64** — copy `softrdp-pj64.dll` into the emulator's `Plugin/GFX`
directory and select it under Options.

**Mupen64Plus** — copy `mupen64plus-video-softrdp.dll` (or `.so`) next to the
other video plugins and select it in your frontend.

## Configuration

There are six settings, the same in both plugins.

`Workers` sets how many threads render. It is 0 by default, which picks a count
to suit your CPU. Set it to 1 to render on a single core, or to a specific
number, up to 12, to hold it there.

`Scale` sets the internal resolution. It is 1 by default, which is what the
console drew. Set it to 2 for twice the resolution on each axis, as described
above. Anything outside that range is clamped to the supported range of 1–2.

The remaining four switch off parts of the VI, the chip that turned the
console's framebuffer into its video signal. `DisableVIDitherFilter`,
`DisableVIDivotFilter`, `DisableVIGammaDither` and `DisableVIAA` each turn one
off even when a game asks for it. All four are 0 by default, which leaves the VI
behaving as the console did. Set one to 1 for a cleaner picture, but doing so makes the output less faithful to the original console.

Every setting is read when a game is loaded, so a change takes effect the next
time you load one.

**Project64** reads `softrdp.ini` next to the plugin DLL. Any key missing from
the file is written back with its default, so the file documents itself the
first time the plugin runs:

```ini
[SoftRDP]
Workers=0
Scale=1
DisableVIDitherFilter=0
DisableVIDivotFilter=0
DisableVIGammaDither=0
DisableVIAA=0
```

**Mupen64Plus** reads the `[Video-SoftRDP]` section of `mupen64plus.cfg`. Window
size and fullscreen come from the emulator's own `Video-General` section, as
with any Mupen64Plus video plugin.

## How it works

### Threading

SoftRDP implements scanline-parallel rendering with automatic hazard tracking,
allowing multithreading to preserve accuracy without selectable compatibility
profiles.

Every worker replays the same ordered command batch with private RDP and TMEM
state while rendering separate scanlines. As commands are collected, SoftRDP
tracks pending framebuffer and depth buffer writes and flushes the batch when a
render target changes or a texture load overlaps data that is still being
produced.

This applies read-after-write dependency handling directly to the memory
behavior of the N64 RDP, deriving synchronization points from the command stream
and its memory dependencies.

The scanline interleaving architecture was previously demonstrated successfully
by ata4 in Angrylion RDP Plus and provided an important reference for
SoftRDP. Angrylion RDP Plus handles synchronization through compatibility
profiles, while SoftRDP develops the approach further by deriving synchronization
points automatically from the command stream.

### 2x rendering

SoftRDP implements 2x internal rendering on the CPU by representing each RDRAM
pixel as a 2 by 2 sample grid, with corresponding color, depth, and hidden
coverage data.

This follows the multisampled RDRAM model demonstrated on the GPU by
Themaister's paraLLEl-RDP, including the use of a reference copy to maintain
coherence with normal emulated memory.

SoftRDP keeps the first sample in ordinary RDRAM while storing the remaining
samples in address-indexed side planes. Other emulator components and RDP
texture loads therefore retain the normal RDRAM view, while the renderer and VI
can access the complete high-resolution sample grid.

Changes made to RDRAM outside the renderer are detected and propagated to the
additional samples, and address-based storage preserves overlapping framebuffer
layouts.

To make this practical on the CPU, SoftRDP compiles the scale-dependent core
into separate 1x and 2x variants. Selection occurs when the rendering context is
created, allowing scale calculations and conditional paths to be removed from
the performance-critical pixel loops.

## Acknowledgements

SoftRDP is a distinct RDP implementation informed by the research and reference work of Angrylion, Ville Linde, MooglyGuy, and others.

SoftRDP incorporates and adapts code and implementation techniques from Rupert Carmichael's CEN64 RDP, based on work by Ville Linde, Ryan Holtz, Vas Crabb, Aaron Giles, and other contributors. It also includes MIT-licensed material derived from Themaister's paraLLEl-RDP and draws on ideas and techniques demonstrated in ata4's angrylion-rdp-plus.

See THIRD_PARTY_NOTICES.md for attribution, copyright notices, and license terms applicable to third-party material.

## License

SoftRDP is licensed under the MIT License, except for third-party material that remains subject to its respective license terms.

This project contains or derives material from:

* **CEN64 RDP**, licensed under the BSD 3-Clause License.
* **paraLLEl-RDP** by Themaister, licensed under the MIT License.

See `THIRD_PARTY_NOTICES.md` and the `LICENSES/` directory for the applicable third-party copyright notices and license texts.
