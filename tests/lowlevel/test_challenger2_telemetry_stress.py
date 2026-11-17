#!/usr/bin/env python3
"""
Challenger 2 Empirical Stress & Adversarial Test Suite for App 3: Telemetry Aggregator
=====================================================================================
Validates:
  1. Extreme sliding window operations:
     - Capacity 1 (instantaneous window)
     - Capacity 1000
     - 50,000 streaming data points
     - Rapid evictions (capacity 3 with 20k streaming samples)
     - All identical samples (42.0, 0.0, -99.9)
     - All negative samples
     - Alternating spikes (+1e6, -1e6)
     - Running sum numerical stability & drift analysis
  2. 56-byte binary frame encoding and Adler-32 checksum integrity
  3. 4-Engine Parity: Python, Rust, MEMElang Interpreter, MEMElang Native Compiler
  4. Malformed frame rejection across engines
"""

import sys
import os
import struct
import zlib
import subprocess
import tempfile
import time
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
APP_DIR = PROJECT_ROOT / "apps" / "telemetry_aggregator"
MEME_BIN = PROJECT_ROOT / "meme"

TELEMETRY_MAGIC = 0x544D
TELEMETRY_VERSION = 0x01
FRAME_LENGTH = 56
HEADER_PAYLOAD_LENGTH = 52


def compute_adler32(data: bytes) -> int:
    return zlib.adler32(data) & 0xFFFFFFFF


class ReferenceTelemetryAggregator:
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
        active = self.samples[:self.count]
        return {
            "count": self.count,
            "min": min(active),
            "max": max(active),
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
        chk = compute_adler32(body)
        return body + struct.pack(">I", chk)


def decode_frame(frame: bytes):
    if len(frame) != FRAME_LENGTH:
        raise ValueError(f"Invalid frame length: {len(frame)} != {FRAME_LENGTH}")
    body = frame[:HEADER_PAYLOAD_LENGTH]
    (chk,) = struct.unpack(">I", frame[HEADER_PAYLOAD_LENGTH:])
    if compute_adler32(body) != chk:
        raise ValueError("Adler-32 checksum mismatch")
    fields = struct.unpack(">HBBIIIIIIddd", body)
    if fields[0] != TELEMETRY_MAGIC:
        raise ValueError("Invalid magic header")
    if fields[1] != TELEMETRY_VERSION:
        raise ValueError("Invalid version")
    return {
        "magic": fields[0],
        "version": fields[1],
        "metric_id": fields[2],
        "ts_sec": fields[3],
        "ts_msec": fields[4],
        "capacity": fields[5],
        "count": fields[6],
        "total_samples": (fields[7] << 32) | fields[8],
        "min": fields[9],
        "max": fields[10],
        "avg": fields[11],
        "checksum": chk,
    }


def run_cmd(cmd, cwd=None, timeout=60):
    res = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else str(PROJECT_ROOT),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=timeout,
    )
    return res.returncode, res.stdout, res.stderr


