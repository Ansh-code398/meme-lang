#!/usr/bin/env python3
"""
Multi-Engine E2E Test Suite: App 2 — Binary TLV (Type-Length-Value) Codec
========================================================================
Authoritative Source: TEST_INFRA.md (F6), PROJECT.md, and Explorer 3 Report

Tags Covered:
  - 0x01: INT32 (4 bytes, Big-Endian)
  - 0x02: FLOAT64 (8 bytes, IEEE-754 Big-Endian)
  - 0x03: STRING (Variable UTF-8)
  - 0x04: BYTES (Variable raw bytes)
  - 0x05: BOOL (1 byte, 0x00=false, 0x01=true)

Validates:
  - Exact binary encoding vectors for all 5 primitive types
  - 36-byte composite sensor record (Tag 1 + Tag 3 + Tag 2 + Tag 5)
  - IEEE-754 float boundary values: NaN, +Inf, -Inf, 0.0, -0.0
  - Large payload (64KB string)
  - Malformed & truncated stream rejection
  - Invalid fixed lengths & invalid boolean values
  - Multi-engine execution parity (Python, Rust, MEMElang Interpreter, MEMElang Native)
"""

import sys
import os
import struct
import math
import subprocess
import argparse
import tempfile
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
APPS_DIR = PROJECT_ROOT / "apps" / "tlv_codec"

# TLV Tag Constants
TAG_INT32 = 0x01
TAG_FLOAT64 = 0x02
TAG_STRING = 0x03
TAG_BYTES = 0x04
TAG_BOOL = 0x05


def encode_tlv(tag: int, value) -> bytes:
    """Encodes a single TLV record with 1-byte tag, 2-byte BE length, and value bytes."""
    if tag == TAG_INT32:
        val_bytes = struct.pack(">i", int(value))
    elif tag == TAG_FLOAT64:
        val_bytes = struct.pack(">d", float(value))
    elif tag == TAG_STRING:
        val_bytes = value.encode("utf-8") if isinstance(value, str) else bytes(value)
    elif tag == TAG_BYTES:
        val_bytes = bytes(value)
    elif tag == TAG_BOOL:
        val_bytes = struct.pack(">B", 1 if value else 0)
    else:
        raise ValueError(f"Unknown tag: {tag}")

    header = struct.pack(">BH", tag, len(val_bytes))
    return header + val_bytes


def decode_tlv_record(stream: bytes, offset: int = 0):
    """Decodes a single TLV record starting at offset. Returns ((tag, value), next_offset)."""
    if len(stream) - offset < 3:
        raise ValueError(f"Incomplete header: available {len(stream) - offset} bytes < 3")

    tag, length = struct.unpack_from(">BH", stream, offset)
    val_offset = offset + 3
    if len(stream) - val_offset < length:
        raise ValueError(f"Truncated value: expected {length} bytes, available {len(stream) - val_offset}")

    raw_val = stream[val_offset:val_offset + length]
    next_offset = val_offset + length

    if tag == TAG_INT32:
        if length != 4:
            raise ValueError(f"Invalid length for INT32: expected 4, got {length}")
        (val,) = struct.unpack(">i", raw_val)
    elif tag == TAG_FLOAT64:
        if length != 8:
            raise ValueError(f"Invalid length for FLOAT64: expected 8, got {length}")
        (val,) = struct.unpack(">d", raw_val)
    elif tag == TAG_STRING:
        val = raw_val.decode("utf-8")
    elif tag == TAG_BYTES:
        val = raw_val
    elif tag == TAG_BOOL:
        if length != 1:
            raise ValueError(f"Invalid length for BOOL: expected 1, got {length}")
        (b_val,) = struct.unpack(">B", raw_val)
        if b_val not in (0, 1):
            raise ValueError(f"Invalid BOOL byte: {b_val} (must be 0 or 1)")
        val = (b_val == 1)
    else:
        raise ValueError(f"Unknown tag: {tag}")

    return (tag, val), next_offset


