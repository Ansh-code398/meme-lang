// Rust AI Benchmark: Multi-Layer Perceptron (MLP) Neural Network
// Architecture: 2 Inputs -> 4 Hidden Neurons (Tanh) -> 1 Output (Sigmoid)
// Dataset: Non-linear XOR Problem (200 training samples)

use std::time::Instant;

fn sigmoid(x: f64) -> f64 {
    1.0 / (1.0 + (-x).exp())
}

fn main() {
    println!("==========================================================");
    println!("🦀 Rust Neural Network Training Benchmark (opt-level=3)");
    println!("==========================================================");

    let mut x_data: Vec<[f64; 2]> = Vec::with_capacity(200);
    let mut y_data: Vec<f64> = Vec::with_capacity(200);
    let mut seed: u64 = 1234567;

    for s_idx in 0..200 {
        seed = (seed.wrapping_mul(1103515245).wrapping_add(12345)) % 2147483648;
        let r1 = ((seed % 1000) as f64) / 1000.0;
        seed = (seed.wrapping_mul(1103515245).wrapping_add(12345)) % 2147483648;
        let r2 = ((seed % 1000) as f64) / 1000.0;

        let mut x1 = if s_idx % 2 == 0 { 0.0 } else { 1.0 };
        let mut x2 = if (s_idx / 2) % 2 == 0 { 0.0 } else { 1.0 };
        let y = if (x1 != 0.0) ^ (x2 != 0.0) { 1.0 } else { 0.0 };

        x1 += r1 * 0.1 - 0.05;
        x2 += r2 * 0.1 - 0.05;

        x_data.push([x1, x2]);
        y_data.push(y);
    }

    let mut w1 = [
        [0.15, 0.25, 0.35, 0.45],
        [0.55, 0.65, 0.75, 0.85]
    ];
    let mut b1 = [0.1, 0.1, 0.1, 0.1];
    let mut w2 = [0.2, 0.4, 0.6, 0.8];
    let mut b2 = 0.5;

    let learning_rate = 0.25;
    let epochs = 1000;
    let num_samples = 200;

    let start = Instant::now();
    let mut last_loss = 0.0;

    for _ep in 0..epochs {
        let mut total_loss = 0.0;
        for i in 0..num_samples {
            let [x1, x2] = x_data[i];
            let y_true = y_data[i];

            // Forward
            let h0 = (x1 * w1[0][0] + x2 * w1[1][0] + b1[0]).tanh();
            let h1 = (x1 * w1[0][1] + x2 * w1[1][1] + b1[1]).tanh();
            let h2 = (x1 * w1[0][2] + x2 * w1[1][2] + b1[2]).tanh();
            let h3 = (x1 * w1[0][3] + x2 * w1[1][3] + b1[3]).tanh();

            let z2 = h0 * w2[0] + h1 * w2[1] + h2 * w2[2] + h3 * w2[3] + b2;
            let y_pred = sigmoid(z2);

            let err = y_pred - y_true;
            total_loss += err * err;

            // Backward
            let d_out = err * y_pred * (1.0 - y_pred);
            let d_h0 = d_out * w2[0] * (1.0 - h0 * h0);
            let d_h1 = d_out * w2[1] * (1.0 - h1 * h1);
            let d_h2 = d_out * w2[2] * (1.0 - h2 * h2);
            let d_h3 = d_out * w2[3] * (1.0 - h3 * h3);

            w2[0] -= learning_rate * d_out * h0;
            w2[1] -= learning_rate * d_out * h1;
            w2[2] -= learning_rate * d_out * h2;
            w2[3] -= learning_rate * d_out * h3;
            b2 -= learning_rate * d_out;

            w1[0][0] -= learning_rate * d_h0 * x1;
            w1[1][0] -= learning_rate * d_h0 * x2;
            b1[0] -= learning_rate * d_h0;

            w1[0][1] -= learning_rate * d_h1 * x1;
            w1[1][1] -= learning_rate * d_h1 * x2;
            b1[1] -= learning_rate * d_h1;

            w1[0][2] -= learning_rate * d_h2 * x1;
            w1[1][2] -= learning_rate * d_h2 * x2;
            b1[2] -= learning_rate * d_h2;

            w1[0][3] -= learning_rate * d_h3 * x1;
            w1[1][3] -= learning_rate * d_h3 * x2;
            b1[3] -= learning_rate * d_h3;
        }
        last_loss = total_loss / (num_samples as f64);
    }

    let elapsed = start.elapsed().as_secs_f64();

    let mut correct = 0;
    for i in 0..num_samples {
        let [x1, x2] = x_data[i];
        let h0 = (x1 * w1[0][0] + x2 * w1[1][0] + b1[0]).tanh();
        let h1 = (x1 * w1[0][1] + x2 * w1[1][1] + b1[1]).tanh();
        let h2 = (x1 * w1[0][2] + x2 * w1[1][2] + b1[2]).tanh();
        let h3 = (x1 * w1[0][3] + x2 * w1[1][3] + b1[3]).tanh();
        let y_pred = sigmoid(h0 * w2[0] + h1 * w2[1] + h2 * w2[2] + h3 * w2[3] + b2);
        let label = if y_pred >= 0.5 { 1.0 } else { 0.0 };
        if label == y_data[i] {
            correct += 1;
        }
    }

    let acc = (correct as f64 * 100.0) / (num_samples as f64);
    let throughput = (epochs as f64 * num_samples as f64) / elapsed;

    println!("Training Epochs: {}", epochs);
    println!("Total Samples: {}", num_samples);
    println!("Total Backprop Steps: {}", epochs * num_samples);
    println!("Final Mean Squared Loss: {:.6}", last_loss);
    println!("Classification Accuracy (%): {:.1}", acc);
    println!("Elapsed Time (seconds): {:.6}", elapsed);
    println!("Throughput (samples/sec): {:.2}", throughput);
}
