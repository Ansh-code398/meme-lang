const vscode = require('vscode');

/**
 * MEMElang VS Code Extension - IntelliSense, Autocomplete & Tooling
 * Covers standard, Desi meme, WhatsApp Hinglish, Express/Elysia servers,
 * standard libraries, async/threading, and React UI parity.
 */

// 1. Language Keywords
const KEYWORDS = [
  // Variable Declarations
  { label: 'pakad', detail: 'Variable declaration', doc: 'Declare a new variable with lexical scoping (e.g. `pakad x = 10;`)' },
  { label: 'maan_lo', detail: 'Variable declaration (Hinglish)', doc: 'WhatsApp Hinglish variable declaration (e.g. `maan_lo score = 100;`)' },
  { label: 'le', detail: 'Variable declaration alias', doc: 'Short variable declaration (e.g. `le count = 1;`)' },
  { label: 'let', detail: 'Variable declaration alias', doc: 'Standard JavaScript/TypeScript variable declaration' },
  { label: 'const', detail: 'Constant variable declaration', doc: 'Immutable variable declaration' },

  // Functions
  { label: 'jugaad', detail: 'Function declaration', doc: 'Define a named function or anonymous closure (e.g. `jugaad calc(a, b = 0) { ye_le a + b; }`)' },
  { label: 'kaam', detail: 'Function declaration (Hinglish)', doc: 'WhatsApp Hinglish function keyword (e.g. `kaam jod(a, b) { ye_le a + b; }`)' },
  { label: 'fn', detail: 'Function alias', doc: 'Concise function declaration keyword' },
  { label: 'def', detail: 'Function alias (Python)', doc: 'Pythonic function declaration keyword' },
  { label: 'ye_le', detail: 'Return statement', doc: 'Return a value from a function (e.g. `ye_le result;`)' },
  { label: 'de', detail: 'Return alias', doc: 'Return value shorthand (e.g. `de total;`)' },
  { label: 'wapas', detail: 'Return alias', doc: 'Return value alias (e.g. `wapas ans;`)' },
  { label: 'return', detail: 'Return statement (JS/Py)', doc: 'Standard return statement keyword' },

  // Async / Await & Concurrency
  { label: 'async', detail: 'Async function modifier', doc: 'Marks a function as asynchronous, returning a `Vaada` (`Promise`)' },
  { label: 'side_mein', detail: 'Async function modifier (Hinglish)', doc: 'WhatsApp Hinglish alias for async background function' },
  { label: 'asar', detail: 'Async function postfix modifier', doc: 'Postfix async modifier (e.g. `kaam fetchData() asar { ... }`)' },
  { label: 'sabar', detail: 'Await expression', doc: 'Awaits an async Vaada, fetch request, or thread handle (e.g. `res = sabar fetch(url);`)' },
  { label: 'await', detail: 'Await expression (JS/Py)', doc: 'Standard await keyword for promises and async tasks' },

  // Classes & OOP
  { label: 'biradari', detail: 'Class declaration', doc: 'Define a new class/blueprint (e.g. `biradari Khiladi { ... }`)' },
  { label: 'vibe', detail: 'Class declaration alias', doc: 'Modern meme alias for class declaration' },
  { label: 'sancha', detail: 'Class declaration (blueprint)', doc: 'Traditional Hindi/Hinglish blueprint class declaration' },
  { label: 'class', detail: 'Class declaration (JS/Py)', doc: 'Standard OOP class declaration' },
  { label: 'nepo', detail: 'Class inheritance (extends)', doc: 'Inherit from another class (e.g. `biradari Beta nepo Baap`)' },
  { label: 'extends', detail: 'Class inheritance (JS)', doc: 'Standard OOP class inheritance keyword' },
  { label: 'khud', detail: 'Instance reference (this)', doc: 'Current instance pointer inside methods (e.g. `khud.balance`)' },
  { label: 'apun', detail: 'Instance reference (self)', doc: 'Desi alias for current instance (e.g. `apun.naam`)' },
  { label: 'this', detail: 'Instance reference (JS)', doc: 'Standard this pointer keyword' },
  { label: 'self', detail: 'Instance reference (Py)', doc: 'Standard Pythonic self keyword' },
  { label: 'se', detail: 'Super class reference', doc: 'Call superclass constructor or methods (e.g. `se.shuru(...)`)' },
  { label: 'super', detail: 'Super class reference (JS)', doc: 'Call superclass methods' },

  // Control Flow
  { label: 'dekh', detail: 'If condition', doc: 'Branch control flow if condition is truthy (e.g. `dekh score > 50 { ... }`)' },
  { label: 'agar', detail: 'If condition (Hinglish)', doc: 'WhatsApp Hinglish if branch (e.g. `agar age >= 18 { ... }`)' },
  { label: 'if', detail: 'If condition (Standard)', doc: 'Standard if conditional branch' },
  { label: 'warna', detail: 'Else condition', doc: 'Fallback branch when condition is false (e.g. `warna { ... }`)' },
  { label: 'nahito', detail: 'Else condition alias', doc: 'Fallback branch alias (e.g. `nahito { ... }`)' },
  { label: 'varnah', detail: 'Else condition (Hinglish)', doc: 'WhatsApp Hinglish else branch' },
  { label: 'else', detail: 'Else condition (Standard)', doc: 'Standard fallback branch' },
  { label: 'jabtak', detail: 'While loop', doc: 'Loop repeatedly while condition is truthy (e.g. `jabtak i < 10 { ... }`)' },
  { label: 'jabk', detail: 'While loop (Hinglish)', doc: 'WhatsApp Hinglish while loop' },
  { label: 'while', detail: 'While loop (Standard)', doc: 'Standard while loop statement' },
  { label: 'ghoom', detail: 'For loop', doc: 'Iterate over ranges or collections (e.g. `ghoom pakad x mein list { ... }`)' },
  { label: 'har', detail: 'For-each loop (Hinglish)', doc: 'WhatsApp Hinglish for-each loop (e.g. `har x mein items { ... }`)' },
  { label: 'for', detail: 'For loop (Standard)', doc: 'Standard for loop statement' },
  { label: 'mein', detail: 'In keyword', doc: 'Membership operator in loops and collections (e.g. `item mein list`)' },
  { label: 'in', detail: 'In keyword (Standard)', doc: 'Standard collection membership operator' },
  { label: 'bas', detail: 'Break statement', doc: 'Break out of the current loop immediately' },
  { label: 'roko', detail: 'Break statement (Hinglish)', doc: 'WhatsApp Hinglish break statement' },
  { label: 'break', detail: 'Break statement (Standard)', doc: 'Standard loop break keyword' },
  { label: 'chal', detail: 'Continue statement', doc: 'Skip to the next iteration of the loop' },
  { label: 'karo', detail: 'Continue / do statement', doc: 'Continue next iteration or do block' },
  { label: 'continue', detail: 'Continue statement (Standard)', doc: 'Standard loop continue keyword' },

  // Pattern Matching (Switch)
  { label: 'jaanch', detail: 'Pattern match (switch)', doc: 'Pattern matching control flow (e.g. `jaanch status { "ok" => ... }`)' },
  { label: 'parakh', detail: 'Pattern match alias', doc: 'Match value against branches' },
  { label: 'match', detail: 'Pattern match (Python/Rust)', doc: 'Pattern match construct' },
  { label: 'baaki', detail: 'Default pattern / Variadic rest', doc: 'Default match case or variadic rest parameter (*baaki)' },
  { label: 'default', detail: 'Default pattern case', doc: 'Fallback default pattern branch' },

  // Exceptions / Error Handling
  { label: 'panga', detail: 'Try block', doc: 'Begin a safe block to catch runtime errors (e.g. `panga { ... }`)' },
  { label: 'koshish', detail: 'Try block (Hinglish)', doc: 'WhatsApp Hinglish try block' },
  { label: 'try', detail: 'Try block (Standard)', doc: 'Standard try block construct' },
  { label: 'lafda', detail: 'Catch block', doc: 'Catch exceptions thrown in panga block (e.g. `lafda (err) { ... }`)' },
  { label: 'sambhal', detail: 'Catch block alias', doc: 'Catch exception handler block' },
  { label: 'catch', detail: 'Catch block (JS)', doc: 'Standard catch block' },
  { label: 'except', detail: 'Catch block (Python)', doc: 'Pythonic exception catch block' },
  { label: 'pakka', detail: 'Finally block', doc: 'Always executes regardless of error or return (e.g. `pakka { ... }`)' },
  { label: 'hamesha', detail: 'Finally block (Hinglish)', doc: 'WhatsApp Hinglish finally block' },
  { label: 'finally', detail: 'Finally block (Standard)', doc: 'Standard finally block construct' },
  { label: 'chillao', detail: 'Throw error', doc: 'Throw an error or exception (e.g. `chillao Galti("Invalid input");`)' },
  { label: 'fek', detail: 'Throw error alias', doc: 'Throw an exception shorthand' },
  { label: 'throw', detail: 'Throw error (JS)', doc: 'Standard throw statement' },
  { label: 'raise', detail: 'Raise error (Python)', doc: 'Pythonic raise exception statement' },

  // Modules & Imports
  { label: 'bula', detail: 'Module import', doc: 'Import a module or file (e.g. `bula "math" jaise m;`)' },
  { label: 'chura', detail: 'Module import (meme)', doc: 'Playful meme module import' },
  { label: 'lao', detail: 'Module import alias', doc: 'Hinglish import statement' },
  { label: 'import', detail: 'Module import (Standard)', doc: 'Standard module import keyword' },
  { label: 'jaise', detail: 'Import alias (as)', doc: 'Alias imported identifier (e.g. `bula "fs" jaise file;`)' },
  { label: 'as', detail: 'Import alias (Standard)', doc: 'Standard import alias keyword' },

  // Enums
  { label: 'avtar', detail: 'Enum declaration', doc: 'Define an enumeration with variants (e.g. `avtar Disha { UTTAR, DAKSHIN }`)' },
  { label: 'kism', detail: 'Enum declaration alias', doc: 'Define an enum type category' },
  { label: 'enum', detail: 'Enum declaration (Standard)', doc: 'Standard enum type declaration' },

  // Constants
  { label: 'han', detail: 'Boolean true', doc: 'Truthy boolean constant (synonyms: real, sahi, true)' },
  { label: 'real', detail: 'Boolean true alias', doc: 'Truthy boolean constant' },
  { label: 'sahi', detail: 'Boolean true (Hinglish)', doc: 'Truthy boolean constant' },
  { label: 'true', detail: 'Boolean true (Standard)', doc: 'Standard boolean true' },
  { label: 'nhi', detail: 'Boolean false / Logical NOT', doc: 'Falsy boolean constant or prefix logical not' },
  { label: 'fake', detail: 'Boolean false alias', doc: 'Falsy boolean constant' },
  { label: 'galat', detail: 'Boolean false (Hinglish)', doc: 'Falsy boolean constant' },
  { label: 'false', detail: 'Boolean false (Standard)', doc: 'Standard boolean false' },
  { label: 'nalla', detail: 'Null / Monostate', doc: 'Empty, uninitialized or null value constant' },
  { label: 'shunya', detail: 'Null alias (zero/void)', doc: 'Null / void constant' },
  { label: 'khali', detail: 'Null alias (empty)', doc: 'Null / empty constant' },
  { label: 'null', detail: 'Null (Standard)', doc: 'Standard null constant' }
];

