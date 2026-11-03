# Benchmark 3: Cryptographic Hashing & Token Signing in Python
import json
import hashlib
import hmac
import base64
import time

start = time.perf_counter()

with open("benchmarks/data/auth_payloads.json", "r") as f:
    payloads = json.load(f)

secret = b"super_secret_jwt_hmac_signing_key_2026"
verified_count = 0
last_signature = ""

for p in payloads:
    json_str = json.dumps(p)
    sha = hashlib.sha256(json_str.encode("utf-8")).hexdigest()
    b64 = base64.b64encode(sha.encode("utf-8")).decode("utf-8")
    sig = hmac.new(secret, b64.encode("utf-8"), hashlib.sha256).hexdigest()

    expected = hmac.new(secret, b64.encode("utf-8"), hashlib.sha256).hexdigest()
    if sig == expected:
        verified_count += 1
    last_signature = sig

elapsed = (time.perf_counter() - start) * 1000.0
print(f"Python3  | Tokens: {len(payloads)} | Verified: {verified_count} | HashLen: {len(last_signature)} | Time: {elapsed:.2f}ms")
