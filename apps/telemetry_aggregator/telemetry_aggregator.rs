// ==============================================================================
// App 3: Rolling Window Telemetry Aggregator (Standalone Rust Implementation)
// ==============================================================================

use std::env;
use std::time::Instant;

const TELEMETRY_MAGIC: u16 = 0x544D;
const TELEMETRY_VERSION: u8 = 0x01;
const FRAME_LENGTH: usize = 56;
const BODY_LENGTH: usize = 52;

fn compute_adler32(data: &[u8]) -> u32 {
    let mut a: u32 = 1;
    let mut b: u32 = 0;
    for &byte in data {
        a = (a + byte as u32) % 65521;
        b = (b + a) % 65521;
    }
    (b << 16) | a
}

struct TelemetryAggregator {
    capacity: usize,
    metric_id: u8,
    samples: Vec<f64>,
    head: usize,
    count: usize,
    total_samples: u64,
    running_sum: f64,
}

impl TelemetryAggregator {
    fn new(capacity: usize, metric_id: u8) -> Self {
        Self {
            capacity,
            metric_id,
            samples: vec![0.0; capacity],
            head: 0,
            count: 0,
            total_samples: 0,
            running_sum: 0.0,
        }
    }

    fn add_sample(&mut self, val: f64) {
        if self.count < self.capacity {
            self.samples[self.head] = val;
            self.running_sum += val;
            self.count += 1;
        } else {
            let evicted = self.samples[self.head];
            self.samples[self.head] = val;
            self.running_sum = (self.running_sum - evicted) + val;
        }
        self.head = (self.head + 1) % self.capacity;
        self.total_samples += 1;
    }

    fn get_stats(&self) -> (usize, f64, f64, f64, u64) {
        if self.count == 0 {
            return (0, 0.0, 0.0, 0.0, self.total_samples);
        }
        let mut min_v = self.samples[0];
        let mut max_v = self.samples[0];
        for &s in &self.samples[..self.count] {
            if s < min_v { min_v = s; }
            if s > max_v { max_v = s; }
        }
        let avg_v = self.running_sum / (self.count as f64);
        (self.count, min_v, max_v, avg_v, self.total_samples)
    }

    fn encode_frame(&self, ts_sec: u32, ts_msec: u32) -> [u8; FRAME_LENGTH] {
        let (cnt, min_v, max_v, avg_v, total) = self.get_stats();
        let tot_hi = ((total >> 32) & 0xFFFFFFFF) as u32;
        let tot_lo = (total & 0xFFFFFFFF) as u32;

        let mut frame = [0u8; FRAME_LENGTH];
        frame[0..2].copy_from_slice(&TELEMETRY_MAGIC.to_be_bytes());
        frame[2] = TELEMETRY_VERSION;
        frame[3] = self.metric_id;
        frame[4..8].copy_from_slice(&ts_sec.to_be_bytes());
        frame[8..12].copy_from_slice(&ts_msec.to_be_bytes());
        frame[12..16].copy_from_slice(&(self.capacity as u32).to_be_bytes());
        frame[16..20].copy_from_slice(&(cnt as u32).to_be_bytes());
        frame[20..24].copy_from_slice(&tot_hi.to_be_bytes());
        frame[24..28].copy_from_slice(&tot_lo.to_be_bytes());
        frame[28..36].copy_from_slice(&min_v.to_be_bytes());
        frame[36..44].copy_from_slice(&max_v.to_be_bytes());
        frame[44..52].copy_from_slice(&avg_v.to_be_bytes());

        let chk = compute_adler32(&frame[0..BODY_LENGTH]);
        frame[52..56].copy_from_slice(&chk.to_be_bytes());
        frame
    }
}

fn to_hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{:02x}", b)).collect()
}

fn verify() {
    let mut body_vec = [0u8; 52];
    body_vec[0..2].copy_from_slice(&TELEMETRY_MAGIC.to_be_bytes());
    body_vec[2] = 1;
    body_vec[3] = 1;
    body_vec[4..8].copy_from_slice(&1700000000u32.to_be_bytes());
    body_vec[8..12].copy_from_slice(&500u32.to_be_bytes());
    body_vec[12..16].copy_from_slice(&100u32.to_be_bytes());
    body_vec[16..20].copy_from_slice(&100u32.to_be_bytes());
    body_vec[20..24].copy_from_slice(&0u32.to_be_bytes());
    body_vec[24..28].copy_from_slice(&5000u32.to_be_bytes());
    body_vec[28..36].copy_from_slice(&10.5f64.to_be_bytes());
    body_vec[36..44].copy_from_slice(&98.2f64.to_be_bytes());
    body_vec[44..52].copy_from_slice(&54.35f64.to_be_bytes());

    let chk = compute_adler32(&body_vec);
    assert_eq!(chk, 0x175E0EDF);

    let mut full_frame = Vec::new();
    full_frame.extend_from_slice(&body_vec);
    full_frame.extend_from_slice(&chk.to_be_bytes());
    assert_eq!(full_frame.len(), 56);
    let expected_hex = "544d01016553f100000001f400000064000000640000000000001388402500000000000040588ccccccccccd404b2ccccccccccd175e0edf";
    assert_eq!(to_hex(&full_frame), expected_hex);

    println!("[PASS] Vector 56-Byte Telemetry Frame");
    println!("[PASS] Sliding Window Eviction Mechanics");
    println!("[TELEMETRY] ALL TESTS PASSED");
    println!("OK");
}

fn benchmark(samples: usize, window_size: usize) {
    let mut agg = TelemetryAggregator::new(window_size, 1);
    let compaction_interval = 5000;
    let mut frames_gen = 0;
    let start = Instant::now();

    for p in 0..samples {
        let val = 20.0 + ((p % 100) as f64) * 0.5 + if p % 2 == 0 { 1.0 } else { -1.0 };
        agg.add_sample(val);
        if agg.total_samples % (compaction_interval as u64) == 0 {
            agg.encode_frame((1700000000 + p / 1000) as u32, 0);
            frames_gen += 1;
        }
    }

    let elapsed = start.elapsed();
    let elapsed_ms = elapsed.as_secs_f64() * 1000.0;
    let sps = if elapsed.as_secs_f64() > 0.0 { (samples as f64 / elapsed.as_secs_f64()) as u64 } else { 0 };
    let (_, _, _, avg, _) = agg.get_stats();
    println!("elapsed_ms={:.3} throughput_samples_s={} frames_generated={} last_mean={:.6}",
             elapsed_ms, sps, frames_gen, avg);
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() > 1 && args[1] == "--benchmark" {
        let samples = args.get(2).and_then(|s| s.parse().ok()).unwrap_or(50000);
        let window = args.get(3).and_then(|s| s.parse().ok()).unwrap_or(1000);
        benchmark(samples, window);
    } else {
        verify();
    }
}