// 2. Types & Primitive Annotations
const TYPES = [
  { label: 'ank', detail: 'Type: 64-bit integer', doc: 'Primitive 64-bit signed integer (`int64` / `long long`)' },
  { label: 'int', detail: 'Type: 64-bit integer alias', doc: '64-bit signed integer' },
  { label: 'int32', detail: 'Type: 32-bit integer', doc: 'Memory-compact 32-bit signed integer' },
  { label: 'byte', detail: 'Type: 8-bit unsigned byte', doc: 'Unsigned 8-bit byte (0-255) for binary buffers' },
  { label: 'point', detail: 'Type: 64-bit float (double)', doc: 'Double-precision 64-bit IEEE 754 floating point' },
  { label: 'float', detail: 'Type: 64-bit float alias', doc: 'Double-precision float' },
  { label: 'float32', detail: 'Type: 32-bit float', doc: 'Single-precision 32-bit floating point' },
  { label: 'char', detail: 'Type: Character', doc: 'Single Unicode/ASCII character primitive' },
  { label: 'bol', detail: 'Type: String', doc: 'Primitive UTF-8 string text type (e.g. `pakad name: bol = "MEME";`)' },
  { label: 'string', detail: 'Type: String alias', doc: 'UTF-8 string text type' },
  { label: 'bytes', detail: 'Type: Byte array buffer', doc: 'Raw binary byte buffer (`std::vector<uint8_t>`)' },
  { label: 'sach', detail: 'Type: Boolean', doc: 'Primitive boolean truth value type (`han` / `nhi`)' },
  { label: 'bool', detail: 'Type: Boolean alias', doc: 'Primitive boolean type' },
  { label: 'soochi', detail: 'Type: List array', doc: 'Generic dynamic list collection type (e.g. `soochi[ank]`)' },
  { label: 'list', detail: 'Type: List array alias', doc: 'Generic dynamic list collection type' },
  { label: 'khata', detail: 'Type: Hash map', doc: 'Generic key-value map collection type (e.g. `khata[bol, ank]`)' },
  { label: 'map', detail: 'Type: Hash map alias', doc: 'Generic key-value map collection type' },
  { label: 'set', detail: 'Type: Unique element set', doc: 'Fast O(1) hash set for unique elements' },
  { label: 'tuple', detail: 'Type: Immutable tuple', doc: 'Fixed-length immutable heterogeneous sequence' },
  { label: 'har_kism', detail: 'Type: Dynamic any', doc: 'Dynamic escape hatch type that bypasses static type checks' },
  { label: 'any', detail: 'Type: Dynamic any alias', doc: 'Dynamic escape hatch type' },
  { label: 'nalla', detail: 'Type: Null / Void', doc: 'Null or void return type constant' },
  { label: 'void', detail: 'Type: Void return type', doc: 'Function returning nothing' }
];

