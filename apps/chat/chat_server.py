#!/usr/bin/env python3
"""
Python Reference Chat Server for Real-time Benchmark and Parity Testing.
"""
import socket
import select
import sys
import time
from chat_protocol import serialize_msg, parse_msg

class ChatServer:
    def __init__(self, host: str = "127.0.0.1", port: int = 9099):
        self.host = host
        self.port = port
        self.server_sock = None
        self.clients = {}  # sock -> {"user": str, "buf": str}
        self.history = []  # list of message dicts
        self.running = False

    def start(self):
        self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.server_sock.bind((self.host, self.port))
        self.server_sock.listen(128)
        self.server_sock.setblocking(False)
        self.running = True

    def broadcast(self, msg_dict: dict, exclude_sock = None):
        payload = serialize_msg(msg_dict).encode("utf-8")
        dead_socks = []
        for sock in list(self.clients.keys()):
            if sock == exclude_sock:
                continue
            try:
                sock.sendall(payload)
            except Exception:
                dead_socks.append(sock)
        for ds in dead_socks:
            self.remove_client(ds)

    def remove_client(self, sock):
        if sock in self.clients:
            user = self.clients[sock]["user"]
            del self.clients[sock]
            try:
                sock.close()
            except Exception:
                pass
            if user:
                active_users = [c["user"] for c in self.clients.values() if c["user"]]
                self.broadcast({
                    "op": "presence",
                    "action": "leave",
                    "user": user,
                    "users": active_users
                })

    def run_step(self, timeout: float = 0.05):
        """Processes one round of socket IO. Returns number of events processed."""
        if not self.running or not self.server_sock:
            return 0

        read_socks = [self.server_sock] + list(self.clients.keys())
        try:
            readable, _, _ = select.select(read_socks, [], [], timeout)
        except Exception:
            return 0

        events = 0
        for s in readable:
            events += 1
            if s is self.server_sock:
                try:
                    csock, addr = self.server_sock.accept()
                    csock.setblocking(False)
                    csock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                    self.clients[csock] = {"user": "", "buf": ""}
                except Exception:
                    pass
            else:
                try:
                    data = s.recv(4096)
                    if not data:
                        self.remove_client(s)
                        continue
                    self.clients[s]["buf"] += data.decode("utf-8", errors="ignore")
                    while "\n" in self.clients[s]["buf"]:
                        line, self.clients[s]["buf"] = self.clients[s]["buf"].split("\n", 1)
                        if not line.strip():
                            continue
                        msg = parse_msg(line)
                        self.handle_message(s, msg)
                except Exception:
                    self.remove_client(s)
        return events

    def handle_message(self, sock, msg: dict):
        op = msg.get("op", "")
        if op == "join":
            username = msg.get("user", "anon")
            self.clients[sock]["user"] = username
            active_users = [c["user"] for c in self.clients.values() if c["user"]]
            # Send welcome with current users and recent history
            welcome = {
                "op": "welcome",
                "user": username,
                "users": active_users,
                "history": self.history[-20:]
            }
            try:
                sock.sendall(serialize_msg(welcome).encode("utf-8"))
            except Exception:
                pass
            # Broadcast presence
            self.broadcast({
                "op": "presence",
                "action": "join",
                "user": username,
                "users": active_users
            }, exclude_sock=sock)

        elif op == "msg":
            user = self.clients[sock]["user"] or msg.get("user", "anon")
            text = msg.get("text", "")
            ts = time.strftime("%H:%M:%S")
            item = {"op": "broadcast", "user": user, "text": text, "time": ts}
            self.history.append({"user": user, "text": text, "time": ts})
            if len(self.history) > 100:
                self.history.pop(0)
            self.broadcast(item)

        elif op == "ping":
            pong = {"op": "pong", "id": msg.get("id", 0)}
            try:
                sock.sendall(serialize_msg(pong).encode("utf-8"))
            except Exception:
                pass

        elif op == "quit":
            self.remove_client(sock)

    def stop(self):
        self.running = False
        for s in list(self.clients.keys()):
            try:
                s.close()
            except Exception:
                pass
        self.clients.clear()
        if self.server_sock:
            try:
                self.server_sock.close()
            except Exception:
                pass
            self.server_sock = None

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9099
    server = ChatServer(port=port)
    server.start()
    print(f"[CHAT_SERVER] Listening on 127.0.0.1:{port}...")
    try:
        while True:
            server.run_step(0.01)
    except KeyboardInterrupt:
        server.stop()
        print("[CHAT_SERVER] Stopped.")
