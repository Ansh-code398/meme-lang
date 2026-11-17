#!/usr/bin/env python3
# ==============================================================================
# App 1: Packet Framer & Circular Ring Buffer (Python Standard Library Reference)
# ==============================================================================

import sys
import time
import struct
import zlib

MAGIC_HEADER = 0xAA55
HEADER_SIZE = 8
TRAILER_SIZE = 4
MIN_FRAME_SIZE = HEADER_SIZE + TRAILER_SIZE


def compute_adler32(data: bytes) -> int:
    return zlib.adler32(data) & 0xFFFFFFFF


def pack_frame(seq_id: int, payload: bytes) -> bytes:
    header = struct.pack(">HIH", MAGIC_HEADER, seq_id, len(payload))
    checksum = compute_adler32(payload)
    trailer = struct.pack(">I", checksum)
    return header + payload + trailer


def unpack_frame(frame: bytes):
    if len(frame) < MIN_FRAME_SIZE:
        raise ValueError(f"Frame truncated: {len(frame)} < {MIN_FRAME_SIZE}")
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


class RingBuffer:
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
            return b""
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

    def pop_frame(self):
        while self.size >= MIN_FRAME_SIZE:
            hdr = self.peek(0, HEADER_SIZE)
            if len(hdr) < HEADER_SIZE:
                return None
            magic, seq_id, p_len = struct.unpack(">HIH", hdr)
            if magic == MAGIC_HEADER:
                total = HEADER_SIZE + p_len + TRAILER_SIZE
                if self.size >= total:
                    frame_bytes = self.read(total)
                    try:
                        return unpack_frame(frame_bytes)
                    except ValueError:
                        pass
                else:
                    return None
            else:
                self.skip(1)
        return None


def run_verify():
    # Vector 1.1: Empty payload
    f1 = pack_frame(0, b"")
    assert f1.hex() == "aa5500000000000000000001"
    assert unpack_frame(f1) == (0, b"")

    # Vector 1.2: PING
    f2 = pack_frame(1, b"PING")
    assert f2.hex() == "aa5500000001000450494e470302012f"
    assert unpack_frame(f2) == (1, b"PING")

    # Vector 1.3: HELLO MEMELANG
    f3 = pack_frame(42, b"HELLO MEMELANG")
    assert f3.hex() == "aa550000002a000e48454c4c4f204d454d454c414e471cd103db"
    assert unpack_frame(f3) == (42, b"HELLO MEMELANG")

    # Vector 1.4: 16 Zeros
    f4 = pack_frame(100, b"\x00" * 16)
    assert f4[-4:].hex() == "00100001"
    assert unpack_frame(f4) == (100, b"\x00" * 16)

    # Vector 1.5: 16 Ones
    f5 = pack_frame(101, b"\xFF" * 16)
    assert f5[-4:].hex() == "87880ff1"
    assert unpack_frame(f5) == (101, b"\xFF" * 16)

    # Ring Buffer Wrap-around
    rb = RingBuffer(64)
    rb.write(b"A" * 50)
    rb.read(40)
    assert rb.write(f3) == 26
    rb.read(10)
    assert rb.read(26) == f3

    # Corrupted stream resynchronization
    garbage = bytes.fromhex("ffdeadbeef00")
    rb_sync = RingBuffer(64)
    rb_sync.write(garbage + f2)
    assert rb_sync.pop_frame() == (1, b"PING")

    print("[PACKET_FRAMER] ALL TESTS PASSED [PASS] OK")


def run_benchmark(iterations: int, payload_size: int):
    payload = b"\x42" * payload_size
    rb = RingBuffer(262144)
    frame = pack_frame(1, payload)
    total_bytes = len(frame) * iterations

    t0 = time.perf_counter()
    frames_processed = 0
    checksum_acc = 0

    for _ in range(iterations):
        rb.write(frame)
        res = rb.pop_frame()
        if res is not None:
            frames_processed += 1
            checksum_acc = (checksum_acc + compute_adler32(res[1])) & 0xFFFFFFFFFFFFFFFF

    t1 = time.perf_counter()
    elapsed_sec = max(t1 - t0, 1e-9)
    elapsed_ms = elapsed_sec * 1000.0
    throughput_mb_s = (total_bytes / (1024.0 * 1024.0)) / elapsed_sec

    print(f"elapsed_ms={elapsed_ms:.2f} throughput_mb_s={throughput_mb_s:.2f} frames_processed={frames_processed} checksum_acc={checksum_acc}")


def main():
    if len(sys.argv) >= 2 and sys.argv[1] == "--benchmark":
        iters = int(sys.argv[2]) if len(sys.argv) > 2 else 10000
        p_size = int(sys.argv[3]) if len(sys.argv) > 3 else 256
        run_benchmark(iters, p_size)
    else:
        run_verify()


if __name__ == "__main__":
    main()
