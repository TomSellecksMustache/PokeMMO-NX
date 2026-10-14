# NXpanded 2.0 — Egg routes and offline market prices

NXpanded's existing IV/nature breeding roadmap remains intact. The 2.0 RC adds
**separate, bounded egg-move donor route guidance** and **offline item/breeder
price estimates**. It does not automatically insert donor transfers into the IV
plan, and cannot guarantee the global lowest-cost breeding chain. Do not spend
items before confirming the donor relationship in PokeMMO.

## Egg-move route file

Copy `egg-routes.tsv` to `sdmc:/switch/PokeMMO-NXpanded/` with five tab-separated
columns:

```
# receiver   move   donor   known|inherited   verified
```

- `known`: the donor can obtain the move without another breeding transfer.
- `inherited`: the donor must itself receive the move from a compatible donor.
- Every edge must have the final `verified` field, and be verified against the
  current PokeMMO version. The 649-species egg-group table is used to reject
  incompatible pairs. The planner searches at most five transfer generations,
  rejects cycles, and prefers an owned terminal donor when paths tie.
- Only verified route records are offered as route suggestions. If the path is
  incomplete, the UI says so rather than inventing a parent.
- From **Breeding > Costs**, press **Y** for routes. **Up/Down** selects a move.
- Use `tools/import_verified_egg_paths.py` on Ubuntu to validate the TSV. It can
  optionally validate move names against a locally provided PokeMMO Hub
  `moves.json`, but move-name validation does **not** verify inheritance.

## Offline market file

Copy `market-prices.tsv` to `sdmc:/switch/PokeMMO-NXpanded/`.
Format (tabs, not spaces):

```
#nx-market-v1    My manual GTL observations    1791970000
item    0    10000
breeder    *    1    5000
```

The header contains a source label and Unix UTC capture timestamp. Valid
item indexes are 0–5 (six Power items) and 6 (Everstone). IV masks are bits:
HP=1, Atk=2, Def=4, SpA=8, SpD=16, Speed=32. A listing matching more IVs than
required is permitted. Use `*` for a generic compatible breeder estimate.
Specific-species quotes are stored but **not** applied to generic breeder
leaves: the IV planner does not yet determine their species, gender or nature.
Quotes expire after seven days; stale quotes **never** silently override your
saved manual prices. Missing prices remain unknown, never zero. Quotes are
estimates, not verified current offers; nature, gender and legality must be
checked before purchasing. NXpanded does not connect to GTL or the client.

`tools/import_market_snapshot.py` converts user-observed prices into the
required validated format and timestamps them. Example:

```
python3 tools/import_market_snapshot.py observations.tsv market-prices.tsv \
  --source 'My GTL notes'
```

The **Market** hub tile displays snapshot source, quote count, and freshness.
**Breeding > Costs** displays a price estimate using current saved inventory.
There is no automatic price feed because no approved stable GTL feed has been
verified. Updates can be imported later without changing the `.nro`.

### Data attribution and safety

PokeMMO Hub and [PokeMMO-Tools/pokemmo-data](https://github.com/PokeMMO-Tools/pokemmo-data)
are external references for move names and formats. This distribution does not
bundle their source data, icons, proprietary assets or web app. The reference
maintainers request attribution and contributions. NXpanded does not read
client memory, capture account traffic, or automate GTL interactions.

### Graphics AA status

The menu's 2×/4×/8× request asks EGL for a multisampled default framebuffer.
That does **not** mean the Java game rendered its offscreen scene with MSAA.
NXpanded queries the actual default framebuffer sample count after binding the
GL context and reports it. A lightweight screen-space edge-smoothing fallback
runs at presentation when any AA level is selected; it works regardless of how
the game rendered its own scene. **This is not true 2×/4×/8× MSAA for an
internal game FBO.** Restarting can still allow supported EGL MSAA. Measure
sharpness and framerate on original Switch hardware before deciding quality.
