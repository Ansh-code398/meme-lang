// Standalone Rust implementation for App 2 TLV Codec
// Compile with: rustc -O apps/tlv_codec/tlv_codec.rs -o ...
use std::convert::TryInto;
use std::env;
use std::time::Instant;

pub const TAG_INT32: u8 = 0x01;
pub const TAG_FLOAT64: u8 = 0x02;
pub const TAG_STRING: u8 = 0x03;
pub const TAG_BYTES: u8 = 0x04;
pub const TAG_BOOL: u8 = 0x05;

#[derive(Debug, PartialEq, Clone)]
pub enum TlvValue {
    Int32(i32),
    Float64(f64),
    StringVal(String),
    BytesVal(Vec<u8>),
    BoolVal(bool),
}

pub fn encode_tlv(val: &TlvValue) -> Result<Vec<u8>, String> {
    let mut out = Vec::new();
    let (tag, payload): (u8, Vec<u8>) = match val {
        TlvValue::Int32(v) => (TAG_INT32, v.to_be_bytes().to_vec()),
        TlvValue::Float64(v) => (TAG_FLOAT64, v.to_be_bytes().to_vec()),
        TlvValue::StringVal(s) => (TAG_STRING, s.as_bytes().to_vec()),
        TlvValue::BytesVal(b) => (TAG_BYTES, b.clone()),
        TlvValue::BoolVal(b) => (TAG_BOOL, vec![if *b { 1 } else { 0 }]),
    };
    if payload.len() > 65535 {
        return Err(format!("Payload exceeds max uint16 length: {}", payload.len()));
    }
    out.push(tag);
    let len_bytes = (payload.len() as u16).to_be_bytes();
    out.extend_from_slice(&len_bytes);
    out.extend_from_slice(&payload);
    Ok(out)
}

pub fn decode_tlv_record(stream: &[u8], offset: usize) -> Result<(TlvValue, usize), String> {
    if stream.len() - offset < 3 {
        return Err(format!("Incomplete header: available {} bytes < 3", stream.len() - offset));
    }
    let tag = stream[offset];
    let len = u16::from_be_bytes([stream[offset + 1], stream[offset + 2]]) as usize;
    let val_offset = offset + 3;
    if stream.len() - val_offset < len {
        return Err(format!("Truncated value: expected {} bytes, available {}", len, stream.len() - val_offset));
    }
    let raw_val = &stream[val_offset..val_offset + len];
    let next_offset = val_offset + len;

    let val = match tag {
        TAG_INT32 => {
            if len != 4 {
                return Err(format!("Invalid length for INT32: expected 4, got {}", len));
            }
            let arr: [u8; 4] = raw_val.try_into().map_err(|_| "slice conversion failed")?;
            TlvValue::Int32(i32::from_be_bytes(arr))
        }
        TAG_FLOAT64 => {
            if len != 8 {
                return Err(format!("Invalid length for FLOAT64: expected 8, got {}", len));
            }
            let arr: [u8; 8] = raw_val.try_into().map_err(|_| "slice conversion failed")?;
            TlvValue::Float64(f64::from_be_bytes(arr))
        }
        TAG_STRING => {
            let s = String::from_utf8(raw_val.to_vec())
                .map_err(|e| format!("Invalid UTF-8: {}", e))?;
            TlvValue::StringVal(s)
        }
        TAG_BYTES => TlvValue::BytesVal(raw_val.to_vec()),
        TAG_BOOL => {
            if len != 1 {
                return Err(format!("Invalid length for BOOL: expected 1, got {}", len));
            }
            match raw_val[0] {
                0 => TlvValue::BoolVal(false),
                1 => TlvValue::BoolVal(true),
                b => return Err(format!("Invalid BOOL byte: {} (must be 0 or 1)", b)),
            }
        }
        _ => return Err(format!("Unknown tag: {}", tag)),
    };
    Ok((val, next_offset))
}

pub fn decode_tlv_stream(stream: &[u8]) -> Result<Vec<TlvValue>, String> {
    let mut records = Vec::new();
    let mut offset = 0;
    while offset < stream.len() {
        let (rec, next_offset) = decode_tlv_record(stream, offset)?;
        records.push(rec);
        offset = next_offset;
    }
    Ok(records)
}

fn to_hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{:02x}", b)).collect()
}