// 3. Built-in Global Functions & Constructors
const BUILTINS = [
  { label: 'dikha', detail: 'Builtin: Print output', doc: 'Print expressions separated by space, ending with newline (e.g. `dikha "Hello", name;`)' },
  { label: 'print', detail: 'Builtin: Print alias', doc: 'Print expressions to standard output' },
  { label: 'pooch', detail: 'Builtin: Console stdin prompt', doc: 'Prompt user for input and return string (e.g. `name = pooch("Naam? ");`)' },
  { label: 'input', detail: 'Builtin: Console stdin prompt alias', doc: 'Prompt user for input' },
  { label: 'size', detail: 'Builtin: Length / size measurement', doc: 'Returns length of string, list, map, set, or custom object with __size__' },
  { label: 'kya_hai', detail: 'Builtin: Type name inspection', doc: 'Returns string representation of type (e.g. "int", "float", "string", "list", "map")' },
  { label: 'kundli', detail: 'Builtin: Reflection inspector', doc: 'Returns map of object fields or list of class methods' },
  { label: 'nakal', detail: 'Builtin: Deep clone', doc: 'Creates a deep copy clone of any value or collection' },
  { label: 'bina_point', detail: 'Builtin: Integer conversion', doc: 'Converts float, string, or boolean to 64-bit integer' },
  { label: 'point_wala', detail: 'Builtin: Float conversion', doc: 'Converts integer or string to 64-bit float' },
  { label: 'ruk', detail: 'Builtin: Sleep', doc: 'Pause execution for N milliseconds (e.g. `ruk(500);`)' },
  { label: 'ab', detail: 'Builtin: Current timestamp', doc: 'Returns current Unix epoch time in milliseconds' },
  { label: 'Galti', detail: 'Constructor: Structured Error', doc: 'Creates a structured error object `Galti(message, type = "UserLafda")`' },
  { label: 'Error', detail: 'Constructor: Structured Error alias', doc: 'Creates a structured error object `Error(message, type)`' },
  { label: 'fetch', detail: 'Builtin: HTTP fetch client', doc: 'Sends an HTTP request: `res = sabar fetch(url, options)`' },
  { label: 'express', detail: 'Framework: Express server factory', doc: 'Creates an Express-compatible HTTP server instance: `app = express()`' },
  { label: 'elysia', detail: 'Framework: Elysia server factory', doc: 'Creates an Elysia-compatible type-safe HTTP server instance: `app = elysia()`' },
  { label: 'Elysia', detail: 'Framework: Elysia constructor', doc: 'Instantiates an Elysia type-safe HTTP server: `app = Elysia()`' },
  { label: 'dhaba', detail: 'Framework: Fast HTTP server module', doc: 'Native HTTP server module with `.banao()`, `.router()`, `.json()`, `.cors()`' },
  { label: 'server', detail: 'Framework: HTTP server factory alias', doc: 'Factory alias for creating an HTTP server: `app = server()`' },
  { label: 't', detail: 'Validation: TypeBox Schema Builder', doc: 'Elysia TypeBox schema builder: `t.String()`, `t.Integer()`, `t.Object()`, etc.' },
  { label: 'schema', detail: 'Validation: Schema Builder alias', doc: 'Alias for TypeBox schema builder `t`' },
  { label: 'nirmaan', detail: 'Validation: Schema Builder (Desi)', doc: 'Desi alias for TypeBox schema builder `t`' }
];

// 4. Standard Modules
const MODULES = [
  { label: 'math', detail: 'Module: Advanced Mathematics', doc: 'Standard math library: `math.sqrt`, `math.pow`, `math.sin`, `math.cos`, `math.pi`, etc.' },
  { label: 'ganit', detail: 'Module: Mathematics (Desi)', doc: 'Desi alias for standard math library' },
  { label: 'random', detail: 'Module: Cryptographic & Pseudo RNG', doc: 'Random generation: `random.randint`, `random.choice`, `random.shuffle`, `random.uniform`' },
  { label: 'stats', detail: 'Module: Statistics', doc: 'Statistical math: `stats.mean`, `stats.median`, `stats.stdev`, `stats.variance`, `stats.sum`' },
  { label: 'crypto', detail: 'Module: Cryptography & Hashing', doc: 'Cryptographic hashing: `crypto.sha256`, `crypto.md5`, `crypto.base64_encode`, `crypto.random_uuid`' },
  { label: 'regex', detail: 'Module: Regular Expressions', doc: 'Regex parsing: `regex.match`, `regex.search`, `regex.findall`, `regex.replace`, `regex.split`' },
  { label: 'samay', detail: 'Module: DateTime & Timers', doc: 'Date & time: `samay.ab()`, `samay.now()`, `samay.tarikh()`, `samay.parse()`, `samay.format()`' },
  { label: 'datetime', detail: 'Module: DateTime alias', doc: 'Standard datetime library module' },
  { label: 'itertools', detail: 'Module: Iteration Tools', doc: 'Combinatorics & generators: `itertools.range`, `itertools.zip`, `itertools.enumerate`, `itertools.flatten`' },
  { label: 'collections', detail: 'Module: Advanced Collections', doc: 'High-performance data structures: `collections.Counter`, `collections.Deque`, `collections.DefaultDict`' },
  { label: 'csv', detail: 'Module: CSV Parsing & Serialization', doc: 'Fast CSV processing: `csv.parse`, `csv.stringify`, `csv.read`, `csv.write`' },
  { label: 'test', detail: 'Module: Unit Testing Framework', doc: 'Testing harness: `test.describe`, `test.it`, `test.assert_eq`, `test.assert_true`, `test.assert_throws`' },
  { label: 'file', detail: 'Module: File System', doc: 'File operations: `file.padh`, `file.likh`, `file.lines`, `file.padh_csv`, `file.nakal`, `file.folder_bana`' },
  { label: 'fs', detail: 'Module: File System (Standard)', doc: 'File operations: `fs.read`, `fs.write`, `fs.read_lines`, `fs.mkdir`, `fs.glob`, `fs.remove`' },
  { label: 'rasta', detail: 'Module: Path Utilities', doc: 'Cross-platform path utils: `rasta.jod`, `rasta.naam`, `rasta.folder`, `rasta.asli`, `rasta.ext`' },
  { label: 'path', detail: 'Module: Path Utilities (Standard)', doc: 'Path manipulation: `path.join`, `path.basename`, `path.dirname`, `path.resolve`, `path.ext`' },
  { label: 'dhaga', detail: 'Module: Multi-Threading', doc: 'Hardware threads: `dhaga.chala(fn, ...args)`, `dhaga.tala()` (mutex), `dhaga.naali()` (channel)' },
  { label: 'thread', detail: 'Module: Multi-Threading (Standard)', doc: 'Thread spawning and synchronization: `thread.spawn`, `thread.mutex`, `thread.channel`' },
  { label: 'kaamgar', detail: 'Module: Multi-Processing', doc: 'Multi-core parallelism: `kaamgar.sab_pe(fn, list, cores)`, `kaamgar.alag(cmd)`' },
  { label: 'mp', detail: 'Module: Multi-Processing (Standard)', doc: 'Multi-core map & subprocess: `mp.map(fn, list, cores)`, `mp.spawn(cmd)`, `mp.cpu_count()`' },
  { label: 'Vaada', detail: 'Module: Promise Concurrency', doc: 'Promises: `Vaada.sab([p1, p2])`, `Vaada.race([p1, p2])`, `Vaada.ruk(ms)`' },
  { label: 'Promise', detail: 'Module: Promise (Standard)', doc: 'Promise combinators: `Promise.all([p1, p2])`, `Promise.race([p1, p2])`, `Promise.sleep(ms)`' },
  { label: 'jaal', detail: 'Module: HTTP Client', doc: 'HTTP client: `jaal.get`, `jaal.post`, `jaal.put`, `jaal.patch`, `jaal.delete`, `jaal.download`' },
  { label: 'http', detail: 'Module: HTTP Client (Standard)', doc: 'HTTP client: `http.get`, `http.post`, `http.put`, `http.delete`, `http.download`' },
  { label: 'systumm', detail: 'Module: OS & System Runtime', doc: 'System info & process execution: `systumm.env`, `systumm.argv`, `systumm.exit`, `systumm.chalao`' },
  { label: 'os', detail: 'Module: OS & System Runtime (Standard)', doc: 'OS environment and process control' }
];

