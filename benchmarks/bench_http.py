import subprocess
import time
import urllib.request
import os
import signal

def benchmark_server(name, start_cmd, port, total_requests=1000):
    print(f"\n🌐 Benchmarking {name} on port {port} ({total_requests} requests)...")
    proc = subprocess.Popen(start_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    time.sleep(1.0) # Wait for socket binding

    url = f"http://127.0.0.1:{port}/api/user"
    latencies = []

    # Send first test request to confirm readiness
    connected = False
    for attempt in range(10):
        try:
            with urllib.request.urlopen(url, timeout=2) as res:
                if res.status == 200:
                    connected = True
                    break
        except Exception:
            time.sleep(0.2)

    if not connected:
        print(f"  ❌ Failed to connect to {name} on port {port}")
        proc.terminate()
        return None

    t0 = time.perf_counter()
    for _ in range(total_requests):
        req_start = time.perf_counter()
        with urllib.request.urlopen(url, timeout=5) as res:
            res.read()
        latencies.append((time.perf_counter() - req_start) * 1000.0)

    total_time = time.perf_counter() - t0
    rps = total_requests / total_time
    avg_latency = sum(latencies) / len(latencies)

    # Clean shutdown
    proc.terminate()
    try:
        proc.wait(timeout=2)
    except:
        proc.kill()

    print(f"  ⚡ {name:<12}: {rps:8.1f} req/s | Avg Latency: {avg_latency:.2f} ms | Total Time: {total_time:.2f} s")
    return {"rps": round(rps, 1), "avg_latency_ms": round(avg_latency, 2), "total_time_s": round(total_time, 2)}

# 1. MEMElang Server
meme_cmd = ["./meme", "-e", """
app = express();
app.get("/api/user", (req, res) => {
    res.status(200).json({ "status": "ok", "user": "Raju", "role": "admin" });
});
app.listen(19876);
jabtak han {
    ruk(1000);
}
"""]

# 2. Node.js Server
node_server_code = """
const http = require('http');
const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ status: 'ok', user: 'Raju', role: 'admin' }));
});
server.listen(19877, '127.0.0.1');
"""
with open("benchmarks/server_node.js", "w") as f:
    f.write(node_server_code)
node_cmd = ["node", "benchmarks/server_node.js"]

# 3. Bun Server
bun_server_code = """
Bun.serve({
    port: 19878,
    hostname: '127.0.0.1',
    fetch(req) {
        return Response.json({ status: 'ok', user: 'Raju', role: 'admin' });
    }
});
"""
with open("benchmarks/server_bun.js", "w") as f:
    f.write(bun_server_code)
bun_cmd = ["bun", "benchmarks/server_bun.js"]

# 4. Python Server
py_server_code = """
from http.server import HTTPServer, BaseHTTPRequestHandler
import json

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/api/user':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(b'{"status":"ok","user":"Raju","role":"admin"}')
        else:
            self.send_response(404)
            self.end_headers()
    def log_message(self, format, *args):
        pass

server = HTTPServer(('127.0.0.1', 19879), Handler)
server.serve_forever()
"""
with open("benchmarks/server_py.py", "w") as f:
    f.write(py_server_code)
py_cmd = ["python3", "benchmarks/server_py.py"]

meme_res = benchmark_server("MEMElang", meme_cmd, 19876, 1000)
node_res = benchmark_server("Node.js", node_cmd, 19877, 1000)
bun_res = benchmark_server("Bun", bun_cmd, 19878, 1000)
py_res = benchmark_server("Python3", py_cmd, 19879, 1000)

print("\n" + "=" * 65)
print("🏆 REAL HTTP SERVER BENCHMARK RESULTS (1,000 Real HTTP Requests)")
print("=" * 65)
print(f"{'Server Engine':<16} | {'Throughput (req/s)':<20} | {'Avg Latency':<12}")
print("-" * 65)
if meme_res:
    print(f"{'MEMElang':<16} | {meme_res['rps']:>18} req/s | {meme_res['avg_latency_ms']:>8.2f} ms")
if node_res:
    print(f"{'Node.js':<16} | {node_res['rps']:>18} req/s | {node_res['avg_latency_ms']:>8.2f} ms")
if bun_res:
    print(f"{'Bun':<16} | {bun_res['rps']:>18} req/s | {bun_res['avg_latency_ms']:>8.2f} ms")
if py_res:
    print(f"{'Python3':<16} | {py_res['rps']:>18} req/s | {py_res['avg_latency_ms']:>8.2f} ms")
print("=" * 65)
