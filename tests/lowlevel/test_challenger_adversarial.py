#!/usr/bin/env python3
"""
Adversarial Stress Test Suite - Milestone 2
============================================
Evaluated by: Challenger 1 (teamwork_challenger_m2_1)
Role: Empirical Challenger (critic, specialist)

Stress Targets:
  1. App 1: Packet Framer & Circular Ring Buffer (Python, Rust, MEMElang Interp, MEMElang Native)
  2. App 2: Binary TLV Codec (Python, Rust, MEMElang Interp, MEMElang Native)
  3. Parity: Dual-Engine execution equivalence (Interpreter vs Native AOT)
  4. Invariants: 0 forbidden words, sizeof(Value)==32, make test passing
"""

import sys
import os
import math
import struct
import zlib
import random
import subprocess
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(PROJECT_ROOT / "apps" / "packet_framer"))
sys.path.insert(0, str(PROJECT_ROOT / "apps" / "tlv_codec"))

import packet_framer as py_framer
import tlv_codec as py_tlv

MEME_BIN = PROJECT_ROOT / "meme"

passes = []
failures = []
findings = []

def record_pass(name):
    passes.append(name)
    print(f"[PASS] {name}")

def record_fail(name, reason):
    failures.append((name, reason))
    print(f"[FAIL] {name}: {reason}")

def record_finding(severity, title, detail):
    findings.append({"severity": severity, "title": title, "detail": detail})
    print(f"[{severity.upper()} FINDING] {title}: {detail}")


# ==============================================================================
# SECTION 1: App 1 — Packet Framer & Ring Buffer Adversarial Stress
# ==============================================================================
print("\n" + "=" * 70)
print("SECTION 1: App 1 — Packet Framer & Ring Buffer Adversarial Stress")
print("=" * 70)

# Test 1.1: Circular Wrap-Around Stress (100 frames, 1-byte chunk stream ingestion)
def test_app1_circular_wraparound_stress():
    rb = py_framer.RingBuffer(64)
    num_frames = 100
    sent_frames = []
    stream = bytearray()
    
    for i in range(num_frames):
        p = f"msg_{i}_{'X' * (i % 15)}".encode()
        f = py_framer.pack_frame(i, p)
        sent_frames.append((i, p))
        stream.extend(f)
    
    recovered = []
    chunk_size = 1  # 1 byte at a time to maximize boundary splits
    offset = 0
    
    while offset < len(stream) or rb.available() >= 12:
        if offset < len(stream):
            w = rb.write(stream[offset:offset+chunk_size])
            offset += w
        
        while True:
            fr = rb.pop_frame()
            if fr is None:
                break
            recovered.append(fr)
            
        if offset >= len(stream) and rb.available() < 12:
            break
            
    if len(recovered) == num_frames and recovered == sent_frames:
        record_pass("T1_1_app1_circular_wraparound_100_frames_1byte_chunks")
    else:
        record_fail("T1_1_app1_circular_wraparound_100_frames_1byte_chunks",
                    f"Recovered {len(recovered)}/{num_frames} frames")

test_app1_circular_wraparound_stress()


# Test 1.2: Arbitrary Noisy Debris Resynchronization (Multi-Engine)
def test_app1_noise_resync():
    random.seed(1337)
    rb = py_framer.RingBuffer(128)
    frames = [py_framer.pack_frame(seq, f"data_{seq}".encode()) for seq in range(25)]
    
    stream = bytearray()
    for f in frames:
        # Non-magic noise
        noise = bytes([random.choice([b for b in range(256) if b not in (0xAA, 0x55)])
                       for _ in range(random.randint(5, 25))])
        stream.extend(noise)
        stream.extend(f)
    
    recovered = []
    offset = 0
    while offset < len(stream) or rb.available() >= 12:
        if offset < len(stream):
            offset += rb.write(stream[offset:offset+11])
        while True:
            fr = rb.pop_frame()
            if fr is None:
                break
            recovered.append(fr)
        if offset >= len(stream) and rb.available() < 12:
            break
            
    if len(recovered) == 25:
        record_pass("T1_2_app1_noise_stream_recovery_25_frames")
    else:
        record_fail("T1_2_app1_noise_stream_recovery_25_frames", f"Recovered {len(recovered)}/25")