// 5. Express & Elysia Server Methods (Dot completions for app/server)
const SERVER_METHODS = [
  { label: 'get', detail: 'Method: app.get(path, ...handlers)', doc: 'Register a GET route handler: `app.get("/users", (req, res) => { res.json(users); })`' },
  { label: 'post', detail: 'Method: app.post(path, ...handlers, schema?)', doc: 'Register a POST route handler with optional Elysia schema validation' },
  { label: 'put', detail: 'Method: app.put(path, ...handlers, schema?)', doc: 'Register a PUT route handler' },
  { label: 'patch', detail: 'Method: app.patch(path, ...handlers, schema?)', doc: 'Register a PATCH route handler' },
  { label: 'delete', detail: 'Method: app.delete(path, ...handlers, schema?)', doc: 'Register a DELETE route handler' },
  { label: 'head', detail: 'Method: app.head(path, ...handlers)', doc: 'Register a HEAD route handler' },
  { label: 'options', detail: 'Method: app.options(path, ...handlers)', doc: 'Register an OPTIONS route handler' },
  { label: 'all', detail: 'Method: app.all(path, ...handlers)', doc: 'Register a handler matching all HTTP verbs' },
  { label: 'use', detail: 'Method: app.use(prefix?, middleware)', doc: 'Mount a middleware or sub-router: `app.use(express.json())` or `app.use("/api", router)`' },
  { label: 'listen', detail: 'Method: app.listen(port, callback?)', doc: 'Start native multithreaded socket server: `app.listen(3000, () => { dikha "Online"; })`' },
  { label: 'state', detail: 'Method: app.state(key, initialVal)', doc: 'Define global reactive state in Elysia server: `app.state("counter", 0)`' },
  { label: 'derive', detail: 'Method: app.derive(fn)', doc: 'Derive custom properties into Elysia `ctx`: `app.derive(ctx => ({ user: decode(ctx) }))`' },
  { label: 'decorate', detail: 'Method: app.decorate(key, valOrFn)', doc: 'Decorate global helpers into server context: `app.decorate("version", "2.0")`' },
  { label: 'onRequest', detail: 'Hook: app.onRequest(fn)', doc: 'Lifecycle hook called immediately when a request arrives' },
  { label: 'onBeforeHandle', detail: 'Hook: app.onBeforeHandle(fn)', doc: 'Lifecycle hook called before route handler executes' },
  { label: 'onAfterHandle', detail: 'Hook: app.onAfterHandle(fn)', doc: 'Lifecycle hook called after route handler executes to transform response' },
  { label: 'onError', detail: 'Hook: app.onError(fn)', doc: 'Lifecycle hook called when an error occurs during request processing' },
  { label: 'request', detail: 'Method: app.request(method, path, body?, headers?)', doc: 'In-memory fast test request dispatch without opening a network socket' },
  { label: 'handle', detail: 'Method: app.handle(req)', doc: 'Low-level in-memory request handler dispatch' }
];

// 6. Schema Builder Methods (Dot completions for t / schema / nirmaan)
const SCHEMA_METHODS = [
  { label: 'String', detail: 'Schema: t.String(options?)', doc: 'Validate string fields. Options: `{ minLength, maxLength, pattern }`' },
  { label: 'Number', detail: 'Schema: t.Number(options?)', doc: 'Validate numeric floating-point fields. Options: `{ minimum, maximum }`' },
  { label: 'Integer', detail: 'Schema: t.Integer(options?)', doc: 'Validate integer fields with auto-coercion. Options: `{ minimum, maximum }`' },
  { label: 'Boolean', detail: 'Schema: t.Boolean()', doc: 'Validate boolean fields with auto-coercion from "true"/"false"/"han"/"nhi"' },
  { label: 'Array', detail: 'Schema: t.Array(itemSchema, options?)', doc: 'Validate array of items: `t.Array(t.String(), { minItems: 1 })`' },
  { label: 'Object', detail: 'Schema: t.Object(propertiesMap)', doc: 'Validate structured object: `t.Object({ name: t.String(), age: t.Integer() })`' },
  { label: 'Optional', detail: 'Schema: t.Optional(innerSchema)', doc: 'Mark a field as optional: `t.Optional(t.String())`' },
  { label: 'Union', detail: 'Schema: t.Union(schemasList)', doc: 'Validate against any of the listed schema variants' },
  { label: 'Literal', detail: 'Schema: t.Literal(exactValue)', doc: 'Validate exact constant value: `t.Literal("admin")`' },
  { label: 'Record', detail: 'Schema: t.Record(valueSchema)', doc: 'Validate key-value map where all values match `valueSchema`' },
  { label: 'Any', detail: 'Schema: t.Any()', doc: 'Bypass validation for dynamic fields' }
];

// 7. Request & Response Object Methods
const REQ_PROPERTIES = [
  { label: 'params', detail: 'Property: req.params', doc: 'URL path parameters dictionary (e.g. `req.params.id`)' },
  { label: 'query', detail: 'Property: req.query', doc: 'URL query string parameters dictionary (e.g. `req.query.tab`)' },
  { label: 'body', detail: 'Property: req.body', doc: 'Parsed JSON or form body payload' },
  { label: 'headers', detail: 'Property: req.headers', doc: 'HTTP incoming request headers dictionary' },
  { label: 'cookies', detail: 'Property: req.cookies', doc: 'Parsed HTTP cookies dictionary' },
  { label: 'ip', detail: 'Property: req.ip', doc: 'Client IP address string' },
  { label: 'method', detail: 'Property: req.method', doc: 'HTTP method string (GET, POST, etc.)' },
  { label: 'path', detail: 'Property: req.path', doc: 'Request URL pathname' },
  { label: 'get', detail: 'Method: req.get(headerName)', doc: 'Case-insensitive header lookup: `req.get("authorization")`' }
];

const RES_METHODS = [
  { label: 'status', detail: 'Method: res.status(code)', doc: 'Set HTTP response status code (chainable): `res.status(201).json(data)`' },
  { label: 'json', detail: 'Method: res.json(data)', doc: 'Send JSON response with `application/json` header: `res.json({ ok: han })`' },
  { label: 'send', detail: 'Method: res.send(data)', doc: 'Send response data (auto-detects string, html, or json)' },
  { label: 'html', detail: 'Method: res.html(htmlString)', doc: 'Send HTML response with `text/html` header' },
  { label: 'text', detail: 'Method: res.text(plainText)', doc: 'Send plain text response with `text/plain` header' },
  { label: 'redirect', detail: 'Method: res.redirect(url, code = 302)', doc: 'Redirect client to destination URL' },
  { label: 'set', detail: 'Method: res.set(header, value)', doc: 'Set outgoing HTTP response header' },
  { label: 'cookie', detail: 'Method: res.cookie(name, value, opts?)', doc: 'Set HTTP cookie: `res.cookie("token", "xyz", { maxAge: 3600 })`' },
  { label: 'clearCookie', detail: 'Method: res.clearCookie(name)', doc: 'Clear HTTP cookie' },
  { label: 'sendFile', detail: 'Method: res.sendFile(filePath)', doc: 'Stream static file to client with auto MIME-type detection' }
];

