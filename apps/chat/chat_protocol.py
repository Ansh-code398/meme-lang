#!/usr/bin/env python3
"""
Real-time Chat Protocol & Utilities for MEMElang, Rust, and Python.
Provides deterministic message serialization, parsing, and 2D ANSI Canvas layout.
"""
import json
import time

CANVAS_WIDTH = 80
CANVAS_HEIGHT = 24
CHAT_WIDTH = 57
SIDEBAR_WIDTH = 20

def serialize_msg(obj: dict) -> str:
    """Serializes a protocol dictionary into a single-line newline-terminated JSON string."""
    return json.dumps(obj, separators=(',', ':'), sort_keys=True) + "\n"

def parse_msg(line: str) -> dict:
    """Parses a single JSON line into a dictionary."""
    line = line.strip()
    if not line:
        return {}
    return json.loads(line)

def wrap_text(text: str, max_width: int) -> list:
    """Wraps text into lines not exceeding max_width."""
    if len(text) <= max_width:
        return [text]
    words = text.split(' ')
    lines = []
    cur = ""
    for w in words:
        if not cur:
            cur = w
        elif len(cur) + 1 + len(w) <= max_width:
            cur += " " + w
        else:
            lines.append(cur)
            cur = w
    if cur:
        lines.append(cur)
    return lines

def render_canvas(title: str, user: str, port: int, users: list, messages: list, input_buf: str, latency_us: int = 0) -> str:
    """
    Renders an 80x24 deterministic ASCII 2D chat canvas.
    Exactly 24 lines, each line exactly 80 visible characters wide.
    """
    rows = []
    
    # Row 0: Top border
    rows.append("+" + "-" * 78 + "+")
    
    # Row 1: Header / Title bar
    header_info = f" {title} | User: @{user} | Online: {len(users)} | Port: {port} "
    header_line = "|" + header_info.ljust(78) + "|"
    rows.append(header_line[:80])
    
    # Row 2: Header divider between chat and sidebar
    # Total inner: 57 (chat) + 1 (divider) + 20 (sidebar) = 78 chars
    rows.append("+" + "-" * CHAT_WIDTH + "+" + "-" * SIDEBAR_WIDTH + "+")
    
    # Format messages for display (max width = CHAT_WIDTH - 2 = 55 chars)
    formatted_lines = []
    for m in messages:
        sender = m.get("user", "sys")
        txt = m.get("text", "")
        ts = m.get("time", "00:00:00")
        prefix = f"[{ts}] <{sender}> "
        wrapped = wrap_text(prefix + txt, CHAT_WIDTH - 2)
        formatted_lines.extend(wrapped)
        
    # We have 17 rows for main content (rows 3 through 19)
    MAIN_ROWS = 17
    # Scroll: take the last MAIN_ROWS
    chat_slice = formatted_lines[-MAIN_ROWS:] if len(formatted_lines) > MAIN_ROWS else formatted_lines
    chat_display = [""] * (MAIN_ROWS - len(chat_slice)) + chat_slice

    # Format user list for sidebar (max 17 users visible)
    sidebar_lines = []
    for u in sorted(users)[:MAIN_ROWS]:
        is_self = " (you)" if u == user else ""
        sidebar_lines.append(f"* {u}{is_self}")
    while len(sidebar_lines) < MAIN_ROWS:
        sidebar_lines.append("")

    for i in range(MAIN_ROWS):
        c_text = (" " + chat_display[i]).ljust(CHAT_WIDTH)[:CHAT_WIDTH]
        s_text = (" " + sidebar_lines[i]).ljust(SIDEBAR_WIDTH)[:SIDEBAR_WIDTH]
        rows.append("|" + c_text + "|" + s_text + "|")

    # Row 20: Input divider
    rows.append("+" + "-" * CHAT_WIDTH + "+" + "-" * SIDEBAR_WIDTH + "+")

    # Row 21: Input box
    inp_text = f" > {input_buf}_"
    rows.append("|" + inp_text.ljust(78)[:78] + "|")

    # Row 22: Status / hint bar
    status_text = f" Commands: /quit, /ping, /clear | Latency: {latency_us}us | Status: CONNECTED "
    rows.append("|" + status_text.ljust(78)[:78] + "|")

    # Row 23: Bottom border
    rows.append("+" + "-" * 78 + "+")

    return "\n".join(rows)

if __name__ == "__main__":
    canvas = render_canvas(
        title="MEME-CHAT Realtime Hub",
        user="alice",
        port=9099,
        users=["alice", "bob", "charlie"],
        messages=[
            {"user": "sys", "text": "Welcome to MEME-CHAT Hub!", "time": "12:00:00"},
            {"user": "alice", "text": "Hello world from MEMElang!", "time": "12:00:01"},
            {"user": "bob", "text": "Rust and MEMElang realtime parity benchmark.", "time": "12:00:02"}
        ],
        input_buf="type message here...",
        latency_us=45
    )
    print(canvas)
    print(f"Canvas Lines: {len(canvas.splitlines())}, Width: {len(canvas.splitlines()[0])}")
