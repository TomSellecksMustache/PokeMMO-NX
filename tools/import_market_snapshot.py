#!/usr/bin/env python3
"""Validate an OFFLINE price snapshot for NXpanded. No GTL scraping or client access.

Input TSV rows: item<TAB>item_index_0_to_6<TAB>Pokeyen_price
                breeder<TAB>species<TAB>IV_bitmask_0_to_63<TAB>Pokeyen_price
The user's source label and capture time are recorded and the imported
quotes expire in seven days inside NXpanded. Run only with trusted data.
"""
import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path


def make_snapshot(source: Path, output: Path, label: str, timestamp: int) -> int:
    if not (2 <= len(label) <= 47) or any(c in label for c in '\t\n\r'):
        raise ValueError('Source label must be 2-47 printable characters')
    if timestamp < 1500000000:
        raise ValueError('Invalid UTC timestamp')
    records = []
    with source.open(newline='', encoding='utf-8-sig') as f:
        for number, row in enumerate(csv.reader(f, delimiter='\t'), 1):
            if not row or row[0].startswith('#'):
                continue
            if row[0] == 'item' and len(row) == 3:
                ident = int(row[1]); price = int(row[2])
                if not 0 <= ident <= 6:
                    raise ValueError(f'Line {number}: invalid item index')
            elif row[0] == 'breeder' and len(row) == 4:
                if not row[1] or len(row[1]) >= 40 or any(ord(c) < 32 for c in row[1]):
                    raise ValueError(f'Line {number}: invalid species')
                ident = int(row[2]); price = int(row[3])
                if not 0 <= ident < 64:
                    raise ValueError(f'Line {number}: invalid IV bitmask')
            else:
                raise ValueError(f'Line {number}: bad row format')
            if not 1 <= price <= 100_000_000:
                raise ValueError(f'Line {number}: invalid Pokeyen price')
            records.append('\t'.join(row))
    if len(records) > 128:
        raise ValueError('Maximum 128 quotes')
    output.parent.mkdir(parents=True, exist_ok=True)
    body = '#nx-market-v1\t{}\t{}\n'.format(label, timestamp) + '\n'.join(records) + '\n'
    tmp = output.with_name(output.name + '.tmp')
    tmp.write_text(body, encoding='utf-8')
    tmp.replace(output)
    return len(records)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--source', required=True, help='Human-readable source, e.g. My GTL observations')
    parser.add_argument('--as-of-utc', type=int, help='Unix timestamp of observations (default now)')
    args = parser.parse_args()
    timestamp = args.as_of_utc if args.as_of_utc is not None else int(datetime.now(timezone.utc).timestamp())
    count = make_snapshot(args.input, args.output, args.source, timestamp)
    print(f'Wrote {count} OFFLINE quotes to {args.output}; not a live GTL feed.')


if __name__ == '__main__':
    main()
