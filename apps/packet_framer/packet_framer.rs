// ==============================================================================
// App 1: Packet Framer & Circular Ring Buffer (Standalone Rust - Std Only)
// ==============================================================================

use std::env;
use std::time::Instant;

const MAGIC_HEADER: u16 = 0xAA55;
const HEADER_SIZE: usize = 8;
const TRAILER_SIZE: usize = 4;
const MIN_FRAME_SIZE: usize = HEADER_SIZE + TRAILER_SIZE;
const MOD_ADLER: u32 = 65521;

pub fn compute_adler32(data: &[u8]) -> u32 {
    let mut s1: u32 = 1;
    let mut s2: u32 = 0;
    for &b in data {
        s1 = (s1 + b as u32) % MOD_ADLER;
        s2 = (s2 + s1) % MOD_ADLER;
    }
    (s2 << 16) | s1
}

pub fn pack_frame(seq_id: u32, payload: &[u8]) -> Vec<u8> {
    let p_len = payload.len();
    let mut frame = Vec::with_capacity(HEADER_SIZE + p_len + TRAILER_SIZE);
    frame.extend_from_slice(&MAGIC_HEADER.to_be_bytes());
    frame.extend_from_slice(&seq_id.to_be_bytes());
    frame.extend_from_slice(&(p_len as u16).to_be_bytes());
    frame.extend_from_slice(payload);
    let chk = compute_adler32(payload);
    frame.extend_from_slice(&chk.to_be_bytes());
    frame
}

pub fn unpack_frame(frame: &[u8]) -> Result<(u32, Vec<u8>), &'static str> {
    if frame.len() < MIN_FRAME_SIZE {
        return Err("Frame truncated");
    }
    let magic = u16::from_be_bytes([frame[0], frame[1]]);
    if magic != MAGIC_HEADER {
        return Err("Invalid magic");
    }
    let seq_id = u32::from_be_bytes([frame[2], frame[3], frame[4], frame[5]]);
    let p_len = u16::from_be_bytes([frame[6], frame[7]]) as usize;
    if frame.len() < HEADER_SIZE + p_len + TRAILER_SIZE {
        return Err("Frame incomplete");
    }
    let payload = &frame[HEADER_SIZE..HEADER_SIZE + p_len];
    let chk_offset = HEADER_SIZE + p_len;
    let chk = u32::from_be_bytes([
        frame[chk_offset],
        frame[chk_offset + 1],
        frame[chk_offset + 2],
        frame[chk_offset + 3],
    ]);
    let expected_chk = compute_adler32(payload);
    if chk != expected_chk {
        return Err("Checksum mismatch");
    }
    Ok((seq_id, payload.to_vec()))
}

pub struct RingBuffer {
    buffer: Vec<u8>,
    capacity: usize,
    head: usize,
    tail: usize,
    size: usize,
}

impl RingBuffer {
    pub fn new(capacity: usize) -> Self {
        Self {
            buffer: vec![0u8; capacity],
            capacity,
            head: 0,
            tail: 0,
            size: 0,
        }
    }

    pub fn free_space(&self) -> usize {
        self.capacity - self.size
    }

    pub fn available(&self) -> usize {
        self.size
    }

    pub fn write(&mut self, data: &[u8]) -> usize {
        let to_write = data.len().min(self.free_space());
        for i in 0..to_write {
            self.buffer[self.head] = data[i];
            self.head = (self.head + 1) % self.capacity;
        }
        self.size += to_write;
        to_write
    }

    pub fn read(&mut self, length: usize) -> Vec<u8> {
        let to_read = length.min(self.size);
        let mut res = vec![0u8; to_read];
        for i in 0..to_read {
            res[i] = self.buffer[self.tail];
            self.tail = (self.tail + 1) % self.capacity;
        }
        self.size -= to_read;
        res
    }

    pub fn peek(&self, offset: usize, length: usize) -> Option<Vec<u8>> {
        if offset + length > self.size {
            return None;
        }
        let mut res = vec![0u8; length];
        let mut idx = (self.tail + offset) % self.capacity;
        for i in 0..length {
            res[i] = self.buffer[idx];
            idx = (idx + 1) % self.capacity;
        }
        Some(res)
    }

    pub fn skip(&mut self, length: usize) {
        let to_skip = length.min(self.size);
        self.tail = (self.tail + to_skip) % self.capacity;
        self.size -= to_skip;
    }

