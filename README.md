[![NXpanded Build](https://github.com/TomSellecksMustache/PokeMMO-NXpanded/actions/workflows/build.yml/badge.svg)](https://github.com/TomSellecksMustache/PokeMMO-NXpanded/actions/workflows/build.yml)

# PokeMMO-NXpanded

![NXpanded running PokeMMO](https://github.com/user-attachments/assets/43bf8dd0-27b0-4c9e-ba1a-d19e941da828)

**PokeMMO on Nintendo Switch, with practical in-game tools built for a controller.**

NXpanded builds on [PokeMMO-NX](https://github.com/Petit-Prince-dev/PokeMMO-NX) by Petit-Prince-dev (originally forked from v1.1.4). It runs the official PokeMMO client and adds a breeding planner, shopping list, quick progress display, and optional graphics filters.

> Unofficial project. Not affiliated with PokeMMO or Nintendo.

## Features

- **Breeding planner:** Choose a species, nature, and target 31 IVs. See the full breeding chain and what to do next.
- **Inventory-aware queue:** Enter the Pokémon and held items you own. Ready-to-complete breeds take priority without breaking dependencies; the planner also looks for opportunities to save items.
- **Shopping list:** See which Power items and Everstones you still need for the current goal, after subtracting recorded stock.
- **Quick progress toast:** Check your goal, next parents, held items, and remaining steps without leaving gameplay. The toast fades after 10 seconds.
- **Saved projects:** Switch between breeding chains and return later. Species and egg-group data cover Generations 1–5 (649 species).
- **Visual Enhancements:** Optional colour grading, vignette, pixel softening, tilt-shift blur, and bloom in a two-tab, controller-only menu. All effects start off.
- **Readable overlays:** Text wraps when possible; long single-line fields scroll to the end and smoothly reverse. Destructive actions require confirmation.

**Inventory is entered manually.** NXpanded does not read Pokémon or items from the running game.

## Controls

| Shortcut | Action |
|---|---|
| **R + D-pad Right + R3** | Breeding planner |
| **R + D-pad Left + R3** | Shopping list |
| **R + D-pad Up + R3** | 10-second progress toast |
| **R + D-pad Down + R3** | Visual Enhancements |

In the breeding planner: **D-pad** navigates, **A** confirms or completes a step, **B** goes back, **Y** shows items, **X** shows all steps, **+** opens setup, and **−** opens saved chains. In Visual Enhancements: **L/R** changes tabs, **D-pad Up/Down** selects, **A** toggles, and **B** closes.

**No touchscreen required.** Launch in Switch application mode.

## Install and game files

Follow the [upstream setup guide](https://github.com/Petit-Prince-dev/PokeMMO-NX#readme) for client setup and game-file requirements. NXpanded uses these SD-card locations:

```text
/switch/PokeMMO-NXpanded/
├── isolate-root/game/
│   ├── roms/               # Required game ROMs
│   └── data/mods/          # PokeMMO mods
├── breeding.csv            # Recorded stock and breeding data
├── chains/                 # Saved breeding projects
└── visual-effects.cfg      # Optional graphics settings
```

If upgrading from an older build, copy any files you want to keep from `/switch/PokeMMO/` to their matching locations under `/switch/PokeMMO-NXpanded/`. Files are **not** migrated automatically.

## Build from source

Install [Docker](https://www.docker.com/) or [Podman](https://podman.io/), then clone this repository:

```bash
git clone https://github.com/TomSellecksMustache/PokeMMO-NXpanded.git
cd PokeMMO-NXpanded
```

Build with devkitPro's container (run from the repository root):

**Docker**

```bash
docker run --rm -v "${PWD}":/work -w /work/native devkitpro/devkita64 make -j2
```

**Podman**

```bash
podman run --rm -v "${PWD}":/work -w /work/native docker.io/devkitpro/devkita64 make -j2
```

The native build outputs `PokeMMO.nro`. To test the portable breeding planner without a Switch:

```bash
./tools/test_breeding.sh
```

GitHub Actions also builds tagged releases; see [Releases](https://github.com/TomSellecksMustache/PokeMMO-NXpanded/releases) for downloadable builds.

## Design and limitations

NXpanded's helpers live in the native Switch layer. They do not modify the PokeMMO Java client, inspect game memory, or communicate with game servers. Graphics effects process the displayed frame, so PokeMMO's own HUD may be filtered too. Their performance has not been fully benchmarked on Switch hardware.

Visual settings are intended to persist on the SD card; some 1.3.0 builds may report a save error, which still needs a verified hardware fix. Species support currently covers Generations 1–5. Check PokeMMO's rules before using third-party tools.

## Credits and license

Based on [PokeMMO-NX](https://github.com/Petit-Prince-dev/PokeMMO-NX) by **Petit-Prince-dev**. Egg-group data: [PokeAPI](https://github.com/PokeAPI/pokeapi). NXpanded development and publishing: **WyzeCorp**.

MIT licensed; see [LICENSE](LICENSE) and [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) for copyright and third-party attribution.