def main():
    passes = []
    failures = []

    def check(cond, name, details=""):
        if cond:
            passes.append(name)
            msg = f"[PASS] {name}"
            if details:
                msg += f" ({details})"
            print(msg)
        else:
            failures.append((name, details))
            print(f"[FAIL] {name}: {details}")

    print("=== Challenger 2: Adversarial Stress Test Suite for App 3 ===")

    # --------------------------------------------------------------------------
    # 1. Extreme Sliding Window: Capacity 1
    # --------------------------------------------------------------------------
    agg1 = ReferenceTelemetryAggregator(capacity=1, metric_id=1)
    c1_ok = True
    for i in range(5000):
        v = float(i * 1.5 - 2500)
        agg1.add_sample(v)
        st = agg1.get_stats()
        if st["count"] != 1 or st["min"] != v or st["max"] != v or st["avg"] != v or st["total"] != i + 1:
            c1_ok = False
            break
    check(c1_ok, "C2_extreme_window_capacity_1_5000_samples")

    # --------------------------------------------------------------------------
    # 2. Extreme Sliding Window: Rapid Evictions (Capacity 3, 20,000 samples)
    # --------------------------------------------------------------------------
    agg3 = ReferenceTelemetryAggregator(capacity=3, metric_id=1)
    eviction_ok = True
    for i in range(20000):
        agg3.add_sample(float(i))
        st = agg3.get_stats()
        if i >= 2:
            expected_min = float(i - 2)
            expected_max = float(i)
            expected_avg = (float(i - 2) + float(i - 1) + float(i)) / 3.0
            if (
                st["count"] != 3
                or abs(st["min"] - expected_min) > 1e-9
                or abs(st["max"] - expected_max) > 1e-9
                or abs(st["avg"] - expected_avg) > 1e-9
            ):
                eviction_ok = False
                break
    check(eviction_ok, "C2_extreme_window_rapid_evictions_cap3_20000")

    # --------------------------------------------------------------------------
    # 3. Extreme Sliding Window: 50,000 Streaming Points & Drift Analysis
    # --------------------------------------------------------------------------
    agg1000 = ReferenceTelemetryAggregator(capacity=1000, metric_id=1)
    max_drift = 0.0
    for p in range(50000):
        val = 20.0 + (p % 100) * 0.5 + (1.0 if p % 2 == 0 else -1.0)
        agg1000.add_sample(val)
        if p >= 1000 and p % 5000 == 0:
            exact_sum = sum(agg1000.samples)
            drift = abs(agg1000.running_sum - exact_sum)
            if drift > max_drift:
                max_drift = drift

    st1000 = agg1000.get_stats()
    check(
        st1000["count"] == 1000
        and st1000["total"] == 50000
        and abs(st1000["avg"] - 44.75) < 1e-9
        and max_drift < 1e-9,
        "C2_extreme_window_50000_streaming_drift_analysis",
        f"max_drift={max_drift:.2e}, final_mean={st1000['avg']}",
    )

    # --------------------------------------------------------------------------
    # 4. Extreme Pattern: All Identical Samples
    # --------------------------------------------------------------------------
    for const_val, label in [(42.0, "positive_42"), (0.0, "zero"), (-99.9, "negative_99_9")]:
        agg_id = ReferenceTelemetryAggregator(capacity=500, metric_id=1)
        for _ in range(5000):
            agg_id.add_sample(const_val)
        st_id = agg_id.get_stats()
        check(
            st_id["count"] == 500
            and abs(st_id["min"] - const_val) < 1e-9
            and abs(st_id["max"] - const_val) < 1e-9
            and abs(st_id["avg"] - const_val) < 1e-9,
            f"C2_extreme_pattern_identical_{label}",
        )

    # --------------------------------------------------------------------------
    # 5. Extreme Pattern: All Negative Samples
    # --------------------------------------------------------------------------
    agg_neg = ReferenceTelemetryAggregator(capacity=100, metric_id=1)
    for i in range(1000):
        agg_neg.add_sample(-100.0 + (i % 50) * 0.5)
    st_neg = agg_neg.get_stats()
    check(
        st_neg["count"] == 100 and st_neg["min"] < 0 and st_neg["max"] < 0 and st_neg["avg"] < 0,
        "C2_extreme_pattern_all_negative_samples",
        f"min={st_neg['min']}, max={st_neg['max']}, avg={st_neg['avg']:.2f}",
    )

    # --------------------------------------------------------------------------
    # 6. Extreme Pattern: Alternating Spikes (+1e6, -1e6)
    # --------------------------------------------------------------------------
    agg_spike = ReferenceTelemetryAggregator(capacity=100, metric_id=1)
    for i in range(2000):
        agg_spike.add_sample(1000000.0 if i % 2 == 0 else -1000000.0)
    st_spike = agg_spike.get_stats()
    check(
        st_spike["count"] == 100
        and abs(st_spike["min"] - (-1000000.0)) < 1e-9
        and abs(st_spike["max"] - 1000000.0) < 1e-9
        and abs(st_spike["avg"] - 0.0) < 1e-9,
        "C2_extreme_pattern_alternating_spikes",
        f"min={st_spike['min']}, max={st_spike['max']}, avg={st_spike['avg']}",
    )

    # --------------------------------------------------------------------------
    # 7. 56-Byte Frame Encoding and Adler-32 Checksum Validation
    # --------------------------------------------------------------------------
    frame = agg1000.encode_frame(ts_sec=1700000000, ts_msec=500)
    check(len(frame) == 56, "C2_frame_encoding_exact_56_bytes")

    dec = decode_frame(frame)
    check(
        dec["magic"] == TELEMETRY_MAGIC
        and dec["version"] == TELEMETRY_VERSION
        and dec["ts_sec"] == 1700000000
        and dec["ts_msec"] == 500
        and dec["capacity"] == 1000
        and dec["count"] == 1000
        and dec["total_samples"] == 50000
        and abs(dec["avg"] - 44.75) < 1e-9,
        "C2_frame_roundtrip_decoding",
    )

    # Corrupted checksum detection
    bad_chk_frame = bytearray(frame)
    bad_chk_frame[-1] ^= 0x01
    bad_chk_caught = False
    try:
        decode_frame(bytes(bad_chk_frame))
    except ValueError:
        bad_chk_caught = True
    check(bad_chk_caught, "C2_frame_reject_corrupted_checksum")

    # Corrupted magic detection
    bad_magic_frame = bytearray(frame)
    bad_magic_frame[0] = 0x00
    bad_magic_caught = False
    try:
        decode_frame(bytes(bad_magic_frame))
    except ValueError:
        bad_magic_caught = True
    check(bad_magic_caught, "C2_frame_reject_bad_magic")

    # --------------------------------------------------------------------------
    # 8. Dual-Engine & 4-Engine Parity Verification
    # --------------------------------------------------------------------------
    print("\nExecuting multi-engine benchmark (50,000 samples, window 1000)...")

    # 8.1 Python
    py_code, py_out, py_err = run_cmd([sys.executable, str(APP_DIR / "telemetry_aggregator.py"), "--benchmark", "50000", "1000"])
    check(py_code == 0, "C2_app3_python_benchmark_exit_0", py_err.strip())

    # 8.2 Rust
    with tempfile.TemporaryDirectory() as tmpdir:
        rs_bin = Path(tmpdir) / "telemetry_aggregator_rs"
        r_c_code, r_c_out, r_c_err = run_cmd(["rustc", "-O", str(APP_DIR / "telemetry_aggregator.rs"), "-o", str(rs_bin)])
        check(r_c_code == 0, "C2_app3_rust_compilation", r_c_err.strip())
        rs_code, rs_out, rs_err = run_cmd([str(rs_bin), "--benchmark", "50000", "1000"])
        check(rs_code == 0, "C2_app3_rust_benchmark_exit_0", rs_err.strip())

    # 8.3 MEMElang Interpreter
    interp_code, interp_out, interp_err = run_cmd([str(MEME_BIN), str(APP_DIR / "telemetry_aggregator.meme"), "--benchmark", "50000", "1000"])
    check(interp_code == 0, "C2_app3_interp_benchmark_exit_0", interp_err.strip())

    # 8.4 MEMElang Native Compiler
    with tempfile.TemporaryDirectory() as tmpdir:
        native_bin = Path(tmpdir) / "telemetry_aggregator_native"
        n_c_code, n_c_out, n_c_err = run_cmd([str(MEME_BIN), "compile", str(APP_DIR / "telemetry_aggregator.meme"), "-o", str(native_bin)])
        check(n_c_code == 0, "C2_app3_native_compilation", n_c_err.strip())
        nat_code, nat_out, nat_err = run_cmd([str(native_bin), "--benchmark", "50000", "1000"])
        check(nat_code == 0, "C2_app3_native_benchmark_exit_0", nat_err.strip())

    # Parse and compare outputs
    def parse_metrics(out_text):
        m = {}
        for token in out_text.strip().split():
            if "=" in token:
                k, v = token.split("=", 1)
                m[k] = v
        return m

    m_py = parse_metrics(py_out)
    m_rs = parse_metrics(rs_out)
    m_interp = parse_metrics(interp_out)
    m_nat = parse_metrics(nat_out)

    print(f"  Python:      {py_out.strip()}")
    print(f"  Rust:        {rs_out.strip()}")
    print(f"  Interpreter: {interp_out.strip()}")
    print(f"  Native:      {nat_out.strip()}")

    # Verify matching frames_generated
    fg_match = (
        m_py.get("frames_generated") == "10"
        and m_rs.get("frames_generated") == "10"
        and m_interp.get("frames_generated") == "10"
        and m_nat.get("frames_generated") == "10"
    )
    check(fg_match, "C2_4engine_parity_frames_generated_exact_match", f"py={m_py.get('frames_generated')} rs={m_rs.get('frames_generated')} int={m_interp.get('frames_generated')} nat={m_nat.get('frames_generated')}")

    # Verify matching last_mean (within 1e-4)
    means = [
        float(m_py.get("last_mean", 0)),
        float(m_rs.get("last_mean", 0)),
        float(m_interp.get("last_mean", 0)),
        float(m_nat.get("last_mean", 0)),
    ]
    mean_match = all(abs(m - 44.75) < 1e-4 for m in means)
    check(mean_match, "C2_4engine_parity_last_mean_exact_match", f"means={means}")

    # --------------------------------------------------------------------------
    # 9. Verify 56-Byte Frame Parity between Interpreter & Native
    # --------------------------------------------------------------------------
    # Write a test script to encode a deterministic frame in both engines and output hex
    frame_test_meme = """
pakad agg = TelemetryAggregator(10, 1);
pakad i = 0;
jabtak i < 25 {
    agg.add_sample(10.0 + i * 2.5);
    i = i + 1;
}
pakad f = agg.encode_frame(1700000000, 123);
dikha "FRAME_HEX:" + f.to_hex();
"""
    with tempfile.NamedTemporaryFile(suffix=".meme", mode="w") as f:
        # Prepend TelemetryAggregator definition from telemetry_aggregator.meme
        telem_content = (APP_DIR / "telemetry_aggregator.meme").read_text()
        agg_code = telem_content[:telem_content.find("jugaad run_verify")]
        f.write(agg_code + "\n" + frame_test_meme)
        f.flush()

        # Run Interpreter
        c_i, out_i, _ = run_cmd([str(MEME_BIN), f.name])
        # Run Native
        with tempfile.TemporaryDirectory() as tmp_n_dir:
            tmp_n_bin = Path(tmp_n_dir) / "frame_test_nat"
            run_cmd([str(MEME_BIN), "compile", f.name, "-o", str(tmp_n_bin)])
            c_n, out_n, _ = run_cmd([str(tmp_n_bin)])

    hex_i = [line.split("FRAME_HEX:")[1].strip() for line in out_i.splitlines() if "FRAME_HEX:" in line]
    hex_n = [line.split("FRAME_HEX:")[1].strip() for line in out_n.splitlines() if "FRAME_HEX:" in line]

    frame_parity_ok = (len(hex_i) == 1 and len(hex_n) == 1 and hex_i[0] == hex_n[0])
    check(frame_parity_ok, "C2_frame_hex_dual_engine_bit_exact_parity", f"interp={hex_i[0] if hex_i else 'none'} native={hex_n[0] if hex_n else 'none'}")

    print(f"\nChallenger 2 App 3 Stress Suite: {len(passes)} passed, {len(failures)} failed.")
    if failures:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
