// ==============================================================================
// Pure Rust Real-time Chat GUI & Client
// Zero external dependencies, bit-exact ANSI 2D Canvas rendering & benchmarks
// ==============================================================================

use std::env;
use std::io::{Read, Write};
use std::net::TcpStream;
use std::time::{Duration, Instant};

pub const CANVAS_WIDTH: usize = 80;
pub const CANVAS_HEIGHT: usize = 24;
pub const CHAT_WIDTH: usize = 57;
pub const SIDEBAR_WIDTH: usize = 20;

#[derive(Clone, Debug)]
pub struct ChatMessage {
    pub user: String,
    pub text: String,
    pub time: String,
}

pub fn wrap_text(text: &str, max_width: usize) -> Vec<String> {
    if text.len() <= max_width {
        return vec![text.to_string()];
    }
    let words: Vec<&str> = text.split(' ').collect();
    let mut lines = Vec::new();
    let mut cur = String::new();

    for w in words {
        if cur.is_empty() {
            cur.push_str(w);
        } else if cur.len() + 1 + w.len() <= max_width {
            cur.push(' ');
            cur.push_str(w);
        } else {
            lines.push(cur);
            cur = w.to_string();
        }
    }
    if !cur.is_empty() {
        lines.push(cur);
    }
    lines
}

pub fn pad_right(s: &str, target_len: usize) -> String {
    if s.len() >= target_len {
        return s[..target_len].to_string();
    }
    let mut res = s.to_string();
    while res.len() < target_len {
        res.push(' ');
    }
    res
}

pub fn render_canvas(
    title: &str,
    user: &str,
    port: u16,
    users: &[String],
    messages: &[ChatMessage],
    input_buf: &str,
    latency_us: u64,
) -> String {
    let mut rows = Vec::with_capacity(CANVAS_HEIGHT);

    // Row 0: Top border
    let mut top_border = String::from("+");
    for _ in 0..78 {
        top_border.push('-');
    }
    top_border.push('+');
    rows.push(top_border);

    // Row 1: Header / Title bar
    let header_info = format!(" {} | User: @{} | Online: {} | Port: {} ", title, user, users.len(), port);
    let padded_header = pad_right(&header_info, 78);
    rows.push(format!("|{}|", padded_header));

    // Row 2: Divider
    let mut mid_div = String::from("+");
    for _ in 0..CHAT_WIDTH {
        mid_div.push('-');
    }
    mid_div.push('+');
    for _ in 0..SIDEBAR_WIDTH {
        mid_div.push('-');
    }
    mid_div.push('+');
    rows.push(mid_div);

    // Format chat messages
    let mut formatted_lines = Vec::new();
    for m in messages {
        let prefix = format!("[{}] <{}> ", m.time, m.user);
        let full = format!("{}{}", prefix, m.text);
        let wrapped = wrap_text(&full, CHAT_WIDTH - 2);
        formatted_lines.extend(wrapped);
    }

    const MAIN_ROWS: usize = 17;
    let chat_display: Vec<String> = if formatted_lines.len() > MAIN_ROWS {
        formatted_lines[formatted_lines.len() - MAIN_ROWS..].to_vec()
    } else {
        let mut padded = vec![String::new(); MAIN_ROWS - formatted_lines.len()];
        padded.extend(formatted_lines);
        padded
    };

    // Format user list for sidebar
    let mut sorted_users = users.to_vec();
    sorted_users.sort();
    let mut sidebar_lines = Vec::new();
    for u in sorted_users.iter().take(MAIN_ROWS) {
        let is_self = if u == user { " (you)" } else { "" };
        sidebar_lines.push(format!("* {}{}", u, is_self));
    }
    while sidebar_lines.len() < MAIN_ROWS {
        sidebar_lines.push(String::new());
    }

    // Rows 3 through 19: Content rows
    for i in 0..MAIN_ROWS {
        let c_text = pad_right(&format!(" {}", chat_display[i]), CHAT_WIDTH);
        let s_text = pad_right(&format!(" {}", sidebar_lines[i]), SIDEBAR_WIDTH);
        rows.push(format!("|{}|{}|", c_text, s_text));
    }

    // Row 20: Input divider
    let mut bot_div = String::from("+");
    for _ in 0..CHAT_WIDTH {
        bot_div.push('-');
    }
    bot_div.push('+');
    for _ in 0..SIDEBAR_WIDTH {
        bot_div.push('-');
    }
    bot_div.push('+');
    rows.push(bot_div);

    // Row 21: Input box
    let inp_text = format!(" > {}_", input_buf);
    let padded_inp = pad_right(&inp_text, 78);
    rows.push(format!("|{}|", padded_inp));

    // Row 22: Status / hint bar
    let status_text = format!(" Commands: /quit, /ping, /clear | Latency: {}us | Status: CONNECTED ", latency_us);
    let padded_status = pad_right(&status_text, 78);
    rows.push(format!("|{}|", padded_status));

    // Row 23: Bottom border
    let mut bottom_border = String::from("+");
    for _ in 0..78 {
        bottom_border.push('-');
    }
    bottom_border.push('+');
    rows.push(bottom_border);

    rows.join("\n")
}