test_app1_noise_resync()


# Test 1.3: Corrupted Checksum Rejection & Resync
def test_app1_bad_checksum_rejection():
    rb = py_framer.RingBuffer(64)
    good_frame1 = py_framer.pack_frame(1, b"FIRST")
    bad_frame = bytearray(py_framer.pack_frame(2, b"CORRUPTED"))
    bad_frame[-1] ^= 0xFF # Flip last byte of checksum
    good_frame2 = py_framer.pack_frame(3, b"THIRD")
    
    rb.write(good_frame1)
    rb.write(bytes(bad_frame))
    rb.write(good_frame2)
    
    f1 = rb.pop_frame()
    # Bad frame should fail unpack, so pop_frame either skips or yields next
    f2 = rb.pop_frame()
    f3 = rb.pop_frame()
    
    results = [fr for fr in [f1, f2, f3] if fr is not None]
    if len(results) == 2 and results[0] == (1, b"FIRST") and results[1] == (3, b"THIRD"):
        record_pass("T1_3_app1_bad_checksum_rejection_and_skip")
    else:
        record_fail("T1_3_app1_bad_checksum_rejection_and_skip", f"Got: {results}")

test_app1_bad_checksum_rejection()


# Test 1.4: Empirical Vulnerability Assessment — False Magic with Length > Capacity (Deadlock Test)
def test_app1_deadlock_vulnerability():
    # In a 64-byte RingBuffer, if noise injects 0xAA55 with length 1000 (> 64-12):
    rb = py_framer.RingBuffer(64)
    fake_hdr = bytes([0xAA, 0x55, 0x00, 0x00, 0x00, 0x00, 0x03, 0xE8]) # 8 bytes, p_len=1000
    real_frame = py_framer.pack_frame(42, b"VALID")
    
    rb.write(fake_hdr)
    rb.write(real_frame)
    
    popped = rb.pop_frame()
    if popped is None and rb.available() == (len(fake_hdr) + len(real_frame)):
        record_finding(
            "high",
            "App 1 RingBuffer Deadlock on Oversized False Magic Header",
            "When stream contains magic 0xAA55 with payload_length > (capacity - 12), "
            "pop_frame() returns None without skipping the false header byte. "
            "Because buffer size can never reach total_bytes, head/tail never advance, "
            "permanently blocking all subsequent frames."
        )
        record_pass("T1_4_app1_deadlock_vulnerability_reproduced_empirically")
    else:
        record_pass("T1_4_app1_deadlock_vulnerability_handled")

test_app1_deadlock_vulnerability()


# ==============================================================================
# SECTION 2: App 2 — Binary TLV Codec Adversarial Stress
# ==============================================================================
print("\n" + "=" * 70)
print("SECTION 2: App 2 — Binary TLV Codec Adversarial Stress")
print("=" * 70)

# Test 2.1: IEEE-754 Boundary Float Values across Python
def test_app2_ieee754_floats():
    float_cases = [
        ("pos_inf", float("inf")),
        ("neg_inf", float("-inf")),
        ("nan", float("nan")),
        ("pos_zero", 0.0),
        ("neg_zero", -0.0),
        ("subnormal_min", 5e-324),
        ("subnormal_large", 1e-308),
        ("max_normal", 1.7976931348623157e+308),
        ("min_normal", 2.2250738585072014e-308),
    ]
    
    all_ok = True
    for name, val in float_cases:
        enc = py_tlv.encode_tlv(py_tlv.TAG_FLOAT64, val)
        if len(enc) != 11 or enc[0] != py_tlv.TAG_FLOAT64:
            all_ok = False
            break
        (t, dec), nxt = py_tlv.decode_tlv_record(enc, 0)
        if t != py_tlv.TAG_FLOAT64 or nxt != 11:
            all_ok = False
            break
        if math.isnan(val):
            if not math.isnan(dec):
                all_ok = False
                break
        elif val == 0.0 and math.copysign(1.0, val) == -1.0:
            if not (dec == 0.0 and math.copysign(1.0, dec) == -1.0):
                all_ok = False
                break
        else:
            if dec != val:
                all_ok = False
                break
                
    if all_ok:
        record_pass("T2_1_app2_ieee754_boundary_floats_python")
    else:
        record_fail("T2_1_app2_ieee754_boundary_floats_python", "Mismatch in float decoding")

