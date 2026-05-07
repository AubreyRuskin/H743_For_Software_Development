#!/usr/bin/env python3
"""Read external flash with pyOCD in chunks and show progress."""

import argparse
import os
import subprocess
import sys
import tempfile
import time


def int_base_0(value):
    return int(value, 0)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Read a pyOCD flash region to a binary file with progress."
    )
    parser.add_argument("output", help="Output binary path")
    parser.add_argument("--base", type=int_base_0, default=0x90000000)
    parser.add_argument("--size", type=int_base_0, required=True)
    parser.add_argument("--chunk", type=int_base_0, default=0x40000)
    parser.add_argument("--target", default="stm32h743xx")
    parser.add_argument("--freq", type=int_base_0, default=4000000)
    parser.add_argument("--script", required=True, help="pyOCD user script path")
    parser.add_argument("--pyocd", default="pyocd", help="pyOCD executable")
    return parser.parse_args()


def format_bytes(value):
    units = ["B", "KiB", "MiB", "GiB"]
    amount = float(value)
    for unit in units:
        if amount < 1024.0 or unit == units[-1]:
            return f"{amount:.1f} {unit}"
        amount /= 1024.0


def print_progress(done, total, start_time):
    width = 36
    ratio = 1.0 if total == 0 else min(done / total, 1.0)
    filled = int(width * ratio)
    bar = "#" * filled + "-" * (width - filled)

    elapsed = max(time.monotonic() - start_time, 0.001)
    rate = done / elapsed
    remaining = max(total - done, 0)
    eta = int(remaining / rate) if rate > 0 else 0

    sys.stderr.write(
        "\r"
        f"Reading [{bar}] {ratio * 100:6.2f}% "
        f"{format_bytes(done)}/{format_bytes(total)} "
        f"{format_bytes(rate)}/s ETA {eta}s"
    )
    sys.stderr.flush()


def run_pyocd_savemem(args, address, size, chunk_path):
    cmd = [
        args.pyocd,
        "commander",
        "-q",
        "-t",
        args.target,
        "-f",
        str(args.freq),
        "--script",
        args.script,
        "-c",
        f"savemem 0x{address:08X} {size} {chunk_path}",
    ]
    result = subprocess.run(cmd, text=True, capture_output=True)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip() or "pyOCD failed")

    actual_size = os.path.getsize(chunk_path) if os.path.exists(chunk_path) else 0
    if actual_size != size:
        detail = (result.stderr.strip() or result.stdout.strip()).strip()
        if detail:
            detail = f": {detail}"
        raise RuntimeError(
            f"pyOCD readback chunk incomplete at 0x{address:08X}: "
            f"{actual_size}/{size} bytes{detail}"
        )


def main():
    args = parse_args()
    if args.size <= 0:
        raise SystemExit("Error: --size must be positive")
    if args.chunk <= 0:
        raise SystemExit("Error: --chunk must be positive")

    output = os.path.abspath(args.output)
    tmp_output = output + ".tmp"
    os.makedirs(os.path.dirname(output), exist_ok=True)

    if os.path.exists(tmp_output):
        os.unlink(tmp_output)

    done = 0
    start_time = time.monotonic()
    print_progress(done, args.size, start_time)

    try:
        with open(tmp_output, "wb") as out:
            while done < args.size:
                chunk_size = min(args.chunk, args.size - done)
                fd, chunk_path = tempfile.mkstemp(prefix="h743-flash-", suffix=".bin")
                os.close(fd)
                try:
                    run_pyocd_savemem(args, args.base + done, chunk_size, chunk_path)
                    with open(chunk_path, "rb") as chunk_file:
                        out.write(chunk_file.read())
                finally:
                    if os.path.exists(chunk_path):
                        os.unlink(chunk_path)

                done += chunk_size
                print_progress(done, args.size, start_time)

        sys.stderr.write("\n")
        os.replace(tmp_output, output)
        print(f"Saved {args.size} bytes to {output}")
        return 0
    except Exception as exc:
        sys.stderr.write("\n")
        if os.path.exists(tmp_output):
            os.unlink(tmp_output)
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
