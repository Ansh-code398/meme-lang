#!/usr/bin/env python3
# ==============================================================================
# App 3: Rolling Window Telemetry Aggregator (Python Reference Implementation)
# ==============================================================================

import sys
import time
import struct
import zlib

TELEMETRY_MAGIC = 0x544D
TELEMETRY_VERSION = 0x01
FRAME_LENGTH = 56
HEADER_PAYLOAD_LENGTH = 52


def compute_adler32(data: bytes) -> int:
    return zlib.adler32(data) & 0xFFFFFFFF


class TelemetryAggregator:
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


def decode_telemetry_frame(frame: bytes):
    if len(frame) != FRAME_LENGTH:
        raise ValueError(f"Invalid frame length: {len(frame)} != {FRAME_LENGTH}")
    body = frame[:HEADER_PAYLOAD_LENGTH]
    (chk,) = struct.unpack(">I", frame[HEADER_PAYLOAD_LENGTH:])
    expected_chk = compute_adler32(body)
    if chk != expected_chk:
        raise ValueError(f"Checksum mismatch: got {hex(chk)}, expected {hex(expected_chk)}")
    (
        magic,
        version,
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
    total = (tot_hi << 32) | tot_lo
    return {
        "magic": magic,
        "version": version,
        "metric_id": metric_id,
        "ts_sec": ts_sec,
        "ts_msec": ts_msec,
        "capacity": capacity,
        "count": count,
        "total_samples": total,
        "min": min_v,
        "max": max_v,
        "avg": avg_v,
    }


def verify():
    agg = TelemetryAggregator(capacity=100, metric_id=1)
    # Test vector verification
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
    assert frame_vec.hex() == expected_hex
    dec = decode_telemetry_frame(frame_vec)
    assert dec["metric_id"] == 1
    assert dec["ts_sec"] == 1700000000
    assert abs(dec["min"] - 10.5) < 1e-9
    assert abs(dec["max"] - 98.2) < 1e-9
    assert abs(dec["avg"] - 54.35) < 1e-9

    # Sliding window test
    agg2 = TelemetryAggregator(capacity=3, metric_id=1)
    agg2.add_sample(10.0)
    agg2.add_sample(20.0)
    agg2.add_sample(30.0)
    st = agg2.get_stats()
    assert st["count"] == 3 and st["min"] == 10.0 and st["max"] == 30.0 and st["avg"] == 20.0
    agg2.add_sample(40.0)
    st2 = agg2.get_stats()
    assert st2["count"] == 3 and st2["min"] == 20.0 and st2["max"] == 40.0 and st2["avg"] == 30.0

    print("[PASS] Vector 56-Byte Telemetry Frame")
    print("[PASS] Sliding Window Eviction Mechanics")
    print("[TELEMETRY] ALL TESTS PASSED")
    print("OK")


def benchmark(samples: int, window_size: int):
    agg = TelemetryAggregator(capacity=window_size, metric_id=1)
    compaction_interval = 5000
    frames_gen = 0
    t0 = time.perf_counter()
    for p in range(samples):
        val = 20.0 + (p % 100) * 0.5 + (1.0 if p % 2 == 0 else -1.0)
        agg.add_sample(val)
        if agg.total_samples % compaction_interval == 0:
            agg.encode_frame(1700000000 + p // 1000, 0)
            frames_gen += 1
    elapsed_ms = (time.perf_counter() - t0) * 1000.0
    sps = int(samples / (elapsed_ms / 1000.0)) if elapsed_ms > 0 else 0
    stats = agg.get_stats()
    print(f"elapsed_ms={elapsed_ms:.3f} throughput_samples_s={sps} frames_generated={frames_gen} last_mean={stats['avg']:.6f}")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--benchmark":
        s = int(sys.argv[2]) if len(sys.argv) > 2 else 50000
        w = int(sys.argv[3]) if len(sys.argv) > 3 else 1000
        benchmark(s, w)
    else:
        verify()