test_app2_ieee754_floats()


# Test 2.2: Extreme INT32 Boundaries
def test_app2_int32_boundaries():
    int_cases = [
        (2147483647, "0100047fffffff"),
        (-2147483648, "01000480000000"),
        (0, "01000400000000"),
        (-1, "010004ffffffff"),
        (1, "01000400000001"),
    ]
    all_ok = True
    for val, exp_hex in int_cases:
        enc = py_tlv.encode_tlv(py_tlv.TAG_INT32, val)
        if enc.hex() != exp_hex:
            all_ok = False
            break
        (t, dec), _ = py_tlv.decode_tlv_record(enc, 0)
        if t != py_tlv.TAG_INT32 or dec != val:
            all_ok = False
            break
            
    if all_ok:
        record_pass("T2_2_app2_int32_boundaries_python")
    else:
        record_fail("T2_2_app2_int32_boundaries_python", "Mismatch in int32 decoding")

test_app2_int32_boundaries()


# Test 2.3: Comprehensive 16-Case Malformed / Truncated / Invalid Rejection Matrix
def test_app2_error_matrix():
    error_cases = [
        ("incomplete_hdr_1b", b"\x01"),
        ("incomplete_hdr_2b", b"\x01\x00"),
        ("truncated_int32_val", b"\x01\x00\x04\x00\x00"),
        ("truncated_f64_val", b"\x02\x00\x08\x00\x00\x00"),
        ("truncated_str_val", b"\x03\x00\x05\x61\x62"),
        ("truncated_bytes_val", b"\x04\x00\x05\xaa\xbb"),
        ("truncated_bool_val", b"\x05\x00\x01"),
        ("invalid_int32_len_3", b"\x01\x00\x03\x00\x00\x00"),
        ("invalid_int32_len_5", b"\x01\x00\x05\x00\x00\x00\x00\x00"),
        ("invalid_f64_len_4", b"\x02\x00\x04\x00\x00\x00\x00"),
        ("invalid_bool_len_2", b"\x05\x00\x02\x00\x00"),
        ("invalid_bool_val_2", b"\x05\x00\x01\x02"),
        ("invalid_bool_val_255", b"\x05\x00\x01\xff"),
        ("unknown_tag_0", b"\x00\x00\x01\x00"),
        ("unknown_tag_6", b"\x06\x00\x01\x00"),
        ("unknown_tag_255", b"\xff\x00\x01\x00"),
    ]
    rejected = 0
    for name, data in error_cases:
        try:
            py_tlv.decode_tlv_record(data, 0)
        except ValueError:
            rejected += 1
            
    if rejected == len(error_cases):
        record_pass(f"T2_3_app2_error_rejection_matrix_16_cases")
    else:
        record_fail(f"T2_3_app2_error_rejection_matrix_16_cases", f"Only rejected {rejected}/{len(error_cases)}")

test_app2_error_matrix()


# Test 2.4: Maximum (65535-byte) Payload Roundtrip
def test_app2_max_payload_roundtrip():
    payload_64k = "M" * 65535
    enc = py_tlv.encode_tlv(py_tlv.TAG_STRING, payload_64k)
    (t, dec), nxt = py_tlv.decode_tlv_record(enc, 0)
    if t == py_tlv.TAG_STRING and len(dec) == 65535 and dec == payload_64k and nxt == (3 + 65535):
        record_pass("T2_4_app2_max_uint16_payload_65535_bytes")
    else:
        record_fail("T2_4_app2_max_uint16_payload_65535_bytes", "65535-byte string corrupted")

test_app2_max_payload_roundtrip()


# ==============================================================================
# SECTION 3: Multi-Engine Parity & Execution Parity
# ==============================================================================
print("\n" + "=" * 70)
print("SECTION 3: Multi-Engine Parity & Execution Parity")
print("=" * 70)

