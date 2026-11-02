# MEMElang VS Code Extension 🎭

Official Visual Studio Code extension providing **Context-Aware IntelliSense Autocomplete**, **Rich Syntax Highlighting**, **Hover Documentation**, **Parameter Signature Help**, and **Code Snippets** for **MEMElang** (`.meme` files).

## Features

- 🎨 **Rich Syntax Highlighting**:
  - Full TextMate grammar covering variables (`pakad`, `maan_lo`), functions (`jugaad`, `kaam`), lambdas (`=>`), control flow (`dekh`, `agar`, `warna`, `varnah`, `jabtak`, `ghoom`, `har`).
  - **Async / Await Concurrency** (`async`, `sabar`, `await`, `side_mein`, `asar`).
  - **Structured Error Handling** (`panga`, `try`, `lafda`, `catch`, `pakka`, `finally`, `chillao`, `throw`, `Galti`, `Error`).
  - **Preinstalled Standard Libraries** (`math`, `ganit`, `crypto`, `random`, `stats`, `samay`, `datetime`, `itertools`, `collections`, `csv`, `test`, `file`, `fs`, `rasta`, `path`, `dhaga`, `thread`, `kaamgar`, `mp`, `Vaada`, `Promise`, `jaal`, `http`).
  - **Fast HTTP Server & Frameworks** (`express`, `elysia`, `Elysia`, `dhaba`, `server`, `t`, `schema`, `nirmaan`).
  - **Tantrik Magic Methods** (`__jod__`, `__ghata__`, `__guna__`, `__bhaag__`, `__barabar__`, `__chota__`, `__la__`, `__daal__`, `__dikha__`, `__size__`, `__chala__`, `shuru`, `alvida`).
  - **React UI & Tailwind Parity** (`Component`, `useState`, `useEffect`, `useRef`, `VStack`, `HStack`, `Text`, `Button`, `Input`).
  - **F-String Interpolation** (`f"Hello {name}"` with nested expression scopes).

- ⚡ **Context-Aware IntelliSense & Suggestions**:
  - **Server Autocomplete**: Type `app.` or `router.` to see `get`, `post`, `put`, `patch`, `delete`, `use`, `listen`, `state`, `derive`, `decorate`, `onRequest`, etc.
  - **Schema Autocomplete**: Type `t.` or `schema.` to see `String`, `Integer`, `Number`, `Boolean`, `Object`, `Array`, `Optional`, `Union`, `Literal`, `Record`, `Any`.
  - **Request & Response**: Type `req.` to see `params`, `query`, `body`, `headers`, `ip`, or `res.` to see `status`, `json`, `send`, `html`, `redirect`, `cookie`.
  - **Context Object**: Type `ctx.` to see `body`, `params`, `query`, `headers`, `store`, `set`, `error`.
  - **Module-Specific Autocomplete**: Type `math.`, `crypto.`, `random.`, `file.`, `rasta.`, `dhaga.`, `kaamgar.`, `Vaada.` to get tailored method completions.
  - **Collection Transforms**: Type `.` on collections to see `.sab_pe`, `.chhaan`, `.nichod`, `.line_mein`, `.har_ek`, `.pakad_ek`, `.jodo`, `.hatao`.
  - **Tantrik Methods**: Type `__` to see all dunder operator overloading methods.
  - **Type Annotations**: Type `:` after variables to see all primitive & composite types (`ank`, `int32`, `byte`, `point`, `float32`, `char`, `bol`, `bytes`, `sach`, `soochi`, `khata`, `set`, `tuple`, `har_kism`).

- 💡 **Hover Documentation & Signature Help**:
  - Mouse hover over any keyword, standard library function, type, or framework method displays interactive Markdown tooltips with signatures and examples.
  - Typing `(` triggers active parameter signatures for `fetch()`, `app.get()`, `t.String()`, `useState()`, `express()`, and more.

- ✂️ **Productivity Snippets**:
  - `express_server` -> Full Express HTTP server template
  - `elysia_server` -> Elysia type-safe server with state & schema
  - `schema_object` -> `t.Object({ ... })` TypeBox schema definition
  - `async_kaam` -> Async function with `fetch` and `sabar`
  - `try_catch` -> Structured error handling with `panga`, `lafda (err)`, `pakka`
  - `thread_spawn` -> Spawn native OS thread with `dhaga.chala`
  - `channel` -> Thread-safe Go-style queue channel (`dhaga.naali`)
  - `mp_map` -> Multi-core CPU parallel work map (`kaamgar.sab_pe`)
  - `fetch_api` -> Asynchronous HTTP fetch call
  - `file_ops` -> File read and write operations
  - `react_component` -> Declarative UI component with state & Tailwind
  - `use_state` -> React-style `useState` hook
  - `test_suite` -> Unit test block with `test.describe` and assertions

- ⌨️ **Language Configuration**:
  - Auto-closing pairs for `()`, `[]`, `{}`, `""`, `''`, `f""`, `f''`.
  - Indentation rules and comment toggling (`//` and `/* */`).

---

## Installation

### Automatic / Symlink (Recommended)
This workspace is already linked to your VS Code extensions folder:
```bash
ln -s "$(pwd)/tools/vscode-memelang" ~/.vscode/extensions/memelang
```
Simply reload VS Code (`Developer: Reload Window` from the Command Palette `Cmd+Shift+P`) to start editing `.meme` files with full syntax completion!

### VSIX Package Installation
```bash
cd tools/vscode-memelang
npx @vscode/vsce package
code --install-extension memelang-2.0.0.vsix
```