pub struct ChatClient {
    pub host: String,
    pub port: u16,
    pub username: String,
    pub stream: Option<TcpStream>,
    pub users: Vec<String>,
    pub messages: Vec<ChatMessage>,
    pub input_buf: String,
    pub latency_us: u64,
    pub pending: String,
}

impl ChatClient {
    pub fn new(host: &str, port: u16, username: &str) -> Self {
        Self {
            host: host.to_string(),
            port,
            username: username.to_string(),
            stream: None,
            users: vec![username.to_string()],
            messages: Vec::new(),
            input_buf: String::new(),
            latency_us: 0,
            pending: String::new(),
        }
    }

    pub fn connect(&mut self) -> bool {
        let addr = format!("{}:{}", self.host, self.port);
        match TcpStream::connect(addr) {
            Ok(s) => {
                let _ = s.set_nodelay(true);
                let _ = s.set_nonblocking(true);
                self.stream = Some(s);
                let join_packet = format!("{{\"op\":\"join\",\"user\":\"{}\"}}\n", self.username);
                let _ = self.stream.as_mut().unwrap().write_all(join_packet.as_bytes());
                true
            }
            Err(_) => false,
        }
    }

    pub fn ping(&mut self) -> i64 {
        if self.stream.is_none() {
            return -1;
        }
        let t0 = Instant::now();
        let ping_packet = "{\"id\":1,\"op\":\"ping\"}\n";
        let stream = self.stream.as_mut().unwrap();
        if stream.write_all(ping_packet.as_bytes()).is_err() {
            return -1;
        }

        let mut buf = [0u8; 1024];
        let deadline = Instant::now() + Duration::from_millis(1000);
        while Instant::now() < deadline {
            match stream.read(&mut buf) {
                Ok(n) if n > 0 => {
                    let s = String::from_utf8_lossy(&buf[..n]);
                    self.pending.push_str(&s);
                    if let Some(pos) = self.pending.find('\n') {
                        let line = self.pending[..pos].trim().to_string();
                        self.pending = self.pending[pos + 1..].to_string();
                        if line.contains("\"op\":\"pong\"") {
                            let us = t0.elapsed().as_micros() as u64;
                            self.latency_us = us;
                            return us as i64;
                        }
                    }
                }
                _ => {
                    std::thread::sleep(Duration::from_micros(10));
                }
            }
        }
        -1
    }

    pub fn render(&self) -> String {
        render_canvas(
            "MEME-CHAT Hub GUI",
            &self.username,
            self.port,
            &self.users,
            &self.messages,
            &self.input_buf,
            self.latency_us,
        )
    }

    pub fn close(&mut self) {
        if let Some(mut s) = self.stream.take() {
            let quit_packet = format!("{{\"op\":\"quit\",\"user\":\"{}\"}}\n", self.username);
            let _ = s.write_all(quit_packet.as_bytes());
        }
    }
}

fn benchmark_render(client: &ChatClient, frames: usize) -> f64 {
    let t0 = Instant::now();
    for _ in 0..frames {
        let _ = client.render();
    }
    let dt = t0.elapsed().as_secs_f64();
    if dt > 0.0 {
        frames as f64 / dt
    } else {
        0.0
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let mut user = "alice".to_string();
    let mut port = 9099;
    let mut frames = 500;
    let mut headless = false;

    let mut i = 1;
    while i < args.len() {
        if args[i] == "--user" && i + 1 < args.len() {
            user = args[i + 1].clone();
            i += 2;
        } else if args[i] == "--port" && i + 1 < args.len() {
            port = args[i + 1].parse().unwrap_or(9099);
            i += 2;
        } else if args[i] == "--frames" && i + 1 < args.len() {
            frames = args[i + 1].parse().unwrap_or(500);
            i += 2;
        } else if args[i] == "--headless" {
            headless = true;
            i += 1;
        } else {
            i += 1;
        }
    }

    let mut client = ChatClient::new("127.0.0.1", port, &user);
    client.messages = vec![
        ChatMessage {
            user: "sys".to_string(),
            text: "Connected to cluster.".to_string(),
            time: "12:00:00".to_string(),
        },
        ChatMessage {
            user: "alice".to_string(),
            text: "Realtime rendering benchmark initializing.".to_string(),
            time: "12:00:01".to_string(),
        },
        ChatMessage {
            user: "bob".to_string(),
            text: "Measuring frames per second with ANSI character canvas.".to_string(),
            time: "12:00:02".to_string(),
        },
    ];
    client.users = vec!["alice".to_string(), "bob".to_string(), "charlie".to_string()];

    if headless {
        let fps = benchmark_render(&client, frames);
        println!("[RUST_CHAT_CLIENT] Rendered {} frames: {:.2} fps", frames, fps);
    } else {
        println!("{}", client.render());
    }
}
