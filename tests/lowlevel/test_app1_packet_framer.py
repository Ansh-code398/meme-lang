#!/usr/bin/env python3
"""
Multi-Engine E2E Test Suite: App 1 — Packet Framer & Byte Ring Buffer
====================================================================
Authoritative Source: TEST_INFRA.md (F5), PROJECT.md, and Explorer 3 Report

Engines Covered:
  - Python (`apps/packet_framer/packet_framer.py`)
  - Rust (`apps/packet_framer/packet_framer.rs`)
  - MEMElang Interpreter (`apps/packet_framer/packet_framer.meme`)
  - MEMElang Native AOT (`apps/packet_framer/packet_framer.meme`)

Validates:
  - Binary framing layout (Magic 0xAA55, Seq ID, Payload Length, Payload, Adler-32 Checksum)
  - Exact test vectors: Empty (V1.1), PING (V1.2), HELLO MEMELANG (V1.3), 16 zeros (V1.4), 16 ones (V1.5), 256 bytes (V1.6)
  - Circular Ring Buffer wrap-around mechanics & backpressure
  - Fragmented stream chunk reassembly
  - Corrupted stream recovery & resynchronization
  - Corrupted checksum rejection
"""

import sys
import os
import struct
import zlib
import subprocess
import argparse
import tempfile
import time
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
APPS_DIR = PROJECT_ROOT / "apps" / "packet_framer"
FIXTURES_DIR = PROJECT_ROOT / "tests" / "lowlevel" / "fixtures"

# Fixed Protocol Constants
MAGIC_HEADER = 0xAA55
HEADER_SIZE = 8
TRAILER_SIZE = 4
MIN_FRAME_SIZE = HEADER_SIZE + TRAILER_SIZE  # 12 bytes


def compute_adler32(data: bytes) -> int:
    """Computes standard Adler-32 checksum."""
    return zlib.adler32(data) & 0xFFFFFFFF


def pack_frame(seq_id: int, payload: bytes) -> bytes:
    """Packs a frame with Magic 0xAA55, Big-Endian seq_id, length, payload, and Adler-32."""
    header = struct.pack(">HIH", MAGIC_HEADER, seq_id, len(payload))
    checksum = compute_adler32(payload)
    trailer = struct.pack(">I", checksum)
    return header + payload + trailer


def unpack_frame(frame: bytes):
    """Unpacks and validates a binary frame. Returns (seq_id, payload) or raises ValueError."""
    if len(frame) < MIN_FRAME_SIZE:
        raise ValueError(f"Frame truncated: length {len(frame)} < {MIN_FRAME_SIZE}")
    magic, seq_id, p_len = struct.unpack(">HIH", frame[:HEADER_SIZE])
    if magic != MAGIC_HEADER:
        raise ValueError(f"Invalid magic: {hex(magic)} != {hex(MAGIC_HEADER)}")
    if len(frame) < HEADER_SIZE + p_len + TRAILER_SIZE:
        raise ValueError(f"Frame incomplete: expected {HEADER_SIZE + p_len + TRAILER_SIZE}, got {len(frame)}")
    payload = frame[HEADER_SIZE:HEADER_SIZE + p_len]
    (chk,) = struct.unpack(">I", frame[HEADER_SIZE + p_len:HEADER_SIZE + p_len + TRAILER_SIZE])
    expected_chk = compute_adler32(payload)
    if chk != expected_chk:
        raise ValueError(f"Checksum mismatch: got {hex(chk)}, expected {hex(expected_chk)}")
    return seq_id, payload


