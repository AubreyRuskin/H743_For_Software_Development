#!/usr/bin/env python3
"""Incremental flash: compare old/new images, only flash changed sectors."""

import os
import sys
import shutil
import subprocess
import tempfile
import argparse

SECTOR_SIZE = 4096


def find_changed_sectors(old_data, new_data):
    """Return sorted list of sector indices that differ."""
    n = len(new_data) // SECTOR_SIZE
    changed = []
    for i in range(n):
        s = i * SECTOR_SIZE
        e = s + SECTOR_SIZE
        new_sec = new_data[s:e]
        if e <= len(old_data):
            old_sec = old_data[s:e]
        elif s < len(old_data):
            old_sec = old_data[s:] + b'\xff' * (e - len(old_data))
        else:
            old_sec = b'\xff' * SECTOR_SIZE
        if new_sec != old_sec:
            changed.append(i)
    return changed


def merge_consecutive(sectors):
    """Merge consecutive sector indices into (start_idx, count) tuples."""
    if not sectors:
        return []
    ranges = []
    start = prev = sectors[0]
    for s in sectors[1:]:
        if s == prev + 1:
            prev = s
        else:
            ranges.append((start, prev - start + 1))
            start = prev = s
    ranges.append((start, prev - start + 1))
    return ranges


def main():
    p = argparse.ArgumentParser(description='Incremental W25Q64 flash')
    p.add_argument('image', help='New littlefs image binary')
    p.add_argument('--last', required=True,
                   help='Path to store/read last flashed image snapshot')
    p.add_argument('--base', type=lambda x: int(x, 0), default=0x90000000)
    p.add_argument('--target', default='stm32h743xx')
    p.add_argument('--freq', type=int, default=4000000)
    p.add_argument('--script', required=True, help='pyocd user script path')
    args = p.parse_args()

    with open(args.image, 'rb') as f:
        new_data = f.read()

    if os.path.exists(args.last):
        with open(args.last, 'rb') as f:
            old_data = f.read()
    else:
        # First time: treat as blank (all 0xFF)
        old_data = b'\xff' * len(new_data)

    changed = find_changed_sectors(old_data, new_data)
    if not changed:
        print('Images identical -- nothing to flash.')
        return 0

    ranges = merge_consecutive(changed)
    total_kb = sum(cnt * SECTOR_SIZE for _, cnt in ranges) // 1024
    print(f'{len(changed)} sector(s) changed, '
          f'{len(ranges)} contiguous range(s), {total_kb} KB to flash')

    for idx, (start_sec, count) in enumerate(ranges):
        offset = start_sec * SECTOR_SIZE
        length = count * SECTOR_SIZE
        addr = args.base + offset
        data = new_data[offset:offset + length]

        print(f'  [{idx+1}/{len(ranges)}] 0x{addr:08X} +{length//1024}KB '
              f'({count} sectors)')

        tmp_fd, tmp_path = tempfile.mkstemp(suffix='.bin')
        try:
            os.write(tmp_fd, data)
            os.close(tmp_fd)
            subprocess.check_call([
                'pyocd', 'load',
                '-t', args.target,
                '-f', str(args.freq),
                '-e', 'sector',
                '--script', args.script,
                '--format', 'bin',
                '-a', f'0x{addr:08X}',
                tmp_path,
            ])
        except subprocess.CalledProcessError as e:
            print(f'Flash failed at 0x{addr:08X}: {e}', file=sys.stderr)
            return 1
        finally:
            if os.path.exists(tmp_path):
                os.unlink(tmp_path)

    # Save snapshot for next diff
    shutil.copy2(args.image, args.last)
    print(f'Done. Snapshot saved to {args.last}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