const CTX_PROPERTIES = [
  { label: 'body', detail: 'Property: ctx.body', doc: 'Validated request body (Elysia)' },
  { label: 'params', detail: 'Property: ctx.params', doc: 'Validated URL path parameters (Elysia)' },
  { label: 'query', detail: 'Property: ctx.query', doc: 'Validated query string parameters (Elysia)' },
  { label: 'headers', detail: 'Property: ctx.headers', doc: 'Request headers dictionary (Elysia)' },
  { label: 'store', detail: 'Property: ctx.store', doc: 'Global reactive state container in Elysia: `ctx.store.visits`' },
  { label: 'ip', detail: 'Property: ctx.ip', doc: 'Client IP address' },
  { label: 'path', detail: 'Property: ctx.path', doc: 'Request path URL' },
  { label: 'method', detail: 'Property: ctx.method', doc: 'Request HTTP method' },
  { label: 'set', detail: 'Property: ctx.set', doc: 'Elysia response settings: `ctx.set.status = 201`, `ctx.set.headers["X-Key"] = "Val"`' },
  { label: 'error', detail: 'Method: ctx.error(code, message)', doc: 'Return custom HTTP error response in Elysia' }
];

// 8. Math Methods (Dot completions for math / ganit)
const MATH_METHODS = [
  { label: 'sqrt', detail: 'math.sqrt(x)', doc: 'Square root of x' },
  { label: 'cbrt', detail: 'math.cbrt(x)', doc: 'Cube root of x' },
  { label: 'pow', detail: 'math.pow(base, exp)', doc: 'Calculates base raised to power of exp' },
  { label: 'hypot', detail: 'math.hypot(x, y)', doc: 'Euclidean hypotenuse sqrt(x*x + y*y)' },
  { label: 'sin', detail: 'math.sin(rad)', doc: 'Trigonometric sine (in radians)' },
  { label: 'cos', detail: 'math.cos(rad)', doc: 'Trigonometric cosine (in radians)' },
  { label: 'tan', detail: 'math.tan(rad)', doc: 'Trigonometric tangent (in radians)' },
  { label: 'asin', detail: 'math.asin(x)', doc: 'Inverse sine' },
  { label: 'acos', detail: 'math.acos(x)', doc: 'Inverse cosine' },
  { label: 'atan', detail: 'math.atan(x)', doc: 'Inverse tangent' },
  { label: 'atan2', detail: 'math.atan2(y, x)', doc: 'Angle in the plane between positive x-axis and ray to (x, y)' },
  { label: 'exp', detail: 'math.exp(x)', doc: 'Exponential function e^x' },
  { label: 'log', detail: 'math.log(x)', doc: 'Natural logarithm (base e)' },
  { label: 'log10', detail: 'math.log10(x)', doc: 'Base 10 logarithm' },
  { label: 'log2', detail: 'math.log2(x)', doc: 'Base 2 logarithm' },
  { label: 'floor', detail: 'math.floor(x)', doc: 'Rounds down to nearest integer' },
  { label: 'ceil', detail: 'math.ceil(x)', doc: 'Rounds up to nearest integer' },
  { label: 'round', detail: 'math.round(x)', doc: 'Rounds to nearest integer' },
  { label: 'trunc', detail: 'math.trunc(x)', doc: 'Truncates fractional digits toward zero' },
  { label: 'abs', detail: 'math.abs(x)', doc: 'Absolute value' },
  { label: 'pi', detail: 'math.pi (Constant)', doc: 'Mathematical constant Pi (3.1415926535...)' },
  { label: 'e', detail: 'math.e (Constant)', doc: 'Euler\'s mathematical constant e (2.7182818284...)' },
  { label: 'tau', detail: 'math.tau (Constant)', doc: 'Circle constant Tau (2 * Pi)' },
  { label: 'inf', detail: 'math.inf (Constant)', doc: 'Infinity constant' },
  { label: 'nan', detail: 'math.nan (Constant)', doc: 'Not-a-Number constant' }
];

// 9. Crypto Methods
const CRYPTO_METHODS = [
  { label: 'sha256', detail: 'crypto.sha256(textOrBytes)', doc: 'Calculates SHA-256 cryptographic hash hex digest' },
  { label: 'sha1', detail: 'crypto.sha1(textOrBytes)', doc: 'Calculates SHA-1 cryptographic hash hex digest' },
  { label: 'md5', detail: 'crypto.md5(textOrBytes)', doc: 'Calculates MD5 hash hex digest' },
  { label: 'hmac_sha256', detail: 'crypto.hmac_sha256(key, message)', doc: 'Calculates HMAC-SHA256 signature' },
  { label: 'base64_encode', detail: 'crypto.base64_encode(str)', doc: 'Encodes string or bytes to Base64 format' },
  { label: 'base64_decode', detail: 'crypto.base64_decode(b64Str)', doc: 'Decodes Base64 encoded string' },
  { label: 'random_bytes', detail: 'crypto.random_bytes(count)', doc: 'Generates cryptographically secure random bytes buffer' },
  { label: 'random_hex', detail: 'crypto.random_hex(bytesCount)', doc: 'Generates secure random hex string' },
  { label: 'random_uuid', detail: 'crypto.random_uuid()', doc: 'Generates a random RFC 4122 v4 UUID string' }
];

// 10. Random Methods
const RANDOM_METHODS = [
  { label: 'random', detail: 'random.random()', doc: 'Returns random float in range [0.0, 1.0)' },
  { label: 'randint', detail: 'random.randint(min, max)', doc: 'Returns random integer between min and max (inclusive)' },
  { label: 'uniform', detail: 'random.uniform(a, b)', doc: 'Returns random float between a and b' },
  { label: 'choice', detail: 'random.choice(list)', doc: 'Picks a random element from a list' },
  { label: 'shuffle', detail: 'random.shuffle(list)', doc: 'Shuffles elements in a list randomly in-place' },
  { label: 'sample', detail: 'random.sample(list, count)', doc: 'Draws a random sample of `count` elements without replacement' },
  { label: 'seed', detail: 'random.seed(intVal)', doc: 'Seeds the pseudo-random number generator' }
];

