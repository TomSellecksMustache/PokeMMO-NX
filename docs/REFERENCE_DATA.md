# NXpanded — Offline breeding reference

NXpanded's **Costs** tab is an offline estimate, not a live GTL quote. Enter your
own Power item, Everstone and average external-breeder prices with D-pad
Left/Right. Press X to cycle adjustment steps (100 / 1,000 / 10,000 Pokéyen).
The estimate subtracts your manually recorded item stock, counts unowned
breeders in the current dependency plan, and flags missing prices. Values persist
at `/switch/PokeMMO-NXpanded/breeding-prices.cfg`.

In Costs, press Y for **Egg-move notes**. Optional reference notes are read from:

`/switch/PokeMMO-NXpanded/hub-egg-moves.tsv`

This is an offline TSV with columns `species`, `move`, and `donor`, separated by tabs.
Create your own verified list, for example:

```text
# species<TAB>move<TAB>donor
YOUR_SPECIES<TAB>VALID_MOVE<TAB>VERIFIED_DONOR
```

The project includes `tools/import_hub_reference.py`. It validates **move
names** against the PokeMMO Hub community's `pokemmo-data` `moves.json` catalog
and creates the output TSV. Example, from the repository root:

```bash
python3 tools/import_hub_reference.py my-verified-egg-moves.tsv hub-egg-moves.tsv
```

Then copy `hub-egg-moves.tsv` to the location above. You may supply
`--moves-json moves.json` to use a locally downloaded catalog instead.
**Important:** Valid move names do not prove an egg-move inheritance path or
a compatible donor; verify species-specific routes in the current game. The
planner does not automatically incorporate egg moves into its IV tree.

References: [PokeMMO Hub](https://github.com/PokeMMO-Tools/pokemmo-hub) and
[PokeMMO data](https://github.com/PokeMMO-Tools/pokemmo-data). Their community
maintainers request attribution and contributions of corrected data. No web app
code, price feed, or proprietary game assets are bundled into NXpanded.
