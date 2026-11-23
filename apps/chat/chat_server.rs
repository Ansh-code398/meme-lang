// ==============================================================================
// Real-time Chat Server in Pure Rust (Zero External Dependencies)
// Bit-exact protocol compatibility with MEMElang Chat Hub
// ==============================================================================

use std::collections::HashMap;
use std::env;
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

fn get_timestamp() -> String {
    let now = SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or(Duration::from_secs(0));
    let secs = now.as_secs();
    let hours = (secs / 3600) % 24;
    let mins = (secs / 60) % 60;
    let s = secs % 60;
    format!("{:02}:{:02}:{:02}", hours, mins, s)
}

fn escape_json(s: &str) -> String {
    let mut out = String::with_capacity(s.len() + 8);
    for c in s.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            _ => out.push(c),
        }
    }
    out
}

fn extract_str<'a>(json: &'a str, key: &str) -> Option<&'a str> {
    let pattern = format!("\"{}\":\"", key);
    if let Some(pos) = json.find(&pattern) {
        let start = pos + pattern.len();
        if let Some(end) = json[start..].find('"') {
            return Some(&json[start..start + end]);
        }
    }
    None
}

fn extract_int(json: &str, key: &str) -> Option<i64> {
    let pattern = format!("\"{}\":", key);
    if let Some(pos) = json.find(&pattern) {
        let start = pos + pattern.len();
        let s = json[start..].trim_start();
        let mut end = 0;
        for c in s.chars() {
            if c.is_digit(10) || c == '-' {
                end += 1;
            } else {
                break;
            }
        }
        if end > 0 {
            return s[..end].parse::<i64>().ok();
        }
    }
    None
}

#[derive(Clone, Debug)]
pub struct ChatMessage {
    pub user: String,
    pub text: String,
    pub time: String,
}

pub struct ServerState {
    pub clients: HashMap<usize, (String, TcpStream)>,
    pub history: Vec<ChatMessage>,
    pub next_id: usize,
}

impl ServerState {
    pub fn new() -> Self {
        Self {
            clients: HashMap::new(),
            history: Vec::new(),
            next_id: 1,
        }
    }

    pub fn get_user_list(&self) -> Vec<String> {
        let mut users = Vec::new();
        for (_, (u, _)) in &self.clients {
            if !u.is_empty() {
                users.push(u.clone());
            }
        }
        users.sort();
        users
    }

    pub fn broadcast(&mut self, payload: &str, exclude_id: Option<usize>) {
        let data = format!("{}\n", payload);
        let mut dead = Vec::new();
        for (&id, (_, stream)) in self.clients.iter_mut() {
            if Some(id) == exclude_id {
                continue;
            }
            if stream.write_all(data.as_bytes()).is_err() {
                dead.push(id);
            }
        }
        for d in dead {
            self.clients.remove(&d);
        }
    }
}

