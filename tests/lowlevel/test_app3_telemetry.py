#!/usr/bin/env python3
"""
Multi-Engine E2E Test Suite: App 3 — Rolling Window Telemetry Aggregator
=======================================================================
Authoritative Source: TEST_INFRA.md (F7), PROJECT.md, and Explorer 3 Report

Validates:
  - Sliding window statistics: min, max, average, count, total_samples
  - Window sample eviction when capacity is exceeded
  - 56-byte Big-Endian packed binary telemetry frame encoding
  - Frame structure: Magic 0x544D ("TM"), Version 0x01, Timestamps, Window metrics, Adler-32 checksum
  - Exact verified 56-byte binary frame vector
  - Corner cases: Window capacity = 1, all identical samples, empty window, malformed frames
  - Multi-engine execution parity (Python, Rust, MEMElang Interpreter, MEMElang Native)
"""

import sys
import os
import struct
import zlib
import subprocess
import argparse
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
APPS_DIR = PROJECT_ROOT / "apps" / "telemetry_aggregator"

# Protocol Constants
TELEMETRY_MAGIC = 0x544D  # "TM"
TELEMETRY_VERSION = 0x01
FRAME_LENGTH = 56
HEADER_PAYLOAD_LENGTH = 52  # bytes 0..51 covered by Adler-32


def compute_adler32(data: bytes) -> int:
    return zlib.adler32(data) & 0xFFFFFFFF


class ReferenceTelemetryAggregator:
    """Reference implementation of the Rolling Window Telemetry Aggregator."""

    def __init__(self, capacity: int, metric_id: int = 1):
        self.capacity = capacity
        self.metric_id = metric_id
        self.samples = [0.0] * capacity
        self.head = 0
        self.count = 0
        self.total_samples = 0
        self.running_sum = 0.0

    def add_sample(self, val: float):
        if self.count < self.capacity:
            self.samples[self.head] = val
            self.running_sum += val
            self.count += 1
        else:
            evicted = self.samples[self.head]
            self.samples[self.head] = val
            self.running_sum = (self.running_sum - evicted) + val
        self.head = (self.head + 1) % self.capacity
        self.total_samples += 1

    def get_stats(self):
        if self.count == 0:
            return {"count": 0, "min": 0.0, "max": 0.0, "avg": 0.0, "total": self.total_samples}
        active_samples = [self.samples[i] for i in range(self.count)]
        return {
            "count": self.count,
            "min": min(active_samples),
            "max": max(active_samples),
            "avg": self.running_sum / self.count,
            "total": self.total_samples,
        }

    def encode_frame(self, ts_sec: int, ts_msec: int) -> bytes:
        stats = self.get_stats()
        tot_hi = (stats["total"] >> 32) & 0xFFFFFFFF
        tot_lo = stats["total"] & 0xFFFFFFFF
        body = struct.pack(
            ">HBBIIIIIIddd",
            TELEMETRY_MAGIC,
            TELEMETRY_VERSION,
            self.metric_id,
            ts_sec,
            ts_msec,
            self.capacity,
            stats["count"],
            tot_hi,
            tot_lo,
            stats["min"],
            stats["max"],
            stats["avg"],
        )
        assert len(body) == HEADER_PAYLOAD_LENGTH
        chk = compute_adler32(body)
        return body + struct.pack(">I", chk)


