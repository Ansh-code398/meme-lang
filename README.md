# 🎭 MEMElang

> **A high-performance, memory-safe, self-hosting programming language written in C++20 — engineered with Indian Desi meme culture, Gen-Z slang, and zero-compromise systems architecture.**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![Tests](https://img.shields.io/badge/tests-545%2F545%20passing-brightgreen.svg)](#testing)
[![Self-Hosting](https://img.shields.io/badge/self--hosting-stage%203%20bootstrap-gold.svg)](#self-hosting-compiler)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

---

## 🌟 Overview

MEMElang is a statically-typed, gradually-typed programming language that compiles to native machine code via C++20. It features:

- **⚡ AOT Native Compilation** — compile `.meme` files to native binaries with `./meme compile`, achieving Rust-level performance
- **🧠 Self-Hosting Compiler** — the MEMElang compiler (`compiler.meme`) is written entirely in MEMElang itself, with 3-stage bootstrap closure verified
- **🛡️ Rust-Inspired Memory Safety** — ownership, borrow checking, `Option`/`Result` types, and `?` error propagation
- **🌐 Full Polyglot Ecosystem** — async/await, HTTP servers, file I/O, threads, channels, native GUI, Python/Node/Rust FFI bridges
- **📦 Desi Meme Syntax** — dual-syntax keywords: every construct has both a Hinglish meme alias *and* a universal alias (Python/JS/TypeScript style)

---

## ⚡ Performance

Benchmarked on Apple Silicon macOS (Darwin arm64) with process-isolated Peak RSS:

### Runtime Throughput vs Interpreter

| Workload | Interpreted | C++ Native | Self-Hosted | Rust | Node.js | Python | Speedup |
|---|---|---|---|---|---|---|---|
| `fib(35)` | 2.913s | **0.0071s** | 0.0059s | 0.0052s | 0.040s | 0.020s | **411.9×** |
| `sieve(100k)` | 0.483s | **0.0058s** | 0.0059s | 0.0059s | 0.040s | 0.020s | **83.0×** |
| `math_loop(200k)` | 0.518s | **0.0083s** | 0.0065s | 0.0053s | 0.040s | 0.033s | **62.4×** |
| `tree_drop(14)` | 7.604s | **0.0075s** | — | 0.0045s | 0.038s | 0.022s | **1,009×** |
| `quicksort(5k)` | 1.696s | **0.0131s** | 0.0160s | 0.0124s | 0.047s | 0.029s | **129×** |

### Peak RSS Memory (process-isolated)

| Workload | Interpreted | C++ Native | Rust | Node.js | Python |
|---|---|---|---|---|---|
| `fib` | 8.3 MB | **1.4 MB** | 1.6 MB | 51.4 MB | 10.6 MB |
| `sieve` | 11.4 MB | **1.6 MB** | 1.7 MB | 56.0 MB | 11.5 MB |
| `tree_drop` | 14.1 MB | **6.5 MB** | 2.1 MB | 53.1 MB | 12.1 MB |
| `quicksort` | 9.0 MB | **4.0 MB** | 1.6 MB | 58.8 MB | 11.2 MB |

> Run benchmarks: `python3 benchmarks/run_comprehensive_benchmarks.py`

---

## 🚀 Quick Start

### Build from Source

```bash
git clone https://github.com/Ansh-code398/meme-lang.git
cd meme-lang

# Build the native CLI
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

# Run the full test suite (25 suites, 545 E2E tests)
make test
```

### Run Programs

```bash
# REPL
./meme

# Interpret a .meme file
./meme examples/hello.meme

# AOT compile to native binary (Rust-level performance)
./meme compile examples/fibonacci.meme -o fib
./fib

# Use the self-hosting compiler
./meme_stage2 myprogram.meme -o myprogram
./myprogram
```

---

## 📖 Language Syntax

Every construct has dual-syntax: a **Desi/Hinglish alias** and a **universal alias**.

### Variables & Types

```meme
pakad naam = "MEMElang"      // let naam = "MEMElang"
le saal: ank = 2026          // typed declaration
atal PI = 3.14159            // const PI = 3.14159

// Gradual typing
pakad x: ank = 42            // static int
pakad y = "chai"             // inferred string
pakad z: har_kism = na_pata  // dynamic any
```

### Functions & Overloading

```meme
// Basic function
kar greet(naam: soot) -> soot {
    wapas f"Namaste, {naam}!";
}

// Overloaded function (different signatures)
kar add(a: ank, b: ank) -> ank { wapas a + b; }
kar add(a: soot, b: soot) -> soot { wapas a + b; }

// Variadic function
kar sum(*nums) {
    pakad total = 0;
    ghoom n mein nums { total = total + n; }
    wapas total;
}

dikha greet("Duniya");     // Namaste, Duniya!
dikha add(10, 20);         // 30
dikha add("cha", "i");     // chai
```

### Control Flow

```meme
// if / else
dekh score >= 90 {
    dikha "Gold!";
} warna dekh score >= 60 {
    dikha "Pass!";
} warna {
    dikha "Fail!";
}

// for loop
ghoom i se 1 se 10 {
    dikha i;
}

// for-in
ghoom item mein [1, 2, 3] {
    dikha item;
}

// while
jabtak score < 100 {
    score = score + 1;
}
```

### Classes & OOP

```meme
biradari Stack {
    jugaad shuru() {
        apun.items = [];
    }

    jugaad push(val) {
        apun.items.append(val);
    }

    jugaad pop() {
        wapas apun.items.pop();
    }

    jugaad size() {
        wapas apun.items.length();
    }
}

pakad s = Stack();
s.push(10);
s.push(20);
dikha s.pop();   // 20
```

### Memory Safety (Rust-Inspired)

```meme
// Pointers
pakad x: ank = 42;
pakad ptr: *ank = &x;        // take address
dikha *ptr;                  // dereference → 42

// Move semantics
pakad a = [1, 2, 3];
pakad b = chal a;            // move ownership (a invalidated)
dikha b;

// Ownership transfer
pakad owner = "chai";
pakad moved = chal owner;    // owner is now invalid

// Const (immutable)
atal MAX: ank = 100;
// MAX = 200;  ← Error: cannot reassign const
```

### Option & Result Types

```meme
// Option type
kar find_item(id: ank) -> Kuch(soot) {
    dekh id == 1 { wapas Some("Chai"); }
    wapas None;
}

pakad result = find_item(1);
jaanch result {
    Some(val) => dikha f"Found: {val}";
    None      => dikha "Not found";
}

// Result type
kar divide(a: ank, b: ank) -> Sahi(point) {
    dekh b == 0 { wapas Err("Division by zero"); }
    wapas Ok(a / b);
}

// ? operator — propagate errors automatically
kar safe_calc(x: ank) -> Sahi(point) {
    pakad result = divide(x, 2)?;   // returns Err early if fails
    wapas Ok(result * 1.5);
}
```

### Pattern Matching

```meme
jaanch value {
    0           => dikha "Zero";
    1 | 2 | 3   => dikha "Small";
    n agar n > 100 => dikha f"Big: {n}";
    _           => dikha "Other";
}

// Destructuring
jaanch point {
    [x, y] agar x == y  => dikha "On diagonal";
    [x, y]              => dikha f"Point ({x}, {y})";
}
```

### Error Handling

```meme
panga {
    chillao Galti("Connection timeout", "NetworkError");
} lafda (err) {
    dikha err.baat;    // "Connection timeout"
    dikha err.kism;    // "NetworkError"
    dikha err.line;    // source line number
} pakka {
    dikha "Cleaned up!";
}

// Universal aliases work too
try {
    throw Error("oops");
} catch (e) {
    dikha e.message;
}
```

### Modules & Namespaces

```meme
// mymath.meme
potli mymath;
bech kar square(n) { wapas n * n; }
bech atal TAX_RATE = 0.18;

// main.meme
bula "mymath" jaise math;
dikha math.square(5);           // 25
dikha math.TAX_RATE;            // 0.18

// Namespaces
mohalla Geometry {
    kar area(r) { wapas 3.14159 * r * r; }
}
dikha Geometry::area(5);        // 78.54
```

### Async & Concurrency

```meme
// Async/await
kar fetch_data(url: soot) {
    pakad res = sabar fetch(url);
    wapas res.json();
}

// Hardware threads
pakad t = dhaga.chala(kar() {
    dikha "Running on thread!";
});
t.join();

// Channels
pakad ch = dhaga.naali(10);     // buffered channel, size 10
dhaga.bhejo(ch, "ping");
pakad msg = dhaga.lelo(ch);
```

### Rich `dikha` (print) Statement

```meme
// Colors & styles
dikha[rang="green"] "Success!";
dikha[rang="red", bold=sach] "Error!";
dikha[rang="cyan", italic=sach] "Info";

// Custom end character
dikha[ant=""] "No newline";
dikha[ant="\t"] "Tab-separated";

// Separators
dikha[alag=", "] "one" "two" "three";   // one, two, three

// F-strings with expressions
pakad items = 42;
dikha f"Processed {items} items in {time}ms";
```

### Environment Variables (`env` / `paryavaran` / `dotenv`)

MEMElang provides first-class environment variable support with full parity across both the tree-walking interpreter and AOT native binaries:

```meme
// Available globally or via module import
bula "env" jaise env;             // also: bula "paryavaran" jaise p; bula "dotenv" jaise d;

// 1. Reading environment variables
pakad port = env.get("PORT", "3000");   // or env.la("PORT")
pakad db_url = env("DATABASE_URL");     // direct callable syntax
pakad host = env["HOST"];               // map index syntax

// 2. Setting and unsetting
env.set("APP_ENV", "production");       // or env.rakh(...)
env["CACHE_TTL"] = "3600";              // map index assignment
env.delete("TEMP_KEY");                 // or env.hatao(...)

// 3. Inspection and membership
dekh env.has("API_KEY") {               // or env.hai(...)
    dikha "API key is configured!";
}
pakad all_vars = env.all();             // or env.sab() -> Map of all env vars

// 4. Loading .env files (with variable expansion, quotes, comments, export prefix)
env.load(".env.local", sach);           // or env.chadhau(".env")

// 5. Parsing .env strings into Maps
pakad cfg = env.parse("HOST=127.0.0.1\nPORT=8080\nURL=\"http://${HOST}:${PORT}/v1\"");
dikha cfg["URL"];                       // http://127.0.0.1:8080/v1
```

* **Automatic `.env` Loading**: Both the tree-walking interpreter (`./meme`) and AOT-compiled native machine binaries automatically load `.env` from the working directory on launch.
* **CLI `--env-file` Flag**: Run scripts with custom environments: `./meme --env-file=.env.production app.meme`.

---

## 🧠 Self-Hosting Compiler

The MEMElang compiler is written entirely in MEMElang itself (`compiler.meme`, ~1,700 LOC):

```bash
# Stage 0: host interpreter compiles compiler.meme → meme_stage1
./meme compiler.meme -o meme_stage1

# Stage 1: stage1 binary recompiles compiler.meme → meme_stage2
./meme_stage1 compiler.meme -o meme_stage2

# Stage 2: verify fixed-point closure (stage2 ≡ stage3)
./meme_stage2 compiler.meme -o meme_stage3
diff meme_stage2.cpp meme_stage3.cpp   # → 0 diff lines ✅
```

**Bootstrap SHA-256**: `9225137bf534a812b96ef8b7426e89a1c4a8a527925507f4253cb2439d785175`  
(identical for `meme_stage1.cpp`, `meme_stage2.cpp`, `meme_stage3.cpp`)

**Compiler architecture** (all in `compiler.meme`):
- Lexer — tokenizes MEMElang source, 100+ keywords
- Parser — recursive descent, full expression/statement AST
- Scope Resolver — symbol table, declaration tracking
- C++20 Codegen — emits `MemeRuntime.h`-compatible C++20
- CLI Driver — `sys.args` parsing, Clang++ invocation

---

## 📦 Standard Libraries

### Built-in Modules (C++ native, zero-overhead)

| Module | Alias | Key Methods |
|---|---|---|
| `math` | `ganit` | `sqrt`, `pow`, `floor`, `ceil`, `abs`, `sin`, `cos`, `log` |
| `random` | `takdeer` | `int`, `float`, `choice`, `shuffle`, `seed` |
| `file` | `fs` | `read`, `write`, `exists`, `lines`, `append`, `delete` |
| `env` | `paryavaran`, `dotenv` | `get`, `set`, `has`, `delete`, `all`, `load`, `parse` |
| `systumm` | `sys` | `args`, `exec`, `cwd`, `platform`, `exit`, `env` |
| `crypto` | `raaz` | `sha256`, `md5`, `hmac`, `base64_encode/decode` |
| `datetime` | `samay` | `now`, `parse`, `format`, `diff`, `utc` |
| `regex` | `niyam` | `match`, `findall`, `sub`, `split`, `test` |
| `itertools` | — | `range`, `zip`, `enumerate`, `chain`, `repeat`, `take` |
| `collections` | — | `Counter`, `deque`, `defaultdict`, `OrderedDict` |
| `csv` | — | `read`, `write`, `parse`, `DictReader` |
| `test` | — | `assert_eq`, `assert_ne`, `assert_true`, `run_suite` |

### Pure MEMElang Libraries (`lib/`)

```meme
bula "lib/dataframe" jaise { DataFrame, from_records };
bula "lib/jwt" jaise jwt;
bula "lib/colors" jaise colors;

// Tabular analytics
pakad df = from_records([{"item": "Chai", "price": 20}]);
dikha df.sum("price");

// JWT auth
pakad token = jwt.sign({"userId": 42}, "secret", 3600);
pakad user  = jwt.verify(token, "secret");

// Terminal colors
dikha colors.green("Success!");
dikha colors.red(f"Error: {msg}");
```

---

## 🌐 HTTP Servers & Networking

```meme
// Express-style HTTP server
bula "app" jaise app;
app.get("/", kar(req, res) {
    res.json({"status": "ok", "lang": "MEMElang"});
});
app.listen(3000);

// Global fetch
pakad res = sabar fetch("https://api.example.com/data", {
    "method": "POST",
    "token": "bearer_token",
    "body": {"key": "value"},
    "timeout": 5000
});
dikha res.json();
```

---

## 🖥️ Native Desktop GUI

```meme
bula "ui" jaise { View, Text, Button, lukao, dikhaUI };

pakad counter = 0;

pakad app_ui = View([
    Text(f"Count: {counter}", {"font-size": "24px", "color": "#333"}),
    Button("Increment", kar() {
        counter = counter + 1;
        lukao();
    }, {"background": "#4CAF50", "color": "white"}),
]);

dikhaUI(app_ui, {"title": "MEMElang Counter", "width": 400, "height": 300});
```

---

## 🦎 Polyglot FFI Bridges

```meme
// Python bridge (NumPy, Pandas)
bula "chameleon.py" jaise py;
py.import("numpy");
pakad arr = py.call("numpy.array", [1, 2, 3, 4, 5]);
dikha py.call("numpy.mean", arr);

// Node/NPM bridge
bula "chameleon.npm" jaise npm;
npm.import("lodash");
pakad result = npm.call("_.chunk", [1,2,3,4,5], 2);

// Rust bridge (Rayon parallel)
bula "chameleon.rust" jaise rs;
rs.load("my_lib.so");
pakad fast_result = rs.call("parallel_sum", large_array);
```

---

## 🧪 Testing

```bash
# Run all 25 test suites
make test

# Run the full E2E suite (545 tests across 5 tiers)
python3 tests/e2e/run_e2e_tests.py

# Individual suites
python3 tests/e2e/test_compiler_bootstrap_e2e.py   # 12/12
python3 tests/e2e/test_benchmarks_e2e.py            # 8/8
python3 tests/e2e/test_invariants_e2e.py            # 5/5
python3 tests/e2e/test_build_tooling_e2e.py         # 7/7

# Run application tests
./meme test todo_app
./meme test notes_desktop
```

**Test Coverage (E2E)**:
| Tier | Description | Tests |
|---|---|---|
| Tier 1 | Core Feature Coverage | 231 |
| Tier 2 | Boundary & Corner Cases | 105 |
| Tier 3 | Cross-Feature Combinations | 50 |
| Tier 4 | Real-World Scenarios | 39 |
| Tier 5 | Adversarial Hardening | 120 |
| **Total** | | **545** |

---

## 🏗️ Build Tooling (`tapri.json`)

```json
{
  "name": "my-app",
  "version": "1.0.0",
  "main": "src/main.meme",
  "dependencies": {
    "dataframe": "lib/dataframe.meme"
  }
}
```

```bash
meme build          # compile project to native binary
meme test           # run project test suite
meme run src/main   # interpret entry point
```

---

## 🌐 WebAssembly & Docs Playground

```bash
# Build WebAssembly binary
./scripts/build-wasm.sh

# Run interactive docs & playground
cd docs
npm install && npm run dev
```

---

## 📁 Project Structure

```
meme-lang/
├── src/
│   ├── interpreter/        # Tree-walking interpreter (Value, Environment, Lexer, Parser)
│   └── compiler/           # AOT native compiler (NativeCompiler.cpp, MemeRuntime.h)
├── compiler.meme           # Self-hosting compiler (1,700 LOC in pure MEMElang)
├── benchmarks/             # Benchmark suite (7 workloads, 4-way matrix)
│   ├── run_comprehensive_benchmarks.py
│   ├── BENCHMARKS.md       # Full results report
│   └── *.meme / *.rs / *.py / *.js
├── lib/                    # Pure MEMElang standard libraries
│   ├── dataframe.meme
│   ├── jwt.meme
│   └── colors.meme
├── tests/e2e/              # 545-test E2E suite (5 tiers)
├── examples/               # Example programs
├── tools/vscode-memelang/  # VS Code extension
├── docs/                   # Interactive playground (Vite + WASM)
└── Makefile                # Build system
```

---

## 🎨 VS Code Extension

```bash
# Link to VS Code extensions
ln -s "$(pwd)/tools/vscode-memelang" ~/.vscode/extensions/memelang

# Or install VSIX
code --install-extension tools/vscode-memelang/memelang-2.0.0.vsix
```

Features: syntax highlighting, IntelliSense, hover docs, parameter hints, snippets for all MEMElang constructs.

---

## 📜 License

MIT License. Designed and engineered for the community.