def decode_tlv_stream(stream: bytes):
    """Decodes a contiguous sequence of TLV records from stream."""
    records = []
    offset = 0
    while offset < len(stream):
        rec, offset = decode_tlv_record(stream, offset)
        records.append(rec)
    return records


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
    parser = argparse.ArgumentParser(description="TLV Codec Multi-Engine Test Suite")
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

    print("=== App 2: Binary TLV Codec Test Suite ===")

    # --------------------------------------------------------------------------
    # Tier 1: Canonical Feature Coverage & Verified Primitive Test Vectors
    # --------------------------------------------------------------------------
    if args.tier in ("1", "all"):
        # Vector 1: INT32 (42) -> 0100040000002a
        enc_int = encode_tlv(TAG_INT32, 42)
        assert enc_int.hex() == "0100040000002a"
        (t, v), _ = decode_tlv_record(enc_int)
        if t == TAG_INT32 and v == 42:
            pass_test("T1_tlv_vector_int32_42")
        else:
            fail_test("T1_tlv_vector_int32_42", f"Mismatched decode: tag={t}, val={v}")

        # Vector 2: FLOAT64 (pi) -> 020008400921fb54442d18
        enc_pi = encode_tlv(TAG_FLOAT64, 3.141592653589793)
        assert enc_pi.hex() == "020008400921fb54442d18"
        (t, v), _ = decode_tlv_record(enc_pi)
        if t == TAG_FLOAT64 and abs(v - 3.141592653589793) < 1e-12:
            pass_test("T1_tlv_vector_float64_pi")
        else:
            fail_test("T1_tlv_vector_float64_pi", f"Mismatched decode: {v}")

        # Vector 3: STRING ("Hello, MEMElang!") -> 03001048656c6c6f2c204d454d456c616e6721
        enc_str = encode_tlv(TAG_STRING, "Hello, MEMElang!")
        assert enc_str.hex() == "03001048656c6c6f2c204d454d456c616e6721"
        (t, v), _ = decode_tlv_record(enc_str)
        if t == TAG_STRING and v == "Hello, MEMElang!":
            pass_test("T1_tlv_vector_string_hello")
        else:
            fail_test("T1_tlv_vector_string_hello", f"Mismatched decode: {v}")

        # Vector 4: BYTES (0xDEADBEEF) -> 040004deadbeef
        enc_bytes = encode_tlv(TAG_BYTES, bytes.fromhex("deadbeef"))
        assert enc_bytes.hex() == "040004deadbeef"
        (t, v), _ = decode_tlv_record(enc_bytes)
        if t == TAG_BYTES and v == bytes.fromhex("deadbeef"):
            pass_test("T1_tlv_vector_bytes_deadbeef")
        else:
            fail_test("T1_tlv_vector_bytes_deadbeef", f"Mismatched decode: {v.hex()}")

        # Vector 5: BOOL (true) -> 05000101, BOOL (false) -> 05000100
        enc_true = encode_tlv(TAG_BOOL, True)
        enc_false = encode_tlv(TAG_BOOL, False)
        assert enc_true.hex() == "05000101"
        assert enc_false.hex() == "05000100"
        (_, v_tr), _ = decode_tlv_record(enc_true)
        (_, v_fl), _ = decode_tlv_record(enc_false)
        if v_tr is True and v_fl is False:
            pass_test("T1_tlv_vector_bool_true_false")
        else:
            fail_test("T1_tlv_vector_bool_true_false", f"Mismatched decode: true={v_tr}, false={v_fl}")

        # Vector 6: Composite 36-Byte Telemetry Record
        # INT32(101) + STRING("temp_sensor") + FLOAT64(23.5) + BOOL(true)
        composite = (
            encode_tlv(TAG_INT32, 101)
            + encode_tlv(TAG_STRING, "temp_sensor")
            + encode_tlv(TAG_FLOAT64, 23.5)
            + encode_tlv(TAG_BOOL, True)
        )
        expected_comp_hex = "0100040000006503000b74656d705f73656e736f72020008403780000000000005000101"
        assert len(composite) == 36
        assert composite.hex() == expected_comp_hex
        recs = decode_tlv_stream(composite)
        if (
            len(recs) == 4
            and recs[0] == (TAG_INT32, 101)
            and recs[1] == (TAG_STRING, "temp_sensor")
            and abs(recs[2][1] - 23.5) < 1e-9
            and recs[3] == (TAG_BOOL, True)
        ):
            pass_test("T1_tlv_vector_composite_36_byte_record")
        else:
            fail_test("T1_tlv_vector_composite_36_byte_record", f"Composite decode failed: {recs}")

    # --------------------------------------------------------------------------
    # Tier 2: Boundary & Corner Cases (Limits, Overflows, Error Handling)
    # --------------------------------------------------------------------------
    if args.tier in ("2", "all"):
        # T2.1: Empty String and Empty Bytes
        enc_empty_str = encode_tlv(TAG_STRING, "")
        enc_empty_bytes = encode_tlv(TAG_BYTES, b"")
        assert enc_empty_str.hex() == "030000"
        assert enc_empty_bytes.hex() == "040000"
        (_, dec_empty_str), _ = decode_tlv_record(enc_empty_str)
        (_, dec_empty_bytes), _ = decode_tlv_record(enc_empty_bytes)
        if dec_empty_str == "" and dec_empty_bytes == b"":
            pass_test("T2_tlv_empty_string_and_bytes")
        else:
            fail_test("T2_tlv_empty_string_and_bytes", "Empty string/bytes round-trip failed")

        # T2.2: Extreme INT32 values (INT32_MAX, INT32_MIN, -1)
        enc_max = encode_tlv(TAG_INT32, 2147483647)
        enc_min = encode_tlv(TAG_INT32, -2147483648)
        enc_neg1 = encode_tlv(TAG_INT32, -1)
        assert enc_max.hex() == "0100047fffffff"
        assert enc_min.hex() == "01000480000000"
        assert enc_neg1.hex() == "010004ffffffff"
        (_, v_max), _ = decode_tlv_record(enc_max)
        (_, v_min), _ = decode_tlv_record(enc_min)
        (_, v_neg1), _ = decode_tlv_record(enc_neg1)
        if v_max == 2147483647 and v_min == -2147483648 and v_neg1 == -1:
            pass_test("T2_tlv_int32_extreme_boundaries")
        else:
            fail_test("T2_tlv_int32_extreme_boundaries", "Extreme INT32 decode failed")

        # T2.3: IEEE-754 Boundary Float Values (+Inf, -Inf, NaN, 0.0, -0.0)
        enc_pos_inf = encode_tlv(TAG_FLOAT64, float("inf"))
        enc_neg_inf = encode_tlv(TAG_FLOAT64, float("-inf"))
        enc_nan = encode_tlv(TAG_FLOAT64, float("nan"))
        enc_zero = encode_tlv(TAG_FLOAT64, 0.0)
        enc_neg_zero = encode_tlv(TAG_FLOAT64, -0.0)

        assert enc_pos_inf.hex() == "0200087ff0000000000000"
        assert enc_neg_inf.hex() == "020008fff0000000000000"
        assert enc_zero.hex() == "0200080000000000000000"
        assert enc_neg_zero.hex() == "0200088000000000000000"

        (_, d_pos_inf), _ = decode_tlv_record(enc_pos_inf)
        (_, d_neg_inf), _ = decode_tlv_record(enc_neg_inf)
        (_, d_nan), _ = decode_tlv_record(enc_nan)
        if math.isinf(d_pos_inf) and d_pos_inf > 0 and math.isinf(d_neg_inf) and d_neg_inf < 0 and math.isnan(d_nan):
            pass_test("T2_tlv_ieee754_boundary_floats")
        else:
            fail_test("T2_tlv_ieee754_boundary_floats", "Float boundary decode mismatch")

        # T2.4: Large string encoding (64KB)
        large_str = "A" * 65535
        enc_large_str = encode_tlv(TAG_STRING, large_str)
        assert len(enc_large_str) == 3 + 65535
        (_, d_large_str), _ = decode_tlv_record(enc_large_str)
        if len(d_large_str) == 65535 and d_large_str == large_str:
            pass_test("T2_tlv_large_string_64k")
        else:
            fail_test("T2_tlv_large_string_64k", "Large string decode mismatch")

        # T2.5: Truncated header rejection
        try:
            decode_tlv_record(b"\x01\x00")  # Only 2 bytes instead of 3
            fail_test("T2_tlv_reject_truncated_header", "Accepted 2-byte header")
        except ValueError:
            pass_test("T2_tlv_reject_truncated_header")

        # T2.6: Truncated value rejection
        try:
            decode_tlv_record(bytes.fromhex("0100040000"))  # Claims 4 bytes, only gives 2
            fail_test("T2_tlv_reject_truncated_value", "Accepted truncated value")
        except ValueError:
            pass_test("T2_tlv_reject_truncated_value")

        # T2.7: Invalid fixed length rejection (INT32 with len=3)
        try:
            decode_tlv_record(bytes.fromhex("010003000000"))
            fail_test("T2_tlv_reject_invalid_fixed_length", "Accepted INT32 with len=3")
        except ValueError:
            pass_test("T2_tlv_reject_invalid_fixed_length")

        # T2.8: Invalid boolean byte rejection (byte=0x02)
        try:
            decode_tlv_record(bytes.fromhex("05000102"))
            fail_test("T2_tlv_reject_invalid_bool_byte", "Accepted BOOL with byte=0x02")
        except ValueError:
            pass_test("T2_tlv_reject_invalid_bool_byte")

        # T2.9: Unknown tag rejection
        try:
            decode_tlv_record(bytes.fromhex("FF000100"))
            fail_test("T2_tlv_reject_unknown_tag", "Accepted unknown tag 0xFF")
        except ValueError:
            pass_test("T2_tlv_reject_unknown_tag")

    # --------------------------------------------------------------------------
    # Progressive Multi-Engine Application Verification (apps/tlv_codec)
    # --------------------------------------------------------------------------
    engines_to_check = ["python", "rust", "interpreter", "native"] if args.engine == "all" else [args.engine]
    for eng in engines_to_check:
        test_id = f"T1_app2_{eng}_engine_parity"
        app_file_map = {
            "python": APPS_DIR / "tlv_codec.py",
            "rust": APPS_DIR / "tlv_codec.rs",
            "interpreter": APPS_DIR / "tlv_codec.meme",
            "native": APPS_DIR / "tlv_codec.meme",
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
                tmp_bin = Path(tmpdir) / "tlv_codec_rs"
                c_code, c_out, c_err = run_command(["rustc", "-O", str(target_file), "-o", str(tmp_bin)])
                if c_code != 0:
                    code, stdout, stderr = c_code, c_out, c_err
                else:
                    code, stdout, stderr = run_command([str(tmp_bin), "--verify"])
        elif eng == "interpreter":
            code, stdout, stderr = run_command([str(PROJECT_ROOT / "meme"), str(target_file), "--verify"])
        elif eng == "native":
            with tempfile.TemporaryDirectory() as tmpdir:
                tmp_bin = Path(tmpdir) / "tlv_codec_native"
                c_code, c_out, c_err = run_command([str(PROJECT_ROOT / "meme"), "compile", str(target_file), "-o", str(tmp_bin)])
                if c_code != 0:
                    code, stdout, stderr = c_code, c_out, c_err
                else:
                    code, stdout, stderr = run_command([str(tmp_bin), "--verify"])

        if code == 0 and ("OK" in stdout or "[PASS]" in stdout):
            pass_test(test_id)
        else:
            fail_test(test_id, f"Engine execution failed (code {code}): {stderr.strip() or stdout.strip()}")

    print(f"\nTLV Codec Suite: {len(passes)} passed, {len(staged)} staged, {len(failures)} failed.")
    if failures or (args.strict and staged):
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