    pub fn pop_frame(&mut self) -> Option<(u32, Vec<u8>)> {
        while self.size >= MIN_FRAME_SIZE {
            let hdr = match self.peek(0, HEADER_SIZE) {
                Some(h) => h,
                None => return None,
            };
            let magic = u16::from_be_bytes([hdr[0], hdr[1]]);
            if magic == MAGIC_HEADER {
                let p_len = u16::from_be_bytes([hdr[6], hdr[7]]) as usize;
                let frame_total = HEADER_SIZE + p_len + TRAILER_SIZE;
                if self.size >= frame_total {
                    let frame_bytes = self.read(frame_total);
                    if let Ok((seq, payload)) = unpack_frame(&frame_bytes) {
                        return Some((seq, payload));
                    }
                } else {
                    return None;
                }
            } else {
                self.skip(1);
            }
        }
        None
    }
}

fn run_verify() {
    // Vector 1.1: Empty payload
    let f1 = pack_frame(0, b"");
    assert_eq!(unpack_frame(&f1).unwrap(), (0, b"".to_vec()));

    // Vector 1.2: PING
    let f2 = pack_frame(1, b"PING");
    assert_eq!(unpack_frame(&f2).unwrap(), (1, b"PING".to_vec()));

    // Vector 1.3: HELLO MEMELANG
    let f3 = pack_frame(42, b"HELLO MEMELANG");
    assert_eq!(unpack_frame(&f3).unwrap(), (42, b"HELLO MEMELANG".to_vec()));

    // Vector 1.4: 16 Zeros
    let f4 = pack_frame(100, &vec![0u8; 16]);
    assert_eq!(compute_adler32(&vec![0u8; 16]), 0x00100001);
    assert_eq!(unpack_frame(&f4).unwrap(), (100, vec![0u8; 16]));

    // Vector 1.5: 16 Ones
    let f5 = pack_frame(101, &vec![0xFFu8; 16]);
    assert_eq!(compute_adler32(&vec![0xFFu8; 16]), 0x87880FF1);
    assert_eq!(unpack_frame(&f5).unwrap(), (101, vec![0xFFu8; 16]));

    // Ring Buffer Wrap-around
    let mut rb = RingBuffer::new(64);
    rb.write(&vec![b'A'; 50]);
    rb.read(40);
    let w = rb.write(&f3);
    assert_eq!(w, 26);
    rb.read(10);
    let rec = rb.read(26);
    assert_eq!(rec, f3);

    // Corrupted stream resync
    let mut stream = vec![0xFF, 0xDE, 0xAD, 0xBE, 0xEF, 0x00];
    stream.extend_from_slice(&f2);
    let mut rb_sync = RingBuffer::new(64);
    rb_sync.write(&stream);
    assert_eq!(rb_sync.pop_frame().unwrap(), (1, b"PING".to_vec()));

    println!("[PACKET_FRAMER] ALL TESTS PASSED [PASS] OK");
}

fn run_benchmark(iterations: usize, payload_size: usize) {
    let payload = vec![0x42u8; payload_size];
    let mut rb = RingBuffer::new(262144);
    let frame = pack_frame(1, &payload);
    let frame_len = frame.len();
    let total_bytes = frame_len * iterations;

    let start = Instant::now();
    let mut frames_processed = 0;
    let mut checksum_acc: u64 = 0;

    for _ in 0..iterations {
        rb.write(&frame);
        if let Some((_seq, p)) = rb.pop_frame() {
            frames_processed += 1;
            checksum_acc = checksum_acc.wrapping_add(compute_adler32(&p) as u64);
        }
    }

    let elapsed = start.elapsed();
    let elapsed_ms = elapsed.as_secs_f64() * 1000.0;
    let throughput_mb_s = (total_bytes as f64 / (1024.0 * 1024.0)) / elapsed.as_secs_f64();

    println!(
        "elapsed_ms={:.2} throughput_mb_s={:.2} frames_processed={} checksum_acc={}",
        elapsed_ms, throughput_mb_s, frames_processed, checksum_acc
    );
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() >= 2 && args[1] == "--benchmark" {
        let iters = args.get(2).and_then(|s| s.parse().ok()).unwrap_or(10000);
        let p_size = args.get(3).and_then(|s| s.parse().ok()).unwrap_or(256);
        run_benchmark(iters, p_size);
    } else {
        run_verify();
    }
}
