#!/usr/bin/env python3
"""Validate a player-curated, VERIFIED egg-move graph, without guessing donors.

Columns (TSV): receiver<TAB>move<TAB>donor<TAB>known|inherited<TAB>verified
'known' means the donor already learns the move without further breeding;
'inherited' means the donor must itself inherit that move. Every row needs
independent confirmation against current PokeMMO breeding mechanics.
"""
import argparse
import csv
from pathlib import Path


def convert(source: Path, dest: Path, move_names=None):
    rows = ['# NXpanded verified egg-move routes (player supplied; attribution retained)']
    with source.open(newline='', encoding='utf-8-sig') as file:
        for line, row in enumerate(csv.reader(file, delimiter='\t'), 1):
            if not row or row[0].startswith('#'):
                continue
            if len(row) != 5:
                raise ValueError(f'Line {line}: expected five TSV fields')
            child, move, donor, method, verified = row
            if any(not item or len(item) >= limit or any(ord(c) < 32 for c in item)
                   for item, limit in zip((child, move, donor), (40, 52, 40))):
                raise ValueError(f'Line {line}: invalid text')
            if method not in ('known', 'inherited') or verified != 'verified':
                raise ValueError(f'Line {line}: record is not explicitly verified')
            if move_names is not None and move.casefold() not in move_names:
                raise ValueError(f'Line {line}: move absent from supplied catalog: {move}')
            rows.append('\t'.join(row))
    if len(rows) > 129:
        raise ValueError('Maximum 128 edges')
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text('\n'.join(rows) + '\n', encoding='utf-8')
    return len(rows) - 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--moves-json', type=Path, help='Optional local PokeMMO Hub catalog; only validates names')
    args = parser.parse_args()
    moves = None
    if args.moves_json:
        import json
        data = json.loads(args.moves_json.read_text(encoding='utf-8'))
        moves = {item['name'].casefold() for item in data if isinstance(item, dict) and isinstance(item.get('name'), str)}
    count = convert(args.input, args.output, moves)
    print(f'Wrote {count} curated egg-move edges. On-device validation also checks egg-group compatibility.')
    print('Do not treat this validation as independent proof that a donor learns the move.')


if __name__ == '__main__':
    main()
