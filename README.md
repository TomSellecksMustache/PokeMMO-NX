[![NXpanded Build](https://github.com/TomSellecksMustache/PokeMMO-NXpanded/actions/workflows/build.yml/badge.svg)](https://github.com/TomSellecksMustache/PokeMMO-NXpanded/actions/workflows/build.yml)

# PokeMMO-NXpanded

<img width="2306" height="1289" alt="PokeMMO-NXpanded screenshot" src="https://github.com/user-attachments/assets/43bf8dd0-27b0-4c9e-ba1a-d19e941da828" />

A fork of [PokeMMO-NX](https://github.com/Petit-Prince-dev/PokeMMO-NX) by Petit-Prince-dev, which runs the official PokeMMO client on the Nintendo Switch. This fork is based on his version 1.1.4. For game setup, ROMs and launching, follow the [upstream README](https://github.com/Petit-Prince-dev/PokeMMO-NX#readme).

**I've added a breeding overlay.** That is the reason for this fork. The goal is a small modular set of in-game helpers that make PokeMMO simpler to play on a console.

> Unofficial project, not affiliated with PokeMMO or Nintendo.

## Breeding overlay

Say which Pokémon you want (species, nature, stats at 31) and what you own (Power items, Everstones, Pokémon). It plans every breed and shows you the **next step** over the game. Mark a step done and your stock and items update.

- **Smart plans:** uses your recorded Pokémon and items, and accepts up to 3 extra breeds if that saves buying items.
- **Quick HUD toast:** R + D-pad up + R3 shows the goal, next pair, held items, and remaining step count. It fades out over 10 seconds; movement, gameplay, and the regular R3 keyboard shortcut otherwise continue normally.
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
| A                    | Done (asks first)     |
| Y / X                | Items / all steps     |
| + / −                | Setup / chains        |
| B                    | Back, close           |

### Your data

Your data is plain text on the SD card. **Stock is entered manually** in the overlay; it never reads the game client or automatically discovers in-game Pokémon or items:

- `switch/PokeMMO/breeding.csv`
- `switch/PokeMMO/chains/`

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