pub fn handle_client(
    client_id: usize,
    mut stream: TcpStream,
    state: Arc<Mutex<ServerState>>,
) {
    let _ = stream.set_nodelay(true);
    let _ = stream.set_read_timeout(Some(Duration::from_millis(50)));

    let mut buf = vec![0u8; 4096];
    let mut pending = String::new();

    loop {
        match stream.read(&mut buf) {
            Ok(0) => {
                // Connection closed
                break;
            }
            Ok(n) => {
                let chunk = String::from_utf8_lossy(&buf[..n]);
                pending.push_str(&chunk);

                while let Some(nl_pos) = pending.find('\n') {
                    let line = pending[..nl_pos].trim().to_string();
                    pending = pending[nl_pos + 1..].to_string();

                    if line.is_empty() {
                        continue;
                    }

                    let op = extract_str(&line, "op").unwrap_or("");
                    match op {
                        "join" => {
                            let username = extract_str(&line, "user").unwrap_or("anon").to_string();
                            let mut s = state.lock().unwrap();
                            if let Some(entry) = s.clients.get_mut(&client_id) {
                                entry.0 = username.clone();
                            }
                            let users = s.get_user_list();
                            let users_json = users
                                .iter()
                                .map(|u| format!("\"{}\"", escape_json(u)))
                                .collect::<Vec<_>>()
                                .join(",");

                            let history_json = s
                                .history
                                .iter()
                                .rev()
                                .take(20)
                                .collect::<Vec<_>>()
                                .iter()
                                .rev()
                                .map(|m| {
                                    format!(
                                        "{{\"text\":\"{}\",\"time\":\"{}\",\"user\":\"{}\"}}",
                                        escape_json(&m.text),
                                        escape_json(&m.time),
                                        escape_json(&m.user)
                                    )
                                })
                                .collect::<Vec<_>>()
                                .join(",");

                            let welcome = format!(
                                "{{\"history\":[{}],\"op\":\"welcome\",\"user\":\"{}\",\"users\":[{}]}}",
                                history_json,
                                escape_json(&username),
                                users_json
                            );
                            let _ = stream.write_all(format!("{}\n", welcome).as_bytes());

                            let presence = format!(
                                "{{\"action\":\"join\",\"op\":\"presence\",\"user\":\"{}\",\"users\":[{}]}}",
                                escape_json(&username),
                                users_json
                            );
                            s.broadcast(&presence, Some(client_id));
                        }
                        "msg" => {
                            let text = extract_str(&line, "text").unwrap_or("").to_string();
                            let ts = get_timestamp();
                            let mut s = state.lock().unwrap();
                            let user = s
                                .clients
                                .get(&client_id)
                                .map(|(u, _)| u.clone())
                                .unwrap_or_else(|| "anon".to_string());

                            let msg_record = ChatMessage {
                                user: user.clone(),
                                text: text.clone(),
                                time: ts.clone(),
                            };
                            s.history.push(msg_record);
                            if s.history.len() > 100 {
                                s.history.remove(0);
                            }

                            let broadcast_payload = format!(
                                "{{\"op\":\"broadcast\",\"text\":\"{}\",\"time\":\"{}\",\"user\":\"{}\"}}",
                                escape_json(&text),
                                ts,
                                escape_json(&user)
                            );
                            s.broadcast(&broadcast_payload, None);
                        }
                        "ping" => {
                            let ping_id = extract_int(&line, "id").unwrap_or(0);
                            let pong = format!("{{\"id\":{},\"op\":\"pong\"}}\n", ping_id);
                            let _ = stream.write_all(pong.as_bytes());
                        }
                        "quit" => {
                            break;
                        }
                        _ => {}
                    }
                }
            }
            Err(ref e) if e.kind() == std::io::ErrorKind::WouldBlock || e.kind() == std::io::ErrorKind::TimedOut => {
                // Read timeout: continue loop to keep connection active
                thread::sleep(Duration::from_millis(2));
            }
            Err(_) => {
                break;
            }
        }
    }

    // Client cleanup
    let mut s = state.lock().unwrap();
    let old_user = s.clients.remove(&client_id).map(|(u, _)| u).unwrap_or_default();
    if !old_user.is_empty() {
        let users = s.get_user_list();
        let users_json = users
            .iter()
            .map(|u| format!("\"{}\"", escape_json(u)))
            .collect::<Vec<_>>()
            .join(",");
        let presence = format!(
            "{{\"action\":\"leave\",\"op\":\"presence\",\"user\":\"{}\",\"users\":[{}]}}",
            escape_json(&old_user),
            users_json
        );
        s.broadcast(&presence, None);
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let mut port = 9099;
    let mut i = 1;
    while i < args.len() {
        if args[i] == "--port" && i + 1 < args.len() {
            port = args[i + 1].parse().unwrap_or(9099);
            i += 2;
        } else if let Ok(p) = args[i].parse::<u16>() {
            port = p;
            i += 1;
        } else {
            i += 1;
        }
    }

    let listener = TcpListener::bind(format!("127.0.0.1:{}", port))
        .expect("Failed to bind TCP listener");
    println!("[RUST_CHAT_SERVER] Listening on 127.0.0.1:{}...", port);

    let state = Arc::new(Mutex::new(ServerState::new()));

    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let state_clone = Arc::clone(&state);
                let client_id = {
                    let mut s = state.lock().unwrap();
                    let id = s.next_id;
                    s.next_id += 1;
                    let stream_clone = stream.try_clone().expect("Failed to clone stream");
                    s.clients.insert(id, (String::new(), stream_clone));
                    id
                };
                thread::spawn(move || {
                    handle_client(client_id, stream, state_clone);
                });
            }
            Err(e) => {
                eprintln!("[RUST_CHAT_SERVER] Accept error: {}", e);
            }
        }
    }
}