# Test 3.1: Rust Standalone Compilation & Verification
def test_rust_verification():
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_framer_bin = Path(tmpdir) / "framer_rs"
        tmp_tlv_bin = Path(tmpdir) / "tlv_rs"
        
        c1 = subprocess.run(["rustc", "-O", str(PROJECT_ROOT / "apps" / "packet_framer" / "packet_framer.rs"), "-o", str(tmp_framer_bin)],
                            capture_output=True, text=True)
        c2 = subprocess.run(["rustc", "-O", str(PROJECT_ROOT / "apps" / "tlv_codec" / "tlv_codec.rs"), "-o", str(tmp_tlv_bin)],
                            capture_output=True, text=True)
        
        if c1.returncode != 0 or c2.returncode != 0:
            record_fail("T3_1_rust_standalone_compilation", f"Rustc failed: {c1.stderr} {c2.stderr}")
            return
            
        r1 = subprocess.run([str(tmp_framer_bin), "--verify"], capture_output=True, text=True)
        r2 = subprocess.run([str(tmp_tlv_bin), "--verify"], capture_output=True, text=True)
        
        if r1.returncode == 0 and "OK" in r1.stdout and r2.returncode == 0 and "OK" in r2.stdout:
            record_pass("T3_1_rust_apps_compilation_and_verify")
        else:
            record_fail("T3_1_rust_apps_compilation_and_verify", f"Verify failed: r1={r1.stdout}, r2={r2.stdout}")

test_rust_verification()


# Test 3.2: MEMElang Interpreter vs Native AOT Output Equivalence
def test_memelang_dual_engine_parity():
    # App 1 Packet Framer Parity
    framer_meme = PROJECT_ROOT / "apps" / "packet_framer" / "packet_framer.meme"
    tlv_meme = PROJECT_ROOT / "apps" / "tlv_codec" / "tlv_codec.meme"
    
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_framer_native = Path(tmpdir) / "framer_native"
        tmp_tlv_native = Path(tmpdir) / "tlv_native"
        
        # Compile Native AOT
        cn1 = subprocess.run([str(MEME_BIN), "compile", str(framer_meme), "-o", str(tmp_framer_native)],
                             capture_output=True, text=True)
        cn2 = subprocess.run([str(MEME_BIN), "compile", str(tlv_meme), "-o", str(tmp_tlv_native)],
                             capture_output=True, text=True)
        
        if cn1.returncode != 0 or cn2.returncode != 0:
            record_fail("T3_2_native_aot_compilation", f"Meme compile failed: {cn1.stderr} {cn2.stderr}")
            return
            
        # Run Interpreter
        i1 = subprocess.run([str(MEME_BIN), str(framer_meme), "--verify"], capture_output=True, text=True)
        i2 = subprocess.run([str(MEME_BIN), str(tlv_meme), "--verify"], capture_output=True, text=True)
        
        # Run Native
        n1 = subprocess.run([str(tmp_framer_native), "--verify"], capture_output=True, text=True)
        n2 = subprocess.run([str(tmp_tlv_native), "--verify"], capture_output=True, text=True)
        
        # Check exit codes and stdout matching
        if i1.returncode == 0 and n1.returncode == 0 and i2.returncode == 0 and n2.returncode == 0:
            record_pass("T3_2_memelang_dual_engine_verify_exit_codes_0")
        else:
            record_fail("T3_2_memelang_dual_engine_verify_exit_codes_0",
                        f"Non-zero exit: i1={i1.returncode}, n1={n1.returncode}, i2={i2.returncode}, n2={n2.returncode}")
            return
            
        # Strip compiler banner lines from native output if any
        n1_clean = "\n".join(l for l in n1.stdout.strip().splitlines() if not l.startswith("⚡") and not l.startswith("✔"))
        n2_clean = "\n".join(l for l in n2.stdout.strip().splitlines() if not l.startswith("⚡") and not l.startswith("✔"))
        
        if i1.stdout.strip() == n1_clean and i2.stdout.strip() == n2_clean:
            record_pass("T3_2_memelang_dual_engine_bit_exact_stdout_parity")
        else:
            record_fail("T3_2_memelang_dual_engine_bit_exact_stdout_parity",
                        f"Output mismatch:\nApp 1 Interp:\n{i1.stdout.strip()}\nApp 1 Native:\n{n1_clean}\n"
                        f"App 2 Interp:\n{i2.stdout.strip()}\nApp 2 Native:\n{n2_clean}")

test_memelang_dual_engine_parity()


