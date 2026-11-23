#!/usr/bin/env python3
"""
Real-time Chat GUI & Client in Python.
Renders deterministic 80x24 ASCII 2D Canvas and handles TCP protocol.
"""
import sys
import time
import socket
import select
from chat_protocol import render_canvas, serialize_msg, parse_msg

class ChatClient:
    def __init__(self, host: str = "127.0.0.1", port: int = 9099, username: str = "alice"):
        self.host = host
        self.port = port
        self.username = username
        self.sock = None
        self.users = [username]
        self.messages = []
        self.input_buf = ""
        self.latency_us = 0
        self.connected = False
        self.pending_data = ""

    def connect(self, timeout: float = 3.0):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(timeout)
            self.sock.connect((self.host, self.port))
            self.sock.setblocking(False)
            self.connected = True
            # Send join
            join_msg = {"op": "join", "user": self.username}
            self.sock.sendall(serialize_msg(join_msg).encode("utf-8"))
            return True
        except Exception:
            self.connected = False
            return False

    def send_message(self, text: str):
        if not self.connected or not self.sock:
            return
        payload = {"op": "msg", "text": text, "user": self.username}
        try:
            self.sock.sendall(serialize_msg(payload).encode("utf-8"))
        except Exception:
            self.connected = False

    def ping(self) -> int:
        if not self.connected or not self.sock:
            return -1
        t0 = time.perf_counter_ns()
        ping_msg = {"op": "ping", "id": 1}
        try:
            self.sock.sendall(serialize_msg(ping_msg).encode("utf-8"))
            # Busy poll for reply
            t_end = time.perf_counter_ns() + 1_000_000_000
            while time.perf_counter_ns() < t_end:
                readable, _, _ = select.select([self.sock], [], [], 0.001)
                if readable:
                    data = self.sock.recv(4096)
                    if data:
                        self.pending_data += data.decode("utf-8", errors="ignore")
                        if "\n" in self.pending_data:
                            line, self.pending_data = self.pending_data.split("\n", 1)
                            msg = parse_msg(line)
                            if msg.get("op") == "pong":
                                dt_us = (time.perf_counter_ns() - t0) // 1000
                                self.latency_us = dt_us
                                return dt_us
                            self.handle_incoming(msg)
        except Exception:
            self.connected = False
        return -1

    def pump_events(self, timeout: float = 0.005):
        if not self.connected or not self.sock:
            return
        try:
            readable, _, _ = select.select([self.sock], [], [], timeout)
            if readable:
                data = self.sock.recv(4096)
                if not data:
                    self.connected = False
                    return
                self.pending_data += data.decode("utf-8", errors="ignore")
                while "\n" in self.pending_data:
                    line, self.pending_data = self.pending_data.split("\n", 1)
                    if line.strip():
                        msg = parse_msg(line)
                        self.handle_incoming(msg)
        except Exception:
            self.connected = False

    def handle_incoming(self, msg: dict):
        op = msg.get("op", "")
        if op == "welcome":
            self.users = sorted(msg.get("users", [self.username]))
            self.messages.extend(msg.get("history", []))
        elif op == "presence":
            self.users = sorted(msg.get("users", self.users))
            action = msg.get("action", "")
            user = msg.get("user", "")
            notice = f"*** {user} {action}ed the room ***"
            self.messages.append({"user": "sys", "text": notice, "time": time.strftime("%H:%M:%S")})
        elif op == "broadcast":
            self.messages.append({
                "user": msg.get("user", "anon"),
                "text": msg.get("text", ""),
                "time": msg.get("time", time.strftime("%H:%M:%S"))
            })
            if len(self.messages) > 100:
                self.messages.pop(0)

    def render(self) -> str:
        return render_canvas(
            title="MEME-CHAT Hub GUI",
            user=self.username,
            port=self.port,
            users=self.users,
            messages=self.messages,
            input_buf=self.input_buf,
            latency_us=self.latency_us
        )

    def close(self):
        if self.sock:
            try:
                self.sock.sendall(serialize_msg({"op": "quit", "user": self.username}).encode("utf-8"))
                self.sock.close()
            except Exception:
                pass
            self.sock = None
        self.connected = False

def benchmark_render(client: ChatClient, frames: int = 1000) -> float:
    """Renders N frames and returns frames per second."""
    t0 = time.perf_counter()
    for _ in range(frames):
        _ = client.render()
    dt = time.perf_counter() - t0
    return frames / dt if dt > 0 else 0.0

if __name__ == "__main__":
    args = sys.argv[1:]
    user = "alice"
    port = 9099
    frames = 500
    headless = False

    i = 0
    while i < len(args):
        if args[i] == "--user" and i + 1 < len(args):
            user = args[i + 1]
            i += 2
        elif args[i] == "--port" and i + 1 < len(args):
            port = int(args[i + 1])
            i += 2
        elif args[i] == "--frames" and i + 1 < len(args):
            frames = int(args[i + 1])
            i += 2
        elif args[i] == "--headless":
            headless = True
            i += 1
        else:
            i += 1

    client = ChatClient(port=port, username=user)
    client.messages = [
        {"user": "sys", "text": "Connected to cluster.", "time": "12:00:00"},
        {"user": "alice", "text": "Realtime rendering benchmark initializing.", "time": "12:00:01"},
        {"user": "bob", "text": "Measuring frames per second with ANSI character canvas.", "time": "12:00:02"}
    ]
    client.users = ["alice", "bob", "charlie"]

    if headless:
        fps = benchmark_render(client, frames)
        print(f"[PY_CHAT_CLIENT] Rendered {frames} frames: {fps:.2f} fps")
    else:
        frame = client.render()
        print(frame)
