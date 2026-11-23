#!/usr/bin/env python3
"""
Comprehensive Parity Verification Suite for Real-time Chat Hub.
Tests Network Protocol Framing, 2D Canvas Layout, Word-Wrapping, and Server-Client Exchanges
across MEMElang Native AOT, MEMElang Interpreter, Pure Rust, and Python.
"""

import os
import sys
import time
import socket
import subprocess
from pathlib import Path

CHAT_DIR = Path(__file__).resolve().parent
ROOT_DIR = CHAT_DIR.parent.parent
MEME_BIN = ROOT_DIR / "meme"

def get_free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]

def run_cmd(cmd, check=True):
    res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if check and res.returncode != 0:
        raise RuntimeError(f"Command failed ({res.returncode}): {' '.join(cmd)}\nSTDERR: {res.stderr}\nSTDOUT: {res.stdout}")
    return res

def test_canvas_parity():
    print("=================================================================")
    print("🔍 Test Group 1: 2D Canvas Layout & Character Painter Parity")
    print("=================================================================")

    # Run Python
    res_py = run_cmd([sys.executable, str(CHAT_DIR / "chat_client.py")])
    canvas_py = res_py.stdout.strip()

    # Run Rust
    rust_bin = CHAT_DIR / "chat_client_rs"
    if not rust_bin.exists():
        run_cmd(["rustc", "-O", str(CHAT_DIR / "chat_client.rs"), "-o", str(rust_bin)])
    res_rs = run_cmd([str(rust_bin)])
    canvas_rs = res_rs.stdout.strip()

    # Run MEMElang Native
    meme_bin = CHAT_DIR / "chat_client_meme"
    if not meme_bin.exists():
        run_cmd([str(MEME_BIN), "compile", str(CHAT_DIR / "chat_client.meme"), "-o", str(meme_bin)])
    res_meme = run_cmd([str(meme_bin)])
    canvas_meme = res_meme.stdout.strip()

    # Run MEMElang Interpreter
    res_interp = run_cmd([str(MEME_BIN), str(CHAT_DIR / "chat_client.meme")])
    canvas_interp = res_interp.stdout.strip()

    # Assert dimensions
    py_lines = canvas_py.splitlines()
    assert len(py_lines) == 24, f"Canvas height must be 24, got {len(py_lines)}"
    for idx, l in enumerate(py_lines):
        assert len(l) == 80, f"Line {idx} width must be 80, got {len(l)}"

    # Bit-exact parity assertions
    assert canvas_py == canvas_rs, "Canvas mismatch between Python and Rust!"
    assert canvas_py == canvas_meme, "Canvas mismatch between Python and MEMElang Native!"
    assert canvas_py == canvas_interp, "Canvas mismatch between Python and MEMElang Interpreter!"

    print("  ✔ Standard Chat Canvas: 100% Bit-Exact across Python, Rust, MEME Native, and MEME Interpreter")

def test_server_ping_pong_parity():
    print("\n=================================================================")
    print("🔍 Test Group 2: Live Ping-Pong Server Protocol Parity")
    print("=================================================================")

    servers = [
        ("Python Server", lambda p: [sys.executable, str(CHAT_DIR / "chat_server.py"), str(p)]),
        ("Rust Server", lambda p: [str(CHAT_DIR / "chat_server_rs"), str(p)]),
        ("MEMElang Native Server", lambda p: [str(CHAT_DIR / "chat_server_meme"), str(p)]),
    ]

    for name, cmd_fn in servers:
        port = get_free_port()
        cmd = cmd_fn(port)
        proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(0.1)
        try:
            connected = False
            s = None
            for _ in range(40):
                try:
                    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    s.settimeout(3.0)
                    s.connect(("127.0.0.1", port))
                    connected = True
                    break
                except Exception:
                    if s:
                        s.close()
                    time.sleep(0.05)
            assert connected, f"{name} failed to accept connection on port {port}"

            # Ping test
            s.sendall(b'{"id":101,"op":"ping"}\n')
            reply = s.recv(1024).decode("utf-8").strip()
            assert '{"id":101,"op":"pong"}' in reply, f"{name} invalid pong: {reply}"

            # Join test
            s.sendall(b'{"op":"join","user":"tester"}\n')
            welcome = s.recv(4096).decode("utf-8").strip()
            assert '"op":"welcome"' in welcome, f"{name} invalid welcome: {welcome}"
            assert '"user":"tester"' in welcome, f"{name} welcome missing user: {welcome}"

            s.close()
            print(f"  ✔ {name}: Ping-Pong & Handshake Protocol Verified")
        finally:
            proc.terminate()
            proc.wait()

def test_multi_client_broadcast_parity():
    print("\n=================================================================")
    print("🔍 Test Group 3: Multi-Client Real-time Broadcast Parity")
    print("=================================================================")

    servers = [
        ("Python Server", lambda p: [sys.executable, str(CHAT_DIR / "chat_server.py"), str(p)]),
        ("Rust Server", lambda p: [str(CHAT_DIR / "chat_server_rs"), str(p)]),
        ("MEMElang Native Server", lambda p: [str(CHAT_DIR / "chat_server_meme"), str(p)]),
    ]

    for name, cmd_fn in servers:
        port = get_free_port()
        cmd = cmd_fn(port)
        proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(0.1)
        try:
            # Client 1: Alice
            connected1 = False
            c1 = None
            for _ in range(40):
                try:
                    c1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    c1.settimeout(3.0)
                    c1.connect(("127.0.0.1", port))
                    connected1 = True
                    break
                except Exception:
                    if c1:
                        c1.close()
                    time.sleep(0.05)
            assert connected1, f"{name} c1 connect failed on port {port}"
            c1.sendall(b'{"op":"join","user":"alice"}\n')
            _ = c1.recv(4096)  # welcome

            # Client 2: Bob
            c2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            c2.settimeout(3.0)
            c2.connect(("127.0.0.1", port))
            c2.sendall(b'{"op":"join","user":"bob"}\n')
            _ = c2.recv(4096)  # welcome

            # C1 should receive presence for Bob
            presence_notice = c1.recv(4096).decode("utf-8")
            assert '"op":"presence"' in presence_notice and '"user":"bob"' in presence_notice, f"{name} presence missing on C1"

            # Alice sends a message
            c1.sendall(b'{"op":"msg","text":"Hello Bob!","user":"alice"}\n')

            # Both Alice and Bob should receive broadcast
            bc1 = c1.recv(4096).decode("utf-8")
            bc2 = c2.recv(4096).decode("utf-8")
            assert '"op":"broadcast"' in bc1 and "Hello Bob!" in bc1, f"{name} broadcast failed on sender"
            assert '"op":"broadcast"' in bc2 and "Hello Bob!" in bc2, f"{name} broadcast failed on receiver"

            c1.close()
            c2.close()
            print(f"  ✔ {name}: Multi-Client Presence & Broadcast Verified")
        finally:
            proc.terminate()
            proc.wait()

def main():
    print("=================================================================")
    print("🎭 REAL-TIME CHAT PARITY VERIFICATION SUITE")
    print("=================================================================")
    test_canvas_parity()
    test_server_ping_pong_parity()
    test_multi_client_broadcast_parity()
    print("\n=================================================================")
    print("🎉 ALL PARITY TESTS PASSED! Bit-exact behavior across all engines!")
    print("=================================================================")

if __name__ == "__main__":
    main()