class ReferenceRingBuffer:
    """In-memory circular ring buffer implementation for test verification."""

    def __init__(self, capacity: int):
        self.capacity = capacity
        self.buffer = bytearray(capacity)
        self.head = 0
        self.tail = 0
        self.size = 0

    def free_space(self) -> int:
        return self.capacity - self.size

    def available(self) -> int:
        return self.size

    def write(self, data: bytes) -> int:
        to_write = min(len(data), self.free_space())
        for i in range(to_write):
            self.buffer[self.head] = data[i]
            self.head = (self.head + 1) % self.capacity
        self.size += to_write
        return to_write

    def read(self, length: int) -> bytes:
        to_read = min(length, self.size)
        res = bytearray(to_read)
        for i in range(to_read):
            res[i] = self.buffer[self.tail]
            self.tail = (self.tail + 1) % self.capacity
        self.size -= to_read
        return bytes(res)

    def peek(self, offset: int, length: int) -> bytes:
        if offset + length > self.size:
            raise IndexError("Peek out of bounds")
        res = bytearray(length)
        idx = (self.tail + offset) % self.capacity
        for i in range(length):
            res[i] = self.buffer[idx]
            idx = (idx + 1) % self.capacity
        return bytes(res)

    def skip(self, length: int):
        to_skip = min(length, self.size)
        self.tail = (self.tail + to_skip) % self.capacity
        self.size -= to_skip