fn run_verify() {
    assert_eq!(to_hex(&encode_tlv(&TlvValue::Int32(42)).unwrap()), "0100040000002a");
    assert_eq!(to_hex(&encode_tlv(&TlvValue::Float64(3.141592653589793)).unwrap()), "020008400921fb54442d18");
    assert_eq!(to_hex(&encode_tlv(&TlvValue::StringVal("Hello, MEMElang!".to_string())).unwrap()), "03001048656c6c6f2c204d454d456c616e6721");
    assert_eq!(to_hex(&encode_tlv(&TlvValue::BytesVal(vec![0xde, 0xad, 0xbe, 0xef])).unwrap()), "040004deadbeef");
    assert_eq!(to_hex(&encode_tlv(&TlvValue::BoolVal(true)).unwrap()), "05000101");
    assert_eq!(to_hex(&encode_tlv(&TlvValue::BoolVal(false)).unwrap()), "05000100");

    let mut composite = Vec::new();
    composite.extend_from_slice(&encode_tlv(&TlvValue::Int32(101)).unwrap());
    composite.extend_from_slice(&encode_tlv(&TlvValue::StringVal("temp_sensor".to_string())).unwrap());
    composite.extend_from_slice(&encode_tlv(&TlvValue::Float64(23.5)).unwrap());
    composite.extend_from_slice(&encode_tlv(&TlvValue::BoolVal(true)).unwrap());
    assert_eq!(composite.len(), 36);
    assert_eq!(to_hex(&composite), "0100040000006503000b74656d705f73656e736f72020008403780000000000005000101");

    let recs = decode_tlv_stream(&composite).unwrap();
    assert_eq!(recs.len(), 4);

    // Extreme boundaries
    assert_eq!(to_hex(&encode_tlv(&TlvValue::Int32(2147483647)).unwrap()), "0100047fffffff");
    assert_eq!(to_hex(&encode_tlv(&TlvValue::Int32(-2147483648)).unwrap()), "01000480000000");
    assert_eq!(to_hex(&encode_tlv(&TlvValue::Int32(-1)).unwrap()), "010004ffffffff");

    // Large string
    let large_s = "A".repeat(65535);
    let enc_large = encode_tlv(&TlvValue::StringVal(large_s.clone())).unwrap();
    let (dec_large, _) = decode_tlv_record(&enc_large, 0).unwrap();
    assert_eq!(dec_large, TlvValue::StringVal(large_s));

    // Errors
    assert!(decode_tlv_record(&[0x01, 0x00], 0).is_err());
    assert!(decode_tlv_record(&[0x01, 0x00, 0x04, 0x00, 0x00], 0).is_err());
    assert!(decode_tlv_record(&[0x05, 0x00, 0x01, 0x02], 0).is_err());

    println!("[PASS] Vector INT32 42");
    println!("[PASS] Vector FLOAT64 pi");
    println!("[PASS] Vector STRING Hello");
    println!("[PASS] Vector BYTES deadbeef");
    println!("[PASS] Vector BOOL true/false");
    println!("[PASS] Vector Composite 36B Record");
    println!("[PASS] Boundary Cases & Error Rejections");
    println!("[TLV_CODEC] ALL TESTS PASSED");
    println!("OK");
}

fn run_benchmark(iterations: usize) {
    let t0 = Instant::now();
    let mut bytes_enc = 0;
    let mut records_cnt = 0;
    for _ in 0..iterations {
        let mut composite = Vec::new();
        composite.extend_from_slice(&encode_tlv(&TlvValue::Int32(101)).unwrap());
        composite.extend_from_slice(&encode_tlv(&TlvValue::StringVal("temp_sensor".to_string())).unwrap());
        composite.extend_from_slice(&encode_tlv(&TlvValue::Float64(23.5)).unwrap());
        composite.extend_from_slice(&encode_tlv(&TlvValue::BoolVal(true)).unwrap());
        let recs = decode_tlv_stream(&composite).unwrap();
        bytes_enc += composite.len();
        records_cnt += recs.len();
    }
    let elapsed = t0.elapsed();
    let elapsed_ms = elapsed.as_secs_f64() * 1000.0;
    let throughput = if elapsed_ms > 0.0 { (records_cnt as f64) / (elapsed_ms / 1000.0) } else { 0.0 };
    println!("elapsed_ms={:.2} throughput_ops_s={:.2} records_processed={} bytes_encoded={}",
             elapsed_ms, throughput, records_cnt, bytes_enc);
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() > 1 && args[1] == "--benchmark" {
        let iters: usize = if args.len() > 2 { args[2].parse().unwrap_or(10000) } else { 10000 };
        run_benchmark(iters);
        return;
    }
    run_verify();
}
