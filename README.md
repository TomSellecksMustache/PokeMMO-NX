[![NXpanded Build](https://github.com/TomSellecksMustache/PokeMMO-NXpanded/actions/workflows/build.yml/badge.svg)](https://github.com/TomSellecksMustache/PokeMMO-NXpanded/actions/workflows/build.yml)

# PokeMMO-NXpanded

<img width="2306" height="1289" alt="PokeMMO-NXpanded screenshot" src="https://github.com/user-attachments/assets/43bf8dd0-27b0-4c9e-ba1a-d19e941da828" />

A fork of [PokeMMO-NX](https://github.com/Petit-Prince-dev/PokeMMO-NX) by Petit-Prince-dev, which runs the official PokeMMO client on the Nintendo Switch. This fork is based on his version 1.1.4. For game setup, ROMs and launching, follow the [upstream README](https://github.com/Petit-Prince-dev/PokeMMO-NX#readme).

**NXpanded includes a breeding overlay.** That is the reason for this fork. The goal is a small modular set of in-game helpers that make PokeMMO simpler to play on a console.

> Unofficial project, not affiliated with PokeMMO or Nintendo.

## Breeding overlay

Say which Pokémon you want (species, nature, stats at 31) and what you own (Power items, Everstones, Pokémon). It plans every breed and shows you the **next step** over the game. Mark a step done and your stock and items update.

- **Smart plans:** uses your recorded Pokémon and items, and accepts up to 3 extra breeds if that saves buying items.
- **Quick HUD toast:** R + D-pad up + R3 shows the goal, next pair, held items, and remaining step count. It fades out over 10 seconds; movement, gameplay, and the regular R3 keyboard shortcut otherwise continue normally.
- **Visual Enhancements (experimental):** R + D-pad down + R3 opens a controller-only two-tab menu with five independent ON/OFF settings. Use **L/R** to cycle tabs, **D-pad up/down** to select, **A** to toggle, and **B** to close. Tab 1 (Colour & finish) offers colour grading and vignette; tab 2 (Soft focus) contains softened pixels, tilt-shift blur, and bloom. All default OFF and save to the SD card. NXpanded's shader processes only the displayed game frame; NXpanded's overlays are drawn afterward. Game UI may also be filtered. On shader initialization failure the original frame remains visible.
- **Shopping list:** R + D-pad left + R3 opens a read-only checklist of missing Power items and Everstones for the entire active goal (required minus owned).
- **Stock-first queue:** independent steps using owned parents and held items move ahead of steps needing new purchases, without skipping prerequisite breeds. The next-step screen reports stock coverage and the full queue marks fully stocked steps with `*`.
- **Set up on the console:** type the species with the console keyboard (the list narrows as you type); set nature, stats, items and stock with the D-pad.
- **Breeding chains:** put one project aside, start another, and come back later. Nothing is lost.
- **Egg groups:** all 649 species of generations 1 to 5, built in.
- **Safe:** anything destructive asks first, with No selected by default.

### Controls

| Button               | Does                  |
| -------------------- | --------------------- |
| R + D-pad right + R3 | Open the main overlay |
| R + D-pad left + R3  | Show goal shopping list |
| R + D-pad up + R3    | 10-second non-modal progress toast |
| R + D-pad down + R3  | Visual Enhancements: L/R tabs, D-pad select, A toggle, B close |
| A                    | Done (asks first)     |
| Y / X                | Items / all steps     |
| + / −                | Setup / chains        |
| B                    | Back, close           |

### Visual-enhancements note

The experimental filter is optional, OFF by default and not hardware-benchmarked. It uses an OpenGL 2.1 / GLSL 1.20 single-pass fullscreen presentation shader after PokeMMO finishes drawing, before NXpanded draws its overlays. This does **not** alter the Java client, but the captured game image includes its own HUD, text and menus. All five effects may be independently combined. If the graphics shader is unsupported the renderer bypasses it. Benchmark on real hardware before publishing production builds.

This development iteration builds on the **v1.2.0 feature set** (shopping list, stock-first queue and progress toast); it is **not** a new v1.2.0 production release. Do not overwrite the existing `v1.2.0-NXpanded` Git tag.

The menu is intentionally small: **Tab 1 = Colour grading, Vignette** (low-cost adjustments); **Tab 2 = Soften pixels, Tilt-shift blur, Soft bloom** (more texture samples). All five switches remain independent and there are no touch-only actions. Shoulder buttons cycle in either direction, with the selection resetting to the first effect when a tab changes. While the menu is open, controller input is captured by NXpanded; it is returned to the game after the menu is closed and the buttons are released.

**Not included deliberately:** cinematic bokeh/depth-of-field and full-resolution multi-pass blur (potentially large performance costs on the original Switch); ray tracing, SSAO or real depth effects (no depth/normal information in the captured image, and unsuitable GPU cost); AI upscaling (no dedicated tensor hardware); film grain, CRT scanlines, chromatic aberration and similar novelty effects (clutter without supporting the chosen soft look). FXAA, sharpening and a full preset/slider system are deferred until Switch measurements show a clear benefit worth the extra complexity. No FPS impact is guaranteed without physical Switch benchmarks.

### Your data

Your data is plain text on the SD card. **Stock is entered manually** in the overlay; it never reads the game client or automatically discovers in-game Pokémon or items:

- `switch/PokeMMO/breeding.csv`
- `switch/PokeMMO/chains/`
- `switch/PokeMMO/visual-effects.cfg` (optional visual settings)

### Design rule

The overlay only draws its own pixels and reads the controller input the loader already has. It never touches the game client, its memory or network, and never sends input to the game. That is a design rule, not a guarantee.

## Build instructions

To ensure a consistent and reproducible build environment across all operating systems (Windows, macOS, and Linux), I recommend building the project in a container.

### Prerequisites

You must have either [Docker](https://www.docker.com/) or [Podman](https://podman.io/) installed on your system.

### Building the project

Run the following command from the **root directory** of this repository. It mounts your current folder into the official devkitPro container and compiles the project.

**Docker (Windows, macOS, most Linux):**

```bash
docker run --rm -v "${PWD}":/work -w /work/native devkitpro/devkita64 make -j2
```

**Podman (Fedora, RHEL, CentOS):**

```bash
podman run --rm -v "${PWD}":/work -w /work/native docker.io/devkitpro/devkita64 make -j2
```

### Test the breeding planner without a Switch

```sh
./tools/test_breeding.sh
```

This checks that owned Pokémon and held items are prioritized, that dependent breeds remain in order, and that stock and item quantities are consumed only once. It compiles the portable planner on your computer; it does not produce an `.nro`.

### Launching

Launch in application mode and press **R + D-pad right + R3**.

## Limits

- Species are generations 1 to 5. Egg groups come from PokeAPI and differ from older data on a few species (Ralts and Trapinch lines).
- I can't say how PokeMMO's team views third-party tools. Read their rules and use this at your own risk.

## License and credits

MIT, see `LICENSE`. Original copyright: Petit_Prince. Additions in this fork are also MIT.

Third-party libraries: see `THIRD-PARTY-NOTICES.md`.

Thanks to Petit-Prince-dev for PokeMMO-NX, the PokeMMO team for the game, and [PokeAPI](https://github.com/PokeAPI/pokeapi) for the egg-group data.

### SD-card installation location

NXpanded loads its game client, ROMs and mods from `switch/PokeMMO-NXpanded/isolate-root/game/`. Place ROMs in `switch/PokeMMO-NXpanded/isolate-root/game/roms/` and game mods in `switch/PokeMMO-NXpanded/isolate-root/game/data/mods/` where supported by the PokeMMO client. Previous `switch/PokeMMO/` data is **not** automatically migrated or deleted; copy the files you wish to retain to the matching new folder before launch. NXpanded saves its breeding files and chains under `switch/PokeMMO-NXpanded/`.

All NXpanded menu text is clipped to its drawing window. Explanatory rows wrap when room permits; longer single-line fields use a slow, pausing, reversible marquee without a jump. This applies to the breeding, shopping, toast and file-picker overlays. No touchscreen is required.
