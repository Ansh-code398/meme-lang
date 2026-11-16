#[no_mangle]
pub extern "C" fn rust_fast_sigmoid(x: f64) -> f64 {
    1.0 / (1.0 + (-x).exp())
}

#[no_mangle]
pub extern "C" fn rust_dot_product(a: *const f64, b: *const f64, len: usize) -> f64 {
    let s_a = unsafe { std::slice::from_raw_parts(a, len) };
    let s_b = unsafe { std::slice::from_raw_parts(b, len) };
    s_a.iter().zip(s_b.iter()).map(|(x, y)| x * y).sum()
}

#[no_mangle]
pub extern "C" fn rust_cosine_similarity(a: *const f64, b: *const f64, len: usize) -> f64 {
    let s_a = unsafe { std::slice::from_raw_parts(a, len) };
    let s_b = unsafe { std::slice::from_raw_parts(b, len) };
    let mut dot = 0.0;
    let mut n1 = 0.0;
    let mut n2 = 0.0;
    for i in 0..len {
        dot += s_a[i] * s_b[i];
        n1 += s_a[i] * s_a[i];
        n2 += s_b[i] * s_b[i];
    }
    let d = (n1 * n2).sqrt();
    if d == 0.0 { 0.0 } else { dot / d }
}