// 11. File System Methods (file / fs)
const FILE_METHODS = [
  { label: 'padh', detail: 'file.padh(path)', doc: 'Reads entire file content as a UTF-8 string: `file.padh("./data.txt")`' },
  { label: 'likh', detail: 'file.likh(path, content)', doc: 'Writes or overwrites text string into file' },
  { label: 'jod', detail: 'file.jod(path, text)', doc: 'Appends text to the end of a file' },
  { label: 'lines', detail: 'file.lines(path)', doc: 'Reads file lines into a list of strings: `file.lines("./log.txt")`' },
  { label: 'padh_csv', detail: 'file.padh_csv(path)', doc: 'Parses CSV file into a list of row maps: `[{"col": val}]`' },
  { label: 'likh_csv', detail: 'file.likh_csv(path, rows)', doc: 'Serializes list of row maps to CSV file' },
  { label: 'nakal', detail: 'file.nakal(src, dest)', doc: 'Copies file or directory recursively' },
  { label: 'hila', detail: 'file.hila(src, dest)', doc: 'Moves or renames a file or directory' },
  { label: 'mita', detail: 'file.mita(path)', doc: 'Deletes a file from the filesystem' },
  { label: 'kundli', detail: 'file.kundli(path)', doc: 'Returns file stats: `{"size": N, "is_file": han, "is_dir": na, "modified": ts}`' },
  { label: 'folder_bana', detail: 'file.folder_bana(path)', doc: 'Creates directory recursively (`mkdir -p`)' },
  { label: 'dekho', detail: 'file.dekho(dirPath)', doc: 'Lists filenames in directory' },
  { label: 'khoj', detail: 'file.khoj(dirPath, pattern)', doc: 'Recursively finds files matching glob pattern (e.g. `file.khoj(".", "*.meme")`)' },
  { label: 'mita_folder', detail: 'file.mita_folder(dirPath)', doc: 'Recursively deletes a directory and all its contents' }
];

// 12. Path Methods (rasta / path)
const PATH_METHODS = [
  { label: 'jod', detail: 'rasta.jod(...parts)', doc: 'Joins path segments with cross-platform separator: `rasta.jod(dir, "sub", "file.txt")`' },
  { label: 'naam', detail: 'rasta.naam(path)', doc: 'Returns file basename (e.g. `rasta.naam("/a/b.txt") => "b.txt"`)' },
  { label: 'folder', detail: 'rasta.folder(path)', doc: 'Returns parent directory path' },
  { label: 'asli', detail: 'rasta.asli(path)', doc: 'Resolves relative path to absolute canonical path' },
  { label: 'ext', detail: 'rasta.ext(path)', doc: 'Extracts file extension (e.g. `".meme"`)' },
  { label: 'alag', detail: 'rasta.alag(path)', doc: 'Splits path into dirname and basename' }
];

// 13. Concurrency Methods (dhaga / kaamgar / Vaada)
const DHAGA_METHODS = [
  { label: 'chala', detail: 'dhaga.chala(fn, ...args)', doc: 'Spawns a real OS hardware thread (`std::thread`): `t = dhaga.chala(worker, 42)`' },
  { label: 'spawn', detail: 'dhaga.spawn(fn, ...args)', doc: 'Standard alias for thread spawning' },
  { label: 'tala', detail: 'dhaga.tala()', doc: 'Creates a thread-safe Mutex lock with `m.surakshit(() => { ... })`' },
  { label: 'mutex', detail: 'dhaga.mutex()', doc: 'Standard alias for mutex lock' },
  { label: 'naali', detail: 'dhaga.naali()', doc: 'Creates a Go-style thread-safe channel with `.bhej(val)` and `.le()`' },
  { label: 'channel', detail: 'dhaga.channel()', doc: 'Standard alias for thread-safe channel' }
];

const KAAMGAR_METHODS = [
  { label: 'sab_pe', detail: 'kaamgar.sab_pe(fn, list, cores?)', doc: 'Splits heavy CPU workloads across all CPU cores in parallel (`mp.map`)' },
  { label: 'map', detail: 'kaamgar.map(fn, list, cores?)', doc: 'Standard alias for parallel pool map' },
  { label: 'alag', detail: 'kaamgar.alag(commandOrFile)', doc: 'Spawns an OS subprocess with `.ruk()`, `.maar()`, `.out()`' },
  { label: 'spawn', detail: 'kaamgar.spawn(cmd)', doc: 'Standard alias for OS child process' },
  { label: 'cpu_count', detail: 'kaamgar.cpu_count()', doc: 'Returns total available hardware CPU thread cores' }
];

const VAADA_METHODS = [
  { label: 'sab', detail: 'Vaada.sab(promisesList)', doc: 'Awaits multiple async tasks in parallel (`Promise.all([p1, p2])`)' },
  { label: 'all', detail: 'Promise.all(promisesList)', doc: 'Runs all promises concurrently and returns list of results' },
  { label: 'race', detail: 'Vaada.race(promisesList)', doc: 'Returns the result of whichever async task resolves first' },
  { label: 'ruk', detail: 'Vaada.ruk(ms)', doc: 'Non-blocking async sleep delay (`Promise.sleep(ms)`)' },
  { label: 'sleep', detail: 'Promise.sleep(ms)', doc: 'Standard non-blocking async delay' },
  { label: 'resolve', detail: 'Promise.resolve(val)', doc: 'Returns an immediately resolved Promise' },
  { label: 'reject', detail: 'Promise.reject(err)', doc: 'Returns an immediately rejected Promise' }
];

// 14. React UI Hooks & Native Declarative Components
const REACT_UI = [
  { label: 'Component', detail: 'UI: Base Component Class', doc: 'Base class for declarative UI components: `biradari App nepo Component { ... }`' },
  { label: 'useState', detail: 'Hook: State Hook', doc: 'Stateful React hook: `pakad [count, setCount] = useState(0);`' },
  { label: 'useEffect', detail: 'Hook: Side Effect Hook', doc: 'Lifecycle effect hook: `useEffect(() => { ... }, [deps]);`' },
  { label: 'useRef', detail: 'Hook: Mutable Ref Hook', doc: 'Persistent mutable ref object: `pakad ref = useRef(null);`' },
  { label: 'useMemo', detail: 'Hook: Memoized Calculation', doc: 'Memoizes expensive calculations: `useMemo(() => calc(a), [a])`' },
  { label: 'useCallback', detail: 'Hook: Memoized Callback', doc: 'Memoizes callback function references' },
  { label: 'useContext', detail: 'Hook: Context Consumer', doc: 'Consumes values from a Context provider' },
  { label: 'createContext', detail: 'Hook: Context Creator', doc: 'Creates a sharable React context container' },
  { label: 'useNavigate', detail: 'Hook: Router Navigator', doc: 'Client-side routing navigation hook' },
  { label: 'useLocation', detail: 'Hook: Router Location', doc: 'Current pathname and search location hook' },
  { label: 'useParams', detail: 'Hook: Router URL Parameters', doc: 'Extracts router path parameters' },
  { label: 'VStack', detail: 'Component: Vertical Layout Container', doc: 'Vertical flex stack layout component' },
  { label: 'HStack', detail: 'Component: Horizontal Layout Container', doc: 'Horizontal flex stack layout component' },
  { label: 'ZStack', detail: 'Component: Overlapping Stack Container', doc: 'Z-index overlay stack layout component' },
  { label: 'Text', detail: 'Component: Typography Text', doc: 'Renders styled text: `Text({ value: "Chai", class: "text-lg font-bold" })`' },
  { label: 'Button', detail: 'Component: Interactive Button', doc: 'Interactive button: `Button({ title: "Submit", onClick: handler })`' },
  { label: 'Input', detail: 'Component: Text Input Box', doc: 'Text input field: `Input({ value: val, onChange: fn, placeholder: "Type..." })`' },
  { label: 'Card', detail: 'Component: Container Card Box', doc: 'Container card box with borders and shadows' },
  { label: 'Badge', detail: 'Component: Status Badge Chip', doc: 'Pill / badge status chip indicator' }
];