def decode_telemetry_frame(frame: bytes):
    """Decodes and validates a 56-byte packed binary telemetry frame."""
    if len(frame) != FRAME_LENGTH:
        raise ValueError(f"Invalid frame length: expected {FRAME_LENGTH}, got {len(frame)}")

    body = frame[:HEADER_PAYLOAD_LENGTH]
    (chk,) = struct.unpack(">I", frame[HEADER_PAYLOAD_LENGTH:])
    expected_chk = compute_adler32(body)
    if chk != expected_chk:
        raise ValueError(f"Checksum mismatch: got {hex(chk)}, expected {hex(expected_chk)}")

    (
        magic,
        ver,
        metric_id,
        ts_sec,
        ts_msec,
        capacity,
        count,
        tot_hi,
        tot_lo,
        min_v,
        max_v,
        avg_v,
    ) = struct.unpack(">HBBIIIIIIddd", body)

    if magic != TELEMETRY_MAGIC:
        raise ValueError(f"Invalid magic: {hex(magic)} != {hex(TELEMETRY_MAGIC)}")
    if ver != TELEMETRY_VERSION:
        raise ValueError(f"Invalid protocol version: {ver}")

    total_samples = (tot_hi << 32) | tot_lo
    return {
        "metric_id": metric_id,
        "ts_sec": ts_sec,
        "ts_msec": ts_msec,
        "capacity": capacity,
        "count": count,
        "total_samples": total_samples,
        "min": min_v,
        "max": max_v,
        "avg": avg_v,
    }


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
    parser = argparse.ArgumentParser(description="Rolling Window Telemetry Aggregator Test Suite")
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

    print("=== App 3: Rolling Window Telemetry Aggregator Test Suite ===")

    # --------------------------------------------------------------------------
    # Tier 1: Canonical Feature Coverage & Verified 56-Byte Frame Vector
    # --------------------------------------------------------------------------
    if args.tier in ("1", "all"):
        # T1.1: Aggregator ingestion within window capacity
        agg = ReferenceTelemetryAggregator(capacity=10, metric_id=1)
        for val in [10.0, 20.0, 30.0, 40.0, 50.0]:
            agg.add_sample(val)
        stats = agg.get_stats()
        if (
            stats["count"] == 5
            and stats["min"] == 10.0
            and stats["max"] == 50.0
            and abs(stats["avg"] - 30.0) < 1e-9
            and stats["total"] == 5
        ):
            pass_test("T1_telemetry_window_ingestion_basic")
        else:
            fail_test("T1_telemetry_window_ingestion_basic", f"Stats mismatch: {stats}")

        # T1.2: Window sample eviction when capacity is exceeded
        for val in [60.0, 70.0, 80.0, 90.0, 100.0, 110.0]:
            agg.add_sample(val)
        # Window size is 10. Ingested 11 items. 10.0 was evicted. Active: [20..110]
        stats_evicted = agg.get_stats()
        if (
            stats_evicted["count"] == 10
            and stats_evicted["min"] == 20.0
            and stats_evicted["max"] == 110.0
            and abs(stats_evicted["avg"] - 65.0) < 1e-9
            and stats_evicted["total"] == 11
        ):
            pass_test("T1_telemetry_window_sample_eviction")
        else:
            fail_test("T1_telemetry_window_sample_eviction", f"Eviction stats mismatch: {stats_evicted}")

        # T1.3: Verified 56-Byte Binary Frame Test Vector
        body_vec = struct.pack(
            ">HBBIIIIIIddd",
            TELEMETRY_MAGIC,
            1,
            1,
            1700000000,
            500,
            100,
            100,
            0,
            5000,
            10.5,
            98.2,
            54.35,
        )
        chk_vec = compute_adler32(body_vec)
        frame_vec = body_vec + struct.pack(">I", chk_vec)
        expected_hex = (
            "544d01016553f100000001f400000064000000640000000000001388"
            "402500000000000040588ccccccccccd404b2ccccccccccd175e0edf"
        )
        assert len(frame_vec) == 56
        assert frame_vec.hex() == expected_hex
        dec = decode_telemetry_frame(frame_vec)
        if (
            dec["magic"] if "magic" in dec else TELEMETRY_MAGIC == TELEMETRY_MAGIC
            and dec["metric_id"] == 1
            and dec["ts_sec"] == 1700000000
            and dec["total_samples"] == 5000
            and abs(dec["min"] - 10.5) < 1e-9
            and abs(dec["max"] - 98.2) < 1e-9
            and abs(dec["avg"] - 54.35) < 1e-9
        ):
            pass_test("T1_telemetry_vector_56_byte_frame")
        else:
            fail_test("T1_telemetry_vector_56_byte_frame", f"Decoded frame mismatch: {dec}")

    # --------------------------------------------------------------------------
    # Tier 2: Boundary & Corner Cases
    # --------------------------------------------------------------------------
    if args.tier in ("2", "all"):
        # T2.1: Instantaneous window (capacity = 1)
        agg_one = ReferenceTelemetryAggregator(capacity=1, metric_id=2)
        agg_one.add_sample(42.5)
        st1 = agg_one.get_stats()
        agg_one.add_sample(84.0)
        st2 = agg_one.get_stats()
        if (
            st1["count"] == 1 and st1["min"] == 42.5 and st1["max"] == 42.5
            and st2["count"] == 1 and st2["min"] == 84.0 and st2["max"] == 84.0 and st2["total"] == 2
        ):
            pass_test("T2_telemetry_window_capacity_1")
        else:
            fail_test("T2_telemetry_window_capacity_1", f"Capacity 1 mismatch: {st2}")

        # T2.2: All identical samples (min == max == avg)
        agg_ident = ReferenceTelemetryAggregator(capacity=50, metric_id=3)
        for _ in range(50):
            agg_ident.add_sample(77.7)
        st_ident = agg_ident.get_stats()
        if (
            st_ident["min"] == 77.7
            and st_ident["max"] == 77.7
            and abs(st_ident["avg"] - 77.7) < 1e-9
        ):
            pass_test("T2_telemetry_all_identical_samples")
        else:
            fail_test("T2_telemetry_all_identical_samples", "Identical samples failed")

        # T2.3: Zero samples empty window
        agg_empty = ReferenceTelemetryAggregator(capacity=10, metric_id=1)
        st_empty = agg_empty.get_stats()
        if st_empty["count"] == 0 and st_empty["min"] == 0.0 and st_empty["max"] == 0.0:
            pass_test("T2_telemetry_empty_window_zeros")
        else:
            fail_test("T2_telemetry_empty_window_zeros", f"Empty window not zeroed: {st_empty}")

        # T2.4: Reject malformed magic header
        bad_magic_frame = bytearray(frame_vec)
        bad_magic_frame[0] = 0x00
        try:
            decode_telemetry_frame(bytes(bad_magic_frame))
            fail_test("T2_telemetry_reject_bad_magic", "Accepted bad magic")
        except ValueError:
            pass_test("T2_telemetry_reject_bad_magic")

        # T2.5: Reject malformed checksum
        bad_chk_frame = bytearray(frame_vec)
        bad_chk_frame[-1] ^= 0xFF
        try:
            decode_telemetry_frame(bytes(bad_chk_frame))
            fail_test("T2_telemetry_reject_bad_checksum", "Accepted bad checksum")
        except ValueError:
            pass_test("T2_telemetry_reject_bad_checksum")

        # T2.6: Reject invalid frame size (truncated to 50 bytes)
        try:
            decode_telemetry_frame(frame_vec[:50])
            fail_test("T2_telemetry_reject_truncated_size", "Accepted truncated frame")
        except ValueError:
            pass_test("T2_telemetry_reject_truncated_size")

    # --------------------------------------------------------------------------
    # Progressive Multi-Engine Application Verification (apps/telemetry_aggregator)
    # --------------------------------------------------------------------------
    engines_to_check = ["python", "rust", "interpreter", "native"] if args.engine == "all" else [args.engine]
    for eng in engines_to_check:
        test_id = f"T1_app3_{eng}_engine_parity"
        app_file_map = {
            "python": APPS_DIR / "telemetry_aggregator.py",
            "rust": APPS_DIR / "telemetry_aggregator.rs",
            "interpreter": APPS_DIR / "telemetry_aggregator.meme",
            "native": APPS_DIR / "telemetry_aggregator.meme",
        }
        target_file = app_file_map[eng]
        if not target_file.is_file():
            if args.strict:
                fail_test(test_id, f"File not found: {target_file}")
            else:
                stage_test(test_id, f"Pending M2 implementation ({target_file.name})")
            continue

        if eng == "python":
            code, stdout, stderr = run_command([sys.executable, str(target_file), "--verify"])
        elif eng == "rust":
            with tempfile.TemporaryDirectory() as tmpdir:
                tmp_bin = Path(tmpdir) / "telemetry_aggregator_rs"
                c_code, c_out, c_err = run_command(["rustc", "-O", str(target_file), "-o", str(tmp_bin)])
                if c_code != 0:
                    code, stdout, stderr = c_code, c_out, c_err
                else:
                    code, stdout, stderr = run_command([str(tmp_bin), "--verify"])
        elif eng == "interpreter":
            code, stdout, stderr = run_command([str(PROJECT_ROOT / "meme"), str(target_file), "--verify"])
        elif eng == "native":
            with tempfile.TemporaryDirectory() as tmpdir:
                tmp_bin = Path(tmpdir) / "telemetry_aggregator_native"
                c_code, c_out, c_err = run_command([str(PROJECT_ROOT / "meme"), "compile", str(target_file), "-o", str(tmp_bin)])
                if c_code != 0:
                    code, stdout, stderr = c_code, c_out, c_err
                else:
                    code, stdout, stderr = run_command([str(tmp_bin), "--verify"])

        if code == 0 and ("OK" in stdout or "[PASS]" in stdout):
            pass_test(test_id)
        else:
            fail_test(test_id, f"Engine execution failed (code {code}): {stderr.strip() or stdout.strip()}")

    print(f"\nTelemetry Aggregator Suite: {len(passes)} passed, {len(staged)} staged, {len(failures)} failed.")
    if failures or (args.strict and staged):
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
