# PokeMMO-NXpanded

A fork of [PokeMMO-NX](https://github.com/Petit-Prince-dev/PokeMMO-NX) by Petit_Prince, which runs the official PokeMMO client on the Nintendo Switch. The loader is entirely his work (as far as I know, built largely with AI assistance) and this fork is based on his version 1.1.4. For game setup, ROMs and launching, follow the [upstream README](https://github.com/Petit-Prince-dev/PokeMMO-NX#readme).

**I've added a breeding overlay.** That is the reason for this fork. The goal is a small modular set of in-game helpers that make PokeMMO simpler to play on a console. Breeding is the first one, and the code is written with AI assistance (Claude).

Unofficial project, not affiliated with PokeMMO or Nintendo.

## Breeding overlay

Say which Pokémon you want (species, nature, stats at 31) and what you own (Power items, Everstones, Pokémon). It plans every breed and shows you the **next step** over the game. Mark a step done and your stock and items update.

- **Smart plans:** uses what you already own, and accepts up to 3 extra breeds if that saves buying items.
- **Set up on the console:** type the species with the console keyboard (the list narrows as you type); set nature, stats, items and stock with the D-pad.
- **Breeding chains:** put one project aside, start another, and come back later. Nothing is lost.
- **Egg groups** for all 649 species of generations 1 to 5, built in.
- **Safe:** anything destructive asks first, with No selected by default.

| Button | Does |
| --- | --- |
| R + D-pad right + R3 | Open the overlay |
| A | Done (asks first) |
| Y / X | Items / all steps |
| + / − | Setup / chains |
| B | Back, close |

Your data is plain text on the SD card: `switch/PokeMMO/breeding.csv` and `switch/PokeMMO/chains/`.

The overlay only draws its own pixels and reads the controller input the loader already has. It never touches the game client, its memory or network, and never sends input to the game. That is a design rule, not a guarantee.

## Install and build

1. Set up PokeMMO-NX as described upstream.
2. Build and replace `switch/PokeMMO/PokeMMO.nro` (keep a backup of the old one):

```bash
podman run --rm -v "$PWD":/work:Z -w /work/native docker.io/devkitpro/devkita64 make -j2
```

3. Launch in application mode and press R + D-pad right + R3.

## Limits

- Species are generations 1 to 5. Egg groups come from PokeAPI and differ from older data on a few species (Ralts and Trapinch lines).
- The planner is tested against a reference on a PC. The newer screens are tested mostly by playing, so expect rough edges.
- I can't say how PokeMMO's team views third-party tools. Read their rules and use this at your own risk.

## License and credits

MIT, see `LICENSE`. Original copyright: Petit_Prince. Additions in this fork are also MIT. Third-party libraries: `THIRD-PARTY-NOTICES.md`.

Thanks to Petit_Prince for PokeMMO-NX, the PokeMMO team for the game, and [PokeAPI](https://github.com/PokeAPI/pokeapi) for the egg-group data.