// 15. Collection Transforms & Methods
const TRANSFORMS = [
  { label: 'sab_pe', detail: 'Transform: map(fn)', doc: 'Applies `fn(item)` or `fn(item, idx)` to each element and returns new Array (e.g. `arr.sab_pe(x => x * 2)`)' },
  { label: 'badal_sab', detail: 'Transform: map(fn) alias', doc: 'Desi alias for `.sab_pe(fn)`' },
  { label: 'chhaan', detail: 'Transform: filter(fn)', doc: 'Filters elements where `fn(item)` evaluates to truthy (e.g. `arr.chhaan(x => x % 2 == 0)`)' },
  { label: 'filter_kar', detail: 'Transform: filter(fn) alias', doc: 'Desi alias for `.chhaan(fn)`' },
  { label: 'nichod', detail: 'Transform: reduce(fn, init?)', doc: 'Folds list from left to right: `acc = fn(acc, item)` (e.g. `arr.nichod((acc, x) => acc + x, 0)`)' },
  { label: 'sameto', detail: 'Transform: reduce(fn, init?) alias', doc: 'Desi alias for `.nichod(fn, init)`' },
  { label: 'line_mein', detail: 'Transform: sort(comparator?)', doc: 'Sorts list in-place and returns it (uses `__chota__` for custom objects)' },
  { label: 'chhaant', detail: 'Transform: sort alias', doc: 'Desi alias for `.line_mein()`' },
  { label: 'har_ek', detail: 'Transform: forEach(fn)', doc: 'Executes `fn(item)` for each element without returning a new list' },
  { label: 'pakad_ek', detail: 'Transform: find(fn)', doc: 'Returns the first element matching `fn(item)`, or `nalla` if none match' },
  { label: 'chala_de', detail: 'Reflection: Dynamic method invoke', doc: 'Dynamically calls a method by string name (e.g. `obj.chala_de("method_name", [args])`)' },
  { label: 'jodo', detail: 'List: append(val)', doc: 'Appends an element to the list in-place' },
  { label: 'hatao', detail: 'List: pop()', doc: 'Removes and returns the last element of the list' },
  { label: 'has', detail: 'Map: containsKey(key)', doc: 'Returns `han` if key exists in map, else `nhi`' },
  { label: 'keys', detail: 'Map: keys()', doc: 'Returns a list containing all keys in map' },
  { label: 'values', detail: 'Map: values()', doc: 'Returns a list containing all values in map' },
  { label: 'baat', detail: 'Error: message property', doc: 'Human-readable error message string on structured error' },
  { label: 'message', detail: 'Error: message alias', doc: 'Error message string' },
  { label: 'kism', detail: 'Error: type property', doc: 'Error category category (e.g. "MathLafda", "NetworkLafda", "TypeLafda")' },
  { label: 'type', detail: 'Error: type alias', doc: 'Error category category' },
  { label: 'line', detail: 'Error: line number', doc: 'Source line number where error occurred' }
];

// 16. Tantrik Magic Dunder Methods
const TANTRIK_METHODS = [
  { label: '__jod__', detail: 'Tantrik: Overload +', doc: 'Invoked by `a + b`. Accepts `(other)` and returns result.' },
  { label: '__ghata__', detail: 'Tantrik: Overload -', doc: 'Invoked by `a - b`. Accepts `(other)` and returns result.' },
  { label: '__guna__', detail: 'Tantrik: Overload *', doc: 'Invoked by `a * b`. Accepts `(other)` and returns result.' },
  { label: '__bhaag__', detail: 'Tantrik: Overload /', doc: 'Invoked by `a / b`. Accepts `(other)` and returns result.' },
  { label: '__bacha__', detail: 'Tantrik: Overload %', doc: 'Invoked by `a % b` (modulo). Accepts `(other)` and returns result.' },
  { label: '__barabar__', detail: 'Tantrik: Overload ==', doc: 'Invoked by `a == b` (auto-derives `!=`). Returns boolean.' },
  { label: '__chota__', detail: 'Tantrik: Overload <', doc: 'Invoked by `a < b` (auto-derives `>`, `<=`, `>=`). Drives `.line_mein()` sorting.' },
  { label: '__bada__', detail: 'Tantrik: Overload >', doc: 'Invoked by `a > b` when explicitly defined.' },
  { label: '__la__', detail: 'Tantrik: Indexing read obj[key]', doc: 'Invoked by `obj[key]`. Accepts `(key)` and returns value.' },
  { label: '__daal__', detail: 'Tantrik: Indexing write obj[key] = val', doc: 'Invoked by `obj[key] = val`. Accepts `(key, val)`.' },
  { label: '__rakh__', detail: 'Tantrik: Indexing write alias', doc: 'Desi alias for `__daal__(key, val)`.' },
  { label: '__dikha__', detail: 'Tantrik: String conversion', doc: 'Invoked by `dikha obj`, `"str" + obj`, and `f"{obj}"`. Must return string.' },
  { label: '__size__', detail: 'Tantrik: Length measurement', doc: 'Invoked by `size(obj)`. Must return integer length.' },
  { label: '__chala__', detail: 'Tantrik: Function call obj(...)', doc: 'Invoked when calling object instance like a function: `obj(args...)`' },
  { label: '__call__', detail: 'Tantrik: Function call alias', doc: 'Standard alias for `__chala__(args...)`' },
  { label: 'shuru', detail: 'OOP: Constructor initializer', doc: 'Constructor initializer called on class instantiation: `Class(args)`' },
  { label: 'paida', detail: 'OOP: Constructor initializer alias', doc: 'Desi meme alias for constructor' },
  { label: 'alvida', detail: 'OOP: RAII Destructor', doc: 'Destructor called automatically when object scope ends' }
];

function createItem(entry, kind) {
  const item = new vscode.CompletionItem(entry.label, kind);
  item.detail = entry.detail;
  item.documentation = new vscode.MarkdownString(entry.doc);
  return item;
}