# Test 3.3: Benchmark Metric Consistency across all 4 Engines
def test_all_four_engines_benchmark_consistency():
    # Run benchmark mode for App 1 (100 iterations, 128 payload bytes)
    iters = 100
    p_size = 128
    
    # 1. Python
    py_cmd = [sys.executable, str(PROJECT_ROOT / "apps" / "packet_framer" / "packet_framer.py"), "--benchmark", str(iters), str(p_size)]
    r_py = subprocess.run(py_cmd, capture_output=True, text=True)
    
    # 2. Rust
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_bin = Path(tmpdir) / "framer_rs"
        subprocess.run(["rustc", "-O", str(PROJECT_ROOT / "apps" / "packet_framer" / "packet_framer.rs"), "-o", str(tmp_bin)], check=True)
        r_rs = subprocess.run([str(tmp_bin), "--benchmark", str(iters), str(p_size)], capture_output=True, text=True)
        
        # 3. MEMElang Interpreter
        r_interp = subprocess.run([str(MEME_BIN), str(PROJECT_ROOT / "apps" / "packet_framer" / "packet_framer.meme"), "--benchmark", str(iters), str(p_size)], capture_output=True, text=True)
        
        # 4. MEMElang Native AOT
        tmp_native = Path(tmpdir) / "framer_native"
        subprocess.run([str(MEME_BIN), "compile", str(PROJECT_ROOT / "apps" / "packet_framer" / "packet_framer.meme"), "-o", str(tmp_native)], check=True, capture_output=True)
        r_nat = subprocess.run([str(tmp_native), "--benchmark", str(iters), str(p_size)], capture_output=True, text=True)
        
    outputs = {
        "Python": r_py.stdout.strip(),
        "Rust": r_rs.stdout.strip(),
        "MEMElang Interpreter": r_interp.stdout.strip(),
        "MEMElang Native": "\n".join(l for l in r_nat.stdout.strip().splitlines() if not l.startswith("⚡") and not l.startswith("✔")),
    }
    
    # Verify each output reported frames_processed=100 and identical checksum_acc
    chk_accs = {}
    for eng, out in outputs.items():
        parts = dict(token.split("=") for token in out.split() if "=" in token)
        if "frames_processed" in parts and "checksum_acc" in parts:
            chk_accs[eng] = (int(parts["frames_processed"]), int(parts["checksum_acc"]))
        else:
            record_fail("T3_3_benchmark_metrics_parity", f"{eng} missing key metrics in: {out}")
            return
            
    first_acc = list(chk_accs.values())[0]
    all_match = all(v == first_acc for v in chk_accs.values())
    if all_match:
        record_pass(f"T3_3_benchmark_metrics_parity (frames={first_acc[0]}, checksum_acc={first_acc[1]}) across all 4 engines")
    else:
        record_fail("T3_3_benchmark_metrics_parity", f"Checksum accumulator mismatch: {chk_accs}")

test_all_four_engines_benchmark_consistency()


# ==============================================================================
# SECTION 4: Repository Invariants
# ==============================================================================
print("\n" + "=" * 70)
print("SECTION 4: Repository Invariants Verification")
print("=" * 70)

# Test 4.1: Static Assert sizeof(Value) == 32
def test_sizeof_value():
    val_h = (PROJECT_ROOT / "src" / "interpreter" / "Value.h").read_text()
    if "static_assert(sizeof(Value) == 32" in val_h:
        record_pass("T4_1_sizeof_value_32_static_assert")
    else:
        record_fail("T4_1_sizeof_value_32_static_assert", "Missing static_assert(sizeof(Value) == 32)")

test_sizeof_value()

# Test 4.2: Forbidden Word Scan
def test_forbidden_words():
    inv_res = subprocess.run([sys.executable, str(PROJECT_ROOT / "tests" / "lowlevel" / "test_invariants.py")],
                             capture_output=True, text=True)
    if inv_res.returncode == 0:
        record_pass("T4_2_all_repo_invariants_pass")
    else:
        record_fail("T4_2_all_repo_invariants_pass", inv_res.stdout)

test_forbidden_words()


# ==============================================================================
# Final Summary
# ==============================================================================
print("\n" + "=" * 70)
print(f"ADVERSARIAL STRESS TEST SUMMARY: {len(passes)} Passed, {len(failures)} Failed, {len(findings)} Findings")
print("=" * 70)

if failures:
    sys.exit(1)
sys.exit(0)
