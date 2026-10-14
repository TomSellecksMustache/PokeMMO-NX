#!/usr/bin/env python3
"""Validate player-supplied egg-move pairs against PokeMMO Hub's moves.json.

The community source provides move NAMES, not verified parent-to-child routes.
We never infer that a donor is legal from a move-name match. The user must verify
species inheritance and donor eligibility before using the offline notes.

Input TSV: species TAB egg_move TAB donor_species
Output: NxHub reference TSV, for /switch/PokeMMO-NXpanded/hub-egg-moves.tsv
"""
import argparse
import csv
import json
import pathlib
import urllib.request

URL = "https://raw.githubusercontent.com/PokeMMO-Tools/pokemmo-data/main/data/moves.json"


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('input',type=pathlib.Path,help='Curated egg move relationships, TSV')
    ap.add_argument('output',type=pathlib.Path,help='Validated offline NXpanded reference TSV')
    ap.add_argument('--moves-json',type=pathlib.Path,help='Locally downloaded PokeMMO Hub moves.json (offline use)')
    args=ap.parse_args()
    if args.moves_json:
        catalog=json.loads(args.moves_json.read_text(encoding='utf-8'))
    else:
        with urllib.request.urlopen(URL,timeout=20) as response:
            catalog=json.load(response)
    allowed={entry['name'].casefold() for entry in catalog if isinstance(entry,dict) and isinstance(entry.get('name'),str)}
    lines=['# NXpanded offline egg-move notes (user verified); names validated against PokeMMO-Tools/pokemmo-data moves.json']
    with args.input.open(newline='',encoding='utf-8') as f:
        for row in csv.reader(f,delimiter='\t'):
            if not row or row[0].startswith('#'): continue
            if len(row)!=3 or any(not r.strip() or len(r)>50 for r in row):
                raise ValueError(f'Invalid species/move/donor row: {row!r}')
            species,move,donor=[r.strip() for r in row]
            if move.casefold() not in allowed:
                raise ValueError(f'Move not found in PokeMMO Hub reference catalog: {move}')
            lines.append('\t'.join((species,move,donor)))
    args.output.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(f'Validated {len(lines)-1} player-curated notes against PokeMMO Hub move names.')
    print('Donor compatibility is NOT automatically verified.')

if __name__=='__main__': main()
