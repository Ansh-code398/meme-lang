// Benchmark 3: Cryptographic Hashing & Token Signing in JavaScript (Node/Bun)
const fs = require('fs');
const crypto = require('crypto');

const start = performance.now();

const raw = fs.readFileSync('benchmarks/data/auth_payloads.json', 'utf8');
const payloads = JSON.parse(raw);

const secret = 'super_secret_jwt_hmac_signing_key_2026';
let verifiedCount = 0;
let lastSignature = '';

for (const p of payloads) {
    const jsonStr = JSON.stringify(p);
    const sha = crypto.createHash('sha256').update(jsonStr).digest('hex');
    const b64 = Buffer.from(sha).toString('base64');
    const sig = crypto.createHmac('sha256', secret).update(b64).digest('hex');

    const expected = crypto.createHmac('sha256', secret).update(b64).digest('hex');
    if (sig === expected) {
        verifiedCount++;
    }
    lastSignature = sig;
}

const elapsed = performance.now() - start;
const runtime = typeof Bun !== 'undefined' ? 'Bun      ' : 'Node.js  ';
console.log(`${runtime}| Tokens: ${payloads.length} | Verified: ${verifiedCount} | HashLen: ${lastSignature.length} | Time: ${elapsed.toFixed(2)}ms`);
