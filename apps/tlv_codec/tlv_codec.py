#!/usr/bin/env python3
# ==============================================================================
# App 2: Binary TLV Serializer/Deserializer (Python Reference Implementation)
# ==============================================================================

import sys
import struct
import math
import time

TAG_INT32 = 0x01
TAG_FLOAT64 = 0x02
TAG_STRING = 0x03
TAG_BYTES = 0x04
TAG_BOOL = 0x05


def encode_tlv(tag: int, value) -> bytes:
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
    return struct.pack(">BH", tag, len(val_bytes)) + val_bytes


def decode_tlv_record(stream: bytes, offset: int = 0):
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
    records = []
    offset = 0
    while offset < len(stream):
        rec, offset = decode_tlv_record(stream, offset)
        records.append(rec)
    return records


def run_verify():
    # Vector checks
    assert encode_tlv(TAG_INT32, 42).hex() == "0100040000002a"
    assert encode_tlv(TAG_FLOAT64, 3.141592653589793).hex() == "020008400921fb54442d18"
    assert encode_tlv(TAG_STRING, "Hello, MEMElang!").hex() == "03001048656c6c6f2c204d454d456c616e6721"
    assert encode_tlv(TAG_BYTES, bytes.fromhex("deadbeef")).hex() == "040004deadbeef"
    assert encode_tlv(TAG_BOOL, True).hex() == "05000101"
    assert encode_tlv(TAG_BOOL, False).hex() == "05000100"

    comp = (encode_tlv(TAG_INT32, 101) +
            encode_tlv(TAG_STRING, "temp_sensor") +
            encode_tlv(TAG_FLOAT64, 23.5) +
            encode_tlv(TAG_BOOL, True))
    assert comp.hex() == "0100040000006503000b74656d705f73656e736f72020008403780000000000005000101"
    assert len(comp) == 36

    # Edge cases
    assert encode_tlv(TAG_STRING, "").hex() == "030000"
    assert encode_tlv(TAG_BYTES, b"").hex() == "040000"
    assert encode_tlv(TAG_INT32, 2147483647).hex() == "0100047fffffff"
    assert encode_tlv(TAG_INT32, -2147483648).hex() == "01000480000000"
    assert encode_tlv(TAG_INT32, -1).hex() == "010004ffffffff"

    large_s = "A" * 65535
    enc_large = encode_tlv(TAG_STRING, large_s)
    (_, dec_large), _ = decode_tlv_record(enc_large)
    assert dec_large == large_s

    # Error handling
    try:
        decode_tlv_record(b"\x01\x00")
        assert False
    except ValueError:
        pass

    try:
        decode_tlv_record(bytes.fromhex("0100040000"))
        assert False
    except ValueError:
        pass

    print("[PASS] Vector INT32 42")
    print("[PASS] Vector FLOAT64 pi")
    print("[PASS] Vector STRING Hello")
    print("[PASS] Vector BYTES deadbeef")
    print("[PASS] Vector BOOL true/false")
    print("[PASS] Vector Composite 36B Record")
    print("[PASS] Boundary Cases & Error Rejections")
    print("[TLV_CODEC] ALL TESTS PASSED")
    print("OK")
    return 0


def run_benchmark(iterations: int):
    t0 = time.perf_counter()
    bytes_enc = 0
    records_cnt = 0
    for _ in range(iterations):
        comp = (encode_tlv(TAG_INT32, 101) +
                encode_tlv(TAG_STRING, "temp_sensor") +
                encode_tlv(TAG_FLOAT64, 23.5) +
                encode_tlv(TAG_BOOL, True))
        recs = decode_tlv_stream(comp)
        bytes_enc += len(comp)
        records_cnt += len(recs)
    t1 = time.perf_counter()
    elapsed_ms = (t1 - t0) * 1000.0
    throughput = (records_cnt / (elapsed_ms / 1000.0)) if elapsed_ms > 0 else 0
    print(f"elapsed_ms={elapsed_ms:.2f} throughput_ops_s={throughput:.2f} records_processed={records_cnt} bytes_encoded={bytes_enc}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--benchmark":
        iters = int(sys.argv[2]) if len(sys.argv) > 2 else 10000
        sys.exit(run_benchmark(iters))
    sys.exit(run_verify())
