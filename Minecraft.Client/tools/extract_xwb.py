#!/usr/bin/env python3
"""Extract all sound entries from XWB wave banks using vgmstream-cli.

Extracts sounds from resident.xwb and additional.xwb into a flat sounds/ directory
with the original XWB entry names (e.g., mob_cow1.wav, step_stone2.wav).

The AppleAudio runtime converts SoundEngine paths (mob/cow) to XWB names (mob_cow)
and picks random numbered variants.
"""

import struct
import subprocess
import os
import sys

VGMSTREAM = "vgmstream-cli"
MCE_CLIENT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOUNDS_DIR = os.path.join(MCE_CLIENT, "sounds")


def parse_xwb(path):
    """Parse XWB header, return list of entry names."""
    with open(path, "rb") as f:
        magic = f.read(4)
        if magic != b"DNBW":
            raise ValueError(f"Not an XWB file (magic={magic})")

        version, hdr_ver = struct.unpack(">II", f.read(8))
        segs = []
        for _ in range(5):
            off, length = struct.unpack(">II", f.read(8))
            segs.append((off, length))

        # Bank data
        f.seek(segs[0][0])
        flags, entry_count = struct.unpack(">II", f.read(8))
        bank_name = f.read(64).split(b"\x00")[0].decode("ascii")

        # Entry names (segment 3)
        names = []
        if segs[3][1] > 0:
            f.seek(segs[3][0])
            for _ in range(entry_count):
                raw = f.read(64)
                name = raw.split(b"\x00")[0].decode("ascii", errors="replace")
                names.append(name)
        else:
            # No names - generate sequential names
            for i in range(entry_count):
                names.append(f"{bank_name}_{i:03d}")

        return bank_name, names


def extract_bank(xwb_path, output_dir, names):
    """Extract all entries from an XWB using vgmstream-cli."""
    os.makedirs(output_dir, exist_ok=True)

    total = len(names)
    extracted = 0
    skipped = 0

    for i, name in enumerate(names):
        # Sanitize filename (replace spaces with underscores for filesystem safety,
        # but keep original for the index)
        safe_name = name.replace(" ", "_")
        out_path = os.path.join(output_dir, f"{safe_name}.wav")

        if os.path.exists(out_path):
            skipped += 1
            continue

        # vgmstream uses 1-based subsong indexing
        subsong = i + 1
        try:
            result = subprocess.run(
                [VGMSTREAM, "-s", str(subsong), "-o", out_path, xwb_path],
                capture_output=True,
                text=True,
                timeout=30,
            )
            if result.returncode == 0:
                extracted += 1
                # Print progress every 20 entries
                if extracted % 20 == 0:
                    print(f"  [{extracted}/{total}] Extracted {safe_name}.wav")
            else:
                print(f"  WARN: Failed to extract {name} (subsong {subsong}): {result.stderr.strip()}")
        except subprocess.TimeoutExpired:
            print(f"  WARN: Timeout extracting {name}")
        except FileNotFoundError:
            print(f"ERROR: {VGMSTREAM} not found. Install with: brew install vgmstream")
            sys.exit(1)

    return extracted, skipped


def main():
    resident_xwb = os.path.join(MCE_CLIENT, "Common", "res", "audio", "resident.xwb")
    additional_xwb = os.path.join(MCE_CLIENT, "Common", "res", "TitleUpdate", "audio", "additional.xwb")

    print(f"Output directory: {SOUNDS_DIR}")
    print()

    # Extract resident.xwb
    if os.path.exists(resident_xwb):
        bank_name, names = parse_xwb(resident_xwb)
        print(f"=== {bank_name}.xwb: {len(names)} entries ===")
        extracted, skipped = extract_bank(resident_xwb, SOUNDS_DIR, names)
        print(f"  Done: {extracted} extracted, {skipped} skipped (already exist)")
    else:
        print(f"WARNING: {resident_xwb} not found")

    print()

    # Extract additional.xwb (TU14+ sounds)
    if os.path.exists(additional_xwb):
        bank_name, names = parse_xwb(additional_xwb)
        print(f"=== {bank_name}.xwb: {len(names)} entries ===")
        extracted, skipped = extract_bank(additional_xwb, SOUNDS_DIR, names)
        print(f"  Done: {extracted} extracted, {skipped} skipped (already exist)")
    else:
        print(f"WARNING: {additional_xwb} not found")

    # Print summary
    wav_count = len([f for f in os.listdir(SOUNDS_DIR) if f.endswith(".wav")])
    print(f"\nTotal WAV files in sounds/: {wav_count}")


if __name__ == "__main__":
    main()
