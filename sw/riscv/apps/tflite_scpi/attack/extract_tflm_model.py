#!/usr/bin/env python3
"""
extract_tflm_model.py – Extract a TensorFlow Lite Micro model from a raw
x_heep_uart_dump.bin memory‑dump (or any dump that embeds a model with the
"TFL3" magic).

The script looks for the ASCII marker "TFL3", then backs up 4 bytes (the
standard start‑of‑model offset used by TFLite‑Micro). By default it copies
from that position to the end of the file, writing the result to a new file
that you can load directly with TFLM.

Usage (simple):
    python extract_tflm_model.py x_heep_uart_dump.bin model.tflite

Advanced options:
    --end-marker END     ASCII string or hex (e.g. 0x0000) that marks the end
                         *exclusive* of the model; copy stops right before it.
    --size SIZE          Explicit number of bytes (decimal or hex, e.g. 0x1A00)
                         to copy. Overrides --end-marker.
                         If neither --size nor --end-marker is given, the
                         script copies until EOF.

Examples:
    # 1) Fast & loose – just grab from 4 bytes before "TFL3" to EOF
    python extract_tflm_model.py dump.bin model.tflite

    # 2) Stop at the sentinel string "ENDM"
    python extract_tflm_model.py dump.bin model.tflite --end-marker ENDM

    # 3) Copy exactly 0x1A00 bytes (hex) after the offset
    python extract_tflm_model.py dump.bin model.tflite --size 0x1A00
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Optional

MAGIC = b"TFL3"
OFFSET_BEFORE_MAGIC = 4  # bytes


def parse_size(text: str) -> int:
    """Parse SIZE parameter (decimal or 0xHEX)."""
    text = text.strip()
    if text.lower().startswith("0x"):
        return int(text, 16)
    return int(text)


def bytes_from_text(text: str) -> bytes:
    """Convert ASCII or hex string (e.g. '0x00ABCD') to bytes."""
    text = text.strip()
    if text.lower().startswith("0x"):
        # Remove 0x and possible underscores, then pad to even length
        hexstr = text[2:].replace("_", "")
        if len(hexstr) % 2:
            hexstr = "0" + hexstr
        return bytes.fromhex(hexstr)
    return text.encode()


def extract_model(
    infile: Path,
    outfile: Path,
    *,
    size: Optional[int] = None,
    end_marker: Optional[bytes] = None,
) -> None:
    """Core extraction routine."""

    data = infile.read_bytes()
    idx_magic = data.find(MAGIC)
    if idx_magic == -1:
        raise ValueError(f"Magic marker {MAGIC!r} not found in {infile}")

    start = max(0, idx_magic - OFFSET_BEFORE_MAGIC)

    if size is not None:
        end = start + size
    elif end_marker is not None:
        idx_end = data.find(end_marker, start + len(MAGIC))
        if idx_end == -1:
            raise ValueError(
                f"End marker {end_marker!r} not found after offset {start} in {infile}"
            )
        end = idx_end
    else:
        end = len(data)

    if end <= start:
        raise ValueError("Computed end offset precedes start offset. Check parameters.")

    outfile.write_bytes(data[start:end])
    size_extracted = end - start
    print(
        f"[+] Extracted {size_extracted} bytes from offset {start} to {end} "
        f"into {outfile}"
    )


def build_argparser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Extract TFLM model from memory dump")
    p.add_argument("input", type=Path, help="Path to x_heep_uart_dump.bin (input)")
    p.add_argument(
        "output",
        type=Path,
        nargs="?",
        help="Path for extracted model (default: <input>_model.tflite)",
    )
    p.add_argument("--size", type=str, help="Bytes to copy (decimal or 0xHEX)")
    p.add_argument("--end-marker", type=str, help="ASCII or hex marker delimiting end")
    return p


def main(argv: list[str] | None = None) -> None:
    args = build_argparser().parse_args(argv)

    infile: Path = args.input
    outfile: Path = args.output or infile.with_suffix("_model.tflite")

    size: Optional[int] = parse_size(args.size) if args.size else None
    end_marker: Optional[bytes] = bytes_from_text(args.end_marker) if args.end_marker else None

    try:
        extract_model(infile, outfile, size=size, end_marker=end_marker)
    except Exception as e:
        print(f"[!] Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