function activate(context) {
  // Provider for Context-Aware Autocomplete & Suggestions
  const completionProvider = vscode.languages.registerCompletionItemProvider(
    'memelang',
    {
      provideCompletionItems(document, position, token, context) {
        const lineText = document.lineAt(position).text;
        const linePrefix = lineText.substr(0, position.character);

        // 1. Dot completions (object / module specific)
        if (linePrefix.endsWith('.')) {
          const matchBeforeDot = linePrefix.match(/([a-zA-Z0-9_]+)\.$/);
          if (matchBeforeDot) {
            const caller = matchBeforeDot[1].toLowerCase();

            // app. or router. -> Express & Elysia Server methods
            if (['app', 'server', 'router', 'api', 'dhaba'].includes(caller)) {
              return SERVER_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Method));
            }

            // t. or schema. or nirmaan. -> TypeBox Schema methods
            if (['t', 'schema', 'nirmaan'].includes(caller)) {
              return SCHEMA_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function));
            }

            // req. -> Request properties & methods
            if (['req', 'request', 'prarthana'].includes(caller)) {
              return REQ_PROPERTIES.map((m) => createItem(m, vscode.CompletionItemKind.Property));
            }

            // res. -> Response methods
            if (['res', 'response', 'uttar'].includes(caller)) {
              return RES_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Method));
            }

            // ctx. -> Elysia Context properties & methods
            if (['ctx', 'context'].includes(caller)) {
              return CTX_PROPERTIES.map((m) => createItem(m, vscode.CompletionItemKind.Property));
            }

            // math. or ganit.
            if (['math', 'ganit'].includes(caller)) {
              return MATH_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function));
            }

            // crypto.
            if (caller === 'crypto') {
              return CRYPTO_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function));
            }

            // random.
            if (caller === 'random') {
              return RANDOM_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function));
            }

            // file. or fs.
            if (['file', 'fs'].includes(caller)) {
              return FILE_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function));
            }

            // rasta. or path.
            if (['rasta', 'path'].includes(caller)) {
              return PATH_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function));
            }

            // dhaga. or thread.
            if (['dhaga', 'thread'].includes(caller)) {
              return DHAGA_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function));
            }

            // kaamgar. or mp.
            if (['kaamgar', 'mp'].includes(caller)) {
              return KAAMGAR_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function));
            }

            // vaada. or promise.
            if (['vaada', 'promise'].includes(caller)) {
              return VAADA_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function));
            }
          }

          // General dot access (Collection transforms, string/map methods, error props)
          const allDotMethods = [
            ...TRANSFORMS.map((m) => createItem(m, vscode.CompletionItemKind.Method)),
            ...SERVER_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Method)),
            ...SCHEMA_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Function))
          ];
          return allDotMethods;
        }

        // 2. Tantrik dunder methods trigger
        if (linePrefix.endsWith('__') || linePrefix.includes('jugaad __') || linePrefix.includes('kaam __')) {
          return TANTRIK_METHODS.map((m) => createItem(m, vscode.CompletionItemKind.Method));
        }

        // 3. Type annotation trigger (after colon ':')
        if (linePrefix.trimEnd().endsWith(':')) {
          return TYPES.map((t) => createItem(t, vscode.CompletionItemKind.TypeParameter));
        }

        // 4. General Global Autocomplete (Keywords, Types, Builtins, Modules, UI Hooks, Tantrik)
        const items = [];

        for (const k of KEYWORDS) items.push(createItem(k, vscode.CompletionItemKind.Keyword));
        for (const t of TYPES) items.push(createItem(t, vscode.CompletionItemKind.TypeParameter));
        for (const b of BUILTINS) items.push(createItem(b, vscode.CompletionItemKind.Function));
        for (const m of MODULES) items.push(createItem(m, vscode.CompletionItemKind.Module));
        for (const u of REACT_UI) items.push(createItem(u, vscode.CompletionItemKind.Class));
        for (const tm of TANTRIK_METHODS) items.push(createItem(tm, vscode.CompletionItemKind.Method));

        return items;
      }
    },
    '.', '_', ':' // Trigger completion characters
  );

  // Provider for Hover Documentation (on mouse hover)
  const hoverProvider = vscode.languages.registerHoverProvider('memelang', {
    provideHover(document, position, token) {
      const range = document.getWordRangeAtPosition(position);
      if (!range) return null;
      const word = document.getText(range);

      const all = [
        ...TYPES,
        ...KEYWORDS,
        ...BUILTINS,
        ...MODULES,
        ...SERVER_METHODS,
        ...SCHEMA_METHODS,
        ...REQ_PROPERTIES,
        ...RES_METHODS,
        ...CTX_PROPERTIES,
        ...MATH_METHODS,
        ...CRYPTO_METHODS,
        ...RANDOM_METHODS,
        ...FILE_METHODS,
        ...PATH_METHODS,
        ...DHAGA_METHODS,
        ...KAAMGAR_METHODS,
        ...VAADA_METHODS,
        ...REACT_UI,
        ...TRANSFORMS,
        ...TANTRIK_METHODS
      ];

      const match = all.find((item) => item.label === word);
      if (match) {
        const md = new vscode.MarkdownString();
        md.appendCodeblock(`${match.label} - ${match.detail}`, 'memelang');
        md.appendMarkdown(`\n${match.doc}`);
        return new vscode.Hover(md);
      }
      return null;
    }
  });

  // Provider for Signature Help (when typing parameters inside parentheses)
  const signatureProvider = vscode.languages.registerSignatureHelpProvider('memelang', {
    provideSignatureHelp(document, position, token, context) {
      const lineText = document.lineAt(position).text.substr(0, position.character);
      const funcMatch = lineText.match(/([a-zA-Z0-9_\.]+)\s*\([^)]*$/);
      if (!funcMatch) return null;

      const fullCall = funcMatch[1];
      const fnName = fullCall.includes('.') ? fullCall.split('.').pop() : fullCall;

      const SIGNATURES = {
        'fetch': { label: 'fetch(url: bol, options?: khata): Vaada[Response]', doc: 'Sends HTTP request with options: method, headers, params, body, timeout' },
        'express': { label: 'express(): HttpServer', doc: 'Creates an Express-compatible HTTP server instance' },
        'elysia': { label: 'elysia(): ElysiaServer', doc: 'Creates an Elysia-compatible type-safe HTTP server instance' },
        'Elysia': { label: 'Elysia(): ElysiaServer', doc: 'Creates an Elysia-compatible type-safe HTTP server instance' },
        'get': { label: 'app.get(path: bol, ...handlers: jugaad, schema?: khata)', doc: 'Registers a GET route handler' },
        'post': { label: 'app.post(path: bol, ...handlers: jugaad, schema?: khata)', doc: 'Registers a POST route handler with optional schema' },
        'use': { label: 'app.use(prefixOrMiddleware, middleware?)', doc: 'Mounts a middleware or sub-router' },
        'listen': { label: 'app.listen(port: ank, callback?: jugaad)', doc: 'Starts multithreaded native socket server on port' },
        'String': { label: 't.String(options?: { minLength, maxLength, pattern }): Schema', doc: 'Defines string validation rule' },
        'Integer': { label: 't.Integer(options?: { minimum, maximum }): Schema', doc: 'Defines integer validation rule with auto-coercion' },
        'Number': { label: 't.Number(options?: { minimum, maximum }): Schema', doc: 'Defines floating-point numeric validation rule' },
        'Boolean': { label: 't.Boolean(): Schema', doc: 'Defines boolean validation rule with auto-coercion' },
        'Object': { label: 't.Object(properties: khata): Schema', doc: 'Defines structured schema object' },
        'Array': { label: 't.Array(itemSchema: Schema, options?: { minItems, maxItems }): Schema', doc: 'Defines array items schema' },
        'useState': { label: 'useState<T>(initialValue: T): [T, (newValue: T) => void]', doc: 'State hook returning [state, setState]' },
        'useEffect': { label: 'useEffect(effect: () => void, deps?: soochi): void', doc: 'Side effect hook run on mount and dependency change' },
        'Galti': { label: 'Galti(message: bol, type?: bol = "UserLafda"): Error', doc: 'Constructs structured Error instance' }
      };

      const sigInfo = SIGNATURES[fnName];
      if (sigInfo) {
        const help = new vscode.SignatureHelp();
        const signature = new vscode.SignatureInformation(sigInfo.label, sigInfo.doc);
        help.signatures = [signature];
        help.activeSignature = 0;
        help.activeParameter = 0;
        return help;
      }
      return null;
    }
  }, '(', ',');

  context.subscriptions.push(completionProvider, hoverProvider, signatureProvider);
}

function deactivate() {}

module.exports = {
  activate,
  deactivate
};