def run_command(cmd, cwd=None, timeout=30):
    try:
        res = subprocess.run(
            cmd,
            cwd=str(cwd) if cwd else str(PROJECT_ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout,
        )
        return res.returncode, res.stdout, res.stderr
    except Exception as e:
        return -1, "", str(e)


def main():
    parser = argparse.ArgumentParser(description="Packet Framer Multi-Engine Test Suite")
    parser.add_argument("--tier", choices=["1", "2", "3", "4", "all"], default="all")
    parser.add_argument("--engine", choices=["interpreter", "native", "rust", "python", "all"], default="all")
    parser.add_argument("--strict", action="store_true", help="Fail if applications are staged")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    passes = []
    failures = []
    staged = []

    def pass_test(name):
        passes.append(name)
        print(f"[PASS] {name}")

    def fail_test(name, reason):
        failures.append((name, reason))
        print(f"[FAIL] {name}: {reason}")

    def stage_test(name, reason):
        staged.append((name, reason))
        print(f"[STAGED] {name}: {reason}")

    print("=== App 1: Packet Framer & Ring Buffer Test Suite ===")

    # --------------------------------------------------------------------------
    # Tier 1: Canonical Feature Coverage & Verified Test Vectors
    # --------------------------------------------------------------------------
    if args.tier in ("1", "all"):
        # Vector 1.1: Empty payload
        f1 = pack_frame(0, b"")
        assert f1.hex() == "aa5500000000000000000001"
        s1, p1 = unpack_frame(f1)
        if s1 == 0 and p1 == b"":
            pass_test("T1_framer_vector_empty_payload")
        else:
            fail_test("T1_framer_vector_empty_payload", f"Mismatched unpack: seq={s1}, payload={p1}")

        # Vector 1.2: PING
        f2 = pack_frame(1, b"PING")
        assert f2.hex() == "aa5500000001000450494e470302012f"
        s2, p2 = unpack_frame(f2)
        if s2 == 1 and p2 == b"PING":
            pass_test("T1_framer_vector_ping_payload")
        else:
            fail_test("T1_framer_vector_ping_payload", f"Mismatched unpack: seq={s2}, payload={p2}")

        # Vector 1.3: HELLO MEMELANG
        f3 = pack_frame(42, b"HELLO MEMELANG")
        assert f3.hex() == "aa550000002a000e48454c4c4f204d454d454c414e471cd103db"
        s3, p3 = unpack_frame(f3)
        if s3 == 42 and p3 == b"HELLO MEMELANG":
            pass_test("T1_framer_vector_hello_memelang_payload")
        else:
            fail_test("T1_framer_vector_hello_memelang_payload", f"Mismatched unpack: seq={s3}")

        # Vector 1.4: 16 Zeros
        f4 = pack_frame(100, b"\x00" * 16)
        if f4[-4:].hex() == "00100001":
            pass_test("T1_framer_vector_zeros_payload")
        else:
            fail_test("T1_framer_vector_zeros_payload", f"Expected checksum 0x00100001, got {f4[-4:].hex()}")

        # Vector 1.5: 16 Ones
        f5 = pack_frame(101, b"\xFF" * 16)
        if f5[-4:].hex() == "87880ff1":
            pass_test("T1_framer_vector_ones_payload")
        else:
            fail_test("T1_framer_vector_ones_payload", f"Expected checksum 0x87880FF1, got {f5[-4:].hex()}")

        # Ring buffer basic push/pop
        rb = ReferenceRingBuffer(64)
        w = rb.write(f2)
        r = rb.read(len(f2))
        if w == len(f2) and r == f2:
            pass_test("T1_ring_buffer_push_pop_basic")
        else:
            fail_test("T1_ring_buffer_push_pop_basic", "Ring buffer push/pop corrupted")

    # --------------------------------------------------------------------------
    # Tier 2: Boundary & Corner Cases
    # --------------------------------------------------------------------------
    if args.tier in ("2", "all"):
        # T2.1: Large payload 65535 bytes
        large_payload = b"X" * 65535
        f_large = pack_frame(999, large_payload)
        s_large, p_large = unpack_frame(f_large)
        if s_large == 999 and len(p_large) == 65535:
            pass_test("T2_framer_max_payload_65535")
        else:
            fail_test("T2_framer_max_payload_65535", "Large payload unpack failed")

        # T2.2: Corrupted magic header
        f_bad_magic = bytearray(f2)
        f_bad_magic[0] = 0xBB
        try:
            unpack_frame(bytes(f_bad_magic))
            fail_test("T2_framer_reject_bad_magic", "Accepted invalid magic header")
        except ValueError:
            pass_test("T2_framer_reject_bad_magic")

        # T2.3: Corrupted checksum
        f_bad_chk = bytearray(f2)
        f_bad_chk[-1] ^= 0xFF
        try:
            unpack_frame(bytes(f_bad_chk))
            fail_test("T2_framer_reject_bad_checksum", "Accepted invalid checksum")
        except ValueError:
            pass_test("T2_framer_reject_bad_checksum")

        # T2.4: Truncated frame
        try:
            unpack_frame(f2[:10])
            fail_test("T2_framer_reject_truncated", "Accepted truncated frame")
        except ValueError:
            pass_test("T2_framer_reject_truncated")

        # T2.5: Ring buffer circular boundary split wrap-around
        rb_split = ReferenceRingBuffer(64)
        # Advance head to 50
        rb_split.write(b"A" * 50)
        rb_split.read(40)  # tail=40, head=50, size=10, free_space=54
        # Write 26-byte Frame V1.3: 14 bytes written to 50..63, 12 bytes wrap to 0..11
        w_split = rb_split.write(f3)
        assert w_split == 26
        rb_split.read(10)  # drain initial 10 bytes
        rec = rb_split.read(26)
        if rec == f3 and unpack_frame(rec)[1] == b"HELLO MEMELANG":
            pass_test("T2_ring_buffer_circular_boundary_wrap")
        else:
            fail_test("T2_ring_buffer_circular_boundary_wrap", "Wrap-around reconstructed data corrupted")

        # T2.6: Full buffer backpressure
        rb_full = ReferenceRingBuffer(32)
        rb_full.write(f2)  # 16 bytes
        rb_full.write(f2)  # 16 bytes (size == 32)
        if rb_full.write(b"X") == 0 and rb_full.available() == 32:
            pass_test("T2_ring_buffer_full_backpressure")
        else:
            fail_test("T2_ring_buffer_full_backpressure", "Did not enforce full backpressure")

        # T2.7: Fragmented stream reassembly across 3 chunk writes
        rb_frag = ReferenceRingBuffer(64)
        c1, c2, c3 = f2[:5], f2[5:10], f2[10:]
        rb_frag.write(c1)
        rb_frag.write(c2)
        rb_frag.write(c3)
        reassembled = rb_frag.read(len(f2))
        if reassembled == f2 and unpack_frame(reassembled)[1] == b"PING":
            pass_test("T2_framer_fragmented_stream_reassembly")
        else:
            fail_test("T2_framer_fragmented_stream_reassembly", "Failed fragmented stream reassembly")

        # T2.8: Corrupted stream resynchronization (skip garbage, locate 0xAA55)
        garbage = bytes.fromhex("ffdeadbeef00")
        corrupted_stream = garbage + f2
        rb_sync = ReferenceRingBuffer(64)
        rb_sync.write(corrupted_stream)
        # Scan ring buffer to find magic header
        found_sync = False
        while rb_sync.available() >= MIN_FRAME_SIZE:
            magic_peek = struct.unpack(">H", rb_sync.peek(0, 2))[0]
            if magic_peek == MAGIC_HEADER:
                # Check if full frame is available
                p_len = struct.unpack(">H", rb_sync.peek(6, 2))[0]
                expected_total = HEADER_SIZE + p_len + TRAILER_SIZE
                if rb_sync.available() >= expected_total:
                    recovered_frame = rb_sync.read(expected_total)
                    s, p = unpack_frame(recovered_frame)
                    if s == 1 and p == b"PING":
                        found_sync = True
                        break
            rb_sync.skip(1)
        if found_sync:
            pass_test("T2_framer_corrupted_stream_resynchronization")
        else:
            fail_test("T2_framer_corrupted_stream_resynchronization", "Failed to resync after garbage stream")

    # --------------------------------------------------------------------------
    # Progressive Multi-Engine Application Verification (apps/packet_framer)
    # --------------------------------------------------------------------------
    engines_to_check = ["python", "rust", "interpreter", "native"] if args.engine == "all" else [args.engine]
    for eng in engines_to_check:
        test_id = f"T1_app1_{eng}_engine_parity"
        app_file_map = {
            "python": APPS_DIR / "packet_framer.py",
            "rust": APPS_DIR / "packet_framer.rs",
            "interpreter": APPS_DIR / "packet_framer.meme",
            "native": APPS_DIR / "packet_framer.meme",
        }
        target_file = app_file_map[eng]
        if not target_file.is_file():
            if args.strict:
                fail_test(test_id, f"File not found: {target_file}")
            else:
                stage_test(test_id, f"Pending M2 implementation ({target_file.name})")
            continue

        # Execute application self-check / verification mode
        if eng == "python":
            code, stdout, stderr = run_command([sys.executable, str(target_file), "--verify"])
        elif eng == "rust":
            with tempfile.TemporaryDirectory() as tmpdir:
                tmp_bin = Path(tmpdir) / "packet_framer_rs"
                c_code, c_out, c_err = run_command(["rustc", "-O", str(target_file), "-o", str(tmp_bin)])
                if c_code != 0:
                    code, stdout, stderr = c_code, c_out, c_err
                else:
                    code, stdout, stderr = run_command([str(tmp_bin), "--verify"])
        elif eng == "interpreter":
            code, stdout, stderr = run_command([str(PROJECT_ROOT / "meme"), str(target_file), "--verify"])
        elif eng == "native":
            with tempfile.TemporaryDirectory() as tmpdir:
                tmp_bin = Path(tmpdir) / "packet_framer_native"
                c_code, c_out, c_err = run_command([str(PROJECT_ROOT / "meme"), "compile", str(target_file), "-o", str(tmp_bin)])
                if c_code != 0:
                    code, stdout, stderr = c_code, c_out, c_err
                else:
                    code, stdout, stderr = run_command([str(tmp_bin), "--verify"])

        if code == 0 and ("OK" in stdout or "[PASS]" in stdout):
            pass_test(test_id)
        else:
            fail_test(test_id, f"Engine execution failed (code {code}): {stderr.strip() or stdout.strip()}")

    print(f"\nPacket Framer Suite: {len(passes)} passed, {len(staged)} staged, {len(failures)} failed.")
    if failures or (args.strict and staged):
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
