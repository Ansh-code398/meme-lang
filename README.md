# 🎭 MEMElang (GOAT Lang)

> **The high-performance, memory-safe, polyglot programming language engineered with Indian Desi meme culture, Gen-Z slang, and zero-compromise modern systems architecture.**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![WebAssembly](https://img.shields.io/badge/WASM-Ready-purple.svg)](https://webassembly.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Tests](https://img.shields.io/badge/tests-23%2F23%20passing-brightgreen.svg)]()

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

### 6. Extended Primitives & Optimized Memory Layout

MEMElang features a high-performance 32-byte unified `Value` variant (24 bytes in WebAssembly) with fast numeric ALU execution paths, bitwise operations, and first-class compound collections:

| Datatype | Hindi / Alias | Literal Syntax | Description |
| :--- | :--- | :--- | :--- |
| `byte` | `chota_ank` / `u8` | `255b`, `0xffb`, `0b1010b` | 8-bit unsigned integer with bitwise operators |
| `int32` | `ank32` / `i32` | `int32(42)` | 32-bit signed integer with auto-promotion on overflow |
| `int` / `long` | `ank` / `bada_ank` / `i64` | `42`, `-100`, `0xFF`, `0b101` | 64-bit signed integer (`int64_t`) |
| `float32` | `f32` / `float` | `3.14f`, `-0.5f` | 32-bit single-precision IEEE 754 float |
| `double` | `f64` / `point` | `3.14`, `-0.5`, `1e-4` | 64-bit double-precision IEEE 754 float |
| `char` | `akshar` | `char("A")`, `'A'` | Single character primitive |
| `bytes` | `kaccha_data` / `bytearray` | `b"Hello\x00World"` | Mutable raw binary buffer, hex encoding & binary file I/O |
| `set` | `jhund` / `mandali` | `{1, 2, 3}`, `{"a", "b"}` | Order-preserving O(1) hash set with union `+` and difference `-` |
| `tuple` | `jodi` | `(1, "two", 3.0)`, `(1,)` | Fixed-size immutable sequence with indexing and slicing |

```meme
// 1. Raw Byte Buffers, Mutation & Binary File I/O
pakad buf = b"Hello";
buf[0] = 104;                                  // Mutable byte assignment ('h')
dikha buf.to_string();                         // "hello"
dikha buf.to_hex();                            // "68656c6c6f"

pakad decoded = bytes.from_hex("43686169");   // Decodes hex -> "Chai"
fs.write_bytes("drink.bin", decoded);          // Direct binary write
pakad readData = fs.read_bytes("drink.bin");   // Direct binary read

// 2. Hash Sets & Algebraic Set Operations
pakad s = {1, 2, 2, 3};                        // Auto-deduplicates: {1, 2, 3}
s.add(10);
s.remove(1);
dikha s.has(2);                                // han

pakad unionSet = s + {4, 5};                   // Set union
pakad diffSet = unionSet - {2};                // Set difference

// 3. Immutable Tuples
pakad tup = (10, "chai", 3.14f);
dikha tup[0];                                  // 10
panga {
    tup[0] = 99;                               // Error: tuples are strictly immutable!
} lafda (err) {
    dikha "Tuples cannot be mutated!";
}
```

---

### 7. Zero-WebView Native Desktop GUI

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

### 8. Preinstalled Standard Libraries ("Batteries Included")

Just like Python, MEMElang comes preinstalled with a complete, zero-dependency standard library available globally or via `import "module" as { ... }`:

| Module | WhatsApp Hinglish Alias | Key Capabilities |
| --- | --- | --- |
| **`math`** | `hisab` | Constants (`pi`, `e`, `tau`), roots (`sqrt`, `cbrt`), logs (`log`, `log2`, `log10`), trig (`sin`, `cos`, `tan`), hyperbolic, combinatorics (`gcd`, `lcm`, `factorial`, `comb`, `perm`), geometry (`hypot`, `dist`), and `clamp` |
| **`random`** | `tukka` | Full PRNG suite: `seed`, `random`, `randint`, `uniform`, `choice`, `shuffle`, `sample` |
| **`statistics`** | `stats` / `aankde` | Descriptive statistics: `mean`, `median`, `mode`, `variance`, `stdev`, `quantiles`, `min`, `max`, `sum` |
| **`crypto`** | `hashlib` / `gupt` | Industry standard hashing & encoding: `sha256`, `hmac_sha256`, `md5`, `sha1`, `base64_encode`, `base64_decode`, and RFC4122 `uuid` |
| **`regex`** | `re` / `dhundo` | Full PCRE regular expressions: `test`, `search`, `match`, `find_all`, `replace`, `split` |
| **`datetime`** | `waqt` / `time` | High-precision time and calendar: `now`, `now_ms`, `iso`, `format` (`strftime`), `parse` (`strptime`), year/month/day/hour getters, and non-blocking `sleep` |
| **`itertools`** | `jugaad_tools` | Fast iterators: global `range(stop)` / `range(start, stop, step)`, `zip(...)`, `enumerate(...)`, `chunk(...)`, `flatten(...)`, `repeat(...)` |
| **`collections`** | `mandali` | Specialized structures: `Counter(iterable)` (O(N) frequency tally) and `deque()` (O(1) double-ended queue) |
| **`csv`** | `csv` | Standalone CSV parser and generator: `csv.parse(text)` (with or without headers) and `csv.stringify(rows)` |
| **`test`** | `assert` / `pariksha` | Assertion utilities: `assert(cond, msg)`, `assert_eq(a, b)`, `assert_ne(a, b)`, and `assert_throws(fn)` |

```meme
// 1. Math & Itertools
pakad r = range(1, 10, 2);                      // [1, 3, 5, 7, 9]
pakad roots = r.map((n) => math.sqrt(n));
dikha f"GCD: {math.gcd(24, 36)}, Factorial: {math.factorial(5)}";

// 2. Statistics & Collections
pakad freqs = collections.Counter("antigravity");
dikha f"Letter 'a' count: {freqs.a}";
dikha f"Mean: {stats.mean([10, 20, 30])}, Stdev: {stats.stdev([2, 4, 4, 4, 5, 5, 7, 9])}";

// 3. Cryptography & Regular Expressions
pakad hash = crypto.sha256("chai-and-code");
pakad emailMatch = re.search("([a-z]+)@([a-z.]+)", "user@memelang.org");
dikha f"User: {emailMatch.groups[0]}, Domain: {emailMatch.groups[1]}";

// 4. CSV & Testing Assertions
pakad rawData = "item,qty\nSamosa,10\nJalebi,25";
pakad parsed = csv.parse(rawData);
assert_eq(parsed[0].item, "Samosa");
assert_eq(parsed.size, 2);
```

---

### 9. Fast HTTP Servers (Full Express Parity & Elysia Type Safety)

MEMElang features high-concurrency native multi-threaded HTTP servers powered by real OS sockets (`socket`, `SO_REUSEPORT`, `TCP_NODELAY`, worker thread pool) paired with complete Express middleware/routing parity and Elysia-style TypeBox schema validation:

* **Elysia Type Safety (`t` / `schema`)**: Automatic request & response schema validation with type coercion (`string` numbers coerced to `int` / `float`) and detailed 400 Bad Request error reports.
* **Full Express Parity**: Middleware pipeline (`app.use`), route-level middleware chains (`(req, res, next)`), sub-routers (`express.Router()`), `req.params`, `req.query`, `req.cookies`, `res.status().json()`, `res.sendFile()`, `express.json()`, `express.urlencoded()`, `express.cors()`, `express.static()`.
* **Elysia Ergonomics**: Context parameter (`ctx.body`, `ctx.params`, `ctx.query`, `ctx.store`, `ctx.set`), lifecycle hooks (`onRequest`, `onBeforeHandle`, `onAfterHandle`, `onError`), derivations (`app.derive`), and state management (`app.state`).
* **Universal Aliases**: `express()`, `elysia()`, `dhaba.banao()`, `server()`.

```meme
pakad app = elysia();

// 1. State & Derivations
app.state("visitors", 0);
app.derive((ctx) => {
    ye_le { "role": "admin" };
});

// 2. Elysia-Style Type-Safe Route with TypeBox Schema
app.post("/users/:id", {
    params: t.Object({ "id": t.Integer() }),
    query: t.Object({ "ref": t.Optional(t.String()) }),
    body: t.Object({
        "username": t.String({ "minLength": 3 }),
        "age": t.Integer({ "min": 18 }),
        "email": t.Optional(t.String({ "format": "email" }))
    })
}, (ctx) => {
    ctx.store.visitors = ctx.store.visitors + 1;
    // Parameters, query & body are validated, coerced & guaranteed type-safe
    ye_le {
        "success": han,
        "id": ctx.params.id,              // Coerced integer
        "user": ctx.body.username,
        "age": ctx.body.age,              // Coerced integer
        "role": ctx.role,
        "visits": ctx.store.visitors
    };
});

// 3. Express-Style Route & Sub-Router
pakad apiRouter = express.Router();
apiRouter.get("/health", (req, res) => {
    res.status(200).json({ "status": "ok", "ip": req.ip });
});
app.use("/api", apiRouter);

// 4. Start Live Multi-Threaded HTTP Server
app.listen(3000, () => {
    dikha "Server running on http://localhost:3000";
});
```

---

## 🎨 VS Code Extension & Syntax Suggestions

MEMElang provides first-class developer tooling with the official **MEMElang VS Code Extension** (`tools/vscode-memelang`):

- **Context-Aware IntelliSense & Suggestions**:
  - `app.` / `router.` -> Suggests all Express & Elysia routing, middleware, state, and lifecycle methods (`get`, `post`, `put`, `use`, `listen`, `state`, `derive`, `decorate`, etc.).
  - `t.` / `schema.` -> Suggests TypeBox schema builders (`String`, `Integer`, `Number`, `Boolean`, `Object`, `Array`, `Optional`, `Union`, etc.).
  - `req.` / `res.` / `ctx.` -> Suggests HTTP request properties, response helpers, and context containers.
  - Standard libraries (`math.`, `crypto.`, `random.`, `file.`, `rasta.`, `dhaga.`, `kaamgar.`, `Vaada.`) -> Suggests module methods on typing `.`.
  - Type annotations (after `: `) -> Suggests `ank`, `int32`, `byte`, `point`, `float32`, `char`, `bol`, `bytes`, `sach`, `soochi`, `khata`, `set`, `tuple`, `har_kism`.
- **TextMate Syntax Highlighting**: Full tokenization for `.meme` files across all Desi meme keywords, WhatsApp Hinglish aliases, and modern standard constructs.
- **Hover Documentation**: Interactive markdown tooltips displaying descriptions, type signatures, and example code on mouse hover.
- **Parameter Signature Help**: Active parameter hints when opening parentheses for functions, routes, and schemas.
- **Productivity Snippets**: Instant templates for Express servers, Elysia servers, TypeBox schemas, async functions, hardware threads, channels, and React UI components.

### Installation

```bash
# Link directly to your local VS Code extensions directory:
ln -s "$(pwd)/tools/vscode-memelang" ~/.vscode/extensions/memelang

# Or install the pre-packaged VSIX:
code --install-extension tools/vscode-memelang/memelang-2.0.0.vsix
```

---

## 🧪 Testing

MEMElang comes with an exhaustive test harness covering all 25 compiler, runtime, and ecosystem subsystems:

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
