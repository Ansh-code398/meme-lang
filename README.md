# 🎭 MEMElang (GOAT Lang)

> **The high-performance, memory-safe, polyglot programming language engineered with Indian Desi meme culture, Gen-Z slang, and zero-compromise modern systems architecture.**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![WebAssembly](https://img.shields.io/badge/WASM-Ready-purple.svg)](https://webassembly.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Tests](https://img.shields.io/badge/tests-22%2F22%20passing-brightgreen.svg)]()

---

## 🌟 Overview

**MEMElang** is a statically-typed yet delightfully gradual programming language written in clean C++20. It combines the expressive humor of WhatsApp Hinglish and Gen-Z meme vocabulary with production-grade runtime systems:

* **⚡ True Asynchrony & Promises**: Non-blocking `async / await` (`sabar`) backed by C++ worker threads and Promises (`Vaada.sab`, `Vaada.race`, `.then`, `.catch`).
* **🌐 Native HTTP Client**: Global `fetch()` (`mangwa`) and `jaal` (`http`) client with Bearer auth, query parameter encoding, timeouts, automatic JSON serialization, and deterministic 0ms offline mock testing (`mock://`).
* **🛡️ Structured Errors**: `try / catch / finally / throw` (`panga / lafda / pakka / chillao`) with structured Error objects (`.baat` / `.message`, `.kism` / `.type`, `.line`), `Galti()` constructor, and complete backward compatibility with strings.
* **🧵 Hardware Concurrency**: Real OS hardware threads (`dhaga.chala`), recursive mutexes (`dhaga.tala`), Go-style buffered/unbuffered channels (`dhaga.naali`), parallel work-stealing map across CPU cores (`kaamgar.sab_pe` / `mp.map`), and OS subprocesses (`kaamgar.alag` / `mp.spawn`).
* **📂 Complete File System & Path Suite**: `file` (`fs`) and `rasta` (`path`) with automatic CSV parsing/writing into Lists of Maps, JSON storage, recursive directory walking, and cross-platform path resolution.
* **🖥️ Zero-WebView Native Desktop GUI**: Native macOS Cocoa / Linux / Windows 60 FPS desktop applications with JSX declarative component trees, Tailwind CSS v4 layout engine, and direct GPU draw commands.
* **🦎 Chameleon Polyglot Bridge**: Seamless, in-process zero-IPC bridging with Python (NumPy, Pandas), Node/NPM (Firebase, Ably), and Rust (Rayon, Cryptography).
* **📦 Tapri Package Manager**: Local virtual environments (`.meme_env`), package dependency resolution (`tapri.json`), and clean `@/` path aliasing.

---

## 🚀 Quick Start

### Building from Source

```bash
# Clone the repository
git clone https://github.com/your-username/memelang.git
cd memelang

# Build the native CLI executable
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

# Run the test suite (22 suites passing)
make test
```

### Running Your First Program

```bash
# Execute interactive REPL
./meme

# Run an inline script
./meme -e 'dikha "Wah bete mauj kardi!";'

# Run a .meme file
./meme run examples/hello.meme
```

---

## 📖 Language Syntax & Core Systems

### 1. Structured `try / catch / finally / throw` (`panga / lafda / pakka / chillao`)

MEMElang supports universal keyword aliases across WhatsApp Hinglish, Python, and JavaScript/TypeScript:

| WhatsApp Hinglish | Universal Alias | Description |
| :--- | :--- | :--- |
| `panga { ... }` | `try { ... }` / `koshish` | Block of code that might fail |
| `lafda (err) { ... }` | `catch (err) { ... }` / `except (err)` | Catches structured errors or runtime exceptions |
| `pakka { ... }` | `finally { ... }` / `hamesha` | Guaranteed execution block |
| `chillao expr` | `throw expr` / `raise expr` / `fek` | Throws error string, Map, or `Galti(msg, type)` |

```meme
// Structured error handling with Galti constructor
panga {
    chillao Galti("Database connection timeout", "NetworkLafda");
} lafda (err) {
    dikha "Caught error:", err.baat;     // or err.message
    dikha "Category:", err.kism;          // or err.type
    dikha "Source line:", err.line;       // Source line number
    dikha err;                            // Prints cleanly as plain message string
} pakka {
    dikha "Connection resources closed!";
}

// Universal Python / JS aliases
try {
    raise Error("Invalid schema", "TypeLafda");
} catch (e) {
    dikha f"Error: {e.message} ({e.type})";
}
```

---

### 2. Full-Featured `fetch()` & `jaal` (`http`) Client

```meme
// 1. JS-style global fetch() with query params, Bearer token, and JSON body
pakad res = sabar fetch("https://api.example.com/items", {
    "method": "POST",
    "params": {"category": "chai", "limit": 10},
    "token": "secret_bearer_token_123",
    "body": {"name": "Masala Chai", "price": 25},
    "timeout": 5000
});

dekh res.ok {
    pakad item = res.json();
    dikha f"Created {item.name} in {res.time_ms}ms!";
} warna {
    dikha f"Request failed: {res.status}";
}

// 2. Clean shorthand methods on jaal / http
pakad items   = sabar jaal.get("mock://items", {"params": {"q": "chai"}});
pakad updated = sabar jaal.put("mock://items/1", {"price": 30});
pakad patched = sabar jaal.patch("mock://items/1", {"stock": 100});
pakad deleted = sabar jaal.delete("mock://items/1");
```

---

### 3. `async / await` (`sabar`) & Promises (`Vaada`)

```meme
// Declare asynchronous functions
async kaam fetchUser(id) {
    sabar Vaada.ruk(50); // non-blocking 50ms pause
    ye_le {"id": id, "name": "Raju", "role": "Dev"};
}

// Await async function
pakad user = sabar fetchUser(101);
dikha f"User: {user.name} ({user.role})";

// Parallel execution with Promise.all (Vaada.sab)
pakad p1 = fetchUser(1);
pakad p2 = fetchUser(2);
pakad allUsers = sabar Promise.all([p1, p2]);
dikha f"Loaded {allUsers.lambai()} users concurrently!";

// Promise chaining (.then / .catch)
fetchUser(99)
    .then(u => dikha f"Chained: {u.name}")
    .catch(err => dikha f"Lafda: {err.message}");
```

---

### 4. Multi-Threading (`dhaga`) & Multi-Processing (`kaamgar`)

```meme
// 1. Spawning hardware OS threads
pakad t = dhaga.chala((a, b) => a * b, 6, 7);
pakad product = sabar t; // or t.jod()
dikha f"Thread product: {product}"; // 42

// 2. Mutex Lock (dhaga.tala)
pakad lock = dhaga.tala();
pakad sharedCounter = 0;
lock.surakshit(() => {
    sharedCounter = sharedCounter + 1;
});

// 3. Go-style Channels (dhaga.naali)
pakad ch = dhaga.naali(5);
dhaga.chala(() => {
    ch.bhej("packet_from_worker");
});
pakad msg = ch.le();
dikha f"Channel message: {msg}";

// 4. Parallel work-stealing pool across CPU cores (mp.map / kaamgar.sab_pe)
pakad numbers = [1, 2, 3, 4, 5];
pakad squares = mp.map(x => x * x, numbers);
dikha f"Squares: {squares}"; // [1, 4, 9, 16, 25]

// 5. OS Subprocess (mp.spawn / kaamgar.alag)
pakad proc = mp.spawn("echo 'Child process execution'");
pakad exitCode = proc.ruk();
dikha f"Subprocess output: {proc.out()} (exit: {exitCode})";
```

---

### 5. File System (`file` / `fs`) & Paths (`rasta` / `path`)

```meme
// 1. File reading and line splitting
fs.write("notes.txt", "Pehli line\nDusri line\n");
pakad lines = fs.read_lines("notes.txt");
dikha f"Total lines: {lines.lambai()}";

// 2. CSV Parsing & Writing into Lists of Maps
pakad staff = [
    {"id": "1", "name": "Raju", "role": "Fullstack Dev"},
    {"id": "2", "name": "Baburao", "role": "Team Lead"}
];
fs.write_csv("staff.csv", staff);
pakad readStaff = fs.read_csv("staff.csv");
dikha f"First member: {readStaff[0].name} ({readStaff[0].role})";

// 3. File statistics
pakad stat = fs.stat("staff.csv");
dikha f"File size: {stat.size} bytes, Modified timestamp: {stat.modified}";

// 4. Wildcard recursive search (glob)
pakad memeFiles = fs.glob(".", "*.meme");
dikha f"Found meme scripts: {memeFiles.lambai()}";

// 5. Cross-platform path helpers
pakad full = path.join("workspace", "src", "main.meme");
dikha f"Basename: {path.basename(full)}"; // main.meme
dikha f"Stem: {path.stem(full)}";         // main
dikha f"Extension: {path.ext(full)}";      // .meme
```

---

### 6. Zero-WebView Native Desktop GUI

MEMElang powers real 60 FPS native desktop applications using declarative JSX components and a custom Tailwind CSS engine:

```meme
kaam App() {
    pakad [todos, setTodos] = useState(["Learn MEMElang", "Build Native App"]);
    pakad [input, setInput] = useState("");

    ye_le (
        <div className="flex flex-col w-full h-full bg-slate-900 text-white p-6">
            <h1 className="text-2xl font-bold text-amber-400 mb-4">
                ChaiVault Desktop
            </h1>
            <div className="flex flex-row gap-2 mb-4">
                <input
                    className="flex-1 bg-slate-800 border border-slate-700 px-3 py-2 rounded text-white"
                    value={input}
                    onChange={(e) => setInput(e.target.value)}
                />
                <button
                    className="bg-amber-500 hover:bg-amber-600 text-slate-900 font-bold px-4 py-2 rounded"
                    onClick={() => {
                        setTodos([...todos, input]);
                        setInput("");
                    }}
                >
                    Add
                </button>
            </div>
            <div className="flex flex-col gap-2">
                {todos.map((item, idx) => (
                    <div key={idx} className="p-3 bg-slate-800 rounded border border-slate-700">
                        {item}
                    </div>
                ))}
            </div>
        </div>
    );
}

app.mount(<App />, { "title": "ChaiVault", "width": 800, "height": 600 });
```

---

## 🧪 Testing

MEMElang comes with an exhaustive test harness covering all 22 compiler, runtime, and ecosystem subsystems:

```bash
# Run the full test suite
make test

# Run isolated application tests
./meme test todo_app
./meme test notes_desktop
./meme test polyglot_app
```

---

## 🌐 WebAssembly & Documentation

The interactive web documentation and WebAssembly playground run client-side in the browser:

```bash
# Build the WebAssembly binary
./scripts/build-wasm.sh

# Run documentation development server
cd docs
npm install
npm run dev

# Build documentation for production
npm run build
```

---

## 📜 License

MIT License. Designed and engineered for the community.
