// Compiled from MEMElang AST -> Standard ES6 Modules
import { $m } from './meme-dom.js';

// Module: categories.meme
function $mod_0() {
  if ($mod_0.__cache) return $mod_0.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  function getCategories() {
  return [{"name": "Getting Started", "track": "core", "sectionIds": ["intro", "quickstart"]}, {"name": "Language Basics", "track": "core", "sectionIds": ["variables", "types", "operators", "control-flow", "collections", "pattern-matching"]}, {"name": "Functions & Architecture", "track": "core", "sectionIds": ["functions", "oop", "error-handling", "modules"]}, {"name": "Async, Networking & Concurrency", "track": "core", "sectionIds": ["async-await-promises", "fetch-http-client", "concurrency-threads-mp", "filesystem-paths"]}, {"name": "Type System & Memory Safety", "track": "core", "sectionIds": ["gradual-typing", "memory-safety"]}, {"name": "Standard Library & Errors", "track": "core", "sectionIds": ["stdlib", "builtins", "meme-errors"]}, {"name": "Native UI & Reactivity", "track": "platform", "sectionIds": ["react-jsx-ui", "native-desktop"]}, {"name": "Platform, Permissions & Packaging", "track": "platform", "sectionIds": ["native-permissions", "platform-separation", "native-os-apis", "multiplatform-packaging", "hosting-deploy"]}, {"name": "Universal Interop", "track": "platform", "sectionIds": ["chameleon-bridge"]}, {"name": "Tooling & Package Manager", "track": "platform", "sectionIds": ["tapri-package-manager", "vscode-extension"]}];
  }
  const __ns = { getCategories };
  $mod_0.__cache = __ns;
  return __ns;
}

// Module: sections_core.meme
function $mod_1() {
  if ($mod_1.__cache) return $mod_1.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  function getSectionsCore() {
  return [{"id": "intro", "title": "Introduction & Philosophy", "badge": "Overview", "description": "Learn why MEMElang exists and how it transforms standard compiler misery into Desi meme humor.", "content": [{"type": "text", "text": "MEMElang is a dynamically typed, interpreted programming language written in clean C++20 and compiled to WebAssembly. It blends genuine modern language features (lexical closures, recursive descent parser with precedence climbing, class-based OOP with single inheritance, dynamic field binding, and short-circuit boolean evaluation) with unfiltered Desi internet culture and Bollywood meme energy."}, {"type": "callout", "variant": "info", "title": "Core Philosophy: No Cap, Pure Vibe", "text": "Every programming error message you've ever hated (\"Segmentation fault\", \"TypeError: undefined is not a function\", \"SyntaxError: Unexpected token\") has been replaced with iconic dialogues from Hera Pheri, Shark Tank India, CID, Gangs of Wasseypur, and Indian meme culture."}, {"type": "heading", "level": 2, "text": "Language Feature Highlights"}, {"type": "list", "items": ["Zero Dependencies Runtime: Tree-walk interpreter built from scratch in C++20.", "100% WebAssembly Compatible: Compiles via Emscripten and runs natively in any browser with zero server roundtrips.", "Full Object-Oriented Programming: `biradari` classes, `nepo` inheritance, `apun` self-reference, and automatic `shuru` constructors.", "Functional Goodies: First-class functions (`jugaad`), lexical closures, and recursion.", "Desi Arithmetic: Built-in string multiplication (`\"ha\" * 5` -> `\"hahahahaha\"`), safe division, and 64-bit integer overflow checks.", "Humorous Diagnostic Engine: Every syntax and runtime trap yields an iconic meme error message instead of cryptic stack traces."]}, {"type": "heading", "level": 2, "text": "First Taste: Hello MEMElang"}, {"type": "code", "title": "hello_world.meme", "code": "// Welcome to MEMElang!\npakad naam = \"MEMElang\";\ndikha \"Welcome to\", naam;\ndikha \"Hasi ruki nahi: \" + (\"ha\" * 6);"}]}, {"id": "quickstart", "title": "Quickstart & Installation", "badge": "Getting Started", "description": "How to install and run MEMElang on macOS, Linux, or directly in your browser.", "content": [{"type": "text", "text": "You can use MEMElang in three ways: natively via the CLI binary, interactively in the REPL, or right here in the web browser through WebAssembly."}, {"type": "heading", "level": 2, "text": "1. Build & Run Native CLI"}, {"type": "text", "text": "Clone the repository and compile using any C++20 compiler (`clang++` or `g++`):"}, {"type": "terminal", "commands": ["git clone https://github.com/your-username/memelang.git", "cd memelang", "make", "./meme examples/influence.meme"]}, {"type": "heading", "level": 2, "text": "2. Interactive REPL"}, {"type": "text", "text": "Launch the interactive shell by running `./meme` with no arguments:"}, {"type": "terminal", "commands": ["./meme", "meme > pakad swag = 100;", "meme > dikha swag * 2;", "200", "meme > bas;"]}, {"type": "heading", "level": 2, "text": "3. Run Inline Expressions from Terminal"}, {"type": "terminal", "commands": ["./meme -e 'pakad a = 10; pakad b = 32; dikha \"Jod:\", a + b;'"]}, {"type": "heading", "level": 2, "text": "4. Global CLI Setup & Project Scaffolding"}, {"type": "text", "text": "Link the binary globally to run `meme` from any directory, then scaffold modern apps with built-in templates:"}, {"type": "terminal", "commands": ["./meme setup", "meme naya my-app --template=react", "cd my-app"]}, {"type": "heading", "level": 2, "text": "5. Multiplatform Dev Server & Test Suite"}, {"type": "text", "text": "Run your application across platforms (Zero-WebView native macOS desktop, browser, or terminal) with instant hot-reload:"}, {"type": "terminal", "commands": ["meme dev src/App.meme --platform=desktop", "meme test tests"]}]}, {"id": "variables", "title": "Variables & Lexical Scope", "badge": "Basics", "description": "Declaring variables with pakad, mutating values, and lexical scope rules.", "content": [{"type": "text", "text": "Variables in MEMElang are declared using the keyword `pakad` (or the tolerated alias `le`). Uninitialized variables default to `nalla` (null)."}, {"type": "code", "title": "variables.meme", "code": "// Declare variables\npakad balance = 500;\npakad user = \"Baburao\";\npakad khali_variable; // Evaluates to 'nalla'\n\n// Reassignment\nbalance = balance + 250;\ndikha \"User:\", user, \"| Paisa:\", balance;\ndikha \"Khali kya hai?\", khali_variable;"}, {"type": "heading", "level": 2, "text": "Block Scoping"}, {"type": "text", "text": "MEMElang uses block scoping via braces `{ ... }`. Inner blocks shadow outer declarations and can read/write enclosing scopes seamlessly:"}, {"type": "code", "title": "scoping.meme", "code": "pakad x = \"global\";\n{\n    pakad x = \"local\";\n    dikha \"Inside block:\", x;\n}\ndikha \"Outside block:\", x;"}, {"type": "callout", "variant": "warning", "title": "Undeclared Variable Errors", "text": "Reading an undeclared variable triggers: \"Rasode mein kaun tha?! Ye '<name>' bina bataye kahan se aaya? Pehle 'pakad <name>;' se declare kar!\""}]}, {"id": "types", "title": "Data Types & Literals", "badge": "Types", "description": "Dynamic typing, numbers, strings with repetition, booleans, and null.", "content": [{"type": "text", "text": "MEMElang is dynamically typed. Every value belongs to one of the following fundamental types:"}, {"type": "table", "headers": ["Type", "Literal Example", "Truthy / Falsy", "Notes"], "rows": [["byte", "255b, 0xffb, 0b1010b", "0 is falsy, others truthy", "8-bit unsigned integer (`uint8_t`) with literal suffixes"], ["int32", "int32(42), ank32(100)", "0 is falsy, others truthy", "32-bit fast integer ALU path with auto-promotion on overflow"], ["int / long", "42, -100, 0x1F, 0b101", "0 is falsy, others truthy", "64-bit signed integer (`long long`)"], ["float32", "3.14f, -0.5f", "0.0 is falsy, others truthy", "32-bit single-precision IEEE 754 float (`float`)"], ["float / double", "3.14, -0.5, 1e-4", "0.0 is falsy, others truthy", "64-bit double-precision IEEE 754 float (`double`)"], ["char", "char(\"Namaste\"), 'A'", "Empty or '\\0' is falsy", "Single character primitive (`char`)"], ["string", "\"namaste\", 'bro'", "Empty \"\" is falsy", "Supports string multiplication `\"ha\" * 3`"], ["bytes", "b\"Hello\\x00World\"", "Empty buffer is falsy", "Raw binary buffer with hex decode, mutation & binary I/O"], ["set", "{1, 2, 3}, { \"a\", \"b\" }", "Empty set() is falsy", "Order-preserving O(1) hash set with union `+` & diff `-`"], ["tuple", "(1, \"two\", 3.0), (1,)", "Empty () is falsy", "Fixed-size immutable sequence with indexing and slicing"], ["list", "[1, 2, 3], [\"a\", \"b\"]", "Empty [] is falsy", "Dynamic growable list with spread, push, and pop"], ["map", "{\"key\": \"value\"}", "Empty {} is falsy", "Key-value hash map with destructuring and spread"], ["bool", "real / han, cap / nhi", "real is truthy, cap is falsy", "Gen-Z: `real`/`cap`, Desi: `han`/`nhi`"], ["nalla", "nalla / kuch_nhi", "Always falsy", "Null value (\"unemployed / empty\")"], ["function", "jugaad add(a, b) { ... }", "Always truthy", "First-class callable entity"], ["object", "Influencer(\"Orry\")", "Always truthy", "Instance of a `biradari` class"]]}, {"type": "heading", "level": 2, "text": "Extended Primitives: bytes, set & tuple"}, {"type": "code", "title": "extended_primitives.meme", "code": "// 1. Raw Bytes Buffer\npakad buf = b\"Hello\";\nbuf[0] = 104; // mutable: 'h'\ndikha buf.to_string(); // \"hello\"\ndikha buf.to_hex();    // \"68656c6c6f\"\n\n// Static hex parsing & binary I/O\npakad decoded = bytes.from_hex(\"43686169\"); // \"Chai\"\nfs.write_bytes(\"drink.bin\", decoded);\npakad readBack = fs.read_bytes(\"drink.bin\");\n\n// 2. Set with Deduplication & Algebraic Operations\npakad s = {1, 2, 2, 3}; // size: 3\ns.add(4);\ndikha s.has(2); // han\npakad unionSet = s + {4, 5, 6};\npakad diffSet = unionSet - {1, 2};\n\n// 3. Tuples (Immutable Sequences)\npakad coords = (10, 20, 30);\ndikha coords[0]; // 10\npanga {\n    coords[0] = 99; // Throws error: tuples are immutable!\n} lafda (err) {\n    dikha \"Tuple cannot be modified!\";\n}"}, {"type": "heading", "level": 2, "text": "Interactive Type Inspection with kya_hai()"}, {"type": "code", "title": "types.meme", "code": "dikha kya_hai(255b);     // byte\ndikha kya_hai(int32(42)); // int32\ndikha kya_hai(100);       // int\ndikha kya_hai(3.14f);     // float32\ndikha kya_hai(3.14);      // float\ndikha kya_hai(char(\"A\")); // char\ndikha kya_hai(b\"Data\");   // bytes\ndikha kya_hai({1, 2, 3}); // set\ndikha kya_hai((1, 2, 3)); // tuple\ndikha kya_hai(\"Sholay\");  // string\ndikha kya_hai(real);      // bool\ndikha kya_hai(nalla);     // nalla"}]}, {"id": "operators", "title": "Operators & Short-Circuit Logic", "badge": "Operators", "description": "Arithmetic, comparison, logical short-circuiting, and string multiplication.", "content": [{"type": "heading", "level": 2, "text": "Arithmetic Operators"}, {"type": "table", "headers": ["Operator", "Name", "Example", "Result"], "rows": [["+", "Addition / Concatenation", "10 + 20 or \"Paisa \" + \"Double\"", "30 or \"Paisa Double\""], ["-", "Subtraction", "100 - 35", "65"], ["*", "Multiplication / Repetition", "\"Yo \" * 3", "\"Yo Yo Yo \""], ["/", "Division (Safe)", "10 / 2", "5"], ["%", "Modulo", "17 % 5", "2"]]}, {"type": "heading", "level": 2, "text": "Logical Operators with Short-Circuiting"}, {"type": "text", "text": "Logical operators `aur` (AND) and `ya` (OR) perform short-circuit evaluation. If the left operand determines the outcome, the right operand is completely skipped:"}, {"type": "code", "title": "short_circuit.meme", "code": "// Safe short-circuit: Division by zero on right is never evaluated!\npakad safe_and = cap aur (10 / 0);\npakad safe_or = real ya (10 / 0);\n\ndikha \"Safe AND result:\", safe_and;\ndikha \"Safe OR result:\", safe_or;"}]}, {"id": "control-flow", "title": "Control Flow: dekh, warna, jabtak, ghuma", "badge": "Logic & Loops", "description": "Conditional statements and loop constructs with break and continue.", "content": [{"type": "heading", "level": 2, "text": "Conditionals: dekh & warna"}, {"type": "code", "title": "conditionals.meme", "code": "pakad score = 88;\n\ndekh score >= 90 {\n    dikha \"Full marks! Sharma ji ke ladke jaisa.\";\n} warna dekh score >= 60 {\n    dikha \"First division! Thoda aur jugaad laga lete.\";\n} warna {\n    dikha \"Moye moye! Agli baar padh lena.\";\n}"}, {"type": "heading", "level": 2, "text": "Loops: jabtak (While Loop)"}, {"type": "code", "title": "while_loop.meme", "code": "pakad count = 1;\njabtak count <= 5 {\n    dikha \"Count hai:\", count;\n    count = count + 1;\n}"}, {"type": "heading", "level": 2, "text": "Range Loops: ghuma (For Loop)"}, {"type": "text", "text": "Spin loops over integer ranges using `ghuma <var> se <start> se <end> [se <step>]`:"}, {"type": "code", "title": "range_loop.meme", "code": "// Spin from 1 to 10 with step 2\nghuma i se 1 se 10 se 2 {\n    dekh i == 5 {\n        dikha \"Mil gaya 5, aage badho!\";\n        aage_badh; // continue\n    }\n    dikha \"Step number:\", i;\n}"}]}, {"id": "collections", "title": "Native Collections (Lists & Hash Maps)", "badge": "Data Structures", "description": "Dynamic Arrays (Lists `[]`), Hash Maps (`{}`), negative indexing, and `ghuma ... mein` iteration.", "content": [{"type": "text", "text": "MEMElang features native high-performance Collections with reference semantics (`std::shared_ptr`). Lists and Maps are mutable by default and changes persist across references."}, {"type": "heading", "level": 2, "text": "Dynamic Arrays (Lists)"}, {"type": "text", "text": "Define arrays with square brackets `[elem1, elem2, ...]`. Arrays support positive (0-based) and negative indexing (`arr[-1]` returns the last element)."}, {"type": "code", "title": "arrays_demo.meme", "code": "// Array literals & indexing\npakad gangs = [\"Sardar\", \"Faizal\", \"Definite\"];\ndikha gangs[0];   // Sardar\ndikha gangs[-1];  // Definite (negative indexing)\n\n// Array mutation & index assignment\ngangs[1] = \"Perpendicular\";\ndikha gangs; // [\"Sardar\", \"Perpendicular\", \"Definite\"]\n\n// Built-in Array methods\ngangs.ghusa(\"Tangent\");       // push element\npakad out = gangs.laat_maar(); // pop last element (\"Tangent\")\ndikha gangs.hai_kya(\"Faizal\"); // nhi\ndikha gangs.size();           // 3\ndikha gangs.chipka(\" -> \");    // \"Sardar -> Perpendicular -> Definite\""}, {"type": "heading", "level": 2, "text": "Hash Maps (Dicts / Khata)"}, {"type": "text", "text": "Define key-value maps with curly braces `{ key: value, ... }`. Access properties with bracket indexing `map[\"key\"]` or object dot notation `map.key`."}, {"type": "code", "title": "maps_demo.meme", "code": "// Map literal\npakad villain = {\n    \"naam\": \"Mogambo\",\n    \"dialogue\": \"Mogambo khush hua!\",\n    \"power\": 9000\n};\n\n// Bracket & dot notation access\ndikha villain[\"naam\"];      // Mogambo\ndikha villain.dialogue;     // Mogambo khush hua!\n\n// Map methods\ndikha villain.chaabi();     // [\"naam\", \"dialogue\", \"power\"]\ndikha villain.maal();       // [\"Mogambo\", \"Mogambo khush hua!\", 9000]\ndikha villain.hai_kya(\"power\"); // han\nvillain.hata(\"power\");      // delete key"}, {"type": "heading", "level": 2, "text": "For-Each Iteration (ghuma ... mein)"}, {"type": "text", "text": "Iterate cleanly over Lists, Maps, or Strings using `ghuma item mein collection { ... }`:"}, {"type": "code", "title": "for_each_demo.meme", "code": "// Iterate over array elements\nghuma role mein [\"Hero\", \"Villain\", \"Comedian\"] {\n    dikha \"Role:\", role;\n}\n\n// Iterate over characters of a string\nghuma ch mein \"MEME\" {\n    dikha ch;\n}\n\n// Iterate over keys of a map\npakad scores = {\"Munna\": 95, \"Guddu\": 88};\nghuma student mein scores {\n    dikha student, \"scored:\", scores[student];\n}"}]}, {"id": "pattern-matching", "title": "Enums (avtar) & Pattern Matching (jaanch)", "badge": "Control Flow", "description": "Type-safe Enums (`avtar`) and pattern matching (`jaanch`) with fallback guards (`baaki` / `warna`).", "content": [{"type": "text", "text": "Define type-safe enumeration constants with `avtar` (or `kism`) and dispatch cleanly with `jaanch` (or `parakh`) pattern matching."}, {"type": "code", "title": "enums_and_matching.meme", "code": "// Define an Enum\navtar PaymentStatus {\n    Pending,\n    Approved,\n    Rejected\n}\n\npakad current = PaymentStatus.Approved;\n\n// Pattern match with jaanch\njaanch current {\n    PaymentStatus.Pending => {\n        dikha \"Paisa raste mein hai, intezaar karo!\";\n    }\n    PaymentStatus.Approved => {\n        dikha \"Party shuru! Payment ho gaya!\";\n    }\n    PaymentStatus.Rejected => {\n        dikha \"Khatam, tata, bye-bye! Payment reject!\";\n    }\n    baaki => {\n        dikha \"Aayein? Ye kaisa status hai!\";\n    }\n}"}]}, {"id": "functions", "title": "Functions (jugaad) & Closures", "badge": "Functions", "description": "Writing functions, recursive algorithms, and lexical environment capture.", "content": [{"type": "text", "text": "Every function in MEMElang is a `jugaad`. Functions return values via `ye_le` (or `wapas`). Functions are first-class citizens and capture their lexical scope:"}, {"type": "code", "title": "functions.meme", "code": "// Function declaration\njugaad jod(a, b) {\n    ye_le a + b;\n}\n\npakad ans = jod(15, 27);\ndikha \"15 + 27 =\", ans;"}, {"type": "heading", "level": 2, "text": "Recursion: Factorial"}, {"type": "code", "title": "factorial.meme", "code": "jugaad factorial(n) {\n    dekh n <= 1 {\n        ye_le 1;\n    }\n    ye_le n * factorial(n - 1);\n}\n\ndikha \"Factorial of 6 is:\", factorial(6);"}, {"type": "heading", "level": 2, "text": "Lexical Closures"}, {"type": "code", "title": "closures.meme", "code": "jugaad createMultiplier(multiplier) {\n    jugaad mult(val) {\n        ye_le val * multiplier;\n    }\n    ye_le mult;\n}\n\npakad triple = createMultiplier(3);\ndikha \"Triple of 9 is:\", triple(9);"}]}, {"id": "oop", "title": "Object-Oriented Programming (biradari)", "badge": "OOP", "description": "Classes, inheritance with nepo, apun self-reference, and constructor shuru.", "content": [{"type": "text", "text": "MEMElang brings Desi culture to Object-Oriented Programming. Blueprints are defined with `biradari`, inheritance uses `nepo`, instances refer to themselves via `apun`, and constructors are initialized with `shuru`."}, {"type": "code", "title": "oop_inheritance.meme", "code": "// Base community class\nbiradari AmeeroKiAulaad {\n    jugaad shuru(naam, bank_balance) {\n        apun.naam = naam;\n        apun.balance = point_wala(bank_balance);\n    }\n\n    jugaad kothi_bangle_check() {\n        dikha apun.naam, \"ke paas hai Rs.\", apun.balance;\n    }\n}\n\n// Inherit using 'nepo'\nbiradari Influencer nepo AmeeroKiAulaad {\n    jugaad show_swag() {\n        dekh apun.balance > 50000.0 {\n            dikha apun.naam, \"says: Guys, welcome to my humble struggle vlog!\";\n            ye_le real;\n        } warna {\n            dikha \"Paisa khatam, camera band!\";\n            ye_le cap;\n        }\n    }\n}\n\npakad star = Influencer(\"Orry\", \"99999.50\");\nstar.kothi_bangle_check(); // Inherited from AmeeroKiAulaad\nstar.show_swag();          // Defined on Influencer"}]}, {"id": "error-handling", "title": "Structured try / catch / finally / throw (panga / lafda)", "badge": "Universal Aliases & Structured Errors", "description": "Structured Error objects with .baat/.message, .kism/.type, .line, Galti() constructor, and full Python/JS keyword aliases.", "content": [{"type": "text", "text": "MEMElang provides robust, structured exception handling with full keyword aliases across WhatsApp Hinglish, Python, and JavaScript/TypeScript. Caught errors are rich Error objects with full backward compatibility (printing cleanly in `dikha` and supporting string methods)."}, {"type": "table", "headers": ["WhatsApp Hinglish", "Universal Alias", "Role / Purpose"], "rows": [["panga { ... }", "try { ... } / koshish", "Block of code that might fail or throw an exception"], ["lafda (err) { ... }", "catch (err) { ... } / except (err)", "Catches structured errors or runtime exceptions (optional parens or bare)"], ["pakka { ... }", "finally { ... } / hamesha", "Guaranteed execution block, runs whether an error occurred or not"], ["chillao expr", "throw expr / raise expr / fek", "Throws custom error string, Map, or Galti(msg, type) instance"]]}, {"type": "heading", "level": 2, "text": "Structured Error Object Properties"}, {"type": "table", "headers": ["Property", "Universal Alias", "Description", "Example"], "rows": [["err.baat", "err.message", "Human-readable error description string", "\"Something went wrong\""], ["err.kism", "err.type", "Categorized error category", "\"NetworkLafda\", \"FileLafda\", \"MathLafda\", \"UserLafda\""], ["err.line", "err.line", "Source code line number where exception was raised", "42"]]}, {"type": "code", "title": "structured_errors.meme", "code": "// Throwing structured error with Galti / Error\npanga {\n    chillao Galti(\"Database connection timed out\", \"NetworkLafda\");\n} lafda (err) {\n    dikha \"Caught error message:\", err.baat;      // or err.message\n    dikha \"Error category:\", err.kism;            // or err.type\n    dikha \"Source line:\", err.line;\n    dikha \"Direct print:\", err;                   // Formats cleanly as plain message string\n} pakka {\n    dikha \"Connection cleaned up!\";\n}\n\n// Full Python / JS keyword parity\ntry {\n    raise Error(\"Invalid payload schema\", \"TypeLafda\");\n} catch (e) {\n    dikha f\"Type error: {e.message} (kind: {e.type})\";\n}"}]}, {"id": "async-await-promises", "title": "Async / Await (sabar) & Promises (Vaada)", "badge": "True Asynchrony", "description": "Non-blocking async task scheduling backed by C++ worker threads and Promises with Promise.all and Promise.race.", "content": [{"type": "text", "text": "MEMElang delivers non-blocking asynchronous execution with standard `async / await` semantics and WhatsApp Hinglish aliases (`sabar` / `side_mein kaam`). Async functions automatically return a `Vaada` (`Promise`)."}, {"type": "table", "headers": ["WhatsApp Hinglish", "Universal Alias", "Description"], "rows": [["async kaam fn() / side_mein kaam fn()", "async def fn() / async fn()", "Declares an asynchronous function returning a Promise"], ["sabar expr", "await expr", "Awaits a Promise, Thread handle, or async fetch request"], ["Vaada.de(val)", "Promise.resolve(val)", "Creates an instantly resolved Promise"], ["Vaada.radh(err)", "Promise.reject(err)", "Creates an instantly rejected Promise"], ["Vaada.sab([p1, p2])", "Promise.all([p1, p2])", "Runs promises in parallel and resolves to a list of all results"], ["Vaada.race([p1, p2])", "Promise.race([p1, p2])", "Resolves or rejects as soon as the first promise settles"], ["Vaada.ruk(ms)", "Promise.sleep(ms)", "Non-blocking asynchronous delay in milliseconds"]]}, {"type": "code", "title": "async_demo.meme", "code": "// 1. Asynchronous functions\nasync kaam fetchUserData(id) {\n    sabar Vaada.ruk(50); // non-blocking 50ms pause\n    ye_le {\"id\": id, \"name\": \"Raju\", \"role\": \"Dev\"};\n}\n\n// 2. Awaiting Promises with 'sabar' or 'await'\npakad user = sabar fetchUserData(101);\ndikha f\"Fetched user: {user.name} ({user.role})\";\n\n// 3. Parallel concurrency with Promise.all (Vaada.sab)\npakad p1 = fetchUserData(1);\npakad p2 = fetchUserData(2);\npakad allUsers = sabar Promise.all([p1, p2]);\ndikha f\"Total loaded users: {allUsers.lambai()}\";\n\n// 4. Promise method chaining (.then / .phir and .catch / .pakad)\nfetchUserData(99)\n    .then(u => dikha f\"Chained user: {u.name}\")\n    .catch(err => dikha f\"Lafda: {err.message}\");"}]}, {"id": "fetch-http-client", "title": "Full-Featured fetch() & jaal (http) Client", "badge": "HTTP & Networking", "description": "REST client supporting GET, POST, PUT, PATCH, DELETE, custom headers, query params, Bearer auth, timeouts, and rich Response objects.", "content": [{"type": "text", "text": "The global `fetch(url, options)` (alias `mangwa`) and `jaal` (alias `http`) client provide native HTTP networking with Bearer authentication, auto-serialized JSON bodies, query parameter encoding, and deterministic 0ms offline mock testing via `mock://` URLs."}, {"type": "table", "headers": ["Option / Property", "Type", "Description"], "rows": [["method", "string", "HTTP verb: \"GET\", \"POST\", \"PUT\", \"PATCH\", \"DELETE\", \"HEAD\", \"OPTIONS\" (default: \"GET\")"], ["headers", "Map", "Custom HTTP headers map (e.g. {\"Content-Type\": \"application/json\", \"X-Custom\": \"val\"})"], ["params", "Map", "Query parameters auto-encoded into ?k1=v1&k2=v2 URL query string"], ["auth / token", "string", "Bearer authorization token auto-injected into Authorization: Bearer <token>"], ["body / json", "Map / List / string", "Request payload (maps and lists are auto-serialized to JSON string)"], ["timeout", "number", "Timeout duration in milliseconds"]]}, {"type": "heading", "level": 2, "text": "Response Object Properties & Methods"}, {"type": "table", "headers": ["Member", "Type", "Description"], "rows": [["res.status", "int", "HTTP status code (e.g. 200, 201, 404, 500)"], ["res.ok / res.sahi", "bool", "True if status code is in 200..299 range"], ["res.url", "string", "Final requested URL including encoded query parameters"], ["res.method", "string", "HTTP request method utilized"], ["res.headers", "Map", "Map of response headers"], ["res.body / res.raw", "string", "Raw response body text"], ["res.data / res.maal", "Value", "Parsed JSON map/list if body is JSON, otherwise raw string"], ["res.time_ms", "int", "Elapsed request duration in milliseconds"], ["res.json()", "Callable", "Method parsing response body into a Map / List"], ["res.text()", "Callable", "Method returning raw string body"], ["res.raise_for_status()", "Callable", "Throws a structured NetworkLafda if res.ok is false"]]}, {"type": "code", "title": "http_fetch_demo.meme", "code": "// Global fetch with POST, query params, Bearer auth, and JSON body\npakad res = sabar fetch(\"https://api.example.com/items\", {\n    \"method\": \"POST\",\n    \"params\": {\"category\": \"chai\", \"page\": 1},\n    \"token\": \"secret_bearer_token_123\",\n    \"body\": {\"name\": \"Masala Chai\", \"price\": 20},\n    \"timeout\": 5000\n});\n\ndekh res.ok {\n    pakad item = res.json();\n    dikha f\"Item added in {res.time_ms}ms: {item.name}\";\n} warna {\n    dikha f\"Request failed with status {res.status}\";\n}\n\n// Clean shorthand methods on jaal / http\npakad items = sabar jaal.get(\"mock://items\", {\"params\": {\"q\": \"chai\"}});\npakad updated = sabar jaal.put(\"mock://items/1\", {\"price\": 25});\npakad patched = sabar jaal.patch(\"mock://items/1\", {\"stock\": 100});\npakad deleted = sabar jaal.delete(\"mock://items/1\");"}]}, {"id": "concurrency-threads-mp", "title": "Multi-Threading (dhaga) & Multi-Processing (kaamgar)", "badge": "Hardware Concurrency", "description": "Hardware threads (dhaga.chala), Mutex locks (dhaga.tala), Go channels (dhaga.naali), parallel work-stealing pool (mp.map), and OS child processes (mp.spawn).", "content": [{"type": "text", "text": "MEMElang provides first-class primitives for hardware concurrency. Multi-threading is exposed via `dhaga` (`thread`), thread synchronization via recursive Mutex locks and Go-style channels, parallel data processing via `kaamgar` (`mp`), and OS subprocess execution."}, {"type": "table", "headers": ["WhatsApp Hinglish", "Universal Alias", "Capability"], "rows": [["dhaga.chala(fn, ...args)", "thread.spawn(fn, ...args)", "Spawns an isolated hardware thread running fn with args"], ["t.jod() / sabar t", "t.join() / await t", "Waits for thread execution and returns its result"], ["dhaga.sab([fn1, fn2])", "thread.all([fn1, fn2])", "Spawns and joins multiple threads concurrently, returning results list"], ["dhaga.tala()", "thread.mutex()", "Recursive mutex with .band() (lock), .khol() (unlock), and .surakshit(fn) (with_lock)"], ["dhaga.naali(capacity?)", "thread.channel(capacity?)", "Go-style channel with .bhej(val) (send), .le() (recv), and .band()"], ["dhaga.cores()", "thread.cores()", "Returns number of CPU hardware execution cores"], ["kaamgar.sab_pe(fn, list, cores?)", "mp.map(fn, list, workers?)", "Parallel work-stealing pool mapping across CPU cores"], ["kaamgar.alag(cmd, args?)", "mp.spawn(cmd, args?)", "Spawns OS child process with .ruk() (wait), .code(), and .out() (stdout)"]]}, {"type": "code", "title": "concurrency_showcase.meme", "code": "// 1. Spawning OS threads\npakad t = dhaga.chala((a, b) => a * b, 6, 7);\npakad product = sabar t; // or t.jod()\ndikha f\"Thread product: {product}\"; // 42\n\n// 2. Thread-safe Mutex lock\npakad lock = dhaga.tala();\npakad sharedCounter = 0;\nlock.surakshit(() => {\n    sharedCounter = sharedCounter + 1;\n});\n\n// 3. Go-style channel message passing\npakad ch = dhaga.naali();\ndhaga.chala(() => {\n    ch.bhej(\"packet_from_worker\");\n});\npakad msg = ch.le();\ndikha f\"Received channel message: {msg}\";\n\n// 4. Parallel work-stealing map across CPU cores\npakad numbers = [10, 20, 30, 40, 50];\npakad squares = mp.map(x => x * x, numbers);\ndikha f\"Parallel squares: {squares}\";\n\n// 5. Child processes\npakad proc = mp.spawn(\"echo 'Child process execution'\");\npakad exitCode = proc.ruk();\ndikha f\"Process code {exitCode}: {proc.out()}\";"}]}, {"id": "filesystem-paths", "title": "File System (file / fs) & Paths (rasta / path)", "badge": "File Management", "description": "Complete file operations, CSV table parsing/writing, JSON storage, recursive directory walking, and cross-platform path helpers.", "content": [{"type": "text", "text": "The `file` (`fs`) and `rasta` (`path`) standard libraries offer full file system management, automated CSV reading and writing into Lists of Maps, JSON serialization, recursive file discovery (glob), and cross-platform path manipulation."}, {"type": "table", "headers": ["Category", "WhatsApp Hinglish", "Universal Alias", "Description"], "rows": [["Read / Write", "file.padh(p) / file.likh(p, s)", "fs.read / fs.write", "Reads full text file or overwrites file contents"], ["Append / Lines", "file.jod(p, s) / file.lines(p)", "fs.append / fs.read_lines", "Appends text or reads file into a List of line strings"], ["JSON Storage", "file.padh_json(p) / file.likh_json(p, v)", "fs.read_json / fs.write_json", "Reads and writes JSON structured files automatically"], ["CSV Engine", "file.padh_csv(p) / file.likh_csv(p, rows)", "fs.read_csv / fs.write_csv", "Parses CSV into a List of Maps [{\"col\": val}] and writes back"], ["Copy / Move / Del", "file.nakal(a, b) / file.hila(a, b)", "fs.copy / fs.move / fs.remove", "Recursively copies, renames/moves, or deletes files & directories"], ["Stats & Meta", "file.kundli(p)", "fs.stat(p)", "Returns {\"size\", \"is_file\", \"is_dir\", \"modified\", \"ext\"} map"], ["Search & Glob", "file.khoj(dir, \"*.meme\")", "fs.glob(dir, pattern)", "Recursively walks directories matching wildcard pattern"], ["Path Resolution", "rasta.jod(a, b) / rasta.asli(p)", "path.join / path.resolve", "Joins path components or resolves canonical absolute path"]]}, {"type": "code", "title": "fs_path_demo.meme", "code": "// 1. Reading and writing text files\nfs.write(\"notes.txt\", \"Pehli line\\nDusri line\\n\");\npakad lines = fs.read_lines(\"notes.txt\");\ndikha f\"Total lines: {lines.lambai()}\";\n\n// 2. CSV parsing into List of Maps\npakad staff = [\n    {\"id\": \"1\", \"name\": \"Raju\", \"role\": \"Fullstack Dev\"},\n    {\"id\": \"2\", \"name\": \"Baburao\", \"role\": \"Team Lead\"}\n];\nfs.write_csv(\"staff.csv\", staff);\npakad loadedStaff = fs.read_csv(\"staff.csv\");\ndikha f\"Staff member: {loadedStaff[0].name} ({loadedStaff[0].role})\";\n\n// 3. File statistics\npakad stat = fs.stat(\"staff.csv\");\ndikha f\"File size: {stat.size} bytes, Modified: {stat.modified}\";\n\n// 4. Wildcard globbing\npakad memeFiles = fs.glob(\".\", \"*.meme\");\ndikha f\"Found meme scripts: {memeFiles.lambai()}\";\n\n// 5. Cross-platform path helpers\npakad full = path.join(\"workspace\", \"src\", \"main.meme\");\ndikha f\"Basename: {path.basename(full)}\"; // main.meme\ndikha f\"Stem: {path.stem(full)}\";         // main\ndikha f\"Extension: {path.ext(full)}\";      // .meme\ndikha f\"Dirname: {path.dirname(full)}\";   // workspace/src\n\n// Clean up\nfs.remove(\"notes.txt\");\nfs.remove(\"staff.csv\");"}]}];
  }
  const __ns = { getSectionsCore };
  $mod_1.__cache = __ns;
  return __ns;
}

// Module: sections_stdlib.meme
function $mod_2() {
  if ($mod_2.__cache) return $mod_2.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  function getSectionsStdlib() {
  return [{"id": "modules", "title": "Module & File Imports (bula / jaise)", "badge": "Architecture", "description": "Multi-file modular architecture with `bula` and `jaise`, canonical caching, and circular import detection.", "content": [{"type": "text", "text": "Organize your project into multiple files using `bula \"<path>\" [jaise <alias>];`. Modules run in isolated namespaces and export top-level variables, functions, and classes."}, {"type": "code", "title": "imports_demo.meme", "code": "// Import with alias\nbula \"./math_utils.meme\" jaise mu;\ndikha mu.add(10, 20);\n\n// Import without alias (auto-derives stem 'helpers')\nbula \"./helpers.meme\";\ndikha helpers.formatText(\"namaste\");\n\n// Circular import protection:\n// If a imports b and b imports a, MEMElang aborts with:\n// \"Golmaal hai sab golmaal hai! Circular import pakda gaya: '<path>'\""}]}, {"id": "stdlib", "title": "Preinstalled Standard Library (\"Batteries-Included\")", "badge": "Standard Library", "description": "Comprehensive built-in modules preinstalled just like Python: `math`, `random`, `statistics`, `crypto`, `regex`, `datetime`, `itertools`, `collections`, `csv`, `test`, `json`, `file`, and `systumm`.", "content": [{"type": "text", "text": "MEMElang includes a zero-dependency standard library directly available in global scope or via `bula` / `import`:"}, {"type": "heading", "level": 2, "text": "1. math / hisab (Comprehensive Mathematics)"}, {"type": "table", "headers": ["Member", "WhatsApp Alias", "Description", "Example"], "rows": [["math.pi", "hisab.pi", "Mathematical constant pi (3.14159...)", "math.pi"], ["math.e", "hisab.e", "Euler's constant e (2.71828...)", "math.e"], ["math.tau", "hisab.tau", "Circle constant tau (6.28318...)", "math.tau"], ["math.sqrt(x)", "hisab.jadd(x)", "Square root of x", "math.sqrt(49) -> 7.0"], ["math.cbrt(x)", "-", "Cube root of x", "math.cbrt(27) -> 3.0"], ["math.pow(b, exp)", "hisab.taqat(b, exp)", "Power base^exp", "math.pow(2, 3) -> 8.0"], ["math.log(x, base?)", "hisab.ln(x)", "Natural or custom base logarithm", "math.log(100, 10) -> 2.0"], ["math.sin(x) / cos / tan", "-", "Trigonometric functions", "math.sin(math.pi / 2) -> 1.0"], ["math.hypot(x, y)", "-", "Euclidean distance sqrt(x^2 + y^2)", "math.hypot(3, 4) -> 5.0"], ["math.gcd(a, b)", "hisab.mahatam(a, b)", "Greatest Common Divisor", "math.gcd(48, 18) -> 6"], ["math.lcm(a, b)", "hisab.laghutam(a, b)", "Least Common Multiple", "math.lcm(4, 6) -> 12"], ["math.factorial(n)", "hisab.kramgunan(n)", "Factorial n!", "math.factorial(6) -> 720"], ["math.comb(n, k)", "-", "Combinations n choose k", "math.comb(5, 2) -> 10"], ["math.perm(n, k)", "-", "Permutations nPk", "math.perm(5, 2) -> 20"], ["math.clamp(val, mn, mx)", "-", "Clamps val into [mn, mx]", "math.clamp(15, 0, 10) -> 10"]]}, {"type": "heading", "level": 2, "text": "2. random / tukka (Pseudo-Random Generation)"}, {"type": "table", "headers": ["Member", "Description", "Example"], "rows": [["random.seed(n)", "Seeds pseudo-random number generator", "random.seed(42)"], ["random.random()", "Random float in [0.0, 1.0)", "random.random()"], ["random.randint(min, max)", "Random integer in [min, max] inclusive", "random.randint(1, 100)"], ["random.uniform(a, b)", "Random float in [a, b]", "random.uniform(1.5, 5.5)"], ["random.choice(list)", "Returns random element from sequence", "random.choice([\"chai\", \"samosa\"])"], ["random.shuffle(list)", "Returns shuffled copy of list in-place", "random.shuffle([1, 2, 3])"], ["random.sample(list, k)", "Random sample of k unique elements", "random.sample(cards, 5)"]]}, {"type": "heading", "level": 2, "text": "3. statistics / stats / aankde (Descriptive Statistics)"}, {"type": "table", "headers": ["Member", "Description", "Example"], "rows": [["stats.mean(nums)", "Arithmetic average of sequence", "stats.mean([10, 20, 30]) -> 20.0"], ["stats.median(nums)", "Middle value in sequence", "stats.median([1, 3, 2]) -> 2.0"], ["stats.mode(nums)", "Most frequently occurring item", "stats.mode([1, 2, 2, 3]) -> 2"], ["stats.variance(nums)", "Sample variance of data", "stats.variance([2, 4, 4, 4, 5])"], ["stats.stdev(nums)", "Standard deviation of data", "stats.stdev([2, 4, 4, 4, 5])"], ["stats.min(nums) / max(nums)", "Extremum value in sequence", "stats.min([5, 2, 9]) -> 2"], ["stats.sum(nums)", "Total sum of elements", "stats.sum([1, 2, 3, 4]) -> 10"]]}, {"type": "heading", "level": 2, "text": "4. crypto / hashlib / gupt (Cryptography & Hashes)"}, {"type": "table", "headers": ["Member", "Description", "Example"], "rows": [["crypto.sha256(data)", "Computes SHA-256 hex digest", "crypto.sha256(\"password\")"], ["crypto.hmac_sha256(k, d)", "Computes HMAC-SHA256 signature", "crypto.hmac_sha256(secret, msg)"], ["crypto.md5(data)", "Computes MD5 128-bit checksum", "crypto.md5(\"file_content\")"], ["crypto.sha1(data)", "Computes SHA-1 160-bit checksum", "crypto.sha1(\"data\")"], ["crypto.base64_encode(str)", "Encodes text or bytes to Base64", "crypto.base64_encode(\"hello\")"], ["crypto.base64_decode(b64)", "Decodes Base64 to original string", "crypto.base64_decode(\"aGVsbG8=\")"], ["crypto.uuid()", "Generates random RFC 4122 v4 UUID string", "crypto.uuid()"]]}, {"type": "heading", "level": 2, "text": "5. regex / re / dhundo (Regular Expressions)"}, {"type": "table", "headers": ["Member", "Description", "Example"], "rows": [["re.test(pat, str)", "Returns true if pattern matches string anywhere", "re.test(\"^[0-9]+$\", \"123\")"], ["re.search(pat, str)", "Finds first match with start, end, and groups", "re.search(\"([a-z]+)@\", email)"], ["re.match(pat, str)", "Matches pattern starting from beginning of string", "re.match(\"[A-Z]+\", \"MEME\")"], ["re.find_all(pat, str)", "Returns list of all regex match occurrences", "re.find_all(\"[0-9]+\", text)"], ["re.replace(pat, rep, str)", "Replaces pattern matches with replacement string", "re.replace(\"chai\", \"coffee\", s)"], ["re.split(pat, str)", "Splits string by regex delimiter", "re.split(\"[,; ]+\", tags)"]]}, {"type": "heading", "level": 2, "text": "6. datetime / waqt / time & so_ja"}, {"type": "code", "title": "datetime_demo.meme", "code": "pakad now = datetime.now();\ndikha \"Current Year:\", datetime.year();\ndikha \"ISO String:\", datetime.iso();\ndikha \"Formatted Date:\", datetime.format(\"%Y-%m-%d %H:%M:%S\");\n\n// Sleep for 100 milliseconds\nso_ja(0.1);\npakad elapsed = datetime.now() - now;\ndikha \"Elapsed seconds:\", elapsed;"}, {"type": "heading", "level": 2, "text": "7. itertools & collections (Iterators & Data Structures)"}, {"type": "code", "title": "itertools_collections_demo.meme", "code": "// Itertools: range, zip, enumerate, chunk\npakad r = range(0, 10, 2);             // [0, 2, 4, 6, 8]\npakad zipped = zip([1, 2], [\"A\", \"B\"]); // [(1, \"A\"), (2, \"B\")]\npakad pairs = enumerate([\"Chai\", \"Coffee\"]); // [(0, \"Chai\"), (1, \"Coffee\")]\npakad chunks = itertools.chunk([1, 2, 3, 4, 5], 2); // [[1, 2], [3, 4], [5]]\n\n// Collections: Frequency Counter & Double-Ended Queue (deque)\npakad counts = collections.Counter(\"banana\");\ndikha \"Letter 'a':\", counts.a;         // 3\n\npakad dq = collections.deque([1, 2]);\ndq.push_front(0);\ndq.push_back(3);\ndikha \"Popped front:\", dq.pop_front(); // 0\ndikha \"Popped back:\", dq.pop_back();   // 3"}, {"type": "heading", "level": 2, "text": "8. csv (Standalone Parsing & Generation)"}, {"type": "code", "title": "csv_demo.meme", "code": "pakad rawCsv = \"name,role,salary\\nRaju,Developer,75000\\nBaburao,Manager,90000\";\npakad records = csv.parse(rawCsv);\ndikha \"First employee:\", records[0].name; // \"Raju\"\n\n// Convert back to CSV string\npakad csvOut = csv.stringify(records);\ndikha \"Serialized CSV:\\n\" + csvOut;"}, {"type": "heading", "level": 2, "text": "9. test & assert (Built-in Assertions)"}, {"type": "code", "title": "test_demo.meme", "code": "assert(10 > 5, \"Ten must be greater than five\");\nassert_eq(2 + 2, 4);\nassert_ne(\"chai\", \"coffee\");\nassert_throws(() => {\n    chillao \"Error condition!\";\n});\ndikha \"All test assertions passed!\";"}, {"type": "heading", "level": 2, "text": "10. json (Serialization & Parsing)"}, {"type": "code", "title": "json_demo.meme", "code": "pakad hero = {\n    \"name\": \"Shaktimaan\",\n    \"powers\": [\"Fly\", \"Spin\", \"Laser\"],\n    \"alter_ego\": \"Pandit Gangadhar\"\n};\n\n// Stringify to JSON\npakad serialized = json.likh(hero);\ndikha \"JSON String:\", serialized;\n\n// Parse back into MEMElang Objects & Lists\npakad parsed = json.samajh(serialized);\ndikha \"Powers count:\", parsed.powers.size();"}, {"type": "heading", "level": 2, "text": "11. file & fs (File System I/O)"}, {"type": "code", "title": "file_demo.meme", "code": "// Check existence\ndekh file.hai_kya(\"data.txt\") {\n    dikha \"File content:\", file.padh(\"data.txt\");\n} warna {\n    file.likh(\"data.txt\", \"Initial entry\\n\");\n    file.jod(\"data.txt\", \"Appended log entry\\n\");\n    dikha \"Created and wrote to data.txt!\";\n}"}, {"type": "heading", "level": 2, "text": "12. systumm (OS & Process Control)"}, {"type": "table", "headers": ["Member / Method", "English Alias", "Description", "Example"], "rows": [["systumm.chala(cmd)", "sys.exec(cmd)", "Executes shell command and returns output", "systumm.chala(\"git status\")"], ["systumm.env(key, def?)", "os.env(key, def)", "Reads environment variable", "systumm.env(\"HOME\")"], ["systumm.set_env(k, v)", "os.set_env(k, v)", "Sets environment variable", "systumm.set_env(\"PORT\", \"8080\")"], ["systumm.kahan()", "os.cwd()", "Returns current working directory", "systumm.kahan()"], ["systumm.jao(path)", "os.chdir(path)", "Changes directory", "systumm.jao(\"../\")"], ["systumm.kaun()", "os.platform()", "Returns \"mac\", \"linux\", \"windows\", \"wasm\"", "systumm.kaun()"], ["systumm.copy(text)", "sys.copy(text)", "Copies string to OS clipboard", "systumm.copy(\"Secret\")"], ["systumm.paste()", "sys.paste()", "Reads text from OS clipboard", "systumm.paste()"], ["systumm.nikal(code?)", "sys.exit(code)", "Exits program with exit status code", "systumm.nikal(0)"]]}, {"type": "heading", "level": 2, "text": "6. cli (Interactive Terminal Apps)"}, {"type": "table", "headers": ["Method", "English Alias", "Description", "Example"], "rows": [["cli.args_samajh(args?)", "cli.parse_args(args?)", "Parses flags (--key=val, --flag, -f) into Map", "cli.args_samajh()"], ["cli.chuno(prompt, opts)", "cli.select(prompt, opts)", "Displays numbered interactive choice selection", "cli.chuno(\"Branch?\", [\"main\", \"dev\"])"], ["cli.puch_han_na(prompt)", "cli.confirm(prompt)", "Prompts user with yes/no [han/na] confirmation", "cli.puch_han_na(\"Deploy now?\")"], ["cli.chhupa_puch(prompt)", "cli.password(prompt)", "Prompts password input without echo", "cli.chhupa_puch(\"Token: \")"], ["cli.progress(cur, tot, msg?)", "cli.progress(...)", "Renders in-place terminal progress bar", "cli.progress(50, 100, \"Downloading\")"]]}, {"type": "heading", "level": 2, "text": "7. app (Native Desktop GUI & Notifications)"}, {"type": "table", "headers": ["Method", "English Alias", "Description", "Example"], "rows": [["app.alert(title, msg)", "app.dikha_box(title, msg)", "Native GUI popup alert dialog", "app.alert(\"Ready\", \"Server online\")"], ["app.puch_box(title, q, def?)", "app.input_box(...)", "Native GUI text input dialog", "app.puch_box(\"Login\", \"Username: \")"], ["app.han_na_box(title, q)", "app.confirm_box(...)", "Native GUI Yes/No confirmation dialog", "app.han_na_box(\"Exit\", \"Sure?\")"], ["app.file_chuno(title?)", "app.pick_file(title?)", "Native system file picker dialog", "app.file_chuno(\"Select Image\")"], ["app.notify(title, msg)", "app.notify(...)", "Native OS desktop notification banner", "app.notify(\"Done\", \"Build passed\")"], ["app.khol(target)", "app.open(target)", "Opens URL or file with default system application", "app.khol(\"https://google.com\")"]]}, {"type": "heading", "level": 2, "text": "8. Rich Terminal Styling & Unicode UI Widgets"}, {"type": "table", "headers": ["Feature", "Syntax / Method", "Description"], "rows": [["Inline Color Tags", "dikha \"[hara bold]OK:[/] Done\"", "Terminal styling tags: [laal], [hara], [peela], [neela], [gulabi], [asmani], [safed], [kala], [mota], [line], [tedha], [halka], and [/] reset"], ["String Color Methods", "\"OK\".hara().mota()", "Chainable ANSI methods: .laal(), .hara(), .peela(), .neela(), .gulabi(), .asmani(), .mota(), .line(), .rang_hata()"], ["Unicode Bordered Table", "dikha.mez(data) / table(data)", "Renders List of Maps or List of Lists as bordered Unicode table"], ["Unicode Box Panel", "dikha.dabba(text, title?) / box(...)", "Wraps text in rounded Unicode box container with optional title"], ["Screen Clear", "dikha.saaf() / clear()", "Clears terminal screen and resets cursor position"]]}, {"type": "heading", "level": 2, "text": "9. tijori (Local Persistent Document DB)"}, {"type": "table", "headers": ["Method", "Aliases", "Description", "Example"], "rows": [["tijori.rakh(key, val)", "tijori.set(...)", "Saves a key-value pair to persistent local storage", "tijori.rakh(\"user\", {\"name\": \"Raju\"})"], ["tijori.nikal(key, default?)", "tijori.get(...)", "Retrieves value by key, or returns default if not found", "tijori.nikal(\"user\", khali)"], ["tijori.hai_kya(key)", "tijori.has(...)", "Returns han if key exists in storage, else nhi", "tijori.hai_kya(\"user\")"], ["tijori.hata(key)", "tijori.delete(...)", "Deletes a key from storage and syncs to disk", "tijori.hata(\"user\")"], ["tijori.chaabi()", "tijori.keys()", "Returns a list of all saved keys", "tijori.chaabi()"], ["tijori.maal()", "tijori.values()", "Returns a list of all saved values", "tijori.maal()"], ["tijori.sab()", "tijori.all()", "Returns entire database as a map", "tijori.sab()"], ["tijori.saaf()", "tijori.clear()", "Clears all stored entries", "tijori.saaf()"]]}, {"type": "heading", "level": 2, "text": "10. dhaba / express / elysia (Fast Multi-threaded HTTP Server & Type Safety)"}, {"type": "text", "text": "MEMElang provides blazing fast native multi-threaded HTTP servers with full Express.js middleware parity and complete Elysia-style TypeBox schema validation:"}, {"type": "table", "headers": ["Method / Property", "Framework Parity", "Description", "Example"], "rows": [["express() / elysia()", "Express / Elysia Factory", "Creates a new isolated HTTP server application instance", "app = express()"], ["app.get / post / put / delete", "Express Routing", "Registers HTTP route handlers with `:param` and wildcard matching", "app.get(\"/users/:id\", (req, res) => ...)"], ["app.use(mwOrRouter)", "Express Middleware", "Registers global/prefix middlewares or sub-routers with `(req, res, next)`", "app.use(\"/api\", router)"], ["express.Router()", "Express Sub-Router", "Creates an isolated modular sub-router", "api = express.Router()"], ["express.json() / cors() / static()", "Built-in Middlewares", "Prebuilt middlewares for body parsing, CORS headers, and static files", "app.use(express.cors())"], ["t.Object / String / Integer", "Elysia / TypeBox", "Complete request & response schema definitions with automatic type coercion", "t.Object({ age: t.Integer() })"], ["app.state(key, value)", "Elysia State", "Sets global server state accessible via `ctx.store`", "app.state(\"visits\", 0)"], ["app.derive(fn)", "Elysia Derivations", "Enriches request context before route handlers run", "app.derive(ctx => ({ role: \"admin\" }))"], ["app.listen(port, cb?)", "Multi-Threaded Server", "Spawns high-throughput native TCP socket listener worker pool", "app.listen(3000)"], ["app.request(method, path)", "In-Memory Dispatch", "Dispatches request in microseconds without opening ports (for tests/WASM)", "app.request(\"GET\", \"/ping\")"]]}, {"type": "code", "title": "express_elysia_demo.meme", "code": "pakad app = elysia();\n\napp.use(express.cors());\napp.state(\"counter\", 0);\n\n// Elysia Type-Safe Route with TypeBox Schema\napp.post(\"/users/:id\", {\n    params: t.Object({ \"id\": t.Integer() }),\n    query: t.Object({ \"tag\": t.Optional(t.String()) }),\n    body: t.Object({\n        \"username\": t.String({ \"minLength\": 3 }),\n        \"age\": t.Integer({ \"min\": 18 }),\n        \"email\": t.Optional(t.String({ \"format\": \"email\" }))\n    })\n}, (ctx) => {\n    ctx.store.counter = ctx.store.counter + 1;\n    // Guaranteed type-safe & coerced parameters\n    ye_le {\n        \"success\": han,\n        \"id\": ctx.params.id,              // Coerced integer\n        \"user\": ctx.body.username,\n        \"age\": ctx.body.age,              // Coerced integer\n        \"total_requests\": ctx.store.counter\n    };\n});\n\n// Express Sub-Router\npakad v1 = express.Router();\nv1.get(\"/health\", (req, res) => {\n    res.status(200).json({ \"status\": \"healthy\" });\n});\napp.use(\"/api/v1\", v1);\n\n// Start live server\napp.listen(3000, () => {\n    dikha \"[Fast] Server running on http://localhost:3000\";\n});"}, {"type": "heading", "level": 2, "text": "11. ui (Declarative Cross-Platform UI Framework)"}, {"type": "table", "headers": ["Component / Method", "Description", "Example"], "rows": [["ui.khamba([children])", "Vertical column layout container (alias ui.column)", "ui.khamba([ui.likh(\"Header\"), ui.button(\"OK\")])"], ["ui.katar([children])", "Horizontal row layout container (alias ui.row)", "ui.katar([card1, card2])"], ["ui.likh(text, variant?)", "Typography text node (variant: \"heading\", \"subheading\", \"body\", \"caption\")", "ui.likh(\"Tapri POS\", \"heading\")"], ["ui.dabba(children, title?)", "Bordered card / box container with optional title panel", "ui.dabba([item1, badge], \"Order #1\")"], ["ui.badge(label, color?)", "Status badge pill (color: \"hara\", \"peela\", \"laal\", \"asmani\", \"gulabi\")", "ui.badge(\"Ready\", \"hara\")"], ["ui.puch(placeholder)", "Interactive text input field component", "ui.puch(\"Enter customer name...\")"], ["ui.button(label, onClick)", "Action button with lambda callback handler", "ui.button(\"Order Bhej\", () => dikha \"Bheja!\")"], ["ui.faasla()", "Divider / spacing element (alias ui.divider)", "ui.faasla()"], ["ui.dikha(screen, title?)", "Renders screen directly to the terminal with Unicode box alignment", "ui.dikha(screen, \"POS App\")"], ["ui.html(screen, title?)", "Exports declarative UI tree into HTML5 + Tailwind CSS markup", "ui.html(screen, \"Web View\")"], ["ui.click(screen, label?)", "Programmatically triggers a button click event handler (alias ui.dabao)", "ui.click(screen, \"Order Bhej\")"]]}]}, {"id": "builtins", "title": "Standard Built-in Functions", "badge": "Standard Library", "description": "Type inspection, length checking, numeric conversions, and console inputs.", "content": [{"type": "table", "headers": ["Function", "Aliases", "Parameters", "Return Type", "Description"], "rows": [["kya_hai(val)", "aukaat", "Any value", "string", "Returns the type name: \"int\", \"float\", \"string\", \"bool\", \"nalla\", \"function\", \"object\""], ["size(str)", "kitna_lamba", "string", "int", "Returns the character length of a string."], ["bina_point(val)", "pura_num", "string, float, bool", "int", "Converts value to a whole 64-bit integer (`long long`)."], ["point_wala(val)", "float_bana", "string, int, bool", "float", "Converts value to a decimal floating point (`double`)."], ["bata(prompt)", "pooch", "string (optional)", "string", "Reads a line of text from console input with prompt."]]}, {"type": "code", "title": "builtins.meme", "code": "pakad s = \"Namaste Dosto\";\ndikha \"String size:\", size(s);\ndikha \"Type check:\", kya_hai(s);\n\npakad num_str = \"1234\";\npakad num = bina_point(num_str) + 66;\ndikha \"Converted jod:\", num;\n\npakad pi_str = \"3.14159\";\ndikha \"Decimal multiplied:\", point_wala(pi_str) * 2.0;"}]}, {"id": "memory-safety", "title": "Rust-Style Ownership & Borrow Checker", "badge": "Memory Safety", "description": "Single ownership, moves for heap types, borrowing with udhaar and udhaar_badal, explicit nakal, and RAII alvida destructors.", "content": [{"type": "text", "text": "MEMElang brings Rust-level memory safety to Desi programming. Say goodbye to use-after-free, double drops, and data races! Values have single owners; assignments of heap types move ownership by default; references are checked at runtime with hilarious Bollywood errors."}, {"type": "heading", "level": 2, "text": "1. Single Ownership & Move Semantics"}, {"type": "text", "text": "Primitive types (`int`, `float`, `bool`, `nalla`) have `Copy` semantics. Heap and compound resources (`string`, `biradari` object instances, and closures) have Move semantics. Assigning or passing them transfers ownership permanently:"}, {"type": "code", "title": "move_semantics.meme", "code": "pakad a = \"Baburao ka khazana\";\npakad b = a; // Ownership transferred to b!\n\n// Reading 'a' now throws a borrow-checker error:\n// dikha a; // Error: Pehli fursat mein nikal! 'a' ki value move ho chuki hai!\n\ndikha \"Naya owner b:\", b;\n\n// Re-assigning re-initializes 'a':\na = \"Naye sapne\";\ndikha \"Reinitialized a:\", a;"}, {"type": "heading", "level": 2, "text": "2. Explicit Cloning with nakal()"}, {"type": "text", "text": "To duplicate a heap value without giving up ownership of the original, use `nakal(val)` (or method `val.nakal()`, aliases: `clone`, `copy`):"}, {"type": "code", "title": "explicit_clone.meme", "code": "pakad original = \"Diamond Kohinoor\";\npakad duplicate = nakal(original); // Cloned!\n\ndikha \"Original:\", original; // Still valid!\ndikha \"Duplicate:\", duplicate;\n\nbiradari Khazana {\n    jugaad shuru(coins) {\n        apun.coins = coins;\n    }\n}\n\npakad k1 = Khazana(100);\npakad k2 = k1.nakal(); // Deep clone of instance!\nk2.coins = 500;\n\ndikha \"k1 coins:\", k1.coins; // 100\ndikha \"k2 coins:\", k2.coins; // 500"}, {"type": "heading", "level": 2, "text": "3. Shared Borrowing (udhaar) vs Mutable Borrowing (udhaar_badal)"}, {"type": "text", "text": "Instead of moving ownership, you can lend references using Aliasing XOR Mutability rules:\n- `udhaar x` (`&x`): Immutable shared borrow. Multiple shared borrows can coexist. `x` cannot be mutated or moved while borrowed!\n- `udhaar_badal x` (`&mut x`): Exclusive mutable borrow. Only one can exist at a time; direct access to `x` is suspended until the borrow ends!"}, {"type": "code", "title": "borrowing.meme", "code": "pakad hero = \"Munna Don\";\n\n// 1. Shared Borrowing:\npakad r1 = udhaar hero;\npakad r2 = udhaar hero;\ndikha \"Read via ref1:\", r1;\ndikha \"Read via ref2:\", r2;\n\n// hero = \"Circuit\"; // Error: Doglapan! 'hero' abhi udhaar pe hai!\n\n// 2. Mutable Borrowing in Scopes:\npakad score = 100;\n{\n    pakad m = udhaar_badal score;\n    m = 250; // Mutate through reference!\n    dikha \"Inside scope modified score:\", m;\n}\n// Block exited, borrow released!\ndikha \"Outside scope score:\", score; // 250"}, {"type": "heading", "level": 2, "text": "4. RAII Scope Destructors (alvida)"}, {"type": "text", "text": "Classes can define a destructor method `jugaad alvida() { ... }` (alias: `drop`). MEMElang automatically invokes this destructor when an instance's owner block scope ends!"}, {"type": "code", "title": "raii_destructors.meme", "code": "biradari FileLocker {\n    jugaad shuru(filename) {\n        apun.filename = filename;\n        dikha \"[Open] File opened:\", apun.filename;\n    }\n    jugaad alvida() {\n        dikha \"[Lock] File closed cleanly (RAII):\", apun.filename;\n    }\n}\n\n{\n    pakad locker = FileLocker(\"confidential.docx\");\n    dikha \"Doing important file processing...\";\n}\n// When scope exits, alvida() is called automatically!\ndikha \"Scope ended cleanly!\";"}]}, {"id": "meme-errors", "title": "Iconic Meme Error Catalog", "badge": "Diagnostics", "description": "Complete guide to all iconic Bollywood, Shark Tank, and Desi internet error messages in MEMElang.", "content": [{"type": "text", "text": "Say goodbye to boring stack traces. In MEMElang, compiler and runtime mistakes trigger legendary Indian internet quotes. Run any code below to trigger these errors live:"}, {"type": "table", "headers": ["Error Trigger", "Meme Quote Message", "Cultural Reference"], "rows": [["Unclosed string quote", "[!] String khol ke chhod diya... band tera baap karega?", "Classic Indian Parent Scolding"], ["Illegal alien symbol (@, $)", "[ERR] Chi sasura! Ye kaisa alien symbol ghusa diye ho code mein: <symbol>", "Gangs of Wasseypur"], ["Missing semicolon (;)", "Chhoti bachi ho kya?! Line ke end mein ';' lagana bhool gaye!", "Tiger Shroff / Heropanti"], ["Invalid LHS assignment", "Ye sab doglapan hai! '=' ke left side mein sirf seedha variable ya property aata hai!", "Ashneer Grover / Shark Tank"], ["Accidental '=' in condition", "Control Uday, control! '<context>' mein '=' nahi, '==' lagta hai!", "Welcome Movie"], ["EOF mid-expression", "Moye Moye! Expression adhoori chhod ke code beech mein hi khatam kar diya.", "Viral Moye Moye Meme"], ["Undeclared variable read", "Rasode mein kaun tha?! Ye '<name>' bina bataye kahan se aaya?", "Saath Nibhaana Saathiya"], ["Undeclared variable assign", "Chuna laga diya! '<name>' naam ka koi variable hi nahi hai.", "Panchayat / Popular Meme"], ["Use after move", "Pehli fursat mein nikal! '<name>' ki value move ho chuki hai! Ownership transfer ke baad use karna mana hai!", "Hindustani Bhau"], ["Mutate while borrowed", "Doglapan! '<name>' abhi udhaar (borrowed) pe hai, isko modify nahi kar sakte!", "Ashneer Grover"], ["Direct access while mutably borrowed", "Beech mein mat bolo! '<name>' abhi kisi aur ko udhaar diya hua hai!", "Arnab Goswami Debate"], ["Borrow conflict / Aliasing", "Ek mawaali ek time pe ek hi jagah! '<name>' already borrowed hai!", "Welcome Movie"], ["Dangling reference escape", "Baburao ka chashma! Local variable '<name>' ka udhaar wapas nahi bhej sakte!", "Hera Pheri (Baburao)"], ["Integer overflow in +", "25 din mein paisa double karte-karte integer overflow ho gaya!", "Phir Hera Pheri"], ["Negative string repeat", "Delulu is not the solulu! String ko negative baar kaise multiply karoge?", "Gen-Z Internet Slang"], ["Divide by zero", "Kya coder banega re tu? 0 se divide karke brahmand khatam karega kya!", "Hera Pheri (Baburao)"], ["Type mismatch in comparison", "Seb ko santre se mat tolo! '<t1>' aur '<t2>' ko compare nahi kar sakte!", "Desi Idiom"], ["Calling non-callable", "Beta, tumse na ho payega! '<val>' koi jugaad (function) thodi hai!", "Gangs of Wasseypur (Ramadhir)"], ["Calling nalla (null)", "Jugaad toh 'nalla' (null) nikla! Khali cheez ko call kaise karoge?", "Mumbai Slang"], ["Arity mismatch", "Gaddari korbe! Jugaad ne <N> arguments maange the, aur tumne <M> chipka diye!", "Viral Bengali Meme"], ["Missing object property", "Khopdi tod saale ka! '<class>' biradari ke paas '<prop>' naam ka kuch nahi hai!", "Hera Pheri (Baburao)"], ["Non-class superclass", "Fake nepotism! '<parent>' koi biradari (class) nahi hai!", "Bollywood Nepotism Debate"]]}, {"type": "heading", "level": 2, "text": "Try It Live: Intentional Error Demo"}, {"type": "code", "title": "trigger_error.meme", "code": "// Try running this to see Baburao's zero division error!\npakad ans = 100 / 0;\ndikha ans;"}]}, {"id": "gradual-typing", "title": "Gradual Typing & Static Analysis", "badge": "Type System", "description": "Combine dynamic prototyping flexibility with compile-time type verification using the native TypeChecker.", "content": [{"type": "text", "text": "MEMElang provides a gradual typing system. You can write code without type annotations for lightning-fast prototyping, or add static type annotations to variables, function parameters, and return types for compile-time safety and self-documenting code."}, {"type": "callout", "variant": "info", "title": "Zero Runtime Performance Penalty", "text": "Static types are verified at compile-time by the C++ `TypeChecker`. In production, verified code executes with zero runtime type checking overhead while giving you bulletproof contract guarantees."}, {"type": "heading", "level": 2, "text": "Core Type Keywords & Annotations"}, {"type": "table", "headers": ["Type Keyword", "Alternative Hindi/English", "Underlying C++ Type", "Example"], "rows": [["sankhya / ank", "int, number", "int64_t", "pakad age: sankhya = 25;"], ["point / dashamlav", "float, double", "double (IEEE-754)", "pakad pi: point = 3.14159;"], ["bol / shabd", "string, str", "std::string (interned)", "pakad name: bol = \"Chai\";"], ["sach_jhooth / sach", "bool, boolean", "bool", "pakad isOnline: sach = han;"], ["suchi", "list, array", "std::vector<Value>", "pakad items: suchi = [1, 2, 3];"], ["nakshe", "map, dict", "std::unordered_map", "pakad user: nakshe = {\"id\": 1};"], ["nalla", "null, void", "std::monostate", "pakad empty: nalla = nalla;"], ["har_kism", "any, koi_bhi", "dynamic Value", "pakad flex: har_kism = 42;"]]}, {"type": "heading", "level": 2, "text": "Typed Functions & Signatures"}, {"type": "text", "text": "Function signatures can specify parameter types and return type contracts using the `->` arrow syntax. If a function returns an incompatible type or receives illegal arguments, the compiler flags it before execution starts:"}, {"type": "code", "title": "typed_functions.meme", "code": "// Function with explicit argument and return type contracts\nkaam calculate_tax(amount: point, rate: point) -> point {\n    de amount * rate;\n}\n\n// Typed collections and variables\npakad bill: point = 1250.50;\npakad gstRate: point = 0.18;\npakad totalTax: point = calculate_tax(bill, gstRate);\n\ndikha f\"Total Tax Due: Rs.{totalTax}\";"}, {"type": "heading", "level": 2, "text": "Static Checking via CLI"}, {"type": "text", "text": "You can run static analysis checks over your entire codebase using the compiler without executing the code:"}, {"type": "terminal", "commands": ["meme check src/App.meme", "meme check tests/"]}]}];
  }
  const __ns = { getSectionsStdlib };
  $mod_2.__cache = __ns;
  return __ns;
}

// Module: sections_platform.meme
function $mod_3() {
  if ($mod_3.__cache) return $mod_3.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  function getSectionsPlatform() {
  return [{"id": "react-jsx-ui", "title": "Declarative React JSX & UI Hooks", "badge": "Frontend UI", "description": "Build reactive cross-platform user interfaces using familiar React JSX syntax, useState, useEffect, and controlled components.", "content": [{"type": "text", "text": "MEMElang treats React JSX as first-class syntax. You can build modern, responsive applications using component hierarchies, reactive hooks, and Tailwind CSS utility classes without needing Babel, Webpack, or external transpilers."}, {"type": "heading", "level": 2, "text": "1. First-Class JSX Syntax & Tailwind Styling"}, {"type": "text", "text": "Standard HTML tags (`<div>`, `<button>`, `<input>`, `<span>`) and custom components can be written directly inside expressions. Style elements using standard Tailwind CSS classes:"}, {"type": "code", "title": "simple_jsx.meme", "code": "card = (\n    <div className=\"p-6 bg-slate-900 border border-slate-800 rounded-2xl shadow-xl\">\n        <h2 className=\"text-xl font-bold text-white mb-2\">Tapri Chai</h2>\n        <p className=\"text-slate-400 text-sm mb-4\">Fresh brewed cutting chai with adrak and elaichi</p>\n        <button className=\"px-4 py-2 bg-amber-500 hover:bg-amber-400 text-slate-950 font-bold rounded-lg transition\">\n            Order Now - Rs.15\n        </button>\n    </div>\n);\n\nui.dikha(card, \"Tapri Menu\");"}, {"type": "heading", "level": 2, "text": "2. State Management with useState"}, {"type": "text", "text": "Manage stateful reactivity with the `useState` hook. Use array destructuring `[state, setState]` to capture the value and updater function. Updating state triggers automatic re-renders across all active views:"}, {"type": "code", "title": "counter_state.meme", "code": "kaam Counter() {\n    [count, setCount] = useState(0);\n\n    de (\n        <div className=\"p-6 bg-slate-900 rounded-2xl max-w-sm mx-auto text-center border border-slate-800\">\n            <h1 className=\"text-2xl font-bold text-white mb-2\">Chai Counter</h1>\n            <div className=\"text-4xl font-extrabold text-amber-400 my-4\">{count}</div>\n            <div className=\"flex gap-3 justify-center\">\n                <button\n                    className=\"px-4 py-2 bg-slate-800 hover:bg-slate-700 text-white rounded-lg font-semibold\"\n                    onClick={() => setCount(c => c - 1)}\n                >\n                    - Ghatao\n                </button>\n                <button\n                    className=\"px-4 py-2 bg-amber-500 hover:bg-amber-400 text-slate-950 rounded-lg font-bold\"\n                    onClick={() => setCount(c => c + 1)}\n                >\n                    + Jodo\n                </button>\n            </div>\n        </div>\n    );\n}"}, {"type": "heading", "level": 2, "text": "3. Lifecycle & Side Effects with useEffect"}, {"type": "text", "text": "Use `useEffect` to perform side-effects, such as loading data from local persistent storage (`tijori`) or syncing with remote APIs when the component mounts:"}, {"type": "code", "title": "use_effect.meme", "code": "kaam NotesApp() {\n    [notes, setNotes] = useState([]);\n\n    // Lifecycle hook: runs on mount to hydrate state from persistent vault\n    useEffect(() => {\n        pakad saved = tijori.nikal(\"notes_list\");\n        agar saved != nalla {\n            setNotes(saved);\n        }\n    });\n\n    de (\n        <div className=\"p-6\">\n            <h1 className=\"text-white text-xl font-bold\">Saved Notes ({notes.lambai()})</h1>\n        </div>\n    );\n}"}, {"type": "heading", "level": 2, "text": "4. Controlled Inputs & Two-Way Binding"}, {"type": "text", "text": "Inputs support seamless two-way state binding. Input values and keyboard events sync automatically with the reactive engine:"}, {"type": "code", "title": "controlled_input.meme", "code": "kaam NewNoteForm({ onAdd }) {\n    [text, setText] = useState(\"\");\n\n    kaam handleSubmit() {\n        agar text.lambai() > 0 {\n            onAdd(text);\n            setText(\"\");\n        }\n    }\n\n    de (\n        <div className=\"flex gap-2\">\n            <input\n                className=\"flex-1 bg-slate-800 text-white px-4 py-2 rounded-xl border border-slate-700 focus:outline-none focus:border-amber-500\"\n                placeholder=\"Naya note likho...\"\n                value={text}\n                onChange={(e) => setText(e.target.value)}\n            />\n            <button\n                className=\"px-5 py-2 bg-amber-500 hover:bg-amber-400 text-slate-950 font-bold rounded-xl\"\n                onClick={handleSubmit}\n            >\n                Save\n            </button>\n        </div>\n    );\n}"}, {"type": "heading", "level": 2, "text": "5. Mounting the Application"}, {"type": "text", "text": "Call `ui.mount(<App />)` to launch the top-level root component. The runtime automatically configures the appropriate rendering backend (Native macOS Cocoa Window, Web Canvas / DOM, or Terminal UI) based on your run configuration:"}, {"type": "code", "title": "mount_root.meme", "code": "// Mount top-level application root\nui.mount(<App />, \"ChaiVault Desktop\");"}]}, {"id": "native-desktop", "title": "Zero-WebView Native Desktop Engine", "badge": "Native GUI", "description": "Hardware-accelerated 60 FPS native desktop rendering via Cocoa NSWindow and CoreGraphics without WebViews or Electron overhead.", "content": [{"type": "text", "text": "Traditional desktop frameworks like Electron bundle an entire Chromium browser and Node runtime with every application, resulting in 200MB+ memory usage, slow startup times, and sluggish performance. MEMElang completely rethinks desktop apps with a Zero-WebView Native Engine."}, {"type": "callout", "variant": "info", "title": "Sub-15MB RAM & Instant Launch", "text": "MEMElang native desktop applications run on a pure Cocoa `NSWindow` via CoreGraphics Quartz. Startup takes less than 10 milliseconds, and memory consumption stays under 15MB."}, {"type": "heading", "level": 2, "text": "C++ Tailwind v4 Layout Solver"}, {"type": "text", "text": "At the core of the native desktop engine is `TailwindEngine.h`, a single-pass Flexbox and CSS Grid constraint solver written in modern C++20. It parses standard Tailwind utility classes directly and computes exact layout geometries without any browser engine:"}, {"type": "list", "items": ["Display & Direction: `flex`, `flex-col`, `flex-row`, `grid`, `grid-cols-2`, `grid-cols-3`", "Alignment & Spacing: `items-center`, `justify-between`, `justify-center`, `gap-2`, `gap-4`, `gap-6`", "Dimensions & Box Model: `w-full`, `max-w-md`, `h-screen`, `p-4`, `p-6`, `px-3`, `py-2`, `m-2`", "Borders & Shapes: `rounded-lg`, `rounded-2xl`, `rounded-full`, `border`, `border-slate-800`", "Color Palette: Full Tailwind palette (`bg-slate-950`, `bg-amber-500`, `text-white`, `text-slate-400`, etc.)", "Elevation & Effects: `shadow-sm`, `shadow-md`, `shadow-xl`, `shadow-2xl`"]}, {"type": "heading", "level": 2, "text": "Pure Vector Display List Pipeline"}, {"type": "text", "text": "Once layout computation completes, the engine translates component trees into a compact display list of raw vector draw instructions executed directly by CoreGraphics Quartz:"}, {"type": "table", "headers": ["Vector Command", "Parameters", "Quartz Render Target"], "rows": [["FILL_ROUND_RECT", "x, y, w, h, radius, color", "CGContextFillPath with CGPathAddRoundedRect"], ["STROKE_RECT", "x, y, w, h, radius, strokeWidth, color", "CGContextStrokePath with specified border width"], ["DRAW_TEXT", "x, y, text, fontSize, color, weight", "CTLineDraw / CGContextShowGlyphsAtPoint"]]}, {"type": "heading", "level": 2, "text": "Multiplatform Dev Server & Hot-Reload"}, {"type": "text", "text": "Launch the dev server with your desired target platform. The server features instantaneous state preservation and hot-reloads on file save:"}, {"type": "terminal", "commands": ["# Launch native macOS Cocoa desktop window", "meme dev src/App.meme --platform=desktop", "", "# Or simulate concrete platform targets (iOS / Android / Mac / Web)", "meme dev src/App.meme --platform=ios", "meme app.meme --platform=android", "", "# Or launch web browser canvas target", "meme dev src/App.meme --platform=web", "", "# Or launch terminal rich ASCII preview", "meme dev src/App.meme --platform=terminal"]}, {"type": "heading", "level": 2, "text": "Interactive Keyboard Shortcuts"}, {"type": "text", "text": "When running in dev mode, you can control targets and windows directly from the terminal without restarting the server:"}, {"type": "table", "headers": ["Key", "Action", "Description"], "rows": [["[d]", "Desktop Window", "Opens the hardware-accelerated Cocoa NSWindow"], ["[w]", "Web Browser", "Opens Google Chrome / Safari to the live web application"], ["[t]", "Terminal Preview", "Renders live Unicode ANSI widget frames in terminal"], ["[r]", "Hot Reload", "Forces an immediate recompilation and state reload"], ["[p]", "Switch Platform", "Cycles active default platform between desktop, web, terminal"], ["[q]", "Quit Server", "Cleanly shuts down all windows and terminates the server"]]}]}, {"id": "chameleon-bridge", "title": "Chameleon Universal Interop Bridge", "badge": "Universal Bridge", "description": "Call Python NumPy / Pandas, Node/npm packages, and Rust crates natively in MEMElang with zero configuration.", "content": [{"type": "text", "text": "Why choose between Python for scientific data science, Node/npm for modern cloud packages, and Rust for systems-grade cryptography? The Chameleon Universal Interop Bridge lets MEMElang import libraries from all three ecosystems seamlessly with zero glue code and zero configuration."}, {"type": "callout", "variant": "info", "title": "Universal Ecosystem Syntax", "text": "Use the familiar `bula \"<ecosystem>:<module>\" jaise <alias>;` syntax. The Chameleon Sentinel automatically detects environment runtimes and marshals parameters across languages bidirectionally."}, {"type": "heading", "level": 2, "text": "1. Python Interop (NumPy & Pandas)"}, {"type": "text", "text": "Call Python packages like `numpy`, `pandas`, and `scipy` directly. Array operations, matrix math, and DataFrames operate with native syntax:"}, {"type": "code", "title": "python_interop.meme", "code": "// Direct Python NumPy vector math\nbula \"py:numpy\" jaise np;\n\npakad vec = np.arange(10, 100, 10);\ndikha \"NumPy Vector:\", vec;\ndikha \"Sum:\", np.sum(vec);\ndikha \"Mean:\", np.mean(vec);\n\npakad v1 = np.array([1, 2, 3]);\npakad v2 = np.array([4, 5, 6]);\ndikha \"Dot Product:\", np.dot(v1, v2);\n\n// Direct Python Pandas DataFrames\nbula \"py:pandas\" jaise pd;\n\npakad df = pd.DataFrame([\n    { \"item\": \"Chai\", \"price\": 20, \"sold\": 150 },\n    { \"item\": \"Bun Maska\", \"price\": 40, \"sold\": 80 },\n    { \"item\": \"Maggi\", \"price\": 50, \"sold\": 45 }\n]);\n\ndikha \"Total Menu Items:\", df.col(\"item\").lambai();"}, {"type": "heading", "level": 2, "text": "2. JavaScript & NPM Interop"}, {"type": "text", "text": "Tap into the npm registry for real-time messaging, WebSockets, or cloud databases:"}, {"type": "code", "title": "npm_interop.meme", "code": "// Realtime messaging via Ably\nbula \"npm:ably\" jaise ably;\n\npakad client = ably.Realtime(\"API_KEY\");\npakad channel = client.channels.get(\"orders\");\nchannel.publish(\"new_order\", {\n    \"item\": \"Kullad Chai\",\n    \"qty\": 3,\n    \"timestamp\": systumm.ab()\n});\n\n// Cloud Document DB via Firebase\nbula \"npm:firebase\" jaise fb;\npakad db = fb.firestore();\ndb.collection(\"orders\").add({ \"status\": \"pending\", \"total\": 90 });"}, {"type": "heading", "level": 2, "text": "3. Rust High-Performance Interop"}, {"type": "text", "text": "Leverage Rust's blistering speed for cryptographic hashing, UUID generation, or parallel iteration:"}, {"type": "code", "title": "rust_interop.meme", "code": "// Cryptographic operations & UUIDs\nbula \"rust:crypto\" jaise crypto;\n\npakad secretHash = crypto.sha256(\"MEMElang_Ultra_Secret\");\npakad traceId = crypto.uuid_v4();\n\ndikha \"SHA-256 Digest:\", secretHash;\ndikha \"Trace UUID:\", traceId;\n\n// Parallel computing via Rayon\nbula \"rust:rayon\" jaise rayon;\npakad dataset = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];\npakad parallelSquares = rayon.par_map(dataset, x => x * x);\ndikha \"Parallel Squared:\", parallelSquares;"}, {"type": "heading", "level": 2, "text": "Cross-Language Type Marshaling"}, {"type": "table", "headers": ["MEMElang Value", "Python Type", "JavaScript Type", "Rust Type"], "rows": [["sankhya (42)", "int", "number", "i64 / u64"], ["point (3.14)", "float", "number", "f64"], ["bol (\"namaste\")", "str", "string", "String / &str"], ["sach (han / real)", "bool (True)", "boolean (true)", "bool"], ["suchi ([1, 2, 3])", "list", "Array", "Vec<T>"], ["nakshe ({\"a\": 1})", "dict", "Object", "HashMap<K, V>"], ["nalla (null)", "None", "null / undefined", "Option::None"]]}]}, {"id": "tapri-package-manager", "title": "Tapri Package Manager, Workspaces & Storage", "badge": "Tooling & Storage", "description": "Manage project manifests (tapri.json), lockfiles, root path resolution, persistent local storage, and automated testing.", "content": [{"type": "text", "text": "Tapri is the official package manager, workspace orchestrator, and persistent runtime companion for MEMElang, named after India's ubiquitous roadside tea stalls where every conversation and idea comes together."}, {"type": "heading", "level": 2, "text": "1. tapri.json Project Manifest"}, {"type": "text", "text": "Every MEMElang project is defined by a `tapri.json` file detailing project scripts, entrypoints, and cross-ecosystem dependencies:"}, {"type": "code", "title": "tapri.json", "code": "{\n  \"name\": \"notes-desktop\",\n  \"version\": \"1.0.0\",\n  \"main\": \"src/App.meme\",\n  \"scripts\": {\n    \"dev\": \"meme dev src/App.meme --platform=desktop\",\n    \"web\": \"meme dev src/App.meme --platform=web\",\n    \"start\": \"meme src/App.meme\",\n    \"test\": \"meme test tests\"\n  },\n  \"dependencies\": {\n    \"py:numpy\": \"^1.24.0\",\n    \"rust:crypto\": \"latest\"\n  }\n}"}, {"type": "heading", "level": 2, "text": "2. tapri.lock Determinism"}, {"type": "text", "text": "When dependencies are installed, Tapri computes exact versions and integrity hashes, writing them to `.meme_env/tapri.lock`. This guarantees identical, deterministic builds across every developer machine and CI/CD environment."}, {"type": "heading", "level": 2, "text": "3. Tapri CLI Commands Reference"}, {"type": "table", "headers": ["Command", "Purpose", "Example"], "rows": [["meme naya <name>", "Scaffold project with templates (react, api, fullstack)", "meme naya my-app --template=react"], ["meme tapri init", "Initialize tapri.json in current directory", "meme tapri init"], ["meme tapri install", "Resolve and install dependencies into .meme_env/", "meme tapri install"], ["meme tapri add <pkg>", "Add a package to tapri.json and install it", "meme tapri add py:numpy"], ["meme tapri suchi", "List all installed packages and versions", "meme tapri suchi"], ["meme run <script>", "Execute a script configured in tapri.json", "meme run test"], ["meme setup", "Link meme binary globally into system PATH", "meme setup"]]}, {"type": "heading", "level": 2, "text": "4. Root Path Resolution (@/ Aliases)"}, {"type": "text", "text": "Avoid brittle relative paths like `../../components/Header.meme`. MEMElang automatically resolves the `@/` alias to the root directory where `tapri.json` resides:"}, {"type": "code", "title": "root_imports.meme", "code": "// Clean root-relative imports from any nested directory\nbula \"@/components/Header.meme\" jaise Header;\nbula \"@/components/MetricCard.meme\" jaise MetricCard;\nbula \"@/services/PythonService.meme\" jaise PyService;"}, {"type": "heading", "level": 2, "text": "5. Tijori: Persistent Local Storage Vault"}, {"type": "text", "text": "Applications often need persistent key-value and document storage that survives restarts without setting up heavy external databases. MEMElang includes Tijori (the Vault), a lightweight embedded database:"}, {"type": "code", "title": "tijori_storage.meme", "code": "// 1. Save data persistently to disk\ntijori.rakh(\"user_preferences\", {\n    \"theme\": \"dark\",\n    \"fontSize\": 14,\n    \"lastOpened\": systumm.ab()\n});\n\n// 2. Retrieve saved data\npakad prefs = tijori.nikal(\"user_preferences\");\nagar prefs != nalla {\n    dikha \"Saved Theme:\", prefs[\"theme\"];\n}\n\n// 3. Remove a specific key\ntijori.hatao(\"temporary_session\");\n\n// 4. Safely clear the vault\n// tijori.saaf();"}, {"type": "heading", "level": 2, "text": "6. Automated Testing Framework"}, {"type": "text", "text": "Write clean BDD-style unit and integration tests using `test(name, fn)` and `expect(val).toBe(...)`. Run your entire test suite with `meme test`:"}, {"type": "code", "title": "app_test.meme", "code": "// tests/notes_test.meme\ntest(\"Notes count parity\", () => {\n    pakad defaultNotes = [\n        { \"id\": 1, \"title\": \"Buy tea\" },\n        { \"id\": 2, \"title\": \"Code MEMElang\" }\n    ];\n    expect(defaultNotes.lambai()).toBe(2);\n    expect(defaultNotes[0].title).toBe(\"Buy tea\");\n});\n\ntest(\"Immutability on note append\", () => {\n    pakad original = [\"A\", \"B\"];\n    pakad updated = [...original, \"C\"];\n    expect(updated.lambai()).toBe(3);\n    expect(original.lambai()).toBe(2);\n});"}, {"type": "terminal", "commands": ["meme test tests/", "[OK] Tests passed: 2 | Failed: 0 (100% green)"]}]}, {"id": "native-permissions", "title": "Cross-Platform Runtime Permissions", "badge": "Permissions API", "description": "Check, request, and enforce OS permissions uniformly across iOS, Android, macOS, Windows, Linux, and Web with selective manifest generation.", "content": [{"type": "text", "text": "The `native.permissions` (or `permissions`) module provides a single unified API modeled after Capacitor, Capawesome, and Tauri plugins. It handles OS dialogs, capability normalization, and per-platform manifest entries from `tapri.json`."}, {"type": "callout", "variant": "info", "title": "Standardized Permission States & Dual API Styles", "text": "Every check or request resolves to a canonical status object (`state`, `granted`, `canAskAgain`, `restricted`, `limited`, `platform`, `reason`). States are `granted`, `denied`, `prompt`, `prompt-with-rationale`, `restricted`, `limited`, or `unavailable`. Both trailing callbacks and `.then()` / `.phir()` thenables work identically across the VM and Native AOT compiler."}, {"type": "heading", "level": 2, "text": "1. Checking, Requesting & Opening Settings"}, {"type": "code", "title": "permissions_example.meme", "code": "bula \"permissions\";\nbula \"native\";\n\n// Check without prompting the user\npakad cam = permissions.check(\"camera\");\ndikha \"Camera state:\", cam.state, \"canAskAgain:\", cam.canAskAgain;\n\n// Callback style (non-blocking across UI, iOS, Android, and Web)\npermissions.request(\"notifications\", (res) => {\n    agar (res.granted || res.limited) {\n        native.notify.send(\"MEME-UI\", \"Notifications active on \" + res.platform);\n    } elif (!res.canAskAgain) {\n        permissions.openSettings(\"notifications\");\n    }\n});\n\n// Thenable / Promise style also works seamlessly\npermissions.request(\"location\").then((res) => {\n    dikha \"Location permission:\", res.state;\n});\n\n// Batch-request multiple capabilities\npermissions.requestMany([\"camera\", \"microphone\"], (results) => {\n    dikha \"Camera:\", results.camera.state, \"Mic:\", results.microphone.state;\n});"}, {"type": "heading", "level": 2, "text": "2. Selective Per-Platform tapri.json Permissions"}, {"type": "text", "text": "Declare only the permissions each OS actually needs inside `tapri.json`. During `meme package`, MEMElang emits only the required native manifest keys (preventing App Store / Play Store over-permission rejections):"}, {"type": "code", "title": "tapri.json", "code": "{\n  \"name\": \"camera-notes\",\n  \"bundleId\": \"com.example.cameranotes\",\n  \"permissions\": {\n    \"strict\": true,\n    \"common\": [\"notifications\", \"clipboard\"],\n    \"ios\": {\n      \"camera\": \"Capture whiteboard notes directly inside the editor.\",\n      \"photos\": \"Attach existing photos from your library.\",\n      \"biometrics\": \"Unlock protected vaults with Face ID.\"\n    },\n    \"android\": [\"camera\", \"photos\", \"location\"],\n    \"mac\": {\n      \"camera\": \"Capture photos with your Mac camera.\"\n    },\n    \"web\": [\"notifications\", \"camera\"]\n  }\n}"}, {"type": "table", "headers": ["Target OS", "Generated Manifest / Metadata", "Runtime Mechanism"], "rows": [["iOS (.ipa)", "Selective NS*UsageDescription keys + UIBackgroundModes", "UNUserNotificationCenter, AVFoundation, PHPhotoLibrary, CLLocationManager, LAContext"], ["Android (.apk)", "Selective <uses-permission> tags + MemeNativeActivity.java classes.dex", "ActivityCompat checkSelfPermission, requestPermissions, JNI onRequestPermissionsResult"], ["macOS (.app)", "Info.plist usage strings + codesign entitlements.plist", "TCC status queries, UNUserNotificationCenter, x-apple.securitypreferences URLs"], ["Windows (.exe)", "Embedded RT_MANIFEST XML + consent query", "CapabilityAccessManager ConsentStore Registry + ms-settings:privacy*"], ["Linux (.deb/.rpm/.ipk)", "/usr/share/<pkg>/capabilities.json", "Policy/device presence checks, zenity prompt, Control Center settings"], ["Web (.wasm)", "<meta http-equiv=\"Permissions-Policy\"> HTML tag", "Notification API, navigator.permissions.query, getUserMedia, Geolocation API"]]}]}, {"id": "platform-separation", "title": "Platform Separation & Multi-Target Code", "badge": "WORE + Escape Hatches", "description": "Write shared UI once while separating OS-specific adapters cleanly via platform.select(), <Platform /> JSX, and .ios.meme / .android.meme / .web.meme files.", "content": [{"type": "text", "text": "Write-once doesn't mean one-size-fits-all. MEMElang provides three clean layers for platform-specific code across both the Interpreter and Native C++20 AOT compiler:"}, {"type": "heading", "level": 2, "text": "1. Runtime & Compile-Time platform Module"}, {"type": "code", "title": "platform_api.meme", "code": "bula \"platform\";\nbula \"platform\" jaise { os, select, is, isMobile, isDesktop };\n\ndikha \"OS:\", os();           // \"ios\" | \"android\" | \"mac\" | \"windows\" | \"linux\" | \"web\"\ndikha \"Mobile?:\", isMobile(), \"Apple?:\", is(\"apple\");\n\npakad driverConfig = select({\n    \"ios\": \"UIKit Metal + Haptic Engine\",\n    \"android\": \"NativeActivity + GLES2\",\n    \"mobile\": \"Generic Mobile Fallback\",\n    \"desktop\": \"High-DPI Window Chrome\",\n    \"web\": \"Browser WebGL2 Canvas\",\n    \"default\": \"Universal Mode\"\n});"}, {"type": "heading", "level": 2, "text": "2. Target-Aware Module File Extensions"}, {"type": "text", "text": "Import `bula \"./storage_adapter\"` (or `@/adapters/auth`) and place OS-specific implementations beside each other. Both `./meme --platform=<os>` and `./meme package --target=<t>` automatically pick the highest-priority file for the target and only compile that file's code into the final binary:"}, {"type": "table", "headers": ["Target Platform", "Resolution Order (Highest -> Fallback)"], "rows": [["ios", ".ios.meme -> .mobile.meme -> .apple.meme -> .native.meme -> platforms/ios/ -> .meme"], ["android", ".android.meme -> .mobile.meme -> .native.meme -> platforms/android/ -> .meme"], ["mac", ".mac.meme -> .macos.meme -> .desktop.meme -> .apple.meme -> .native.meme -> platforms/mac/ -> .meme"], ["windows", ".win.meme -> .windows.meme -> .desktop.meme -> .native.meme -> platforms/windows/ -> .meme"], ["linux", ".linux.meme -> .desktop.meme -> .native.meme -> platforms/linux/ -> .meme"], ["web", ".web.meme -> .wasm.meme -> platforms/web/ -> .meme"]]}, {"type": "heading", "level": 2, "text": "3. Declarative <Platform /> JSX Component"}, {"type": "code", "title": "platform_jsx.meme", "code": "bula \"ui:shadcn\" jaise { Platform, Card };\n\nkaam Header() {\n    de (\n        <Card class=\"p-4 flex flex-col gap-2\">\n            <Platform ios android>\n                <span>Rendered strictly on Mobile touch devices</span>\n            </Platform>\n            <Platform only=\"mac,windows\" exclude=\"web\" fallback={<span>Browser Toolbar</span>}>\n                <span>Desktop Native Menu Bar</span>\n            </Platform>\n        </Card>\n    );\n}"}]}, {"id": "native-os-apis", "title": "Native OS & Hardware APIs", "badge": "Hardware & Storage", "description": "Access Clipboard, Notifications, Keychain, KV Storage, Biometrics, Haptics, Camera, Location, Sensors, and Window controls across every platform.", "content": [{"type": "text", "text": "Import `bula \"native\"` (or legacy alias `bula \"app\"`) to call native OS capabilities directly without writing Swift, Kotlin, Win32, or JS glue code."}, {"type": "table", "headers": ["Namespace", "Key Functions", "Cross-Platform Backing"], "rows": [["native.clipboard", "get(), set(text), hasText(), clear()", "NSPasteboard / UIPasteboard / Android Clipboard / Win32 / xclip / Web"], ["native.dialog", "alert, confirm, prompt, pickFile, saveFile, pickFolder", "NSAlert / UIAlertController / Native File Dialogs / Wasm"], ["native.notify", "requestPermission(cb), send(title, body), badge(n)", "UNUserNotificationCenter / osascript / notify-send / Web Notifications"], ["native.storage", "get(k), set(k, v), remove(k), keys()", "AppData JSON store / localStorage"], ["native.keychain", "get(svc, k), set(svc, k, v), remove(svc, k)", "Apple Keychain Services / Android / Windows / Secret store"], ["native.biometrics", "isAvailable(), authenticate(reason, cb)", "LocalAuthentication TouchID/FaceID / BiometricPrompt"], ["native.device", "info(), platform(), screen(), safeArea(), locale(), theme()", "OS metrics, safe area insets & hardware specs"], ["native.power", "battery(), network()", "Battery charge level, transport type (wifi/cellular/ethernet)"], ["native.haptics / audio", "impact(style), vibrate(ms), beep(freq, ms), speak(txt)", "CoreHaptics / AudioServices / Android Vibrator / WebAudio"], ["native.location / camera / sensors", "get(cb), pickPhoto(cb), capture(cb), poll()", "CoreLocation, AVCapture, UIImagePicker, CoreMotion"]]}]}, {"id": "multiplatform-packaging", "title": "Multi-Platform Packaging & Icons", "badge": "8 Targets Zero-WebView", "description": "Build signed native executables, installers, mobile bundles (.apk / .ipa), Linux packages (.deb / .rpm / .ipk), and Zero-DOM Wasm PWAs from one command.", "content": [{"type": "text", "text": "Run `meme package <entry.meme> --target=...` (or `meme build --target=...`) to cross-compile and package your app with automatic icon generation, manifests, entitlements, and Android `classes.dex`."}, {"type": "terminal", "commands": ["# Package all 8 targets cleanly from tapri.json", "meme build --target=all -o dist/", "", "# Or package a single target with custom icon, name, bundle ID, and scheme", "meme package src/App.meme --target=apk --name=\"MyApp\" --id=com.example.myapp --icon=assets/icon.png", "meme package src/App.meme --target=ipa -o dist/", "meme package src/App.meme --target=mac -o dist/", "meme package src/App.meme --target=web -o dist/"]}, {"type": "table", "headers": ["Target Flag", "Output Artifact", "Compiler & Linker Toolchain"], "rows": [["mac", "<App>.app Bundle + .icns + entitlements.plist", "clang++ -x objective-c++ (Cocoa, Metal, CoreGraphics, ad-hoc codesign)"], ["win", "<app>.exe PE32+ x64 + .ico + RT_MANIFEST", "zig c++ -target x86_64-windows-gnu + llvm-rc (Win32, Direct2D, DirectWrite)"], ["deb / rpm / ipk", ".deb / .rpm / .ipk + capabilities.json + 256px PNG", "zig c++ -target x86_64-linux-musl (X11/EGL dynamic loader)"], ["apk", "<app>.apk Dual-ABI + classes.dex + Mipmaps", "Android NDK arm64-v8a & x86_64 + javac + d8 + aapt2 + zipalign + apksigner"], ["ipa", "<App>.ipa + <App>-Simulator.app + Info.plist", "xcrun --sdk iphoneos / iphonesimulator arm64 (UIKit, Metal, UserNotifications)"], ["web", "web/index.html + .js + .wasm PWA Bundle", "Emscripten em++ (Zero-DOM WebGL2 Canvas + Permissions-Policy meta)"]]}]}, {"id": "vscode-extension", "title": "VS Code Extension & IntelliSense Tooling", "badge": "IDE & Developer Experience", "description": "First-class Visual Studio Code support with context-aware autocomplete, syntax highlighting, hover documentation, parameter signatures, and snippets.", "content": [{"type": "text", "text": "MEMElang includes an official Visual Studio Code extension located at `tools/vscode-memelang`. It provides instant syntax suggestions, context-aware completions, parameter signature help, hover documentation, and productivity snippets for all language features."}, {"type": "heading", "level": 2, "text": "1. Intelligent Context-Aware Suggestions"}, {"type": "list", "items": ["Server Autocomplete (`app.` / `router.`): Type `app.` to see all Express and Elysia methods (`get`, `post`, `put`, `delete`, `use`, `listen`, `state`, `derive`, `decorate`, `onRequest`).", "TypeBox Schema Builder (`t.` / `schema.`): Type `t.` to autocomplete TypeBox validation rules (`String`, `Integer`, `Number`, `Boolean`, `Object`, `Array`, `Optional`, `Union`, `Literal`, `Record`, `Any`).", "Request, Response & Context (`req.` / `res.` / `ctx.`): Autocomplete `req.params`, `req.query`, `req.body`, `res.status().json()`, `res.sendFile()`, `ctx.body`, `ctx.store`, and `ctx.set`.", "Preinstalled Modules (`math.`, `crypto.`, `random.`, `file.`, `rasta.`, `dhaga.`, `kaamgar.`, `Vaada.`): Type `.` after any standard library identifier to get tailored method suggestions.", "Type Annotations (`: `): Typing `:` after variables offers instant suggestions for all 15 primitive and composite types (`ank`, `int32`, `byte`, `point`, `float32`, `char`, `bol`, `bytes`, `sach`, `soochi`, `khata`, `set`, `tuple`, `har_kism`).", "Tantrik Magic Methods (`__`): Typing `__` suggests all dunder operator overloading methods (`__jod__`, `__ghata__`, `__barabar__`, `__chota__`, `__la__`, `__daal__`, `__dikha__`, etc.)."]}, {"type": "heading", "level": 2, "text": "2. Hover Documentation & Signature Help"}, {"type": "text", "text": "Hovering over any keyword, standard library function, or framework method displays interactive Markdown documentation with signatures and code examples. Typing `(` inside functions like `fetch()`, `app.get()`, or `useState()` displays active parameter signature help."}, {"type": "heading", "level": 2, "text": "3. Installation"}, {"type": "terminal", "commands": ["# 1. Symlink directly into your VS Code extensions folder (Instant live sync)", "ln -s \"$(pwd)/tools/vscode-memelang\" ~/.vscode/extensions/memelang", "", "# 2. Or package and install the official .vsix bundle", "cd tools/vscode-memelang", "npx @vscode/vsce package", "code --install-extension memelang-2.0.0.vsix"]}]}, {"id": "hosting-deploy", "title": "Hosting & Deployment (Vercel, Netlify, Pages)", "description": "Host 100% pure MEMElang web apps built with --target=web on Vercel, Netlify, Cloudflare Pages, and GitHub Pages with SPA routing and Wasm MIME headers.", "content": [{"type": "text", "text": "When you package a MEMElang UI app with `--target=web`, the packager compiles your `.meme` source via AOT C++ + Emscripten (`em++`) into a self-contained static directory containing `index.html`, `<name>.js`, `<name>.wasm`, `manifest.webmanifest`, app icons, `vercel.json`, `_redirects`, and `404.html`. Because zero backend server is needed, any static host can serve your MEMElang app."}, {"type": "heading", "level": 2, "text": "1. Package Static Web Bundle"}, {"type": "terminal", "commands": ["./meme package docs/src/App.meme --target=web -o docs/.build", "ls -la docs/dist  # index.html, 404.html, memelang_docs.js, .wasm, vercel.json, _redirects"]}, {"type": "callout", "variant": "info", "title": "Automatic SPA Routing & MIME Config", "text": "`--target=web` emits `<base href=\"/\">` in `index.html`, `vercel.json` (rewrites `/(.*) -> /index.html` + `application/wasm` immutable caching + `Permissions-Policy` headers), `_redirects` for Netlify/Cloudflare Pages, and `404.html` for GitHub Pages. `<Router>`, `<Route>`, `<Link>`, and `useRouter()` stay synced with `window.history.pushState` automatically."}, {"type": "heading", "level": 2, "text": "2. Deploying to Vercel (Two Workflows)"}, {"type": "table", "headers": ["Workflow", "When to Use", "Setup"], "rows": [["Prebuilt Static Directory (docs/dist)", "Instant Vercel deploys on git push without Emscripten in Vercel", "Framework Preset: Other | Build Command: (empty) | Output Directory: docs/dist"], ["GitHub Actions -> Vercel CLI", "Keep compiled .wasm binaries out of git history", "CI compiles ./meme + emsdk, packages --target=web, and runs vercel deploy --prebuilt"], ["Netlify / Cloudflare Pages / GitHub Pages", "Zero-config alternative CDNs", "Point publish directory to docs/dist (_redirects & 404.html included)"]]}, {"type": "code", "title": "route_sync_example.meme", "code": "bula \"ui\" jaise { useState, useRouter, Router, Route, Link };\n\njugaad Home() {\n    ye_le <div className=\"p-4 text-zinc-100\">Welcome to MEMElang Web App!</div>;\n}\n\njugaad About() {\n    ye_le <div className=\"p-4 text-emerald-300\">Hosted on Vercel Edge CDN</div>;\n}\n\njugaad App() {\n    pakad router = useRouter();\n    ye_le (\n        <div className=\"min-h-screen bg-zinc-950 p-6 flex flex-col gap-4\">\n            <div className=\"flex flex-row gap-3\">\n                <Link to=\"/\">Home</Link>\n                <Link to=\"/about\">About</Link>\n            </div>\n            <span>Route: {router.path}</span>\n            <Router>\n                <Route path=\"/\" component={Home} />\n                <Route path=\"/about\" component={About} />\n            </Router>\n        </div>\n    );\n}\n\nui.mount(App);"}]}];
  }
  const __ns = { getSectionsPlatform };
  $mod_3.__cache = __ns;
  return __ns;
}

// Module: presets.meme
function $mod_4() {
  if ($mod_4.__cache) return $mod_4.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  function getPlaygroundPresets() {
  return [{"id": "chaivault-desktop", "name": "[UI] ChaiVault Native Desktop App (CoreGraphics 60 FPS)", "description": "Hardware-accelerated native macOS desktop notes app with Tailwind v4 layout, category filter, and persistent Tijori storage", "code": "// ChaiVault Native Desktop App in MEMElang\nbula \"ui:shadcn\" jaise { Button, Badge }\n\nkaam App() {\n    [notes, setNotes] = useState([\n        { \"id\": 1, \"title\": \"Compiler Optimizations\", \"category\": \"Dev\", \"priority\": \"High\" },\n        { \"id\": 2, \"title\": \"Chai break at 5 PM\", \"category\": \"Personal\", \"priority\": \"Low\" },\n        { \"id\": 3, \"title\": \"Zero-WebView GPU Profiling\", \"category\": \"Dev\", \"priority\": \"Urgent\" }\n    ]);\n    [filter, setFilter] = useState(\"all\");\n\n    visibleNotes = notes.filter(n => filter == \"all\" || n.category == filter);\n\n    de (\n        <div className=\"p-8 bg-slate-950 text-white min-h-screen\">\n            <div className=\"max-w-4xl mx-auto\">\n                <div className=\"flex items-center justify-between pb-6 border-b border-slate-800\">\n                    <div>\n                        <h1 className=\"text-3xl font-extrabold text-amber-400\">ChaiVault</h1>\n                        <p className=\"text-sm text-slate-400\">Native Cocoa NSWindow • 60 FPS CoreGraphics</p>\n                    </div>\n                    <div className=\"flex gap-2\">\n                        <button className=\"px-3 py-1 bg-slate-800 hover:bg-slate-700 rounded-lg text-xs\" onClick={() => setFilter(\"all\")}>All</button>\n                        <button className=\"px-3 py-1 bg-amber-500/20 text-amber-300 rounded-lg text-xs\" onClick={() => setFilter(\"Dev\")}>Dev</button>\n                        <button className=\"px-3 py-1 bg-emerald-500/20 text-emerald-300 rounded-lg text-xs\" onClick={() => setFilter(\"Personal\")}>Personal</button>\n                    </div>\n                </div>\n\n                <div className=\"grid grid-cols-3 gap-4 mt-6\">\n                    {visibleNotes.map(n => (\n                        <div key={n.id} className=\"p-4 bg-slate-900 border border-slate-800 rounded-2xl\">\n                            <span className=\"text-xs px-2 py-0.5 rounded bg-amber-500/20 text-amber-300 font-bold\">{n.priority}</span>\n                            <h3 className=\"text-lg font-bold mt-2\">{n.title}</h3>\n                            <p className=\"text-xs text-slate-500 mt-1\">Category: {n.category}</p>\n                        </div>\n                    ))}\n                </div>\n            </div>\n        </div>\n    );\n}\n\nui.mount(<App />, \"ChaiVault Desktop\");"}, {"id": "shadcn-gpu-ui", "name": "[GPU] Zero-WebView Native GPU UI (JSX + Shadcn + Tailwind)", "description": "Direct GPU Canvas rendering powered by React JSX syntax, Shadcn components, and mathematical Tailwind CSS engine", "code": "// [GPU] Zero-WebView Native GPU UI in MEMElang\nbula \"ui:shadcn\" jaise { Card, Button, Badge, CardHeader, CardTitle, CardContent }\n\n// 1. React State Hook with Array Destructuring\n[orders, setOrders] = useState([\n    {\"item\": \"Masala Chai\", \"price\": 20, \"badge\": \"Special\"},\n    {\"item\": \"[Snack] Bun Maska\", \"price\": 40, \"badge\": \"Hot\"},\n    {\"item\": \"[Food] Street Maggi\", \"price\": 50, \"badge\": \"Popular\"}\n])\n\n// 2. Pure React JSX Component Tree with Tailwind Utility Classes\nappTree = (\n    <Card className=\"p-6 bg-slate-900 border-slate-800 rounded-3xl w-full text-slate-100 shadow-2xl\">\n        <CardHeader className=\"flex flex-row items-center justify-between pb-4\">\n            <div>\n                <CardTitle className=\"text-2xl font-bold tracking-tight text-white\">\n                    Tapri Point-Of-Sale\n                </CardTitle>\n                <div className=\"text-sm text-slate-400 mt-1\">\n                    Zero-WebView Direct GPU UI Engine • 60 FPS Canvas\n                </div>\n            </div>\n            <Badge variant=\"outline\" className=\"bg-emerald-950/60 text-emerald-400 border-emerald-700/50\">\n                [OK] Live Terminal\n            </Badge>\n        </CardHeader>\n\n        <CardContent className=\"mt-4 flex flex-col gap-3\">\n            <div className=\"text-xs font-semibold uppercase tracking-wider text-slate-400\">\n                Current Active Orders\n            </div>\n            <div className=\"p-3 bg-slate-800/80 rounded-xl flex items-center justify-between border border-slate-700/50\">\n                <div>\n                    <span className=\"font-medium text-white\">Masala Chai</span>\n                    <span className=\"text-xs text-slate-400 ml-2\">Rs.20</span>\n                </div>\n                <Badge variant=\"default\" className=\"bg-amber-500 text-slate-950 font-bold\">Ready</Badge>\n            </div>\n            <div className=\"p-3 bg-slate-800/80 rounded-xl flex items-center justify-between border border-slate-700/50\">\n                <div>\n                    <span className=\"font-medium text-white\">[Snack] Bun Maska</span>\n                    <span className=\"text-xs text-slate-400 ml-2\">Rs.40</span>\n                </div>\n                <Badge variant=\"secondary\">Cooking</Badge>\n            </div>\n            <div className=\"p-3 bg-slate-800/80 rounded-xl flex items-center justify-between border border-slate-700/50\">\n                <div>\n                    <span className=\"font-medium text-white\">[Food] Street Maggi</span>\n                    <span className=\"text-xs text-slate-400 ml-2\">Rs.50</span>\n                </div>\n                <Badge variant=\"destructive\">Pending</Badge>\n            </div>\n            <Button variant=\"default\" size=\"lg\" className=\"w-full mt-2 bg-gradient-to-r from-amber-500 to-orange-500 font-bold text-slate-950\">\n                [GPU] Process Next Order\n            </Button>\n        </CardContent>\n    </Card>\n)\n\n// 3. Mount to Direct GPU Canvas & Terminal\nui.dikha(appTree, \"Tapri Direct GPU App\")"}, {"id": "ecosystem-bridge", "name": "[Web] Universal Ecosystem Bridge (NumPy, Pandas & Crypto)", "description": "Call Python NumPy / Pandas and Rust cryptographic libraries natively in MEMElang with zero configuration", "code": "// [Web] Universal Ecosystem Bridge in MEMElang\n\n// 1. Python NumPy Math & Vector Operations\nbula \"py:numpy\" jaise np\nnumbers = np.arange(10, 110, 10)\ndikha \"[hara bold]NumPy Vector:[/] \" + numbers\ndikha \"Sum = \" + np.sum(numbers) + \", Mean = \" + np.mean(numbers)\n\nv1 = np.array([1, 2, 3])\nv2 = np.array([4, 5, 6])\ndikha \"Dot Product (v1 • v2) = \" + np.dot(v1, v2)\n\n// 2. Python Pandas DataFrames\nbula \"py:pandas\" jaise pd\nsales = pd.DataFrame([\n    {\"item\": \"Chai\", \"price\": 20, \"sold\": 150},\n    {\"item\": \"Samosa\", \"price\": 15, \"sold\": 80},\n    {\"item\": \"Maggi\", \"price\": 50, \"sold\": 45}\n])\ndikha \"\\n[asmani bold]Pandas DataFrame Summary:[/] Total Rows = \" + sales.col(\"item\").lambai()\n\n// 3. Rust Cryptography (SHA-256 & UUID v4)\nbula \"rust:crypto\" jaise crypto\nsecretKey = crypto.sha256(\"MEMElang_Secret_2026\")\ntraceId = crypto.uuid_v4()\ndikha \"\\n[peela bold]Rust SHA-256:[/] \" + secretKey\ndikha \"[peela bold]Rust UUID v4:[/] \" + traceId"}, {"id": "tapri-pos-fullstack", "name": "Tapri POS Full-Stack (DB, Dhaba & UI)", "description": "Tijori (persistent local DB), Dhaba (FastAPI server), and Declarative Cross-Platform UI Framework", "code": "// Tapri POS Full-Stack Application in MEMElang\n\n// 1. Persistent Document Database (Tijori)\ntijori.rakh(\"orders\", [\n    {\"item\": \"Masala Chai\", \"price\": 20, \"status\": \"Ready\"},\n    {\"item\": \"Bun Maska\", \"price\": 40, \"status\": \"Cooking\"}\n])\n\n// 2. FastAPI-style Backend Server (Dhaba)\ndhaba.get(\"/orders\", (req) => {\n    de {\"ok\": han, \"maal\": tijori.nikal(\"orders\")}\n})\n\n// Test the Dhaba endpoint directly\nres = dhaba.request(\"GET\", \"/orders\")\ndikha \"[hara bold]HTTP \" + res[\"status\"] + \" OK:[/] \" + res[\"body\"]\n\n// 3. Declarative Cross-Platform UI Framework\nscreen = ui.khamba([\n    ui.likh(\"Tapri POS App\", \"heading\"),\n    ui.katar([\n        ui.dabba([\n            ui.likh(\"Masala Chai - Rs.20\"),\n            ui.badge(\"Ready\", \"hara\")\n        ], \"Order #1\"),\n        ui.dabba([\n            ui.likh(\"Bun Maska - Rs.40\"),\n            ui.badge(\"Cooking\", \"peela\")\n        ], \"Order #2\")\n    ]),\n    ui.puch(\"Naya order likho...\"),\n    ui.button(\"Order Bhej\", () => {\n        dikha \"[hara mota]Naya order lag gaya![/]\"\n    })\n])\n\n// 4. Render to Terminal Screen\nui.dikha(screen, \"Tapri Multi-Platform App\")\n\n// 5. Trigger Interactive Button Callback\nui.click(screen, \"Order Bhej\")"}, {"id": "rich-terminal-stdlib", "name": "[UI] Rich Terminal, UI Widgets & Stdlib", "description": "Inline style tags, Unicode tables (dikha.mez), box panels (dikha.dabba), string styling (.laal, .hara), systumm & cli", "code": "// [UI] Rich dikha styling, Unicode tables, boxes, and standard libraries!\n\n// 1. Rich Inline Tags in dikha\ndikha \"[hara bold][OK] Success:[/] Server chalu ho gaya on [asmani line]http://localhost:8080[/]\"\ndikha \"[laal]Error 404:[/] [peela]Resource gayab ho gaya![/]\"\n\n// 2. Chainable Color & Style Methods on Strings\nstatus = \"ONLINE\".hara().mota()\ndikha f\"Database Status: {status}\"\ndikha \"Raw plain text without ANSI: \" + status.rang_hata()\n\n// 3. Unicode Box Panels (dikha.dabba / box)\ndikha.dabba(\"MEMElang v2.0\\nRich UI, Stdlib & WebAssembly Ready!\", \"[Fast] Release Announcement\")\n\n// 4. Unicode Bordered Tables (dikha.mez / table)\ntasks = [\n    { task: \"Lexer & Parser\", status: \"Done\".hara(), priority: \"High\".laal() },\n    { task: \"Rich dikha & Widgets\", status: \"Done\".hara(), priority: \"High\".laal() },\n    { task: \"Standard Libraries\", status: \"Done\".hara(), priority: \"Medium\".peela() },\n    { task: \"Deploy to Production\", status: \"Ready\".asmani(), priority: \"Urgent\".laal().mota() }\n]\ndikha.mez(tasks)\n\n// 5. Expanded Array & String Methods\narr = [10, 20, 30]\narr.daal(40)\ndikha \"Pehla item:\", arr.pehla(), \"| Aakhri:\", arr.aakhri()\ndikha \"Chunked by 2:\", arr.tukde(2)\n\ncode = \"42\"\ndikha \"Padded ID:\", code.aage_bhar(6, \"0\")\ndikha \"Is digit:\", code.number_hai()\n\n// 6. Systumm & CLI APIs\ndikha \"Running on platform:\", systumm.kaun()\nargs = cli.args_samajh([\"--mode=turbo\", \"--workers=4\", \"serve\"])\ndikha \"CLI args parsed:\", args"}, {"id": "whatsapp-hinglish", "name": "WhatsApp Hinglish (Effortless & Clean)", "description": "Zero semicolons, no pakad/le boilerplate, kaam, de, agar/elif, har...mein, .chhan, .badal, .saja, .kul, .dhund", "code": "// WhatsApp Hinglish MEMElang - Clean, Pythonic, Zero Keystroke Waste!\n\n// 1. Python-style variable creation (No pakad / le needed) & optional semicolons\nnumbers = [5, 12, 8, 20, 3, 42, 17]\n\n// 2. Short, natural keywords: 'kaam' (function) and 'de' (return)\nkaam process_scores(arr) {\n    // 3. Underscore-free collection methods: .chhan (filter), .badal (map), .saja (sort)\n    big_doubled = arr\n        .chhan(x => x >= 10)\n        .badal(x => x * 2)\n        .saja()\n\n    de big_doubled\n}\n\ndikha \"Processed Scores:\", process_scores(numbers)\n\n// 4. Clean conditionals with agar, elif, warna\nkaam rank(score) {\n    agar score >= 80 {\n        de \"Aura +10000 (Topper)\"\n    } elif score >= 33 {\n        de \"Pass ho gaye mauj karo\"\n    } warna {\n        de \"Raju, padhai likhai mein dhyan lagao\"\n    }\n}\n\ndikha rank(85)\ndikha rank(12)\n\n// 5. Har item mein list iteration\ntotal = 0\nhar n mein [10, 20, 30] {\n    total = total + n\n}\ndikha \"Total sum:\", total\n\n// 6. Underscore-free .kul (reduce), .dhund (find), .koi (some), .sab (every)\ndikha \"Sum via .kul():\", [1, 2, 3, 4, 5].kul((acc, x) => acc + x, 0)\ndikha \"First > 20:\", [5, 12, 25, 30].dhund(x => x > 20)\ndikha \"Any even? (.koi):\", [1, 3, 4, 7].koi(x => x % 2 == 0)\ndikha \"All positive? (.sab):\", [2, 4, 6].sab(x => x > 0)"}, {"id": "pythonic", "name": "[Py] Pythonic Transforms & Tantrik Methods", "description": "Arrow lambdas, collection pipelines (.chhaan, .sab_pe, .nichod), Tantrik dunders (__jod__, __dikha__), F-strings, and destructuring", "code": "// [Py] Pythonic Superpowers in MEMElang\n\n// 1. Arrow Lambdas & Collection Pipeline\npakad scores = [15, 42, 88, 23, 95, 60];\npakad toppers = scores\n    .chhaan(s => s >= 50)\n    .sab_pe(s => s + 5)\n    .line_mein((a, b) => a > b); // Sort descending\n\ndikha \"Toppers bonus scores:\", toppers;\n\n// 2. Tantrik Operator Overloading & F-Strings\nbiradari Wallet {\n    jugaad shuru(rupaye) {\n        khud.bal = rupaye;\n    }\n    jugaad __jod__(other) {\n        ye_le Wallet(khud.bal + other.bal);\n    }\n    jugaad __dikha__() {\n        ye_le f\"Wallet(Rs.{khud.bal})\";\n    }\n}\n\npakad w1 = Wallet(500);\npakad w2 = Wallet(250);\npakad total = w1 + w2;\ndikha f\"Total money: {total}\";\n\n// 3. Array & Map Destructuring\npakad [first, second, *rest] = [10, 20, 30, 40, 50];\ndikha f\"first: {first}, second: {second}, rest: {rest}\";\n\npakad {naam, city} = { \"naam\": \"Gabbru\", \"city\": \"Delhi\" };\ndikha f\"Player: {naam} from {city}\";"}, {"id": "hello", "name": "Hi Hello World", "description": "Basic variables, printing, and string multiplication", "code": "// Welcome to MEMElang WebAssembly Edition!\npakad welcome = \"Namaste Sansaar!\";\ndikha welcome;\n\npakad laughter = \"ha\" * 6;\ndikha \"Desi laughter:\", laughter;\n\npakad a = 25;\npakad b = 17;\ndikha \"25 + 17 =\", a + b;"}, {"id": "input", "name": "Console Input (pooch / bata)", "description": "Interactive and pre-fed standard input with pooch()", "stdin": "Baburao Apte\nMumbai\n50", "code": "// Interactive Input with pooch() / bata()\ndikha \"=== MEMElang Desi Survey ===\";\npakad naam = pooch(\"Aapka naam kya hai? \");\npakad shahar = pooch(\"Kaunse shahar se ho? \");\npakad umar = pooch(\"Aapki umar kitni hai? \");\n\ndikha \"\";\ndikha \"Swagat hai\", naam, \"dost from\", shahar + \"!\";\ndikha \"5 saal baad aapki umar hogi:\", bina_point(umar) + 5;"}, {"id": "fibonacci", "name": "[Py] Fibonacci Recursion", "description": "Recursive algorithm with range loop generator", "code": "// Recursive Fibonacci in MEMElang\njugaad fib(n) {\n    dekh n <= 0 {\n        ye_le 0;\n    }\n    dekh n == 1 {\n        ye_le 1;\n    }\n    ye_le fib(n - 1) + fib(n - 2);\n}\n\ndikha \"--- Pehle 8 Fibonacci Numbers ---\";\nghuma i se 0 se 7 {\n    dikha \"fib(\" + kya_hai(i) + \" \" + i + \") =\", fib(i);\n}"}, {"id": "fizzbuzz", "name": "[GPU] FizzBuzz Challenge", "description": "Conditionals and modulo loops from 1 to 15", "code": "// Classic FizzBuzz with Desi Memes\nghuma n se 1 se 15 {\n    dekh (n % 15 == 0) {\n        dikha n, \": FizzBuzz (Full Masala)\";\n    } warna dekh (n % 3 == 0) {\n        dikha n, \": Fizz\";\n    } warna dekh (n % 5 == 0) {\n        dikha n, \": Buzz\";\n    } warna {\n        dikha n;\n    }\n}"}, {"id": "oop", "name": "[VIP] OOP & Nepotism", "description": "Classes, nepo inheritance, and dynamic methods", "code": "// Classes & Nepotism\nbiradari Actor {\n    jugaad shuru(naam) {\n        apun.naam = naam;\n        apun.talent = 10;\n    }\n\n    jugaad dialogue() {\n        dikha apun.naam, \"bolta hai: Mere paas talent hai!\";\n    }\n}\n\nbiradari StarKid nepo Actor {\n    jugaad shuru(naam, launch_budget) {\n        apun.naam = naam;\n        apun.budget = point_wala(launch_budget);\n    }\n\n    jugaad interview() {\n        dikha apun.naam, \"says: Humne bohot struggle kiya hai, budget only Rs.\", apun.budget;\n    }\n}\n\npakad kid = StarKid(\"Shanaya\", \"50000000.0\");\nkid.dialogue();  // Inherited from Actor\nkid.interview(); // Defined on StarKid"}, {"id": "errors", "name": "[Meme] Meme Error Showcase", "description": "Uncomment any line to trigger iconic meme error dialogues", "code": "// Uncomment ONE line at a time to test meme errors:\n\n// 1. Baburao divide by zero:\ndikha 100 / 0;\n\n// 2. Rasode mein kaun tha (undeclared variable):\n// dikha rasode_ki_rashi;\n\n// 3. Ashneer Grover Doglapan error:\n// (x) = 50;\n\n// 4. Moye Moye unclosed string:\n// pakad s = \"khula chhod diya;\n\n// 5. Gangs of Wasseypur (Beta tumse na ho payega):\n// pakad num = 42; num();"}, {"id": "borrow-checker", "name": "Rust-Style Borrow Checker", "description": "Single ownership, moves, udhaar (&), udhaar_badal (&mut), and RAII alvida()", "code": "// MEMElang Rust-Style Memory Safety Showcase\n\n// 1. Explicit Cloning with nakal()\npakad secret = \"Tees Maar Khan Khazana\";\npakad safe_copy = nakal(secret);\ndikha \"Safe copy cloned:\", safe_copy;\n\n// 2. Shared Borrowing (udhaar)\npakad hero = \"Majnu Seth\";\npakad r1 = udhaar hero;\npakad r2 = udhaar hero;\ndikha \"Shared read 1:\", r1;\ndikha \"Shared read 2:\", r2;\n\n// 3. Mutable Borrowing (udhaar_badal) in Scopes\npakad score = 10;\n{\n    pakad m = udhaar_badal score;\n    m = 100;\n    dikha \"Mutated through exclusive borrow:\", m;\n}\ndikha \"Score after scope borrow release:\", score;\n\n// 4. RAII Destructors (alvida)\nbiradari SafeLocker {\n    jugaad shuru(name) {\n        apun.name = name;\n        dikha \"[Open] [RAII] Locker opened:\", apun.name;\n    }\n    jugaad alvida() {\n        dikha \"[Lock] [RAII] Locker auto-dropped and sealed:\", apun.name;\n    }\n}\n\n{\n    pakad locker = SafeLocker(\"Swiss Vault\");\n    dikha \"Vault is in active use...\";\n}\ndikha \"Scope finished, locker was cleanly dropped!\";"}, {"id": "collections-demo", "name": "Native Collections & Maps", "description": "Dynamic arrays, hash maps with dot notation, and ghuma ... mein iteration", "code": "// Native Collections Showcase\npakad team = [\"Kabir\", \"Bikram\", \"Guddu\"];\nteam.ghusa(\"Bablu\");\ndikha \"Initial team:\", team;\ndikha \"Captain (negative index):\", team[-1];\n\n// Hash map with dot notation\npakad stats = {\n    \"runs\": 85,\n    \"wickets\": 3,\n    \"mvp\": han\n};\ndikha \"Player runs:\", stats.runs;\ndikha \"All stat keys:\", stats.chaabi();\n\n// Iteration with ghuma ... mein\ndikha \"\\n--- Roster ---\";\nghuma player mein team {\n    dikha \"Player:\", player;\n}"}, {"id": "error-handling-demo", "name": "[!] Error Handling (panga / lafda)", "description": "panga/lafda/pakka try-catch-finally and chillao exception raising", "code": "// [!] User-Level Error Handling Demo\njugaad withdraw(balance, amount) {\n    dekh amount > balance {\n        chillao {\n            \"code\": 402,\n            \"msg\": \"Aukaat ke bahar! Bank mein itna paisa nahi hai!\"\n        };\n    }\n    ye_le balance - amount;\n}\n\npanga {\n    pakad bal = 500;\n    dikha \"Balance after 200 withdrawal:\", withdraw(bal, 200);\n    dikha \"Attempting 1000 withdrawal...\";\n    dikha withdraw(bal, 1000); // Throws!\n} lafda err {\n    dikha \"[ERR] Lafda pakda gaya!\";\n    dikha \"Error Code:\", err.code;\n    dikha \"Error Msg:\", err.msg;\n} pakka {\n    dikha \"[Vault] Transaction session closed cleanly (pakka block always runs)!\";\n}"}, {"id": "enums-pattern-matching", "name": "[Target] Enums & Pattern Matching", "description": "Type-safe avtar enums and jaanch pattern matching with baaki/warna guards", "code": "// [Target] Enums (avtar) & Pattern Matching (jaanch)\navtar OrderState {\n    Cart,\n    Placed,\n    Shipped,\n    Delivered\n}\n\njugaad trackOrder(state) {\n    jaanch state {\n        OrderState.Cart => {\n            dikha \"Cart mein samaan pada hai, checkout karo!\";\n        }\n        OrderState.Placed => {\n            dikha \"Order place ho gaya, packing shuru!\";\n        }\n        OrderState.Shipped => {\n            dikha \"Delivery boy raste mein hai!\";\n        }\n        OrderState.Delivered => {\n            dikha \"[!] Samaan mil gaya! Rating de do bro!\";\n        }\n        baaki => {\n            dikha \"Aayein? Unknown status!\";\n        }\n    }\n}\n\ntrackOrder(OrderState.Shipped);\ntrackOrder(OrderState.Delivered);"}, {"id": "stdlib-demo", "name": "Built-in Stdlib (hisab, waqt, json)", "description": "Math, high-precision timing, and JSON serialization/deserialization", "code": "// Built-in Standard Library Showcase\n\n// 1. Math (hisab)\ndikha \"Math PI:\", hisab.pi;\ndikha \"Square root of 81:\", hisab.jadd(81);\ndikha \"Round 7.8:\", hisab.gol(7.8);\ndikha \"Random lucky number (1-100):\", hisab.tukka_num(1, 100);\n\n// 2. JSON Serialization\npakad player = {\n    \"name\": \"Virat\",\n    \"century\": 80,\n    \"active\": han\n};\npakad jsonStr = json.likh(player);\ndikha \"Serialized JSON:\", jsonStr;\n\npakad decoded = json.samajh(jsonStr);\ndikha \"Decoded player name:\", decoded.name;\n\n// 3. Time (waqt)\npakad start = waqt.ab();\nso_ja(0.05); // Sleep 50ms\ndikha \"Elapsed seconds:\", waqt.ab() - start;"}, {"id": "platform-separation-demo", "name": "Platform Separation & platform.select", "description": "Inspect target OS, architecture, boolean platform predicates, and React Native-style platform.select()", "code": "// Platform Detection & Target Separation in MEMElang\nbula \"platform\";\nbula \"platform\" jaise { os, arch, is, isWeb, isMobile, select };\n\ndikha \"Platform OS:\", os();\ndikha \"Arch:\", arch();\ndikha \"Is Web Target:\", is(\"web\"), \"isWeb():\", isWeb(), \"isMobile():\", isMobile();\n\npakad backend = select({\n    \"ios\": \"UIKit + Metal Zero-WebView\",\n    \"android\": \"Android NativeActivity + GLES2\",\n    \"mac\": \"Cocoa AppKit + Metal\",\n    \"web\": \"WebAssembly Zero-DOM Canvas\",\n    \"default\": \"Universal Engine\"\n});\ndikha \"Select Result:\", backend;"}, {"id": "native-permissions-demo", "name": "Cross-Platform Runtime Permissions", "description": "Check capability availability, normalize aliases, and inspect permission states safely", "code": "// Unified Cross-Platform Permissions API\nbula \"permissions\";\nbula \"permissions\" jaise { check, isSupported };\n\npakad cam = check(\"cam\");\npakad loc = check(\"location\");\npakad bioSupported = isSupported(\"biometrics\");\n\ndikha \"Check Camera:\", cam.permission, cam.state, \"canAskAgain:\", cam.canAskAgain;\ndikha \"Check Location:\", loc.permission, loc.state;\ndikha \"Biometrics Supported Here:\", bioSupported;\ndikha \"Permissions Checked: OK\";"}];
  }
  const __ns = { getPlaygroundPresets };
  $mod_4.__cache = __ns;
  return __ns;
}

// Module: errors.meme
function $mod_5() {
  if ($mod_5.__cache) return $mod_5.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  function getErrorsCatalog() {
  return [{"title": "Baburao Divide By Zero", "scenario": "Division by 0 (e.g. 10 / 0)", "quote": "Kya coder banega re tu? 0 se divide karke brahmand khatam karega kya!", "origin": "Hera Pheri (Baburao)", "category": "Arithmetic", "code": "pakad a = 10;\npakad b = 0;\ndikha a / b;"}, {"title": "Baburao Modulo By Zero", "scenario": "Modulo by 0 (e.g. 15 % 0)", "quote": "0 se modulo (%) karne chale ho? Maths teacher ko heart attack aa jayega!", "origin": "School Trauma / Hera Pheri", "category": "Arithmetic", "code": "pakad x = 15;\ndikha x % 0;"}, {"title": "Chhoti Bachi Ho Kya (Missing Semicolon)", "scenario": "Statement without trailing semicolon ';'", "quote": "Chhoti bachi ho kya?! Line ke end mein ';' lagana bhool gaye!", "origin": "Tiger Shroff (Heropanti)", "category": "Syntax", "code": "pakad a = 42\ndikha a;"}, {"title": "Ashneer Grover Doglapan", "scenario": "Invalid left-hand side in assignment (e.g. (x) = 5)", "quote": "Ye sab doglapan hai! '=' ke left side mein sirf seedha variable ya property aata hai, koi equation ya bracket nahi!", "origin": "Ashneer Grover (Shark Tank India)", "category": "Syntax", "code": "pakad x = 10;\n(x) = 20;"}, {"title": "Control Uday Control", "scenario": "Using single '=' inside an if condition", "quote": "Control Uday, control! 'dekh' mein '=' (assign) nahi, '==' (compare) lagta hai!", "origin": "Majnu Seth / Uday Shetty (Welcome)", "category": "Syntax", "code": "pakad x = 5;\ndekh x = 10 {\n    dikha \"Condition met\";\n}"}, {"title": "Rasode Mein Kaun Tha (Undeclared Read)", "scenario": "Referencing an undeclared identifier", "quote": "Rasode mein kaun tha?! Ye '<name>' bina bataye kahan se aaya? Pehle 'pakad <name>;' se declare kar!", "origin": "Kokilaben (Saath Nibhaana Saathiya)", "category": "Runtime", "code": "dikha rasode_ki_rashi;"}, {"title": "Chuna Laga Diya (Undeclared Assignment)", "scenario": "Assigning to a variable that wasn't declared with pakad", "quote": "Chuna laga diya! '<name>' naam ka koi variable hi nahi hai. Pehle 'pakad <name>;' likh!", "origin": "Panchayat / Indian Memes", "category": "Runtime", "code": "chuna = 500;"}, {"title": "String Khol Ke Chhod Diya", "scenario": "Unterminated string literal", "quote": "[!] String khol ke chhod diya... band tera baap karega?", "origin": "Classic Desi Parent Scolding", "category": "Lexer", "code": "pakad s = \"khula reh gaya;\ndikha s;"}, {"title": "Moye Moye (EOF Mid-Expression)", "scenario": "Code cuts off abruptly mid-expression", "quote": "Moye Moye! Expression adhoori chhod ke code beech mein hi khatam kar diya.", "origin": "Viral Moye Moye Meme", "category": "Parser", "code": "pakad x = 10 +"}, {"title": "Chi Sasura (Alien Symbol)", "scenario": "Typing unmapped symbols like @ or $", "quote": "[ERR] Chi sasura! Ye kaisa alien symbol ghusa diye ho code mein: @", "origin": "Gangs of Wasseypur", "category": "Lexer", "code": "pakad x = @123;"}, {"title": "Paisa Double Overflow", "scenario": "64-bit integer addition overflow", "quote": "25 din mein paisa double karte-karte integer overflow ho gaya! Aukaat ke bahar chala gaya jod.", "origin": "Anuradha / Phir Hera Pheri", "category": "Arithmetic", "code": "// Max 64-bit int overflow\npakad big = 9223372036854775807;\ndikha big + 1;"}, {"title": "Delulu is Not the Solulu", "scenario": "Multiplying a string by a negative number", "quote": "Delulu is not the solulu! String ko negative baar kaise multiply karoge?", "origin": "Gen-Z Pop Culture", "category": "Arithmetic", "code": "pakad str = \"swag\" * -3;"}, {"title": "Seb Ko Santre Se Mat Tolo", "scenario": "Comparing incompatible types (e.g. 10 < 'hello')", "quote": "Seb ko santre se mat tolo! 'int' aur 'string' ko compare nahi kar sakte!", "origin": "Desi Idiom", "category": "Type Error", "code": "dekh 10 < \"hello\" {\n    dikha \"Kamaal hai\";\n}"}, {"title": "Beta Tumse Na Ho Payega", "scenario": "Calling a non-function value (e.g. 42())", "quote": "Beta, tumse na ho payega! '42' koi jugaad (function) thodi hai jo call kar rahe ho!", "origin": "Ramadhir Singh (Gangs of Wasseypur)", "category": "Runtime", "code": "pakad x = 42;\nx();"}, {"title": "Khopdi Tod Saale Ka (Missing OOP Property)", "scenario": "Accessing a non-existent method or property on an object", "quote": "Khopdi tod saale ka! 'Influencer' biradari ke paas 'humble' naam ka kuch nahi hai!", "origin": "Baburao (Hera Pheri)", "category": "OOP", "code": "biradari Influencer {\n    jugaad shuru(naam) {\n        apun.naam = naam;\n    }\n}\npakad i = Influencer(\"Orry\");\ndikha i.humble;"}, {"title": "Fake Nepotism", "scenario": "Inheriting from a primitive value instead of a class", "quote": "Fake nepotism! 'fake_parent' koi biradari (class) nahi hai, isko inherit kaise karoge?", "origin": "Bollywood Industry Debate", "category": "OOP", "code": "pakad fake_parent = 42;\nbiradari Starkid nepo fake_parent {\n}"}, {"title": "Gaddari Korbe (Arity Mismatch)", "scenario": "Calling a function with wrong number of arguments", "quote": "Gaddari korbe! Jugaad ne 2 arguments maange the, aur tumne 1 chipka diye!", "origin": "Viral Bengali Meme", "category": "Runtime", "code": "jugaad jod(a, b) {\n    ye_le a + b;\n}\njod(10);"}, {"title": "Pehli Fursat Mein Nikal (Use After Move)", "scenario": "Reading a heap variable after transferring its ownership", "quote": "Pehli fursat mein nikal! 'a' ki value move ho chuki hai! Ownership transfer ke baad use karna mana hai!", "origin": "Hindustani Bhau", "category": "Ownership & Borrowing", "code": "pakad a = \"Baburao ka khazana\";\npakad b = a;\ndikha a;"}, {"title": "Doglapan (Mutating While Borrowed)", "scenario": "Attempting to modify a variable while an immutable borrow (udhaar) is active", "quote": "Doglapan! 'x' abhi udhaar (borrowed) pe hai, isko modify nahi kar sakte!", "origin": "Ashneer Grover (Shark Tank India)", "category": "Ownership & Borrowing", "code": "pakad x = \"shuru\";\npakad r = udhaar x;\nx = \"badal diya\";"}, {"title": "Beech Mein Mat Bolo (Exclusive Borrow Access)", "scenario": "Accessing owner directly while exclusively borrowed (udhaar_badal)", "quote": "Beech mein mat bolo! 'x' abhi kisi aur ko udhaar diya hua hai!", "origin": "TV News Debate Meme", "category": "Ownership & Borrowing", "code": "pakad x = \"Pavitra data\";\npakad m = udhaar_badal x;\ndikha x;"}, {"title": "Ek Mawaali Ek Time Pe (Borrow Conflict)", "scenario": "Attempting a mutable borrow while a shared borrow is active", "quote": "Ek mawaali ek time pe ek hi jagah! 'x' already udhaar (borrowed) pe hai, exclusive borrow nahi mil sakta!", "origin": "Welcome Movie", "category": "Ownership & Borrowing", "code": "pakad x = \"Khandaani haar\";\npakad r = udhaar x;\npakad m = udhaar_badal x;"}, {"title": "Baburao Ka Chashma (Dangling Reference Escape)", "scenario": "Returning or escaping a reference to a temporary local variable", "quote": "Baburao ka chashma! Local variable 'local_var' ka udhaar wapas nahi bhej sakte, scope khatam hote hi mar jayega!", "origin": "Hera Pheri (Baburao)", "category": "Ownership & Borrowing", "code": "jugaad leaky() {\n    pakad local_var = \"Temporary secret\";\n    ye_le udhaar local_var;\n}\npakad r = leaky();"}];
  }
  const __ns = { getErrorsCatalog };
  $mod_5.__cache = __ns;
  return __ns;
}

// Module: SyntaxHelper.meme
function $mod_6() {
  if ($mod_6.__cache) return $mod_6.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  let useState; useState = $m.ui["useState"]; 
  let HI_TO_EN_KW = {"pakad": "let", "le": "let", "jugaad": "fn", "kaam": "fn", "ye_le": "return", "wapas": "return", "de": "return", "dekh": "if", "agar": "if", "warna": "else", "nahi_toh": "else", "jabtak": "while", "ghoom": "for", "ghuma": "for", "mein": "in", "andar": "in", "bas_kar": "break", "bas": "break", "aage_badh": "continue", "chal": "continue", "biradari": "class", "sancha": "class", "roop": "class", "vibe": "class", "nepo": "extends", "shuru": "init", "apun": "this", "khud": "this", "dikha": "print", "flex": "print", "pooch": "input", "bata": "input", "aur": "and", "ya": "or", "han": "true", "haan": "true", "sach": "true", "nhi": "false", "nahi": "false", "jhooth": "false", "nalla": "null", "kuch_nhi": "null", "kuch_nahi": "null", "khali": "null", "udhaar": "borrow", "udhaar_badal": "borrow_mut", "udhaar_badlo": "borrow_mut", "nakal": "clone", "panga": "try", "koshish": "try", "lafda": "catch", "sambhal": "catch", "pakka": "finally", "hamesha": "finally", "chillao": "throw", "fek": "throw", "side_mein": "async", "asar": "async", "sabar": "await", "bula": "import", "chura": "import", "lao": "import", "shamil": "import", "jaise": "as", "potli": "package", "dibbi": "package", "bech": "export", "khula": "export", "baahar": "export", "mohalla": "namespace", "pados": "namespace", "avtar": "enum", "kism": "enum", "jaanch": "match", "parakh": "match", "baaki": "default", "bahuroop": "overload", "aur_ek": "overload", "ek_aur": "overload", "tantrik": "operator", "kriya": "operator", "patta": "addr", "thikana": "addr", "asli": "deref", "dekh_patta": "deref", "uthha": "deref", "rawana": "move", "khisko": "move", "chal_nikal": "move", "malik": "owned", "kabza": "owned", "malkiyat": "owned", "badal": "mut", "badlo": "mut", "atal": "const", "sthir": "const", "khatam": "drop", "alvida": "drop", "visarjan": "drop", "daayra": "scope", "seema": "scope", "elaqa": "scope", "Vaada": "Promise", "vaada": "Promise", "tijori": "db", "dhaba": "server", "systumm": "sys", "akshar_bana": "chr", "code_de": "ord", "lambai": "size", "kitna_lamba": "size", "bina_point": "int", "pura_num": "int", "bada_ank": "int", "point_wala": "float", "float_bana": "float", "state_bana": "useState", "phir": "then"};
  let EN_TO_HI_KW = {"let": "pakad", "var": "pakad", "fn": "jugaad", "def": "jugaad", "return": "ye_le", "if": "dekh", "else": "warna", "while": "jabtak", "for": "ghoom", "in": "mein", "break": "bas", "continue": "chal", "class": "biradari", "extends": "nepo", "inherits": "nepo", "init": "shuru", "constructor": "shuru", "this": "apun", "self": "apun", "print": "dikha", "input": "pooch", "and": "aur", "or": "ya", "true": "han", "false": "nhi", "null": "nalla", "none": "nalla", "nil": "nalla", "borrow": "udhaar", "borrow_mut": "udhaar_badal", "clone": "nakal", "try": "panga", "catch": "lafda", "finally": "pakka", "throw": "chillao", "raise": "chillao", "await": "sabar", "import": "bula", "as": "jaise", "package": "potli", "export": "bech", "namespace": "mohalla", "enum": "avtar", "match": "jaanch", "default": "baaki", "overload": "bahuroop", "operator": "tantrik", "addr": "patta", "deref": "asli", "move": "rawana", "owned": "malik", "mut": "badal", "const": "atal", "drop": "khatam", "scope": "daayra", "Promise": "Vaada", "db": "tijori", "vault": "tijori", "server": "dhaba", "sys": "systumm", "chr": "akshar_bana", "ord": "code_de"};
  let HI_TO_EN_METHODS = {"rakh": "set", "nikal": "get", "la": "get", "hai_kya": "has", "hatao": "delete", "hata": "delete", "chaabi": "keys", "maal": "values", "saaf": "clear", "banao": "create", "chala": "run", "chalao": "run", "jod": "join", "lambai": "length", "kitna_lamba": "length", "dheere_bol": "lower", "chilla": "upper", "shuru_hota": "startsWith", "khatam_hota": "endsWith", "phir": "then", "ruk": "sleep", "sab": "all", "radh": "reject", "de": "resolve", "khatam": "is_done", "nakal": "clone", "bhejo": "send", "bhej": "send", "thapa_maro": "status", "chapp": "json", "suno": "listen"};
  let TYPE_HI_TO_EN = {"ank": "int", "sankhya": "int", "point": "float", "dashamlav": "float", "bol": "str", "shabd": "str", "sach": "bool"};
  function isWordChar(ch) {
  let c = $m.ord(ch);
  return (((((c >= 97) && (c <= 122)) || ((c >= 65) && (c <= 90))) || ((c >= 48) && (c <= 57))) || $m.eq(c, 95));
  }
  function isDigitChar(ch) {
  let c = $m.ord(ch);
  return ((c >= 48) && (c <= 57));
  }
  function convertCodeSyntax(codeText, mode) {
  if (($m.eq(codeText, null) || $m.eq(codeText, ""))) {
    return "";
  }
  if (((!$m.eq(mode, "en")) && (!$m.eq(mode, "hi")))) {
    return codeText;
  }
  let toEn = $m.eq(mode, "en");
  let out = "";
  let i = 0;
  let n = $m.size(codeText);
  let prevTok = "";
  let prevNonWs = "";
  let inForHeader = false;
  let inClassHeader = false;
  let forSeCount = 0;
  while ((i < n)) {
    let ch = $m.call(codeText, "slice", [i, (i + 1)]);
    if (($m.eq(ch, "\"") || $m.eq(ch, "'"))) {
      let q = ch;
      (out = (out + ch));
      (i = (i + 1));
      while ((i < n)) {
        let sc = $m.call(codeText, "slice", [i, (i + 1)]);
        (out = (out + sc));
        (i = (i + 1));
        if (($m.eq(sc, "\\") && (i < n))) {
          (out = (out + $m.call(codeText, "slice", [i, (i + 1)])));
          (i = (i + 1));
        } else if ($m.eq(sc, q)) {
          break;
        }
      }
      (prevNonWs = q);
      (prevTok = q);
      continue;
    }
    if ((($m.eq(ch, "/") && ((i + 1) < n)) && $m.eq($m.call(codeText, "slice", [(i + 1), (i + 2)]), "/"))) {
      while (((i < n) && (!$m.eq($m.call(codeText, "slice", [i, (i + 1)]), "\n")))) {
        (out = (out + $m.call(codeText, "slice", [i, (i + 1)])));
        (i = (i + 1));
      }
      continue;
    }
    if ((($m.eq(ch, "/") && ((i + 1) < n)) && $m.eq($m.call(codeText, "slice", [(i + 1), (i + 2)]), "*"))) {
      (out = (out + "/*"));
      (i = (i + 2));
      while ((i < n)) {
        if ((($m.eq($m.call(codeText, "slice", [i, (i + 1)]), "*") && ((i + 1) < n)) && $m.eq($m.call(codeText, "slice", [(i + 1), (i + 2)]), "/"))) {
          (out = (out + "*/"));
          (i = (i + 2));
          break;
        }
        (out = (out + $m.call(codeText, "slice", [i, (i + 1)])));
        (i = (i + 1));
      }
      continue;
    }
    if (isDigitChar(ch)) {
      while (((i < n) && isWordChar($m.call(codeText, "slice", [i, (i + 1)])))) {
        (out = (out + $m.call(codeText, "slice", [i, (i + 1)])));
        (i = (i + 1));
      }
      (prevNonWs = "0");
      (prevTok = "0");
      continue;
    }
    if ((($m.eq(ch, "-") && ((i + 1) < n)) && $m.eq($m.call(codeText, "slice", [(i + 1), (i + 2)]), ">"))) {
      (out = (out + "->"));
      (i = (i + 2));
      (prevNonWs = "->");
      (prevTok = "->");
      continue;
    }
    if (isWordChar(ch)) {
      let startIdx = i;
      while (((i < n) && isWordChar($m.call(codeText, "slice", [i, (i + 1)])))) {
        (i = (i + 1));
      }
      let tok = $m.call(codeText, "slice", [startIdx, i]);
      let mapped = tok;
      if ($m.eq(prevNonWs, ".")) {
        if ((toEn && $m.call(HI_TO_EN_METHODS, "has", [tok]))) {
          (mapped = $m.idx(HI_TO_EN_METHODS, tok));
        }
      } else if ((toEn && ($m.eq(prevNonWs, ":") || $m.eq(prevNonWs, "->")))) {
        if ($m.call(TYPE_HI_TO_EN, "has", [tok])) {
          (mapped = $m.idx(TYPE_HI_TO_EN, tok));
        } else if ($m.call(HI_TO_EN_KW, "has", [tok])) {
          (mapped = $m.idx(HI_TO_EN_KW, tok));
        }
      } else if (((inForHeader && toEn) && ($m.eq(tok, "se") || $m.eq(tok, "from")))) {
        (forSeCount = (forSeCount + 1));
        if ($m.eq(forSeCount, 1)) {
          (mapped = "from");
        } else if ($m.eq(forSeCount, 2)) {
          (mapped = "to");
        } else {
          (mapped = "step");
        }
      } else if (((inForHeader && (!toEn)) && ((($m.eq(tok, "from") || $m.eq(tok, "to")) || $m.eq(tok, "step")) || $m.eq(tok, "by")))) {
        (mapped = "se");
      } else if (($m.eq(tok, "se") && toEn)) {
        (mapped = (inClassHeader ? "extends" : "from"));
      } else if (($m.eq(tok, "bol") && toEn)) {
        if (((($m.eq(prevNonWs, "") || $m.eq(prevNonWs, ";")) || $m.eq(prevNonWs, "{")) || $m.eq(prevNonWs, "}"))) {
          (mapped = "print");
        }
      } else if ((toEn && $m.call(HI_TO_EN_KW, "has", [tok]))) {
        (mapped = $m.idx(HI_TO_EN_KW, tok));
      } else if (((!toEn) && $m.call(EN_TO_HI_KW, "has", [tok]))) {
        (mapped = $m.idx(EN_TO_HI_KW, tok));
      }
      if ((($m.eq(tok, "ghoom") || $m.eq(tok, "ghuma")) || $m.eq(tok, "for"))) {
        (inForHeader = true);
        (forSeCount = 0);
      }
      if ((((($m.eq(tok, "biradari") || $m.eq(tok, "sancha")) || $m.eq(tok, "roop")) || $m.eq(tok, "vibe")) || $m.eq(tok, "class"))) {
        (inClassHeader = true);
      }
      (out = (out + mapped));
      (prevTok = tok);
      (prevNonWs = tok);
      continue;
    }
    if (($m.eq(ch, "{") || $m.eq(ch, ";"))) {
      (inForHeader = false);
      (inClassHeader = false);
      (forSeCount = 0);
    }
    (out = (out + ch));
    if (((((!$m.eq(ch, " ")) && (!$m.eq(ch, "\t"))) && (!$m.eq(ch, "\n"))) && (!$m.eq(ch, "\r")))) {
      (prevNonWs = ch);
      (prevTok = ch);
    }
    (i = (i + 1));
  }
  return out;
  }
  function formatSnippetForMode(baseHindiCode, mode) {
  if ($m.eq(mode, "en")) {
    return convertCodeSyntax(baseHindiCode, "en");
  }
  return baseHindiCode;
  }
  function stripAnsiCodes(rawText) {
  if (($m.eq(rawText, null) || $m.eq(rawText, ""))) {
    return "";
  }
  let esc = $m.chr(27);
  let out = "";
  let idx = 0;
  let n = $m.size(rawText);
  while ((idx < n)) {
    let ch = $m.call(rawText, "slice", [idx, (idx + 1)]);
    if ((($m.eq(ch, esc) && ((idx + 1) < n)) && $m.eq($m.call(rawText, "slice", [(idx + 1), (idx + 2)]), "["))) {
      (idx = (idx + 2));
      while ((idx < n)) {
        let mc = $m.call(rawText, "slice", [idx, (idx + 1)]);
        (idx = (idx + 1));
        if (((($m.eq(mc, "m") || $m.eq(mc, "K")) || $m.eq(mc, "J")) || $m.eq(mc, "H"))) {
          break;
        }
      }
    } else {
      (out = (out + ch));
      (idx = (idx + 1));
    }
  }
  return out;
  }
  function classifyCodeLineColor(lineText) {
  let trimmed = $m.call(lineText, "trim", []);
  if ($m.eq(trimmed, "")) {
    return "text-zinc-400";
  }
  if (((($m.call(trimmed, "startsWith", ["//"]) || $m.call(trimmed, "startsWith", ["#"])) || $m.call(trimmed, "startsWith", ["/*"])) || $m.call(trimmed, "startsWith", ["*"]))) {
    return "text-zinc-500";
  }
  if ((((($m.call(trimmed, "startsWith", ["bula "]) || $m.call(trimmed, "startsWith", ["import "])) || $m.call(trimmed, "startsWith", ["chura "])) || $m.call(trimmed, "startsWith", ["bech "])) || $m.call(trimmed, "startsWith", ["export "]))) {
    return "text-cyan-400";
  }
  if ((((((((($m.call(trimmed, "startsWith", ["jugaad "]) || $m.call(trimmed, "startsWith", ["kaam "])) || $m.call(trimmed, "startsWith", ["fn "])) || $m.call(trimmed, "startsWith", ["def "])) || $m.call(trimmed, "startsWith", ["biradari "])) || $m.call(trimmed, "startsWith", ["sancha "])) || $m.call(trimmed, "startsWith", ["class "])) || $m.call(trimmed, "startsWith", ["avtar "])) || $m.call(trimmed, "startsWith", ["enum "]))) {
    return "text-amber-300";
  }
  if ((((((((((((((((((($m.call(trimmed, "startsWith", ["dekh "]) || $m.call(trimmed, "startsWith", ["agar "])) || $m.call(trimmed, "startsWith", ["if "])) || $m.call(trimmed, "startsWith", ["elif "])) || $m.call(trimmed, "startsWith", ["warna"])) || $m.call(trimmed, "startsWith", ["else"])) || $m.call(trimmed, "startsWith", ["jabtak "])) || $m.call(trimmed, "startsWith", ["while "])) || $m.call(trimmed, "startsWith", ["ghoom "])) || $m.call(trimmed, "startsWith", ["ghuma "])) || $m.call(trimmed, "startsWith", ["for "])) || $m.call(trimmed, "startsWith", ["jaanch "])) || $m.call(trimmed, "startsWith", ["match "])) || $m.call(trimmed, "startsWith", ["panga"])) || $m.call(trimmed, "startsWith", ["try"])) || $m.call(trimmed, "startsWith", ["lafda"])) || $m.call(trimmed, "startsWith", ["catch"])) || $m.call(trimmed, "startsWith", ["pakka"])) || $m.call(trimmed, "startsWith", ["finally"]))) {
    return "text-fuchsia-300";
  }
  if (((((($m.call(trimmed, "startsWith", ["ye_le "]) || $m.call(trimmed, "startsWith", ["wapas "])) || $m.call(trimmed, "startsWith", ["de "])) || $m.call(trimmed, "startsWith", ["return "])) || $m.call(trimmed, "startsWith", ["chillao "])) || $m.call(trimmed, "startsWith", ["throw "]))) {
    return "text-emerald-300";
  }
  if (((((((((($m.call(trimmed, "startsWith", ["dikha"]) || $m.call(trimmed, "startsWith", ["print"])) || $m.call(trimmed, "startsWith", ["pakad "])) || $m.call(trimmed, "startsWith", ["let "])) || $m.call(trimmed, "startsWith", ["var "])) || $m.call(trimmed, "startsWith", ["const "])) || $m.call(trimmed, "startsWith", ["mut "])) || $m.call(trimmed, "startsWith", ["borrow"])) || $m.call(trimmed, "startsWith", ["udhaar"])) || $m.call(trimmed, "startsWith", ["ui."]))) {
    return "text-sky-300";
  }
  return "text-zinc-200";
  }
  function renderHighlightedCode(codeText) {
  if (($m.eq(codeText, null) || $m.eq(codeText, ""))) {
    return $m.h("span", { "className": "font-mono text-xs text-zinc-500" }, "// empty snippet");
  }
  let lines = $m.call(codeText, "split", ["\n"]);
  let lineNodes = [];
  let i = 0;
  let total = $m.size(lines);
  while ((i < total)) {
    let ln = $m.idx(lines, i);
    let colCls = classifyCodeLineColor(ln);
    let dispLn = ($m.eq(ln, "") ? " " : ln);
    let lnNum = $m.str((i + 1));
    if (((i + 1) < 10)) {
      (lnNum = (" " + lnNum));
    }
    $m.call(lineNodes, "push", [$m.h("div", { "key": i, "className": "flex flex-row items-start gap-3" }, $m.h("span", { "className": "font-mono text-xs text-zinc-600 select-none shrink-0" }, lnNum), $m.h("span", { "className": ("font-mono text-xs " + colCls) }, dispLn))]);
    (i = (i + 1));
  }
  return $m.h("div", { "className": "flex flex-col gap-0.5" }, lineNodes);
  }
  const __ns = { classifyCodeLineColor, formatSnippetForMode, isWordChar, stripAnsiCodes, isDigitChar, TYPE_HI_TO_EN, HI_TO_EN_METHODS, renderHighlightedCode, convertCodeSyntax, EN_TO_HI_KW, HI_TO_EN_KW };
  $mod_6.__cache = __ns;
  return __ns;
}

// Module: CodeBlock.meme
function $mod_7() {
  if ($mod_7.__cache) return $mod_7.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  let useState; useState = $m.ui["useState"]; 
  let Badge; Badge = $m.shadcn["Badge"]; 
  let Icon; Icon = $m.icons["Icon"]; 
  native = $m.native;
  meme = $m.meme;
  const _mt0 = $mod_6();
  let shSyntax = _mt0;
  function formatMs(msVal) {
  let tenths = $m.int($m.mul(msVal, 10));
  let whole = $m.int((tenths / 10));
  let frac = (tenths - $m.mul(whole, 10));
  if ((frac < 0)) {
    (frac = 0);
  }
  return (($m.str(whole) + ".") + $m.str(frac));
  }
  function CodeBlockView(props) {
  let title = $m.get(props, "title");
  let code = $m.get(props, "code");
  let syntaxMode = ((!$m.eq($m.get(props, "syntaxMode"), null)) ? $m.get(props, "syntaxMode") : "hi");
  let onOpenPlayground = $m.get(props, "onOpenPlayground");
  let evalState;
  let setEvalState;
  [evalState, setEvalState] = $m.iter(useState({"code": "", "mode": "", "copied": false, "show": false, "out": "", "err": false, "ms": "0.0"}));
  let sameCtx = ($m.eq($m.get(evalState, "code"), code) && $m.eq($m.get(evalState, "mode"), syntaxMode));
  let copied = (sameCtx && $m.get(evalState, "copied"));
  let showOut = (sameCtx && $m.get(evalState, "show"));
  let evalOut = (sameCtx ? $m.get(evalState, "out") : "");
  let evalErr = (sameCtx && $m.get(evalState, "err"));
  let evalMs = (sameCtx ? $m.get(evalState, "ms") : "0.0");
  let displayCode = $m.call(shSyntax, "formatSnippetForMode", [code, syntaxMode]);
  let safeTitle = (($m.eq(title, null) || $m.eq(title, "")) ? "snippet.meme" : title);
  let modeLabel = ($m.eq(syntaxMode, "en") ? "English" : "Fun Hindi");
  function handleCopy() {
  $m.call($m.get($m.native, "clipboard"), "write", [displayCode]);
  setEvalState({"code": code, "mode": syntaxMode, "copied": true, "show": showOut, "out": evalOut, "err": evalErr, "ms": evalMs});
  }
  async function handleRun() {
  let res = (await $m.eval(displayCode));
  let outStr = $m.call(shSyntax, "stripAnsiCodes", [$m.get(res, "output")]);
  if ($m.eq(outStr, "")) {
    if ($m.get(res, "hasUi")) {
      (outStr = (("[MEME-UI rendered " + $m.str($m.size($m.get(res, "gpuCommands")))) + " GPU draw calls. Open in Playground for live Canvas preview!]"));
    } else {
      (outStr = "(Execution completed with no stdout output)");
    }
  }
  setEvalState({"code": code, "mode": syntaxMode, "copied": copied, "show": true, "out": outStr, "err": $m.get(res, "isError"), "ms": formatMs($m.get(res, "durationMs"))});
  }
  function handleCloseOut() {
  setEvalState({"code": code, "mode": syntaxMode, "copied": copied, "show": false, "out": "", "err": false, "ms": "0.0"});
  }
  return $m.h("div", { "className": "flex flex-col rounded-xl border border-zinc-800 bg-zinc-950 overflow-hidden my-2" }, $m.h("div", { "className": "flex flex-row items-center justify-between gap-2 px-4 py-2.5 bg-zinc-900 border-b border-zinc-800 flex-wrap" }, $m.h("div", { "className": "flex flex-row items-center gap-2" }, $m.h("span", { "className": "font-mono text-xs font-semibold text-amber-400" }, "</>"), $m.h("span", { "className": "font-mono text-xs text-zinc-200 font-medium" }, safeTitle), $m.h("span", { "className": ($m.eq(syntaxMode, "en") ? "px-1.5 py-0.5 rounded bg-sky-950 border border-sky-800 font-mono text-xs text-sky-300" : "px-1.5 py-0.5 rounded bg-amber-950 border border-amber-800 font-mono text-xs text-amber-300") }, modeLabel)), $m.h("div", { "className": "flex flex-row items-center gap-2" }, $m.h("button", { "className": "inline-flex flex-row items-center gap-1.5 px-2.5 py-1 rounded-md bg-zinc-800 border border-zinc-700 text-xs font-medium text-zinc-200", "onClick": (() => {
  return onOpenPlayground(displayCode);
}) }, $m.h(Icon, { "name": "terminal", "size": 12, "color": "#e4e4e7" }), $m.h("span", null, "Playground")), $m.h("button", { "className": "inline-flex flex-row items-center gap-1.5 px-2.5 py-1 rounded-md bg-zinc-800 border border-zinc-700 text-xs font-medium text-zinc-200", "onClick": handleCopy }, $m.h(Icon, { "name": (copied ? "check" : "copy"), "size": 12, "color": (copied ? "#34d399" : "#e4e4e7") }), $m.h("span", null, (copied ? "Copied!" : "Copy"))), $m.h("button", { "className": "inline-flex flex-row items-center gap-1.5 px-3 py-1 rounded-md bg-zinc-100 text-xs font-semibold text-zinc-950", "onClick": handleRun }, $m.h(Icon, { "name": "play", "size": 12, "color": "#09090b" }), $m.h("span", null, "Run")))), $m.h("div", { "className": "p-4 overflow-x-auto bg-zinc-950" }, $m.call(shSyntax, "renderHighlightedCode", [displayCode])), (showOut ? $m.h("div", { "className": "border-t border-zinc-800 bg-zinc-900 p-4 flex flex-col gap-2" }, $m.h("div", { "className": "flex flex-row items-center justify-between gap-2 pb-2 border-b border-zinc-800" }, $m.h("div", { "className": "flex flex-row items-center gap-2" }, $m.h("span", { "className": "font-mono text-xs font-semibold text-amber-400" }, "WebAssembly Output"), $m.h("span", { "className": (evalErr ? "px-2 py-0.5 rounded bg-rose-950 text-rose-300 font-mono text-xs border border-rose-800" : "px-2 py-0.5 rounded bg-emerald-950 text-emerald-300 font-mono text-xs border border-emerald-800") }, (evalErr ? "Error" : "Success"))), $m.h("div", { "className": "flex flex-row items-center gap-3" }, $m.h("span", { "className": "font-mono text-xs text-zinc-400" }, (evalMs + "ms")), $m.h("button", { "className": "px-2 py-0.5 rounded bg-zinc-800 text-xs text-zinc-300 border border-zinc-700", "onClick": handleCloseOut }, "Close"))), $m.h("div", { "className": (evalErr ? "p-3 rounded-lg bg-rose-950/30 border border-rose-900 font-mono text-xs text-rose-200" : "p-3 rounded-lg bg-zinc-950 border border-zinc-800 font-mono text-xs text-emerald-300") }, evalOut)) : null));
  }
  const __ns = { CodeBlockView, formatMs };
  $mod_7.__cache = __ns;
  return __ns;
}

// Module: DocView.meme
function $mod_8() {
  if ($mod_8.__cache) return $mod_8.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  let useState; useState = $m.ui["useState"]; 
  const _mt0 = $mod_7();
  let cbMod = _mt0;
  let CodeBlockComp = $m.get(cbMod, "CodeBlockView");
  function formatDocText(raw) {
  if ($m.eq(raw, null)) {
    return "";
  }
  return raw;
  }
  function renderDocBlock(item, idx, syntaxMode, onOpenPlayground) {
  let bType = $m.get(item, "type");
  if ($m.eq(bType, "text")) {
    return $m.h("p", { "key": idx, "className": "text-sm text-zinc-300" }, formatDocText($m.get(item, "text")));
  }
  if ($m.eq(bType, "heading")) {
    if ($m.eq($m.get(item, "level"), 3)) {
      return $m.h("h3", { "key": idx, "className": "text-base font-semibold text-amber-300 mt-3" }, $m.get(item, "text"));
    }
    return $m.h("div", { "key": idx, "className": "pt-4 mt-3 border-t border-zinc-800" }, $m.h("h2", { "className": "text-xl font-bold text-zinc-100" }, $m.get(item, "text")));
  }
  if ($m.eq(bType, "code")) {
    return $m.h("div", { "key": idx }, $m.h(CodeBlockComp, { "title": $m.get(item, "title"), "code": $m.get(item, "code"), "syntaxMode": syntaxMode, "onOpenPlayground": onOpenPlayground }));
  }
  if ($m.eq(bType, "callout")) {
    let isWarn = $m.eq($m.get(item, "variant"), "warning");
    let boxCls = (isWarn ? "p-4 rounded-xl border border-rose-900 bg-rose-950/30 flex flex-col gap-1.5 my-1" : "p-4 rounded-xl border border-zinc-800 bg-zinc-900/70 flex flex-col gap-1.5 my-1");
    let titleCls = (isWarn ? "text-sm font-semibold text-rose-200" : "text-sm font-semibold text-amber-300");
    let hasTitle = ((!$m.eq($m.get(item, "title"), null)) && (!$m.eq($m.get(item, "title"), "")));
    return $m.h("div", { "key": idx, "className": boxCls }, (hasTitle ? $m.h("span", { "className": titleCls }, $m.get(item, "title")) : null), $m.h("p", { "className": "text-xs text-zinc-300" }, formatDocText($m.get(item, "text"))));
  }
  if ($m.eq(bType, "table")) {
    let hdrs = $m.get(item, "headers");
    let rows = $m.get(item, "rows");
    let hdrCells = [];
    let hi = 0;
    while ((hi < $m.size(hdrs))) {
      $m.call(hdrCells, "push", [$m.h("div", { "key": hi, "className": "flex-1 font-mono text-xs font-semibold text-zinc-400" }, $m.idx(hdrs, hi))]);
      (hi = (hi + 1));
    }
    let rowNodes = [];
    let ri = 0;
    while ((ri < $m.size(rows))) {
      let rowData = $m.idx(rows, ri);
      let rowCells = [];
      let ci = 0;
      while ((ci < $m.size(rowData))) {
        let cellText = $m.str($m.idx(rowData, ci));
        $m.call(rowCells, "push", [$m.h("div", { "key": ci, "className": "flex-1" }, ($m.eq(ci, 0) ? $m.h("span", { "className": "px-2 py-0.5 rounded bg-zinc-800 border border-zinc-700 font-mono text-xs text-amber-300" }, cellText) : $m.h("span", { "className": "text-xs text-zinc-300" }, cellText)))]);
        (ci = (ci + 1));
      }
      $m.call(rowNodes, "push", [$m.h("div", { "key": ri, "className": "w-full flex flex-row items-start gap-4 px-4 py-3 border-t border-zinc-800" }, rowCells)]);
      (ri = (ri + 1));
    }
    return $m.h("div", { "key": idx, "className": "rounded-xl border border-zinc-800 bg-zinc-900/40 overflow-hidden my-1" }, $m.h("div", { "className": "w-full flex flex-row items-center gap-4 px-4 py-3 bg-zinc-900" }, hdrCells), rowNodes);
  }
  if ($m.eq(bType, "list")) {
    let listItems = $m.get(item, "items");
    let liNodes = [];
    let li = 0;
    while ((li < $m.size(listItems))) {
      $m.call(liNodes, "push", [$m.h("div", { "key": li, "className": "flex flex-row items-start gap-2.5" }, $m.h("span", { "className": "text-amber-400 font-bold text-sm" }, "•"), $m.h("span", { "className": "flex-1 text-sm text-zinc-300" }, formatDocText($m.idx(listItems, li))))]);
      (li = (li + 1));
    }
    return $m.h("div", { "key": idx, "className": "flex flex-col gap-2 pl-2 my-1" }, liNodes);
  }
  if ($m.eq(bType, "terminal")) {
    let cmds = $m.get(item, "commands");
    let cmdNodes = [];
    let ci = 0;
    while ((ci < $m.size(cmds))) {
      $m.call(cmdNodes, "push", [$m.h("div", { "key": ci, "className": "w-full flex flex-row items-start gap-2" }, $m.h("span", { "className": "font-mono text-xs text-zinc-500 shrink-0" }, "$"), $m.h("span", { "className": "flex-1 font-mono text-xs text-emerald-400 font-medium" }, $m.idx(cmds, ci)))]);
      (ci = (ci + 1));
    }
    return $m.h("div", { "key": idx, "className": "rounded-xl border border-zinc-800 bg-zinc-950 p-4 flex flex-col gap-1.5 my-1" }, $m.h("span", { "className": "font-mono text-xs text-zinc-400 pb-1.5 border-b border-zinc-800" }, "Terminal"), cmdNodes);
  }
  return $m.h("div", { "key": idx });
  }
  function DocContentView(props) {
  let section = $m.get(props, "section");
  let prevSection = $m.get(props, "prevSection");
  let nextSection = $m.get(props, "nextSection");
  let trackLabel = ((!$m.eq($m.get(props, "trackLabel"), null)) ? $m.get(props, "trackLabel") : "Core Language");
  let syntaxMode = ((!$m.eq($m.get(props, "syntaxMode"), null)) ? $m.get(props, "syntaxMode") : "hi");
  let onNavigateSection = $m.get(props, "onNavigateSection");
  let onOpenPlayground = $m.get(props, "onOpenPlayground");
  if ($m.eq(section, null)) {
    return $m.h("div", { "className": "p-8 text-zinc-400" }, "Section not found .");
  }
  let blocks = $m.get(section, "content");
  let renderedBlocks = [];
  let bi = 0;
  while ((bi < $m.size(blocks))) {
    $m.call(renderedBlocks, "push", [renderDocBlock($m.idx(blocks, bi), bi, syntaxMode, onOpenPlayground)]);
    (bi = (bi + 1));
  }
  return $m.h("div", { "className": "flex flex-col gap-4 max-w-4xl w-full pb-12" }, $m.h("div", { "className": "pb-5 border-b border-zinc-800 flex flex-col gap-2" }, $m.h("div", { "className": "flex flex-row items-center gap-2 flex-wrap" }, $m.h("span", { "className": "px-2 py-0.5 rounded bg-zinc-800 border border-zinc-700 font-mono text-xs text-emerald-300" }, trackLabel), $m.h("span", { "className": "px-2 py-0.5 rounded bg-zinc-900 border border-zinc-700 font-mono text-xs text-amber-400" }, $m.get(section, "id")), $m.h("span", { "className": ($m.eq(syntaxMode, "en") ? "px-2 py-0.5 rounded bg-sky-950 border border-sky-800 font-mono text-xs text-sky-300" : "px-2 py-0.5 rounded bg-amber-950 border border-amber-800 font-mono text-xs text-amber-300") }, ($m.eq(syntaxMode, "en") ? "English Word Syntax" : "Fun Hindi Syntax"))), $m.h("h1", { "className": "text-2xl font-bold text-zinc-100" }, $m.get(section, "title")), $m.h("p", { "className": "text-sm text-zinc-400" }, $m.get(section, "description"))), $m.h("div", { "className": "flex flex-col gap-3" }, renderedBlocks), $m.h("div", { "className": "mt-6 pt-6 border-t border-zinc-800 flex flex-row items-center justify-between gap-4 flex-wrap" }, ((!$m.eq(prevSection, null)) ? $m.h("button", { "className": "px-4 py-2.5 rounded-xl border border-zinc-800 bg-zinc-900 text-zinc-200 text-xs font-medium text-left", "onClick": (() => {
  return onNavigateSection($m.get(prevSection, "id"));
}) }, ("<- Previous: " + $m.get(prevSection, "title"))) : $m.h("div", null)), ((!$m.eq(nextSection, null)) ? $m.h("button", { "className": "px-4 py-2.5 rounded-xl border border-zinc-700 bg-zinc-800 text-zinc-100 text-xs font-semibold text-right", "onClick": (() => {
  return onNavigateSection($m.get(nextSection, "id"));
}) }, (("Next: " + $m.get(nextSection, "title")) + " ->")) : $m.h("div", null))));
  }
  const __ns = { DocContentView, renderDocBlock, formatDocText, CodeBlockComp };
  $mod_8.__cache = __ns;
  return __ns;
}

// Module: PlaygroundView.meme
function $mod_9() {
  if ($mod_9.__cache) return $mod_9.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  let useState; useState = $m.ui["useState"]; 
  let Icon; Icon = $m.icons["Icon"]; 
  native = $m.native;
  meme = $m.meme;
  const _mt0 = $mod_6();
  let shPg = _mt0;
  function paintPreviewCommands(painter, bounds, cmds) {
  let bw = (($m.get(bounds, "width") > 10) ? $m.get(bounds, "width") : 520);
  let bh = (($m.get(bounds, "height") > 10) ? $m.get(bounds, "height") : 340);
  $m.call(painter, "rect_filled", [0, 0, bw, bh, 10, "#09090b", 1]);
  if (($m.eq(cmds, null) || $m.eq($m.size(cmds), 0))) {
    $m.call(painter, "text", [24, $m.mul(bh, 0.45), "Run UI code (ui.mount(...)) to render Direct GPU Canvas Preview", 13, "#71717a", 500, false, 1]);
    return null;
  }
  let sx = (bw / 800);
  let sy = (bh / 560);
  let sc = ((sx < sy) ? sx : sy);
  if ((sc > 1)) {
    (sc = 1);
  }
  if ((sc < 0.35)) {
    (sc = 0.35);
  }
  let i = 0;
  let total = $m.size(cmds);
  while ((i < total)) {
    let cmd = $m.idx(cmds, i);
    let tp = $m.get(cmd, "type");
    let cx = $m.mul($m.get(cmd, "x"), sc);
    let cy = $m.mul($m.get(cmd, "y"), sc);
    let cw = $m.mul($m.get(cmd, "width"), sc);
    let ch = $m.mul($m.get(cmd, "height"), sc);
    let cr = ((!$m.eq($m.get(cmd, "radius"), null)) ? $m.mul($m.get(cmd, "radius"), sc) : 0);
    let col = (((!$m.eq($m.get(cmd, "color"), null)) && (!$m.eq($m.get(cmd, "color"), ""))) ? $m.get(cmd, "color") : "#e4e4e7");
    let al = ((!$m.eq($m.get(cmd, "alpha"), null)) ? $m.get(cmd, "alpha") : 1);
    if ($m.eq(tp, "FILL_ROUND_RECT")) {
      $m.call(painter, "rect_filled", [cx, cy, cw, ch, cr, col, al]);
    } else if ($m.eq(tp, "STROKE_ROUND_RECT")) {
      let lw = ((!$m.eq($m.get(cmd, "lineWidth"), null)) ? $m.get(cmd, "lineWidth") : 1);
      $m.call(painter, "rect_stroke", [cx, cy, cw, ch, cr, lw, col, al]);
    } else if ($m.eq(tp, "DRAW_TEXT")) {
      let txt = ((!$m.eq($m.get(cmd, "text"), null)) ? $m.get(cmd, "text") : "");
      if ((!$m.eq(txt, ""))) {
        let fs = $m.mul(((!$m.eq($m.get(cmd, "fontSize"), null)) ? $m.get(cmd, "fontSize") : 14), sc);
        let isM = ((!$m.eq($m.get(cmd, "isMono"), null)) ? $m.get(cmd, "isMono") : false);
        $m.call(painter, "text", [cx, (cy - fs), txt, ((fs < 7) ? 7 : fs), col, 400, isM, al]);
      }
    } else if ($m.eq(tp, "DRAW_CIRCLE")) {
      $m.call(painter, "circle_filled", [(cx + $m.mul(cw, 0.5)), (cy + $m.mul(ch, 0.5)), ((cr > 0) ? cr : $m.mul(cw, 0.5)), col, al]);
    } else if ($m.eq(tp, "STROKE_CIRCLE")) {
      let clw = ((!$m.eq($m.get(cmd, "lineWidth"), null)) ? $m.get(cmd, "lineWidth") : 1);
      $m.call(painter, "circle_stroke", [(cx + $m.mul(cw, 0.5)), (cy + $m.mul(ch, 0.5)), ((cr > 0) ? cr : $m.mul(cw, 0.5)), clw, col, al]);
    } else if ($m.eq(tp, "DRAW_LINE")) {
      let llw = ((!$m.eq($m.get(cmd, "lineWidth"), null)) ? $m.get(cmd, "lineWidth") : 1);
      $m.call(painter, "line", [cx, cy, (cx + cw), (cy + ch), llw, col, al]);
    }
    (i = (i + 1));
  }
  }
  function formatPgMs(msVal) {
  let tenths = $m.int($m.mul(msVal, 10));
  let whole = $m.int((tenths / 10));
  let frac = (tenths - $m.mul(whole, 10));
  if ((frac < 0)) {
    (frac = 0);
  }
  return (($m.str(whole) + ".") + $m.str(frac));
  }
  function PlaygroundView(props) {
  let presets = $m.get(props, "presets");
  let initialCode = $m.get(props, "initialCode");
  let syntaxMode = ((!$m.eq($m.get(props, "syntaxMode"), null)) ? $m.get(props, "syntaxMode") : "hi");
  let defaultPreset = $m.idx(presets, 0);
  let hasInitial = ((!$m.eq(initialCode, null)) && (!$m.eq(initialCode, "")));
  let startCode = (hasInitial ? initialCode : $m.call(shPg, "formatSnippetForMode", [$m.get(defaultPreset, "code"), syntaxMode]));
  let selectedId;
  let setSelectedId;
  [selectedId, setSelectedId] = $m.iter(useState((hasInitial ? "custom" : $m.get(defaultPreset, "id"))));
  let codeText;
  let setCodeText;
  [codeText, setCodeText] = $m.iter(useState(startCode));
  let stdinText;
  let setStdinText;
  [stdinText, setStdinText] = $m.iter(useState((($m.call(defaultPreset, "has", ["stdin"]) && (!$m.eq($m.get(defaultPreset, "stdin"), null))) ? $m.get(defaultPreset, "stdin") : "")));
  let showStdin;
  let setShowStdin;
  [showStdin, setShowStdin] = $m.iter(useState(false));
  let outputTab;
  let setOutputTab;
  [outputTab, setOutputTab] = $m.iter(useState("console"));
  let hasRun;
  let setHasRun;
  [hasRun, setHasRun] = $m.iter(useState(false));
  let resOutput;
  let setResOutput;
  [resOutput, setResOutput] = $m.iter(useState(""));
  let resError;
  let setResError;
  [resError, setResError] = $m.iter(useState(false));
  let resDuration;
  let setResDuration;
  [resDuration, setResDuration] = $m.iter(useState("0.0"));
  let resGpuCmds;
  let setResGpuCmds;
  [resGpuCmds, setResGpuCmds] = $m.iter(useState([]));
  let copied;
  let setCopied;
  [copied, setCopied] = $m.iter(useState(false));
  let prevInitial;
  let setPrevInitial;
  [prevInitial, setPrevInitial] = $m.iter(useState(initialCode));
  let prevMode;
  let setPrevMode;
  [prevMode, setPrevMode] = $m.iter(useState(syntaxMode));
  if ((!$m.eq(initialCode, prevInitial))) {
    setPrevInitial(initialCode);
    if (((!$m.eq(initialCode, null)) && (!$m.eq(initialCode, "")))) {
      setCodeText(initialCode);
      setSelectedId("custom");
      setHasRun(false);
      setResGpuCmds([]);
    }
  }
  if ((!$m.eq(syntaxMode, prevMode))) {
    setPrevMode(syntaxMode);
    let matchedBase = null;
    if ((!$m.eq(selectedId, "custom"))) {
      let mi = 0;
      while ((mi < $m.size(presets))) {
        if ($m.eq($m.get($m.idx(presets, mi), "id"), selectedId)) {
          (matchedBase = $m.get($m.idx(presets, mi), "code"));
          break;
        }
        (mi = (mi + 1));
      }
    }
    if ((!$m.eq(matchedBase, null))) {
      setCodeText($m.call(shPg, "formatSnippetForMode", [matchedBase, syntaxMode]));
    } else {
      setCodeText($m.call(shPg, "convertCodeSyntax", [codeText, syntaxMode]));
    }
  }
  function selectPreset(presetObj) {
  setSelectedId($m.get(presetObj, "id"));
  setCodeText($m.call(shPg, "formatSnippetForMode", [$m.get(presetObj, "code"), syntaxMode]));
  let pStdin = (($m.call(presetObj, "has", ["stdin"]) && (!$m.eq($m.get(presetObj, "stdin"), null))) ? $m.get(presetObj, "stdin") : "");
  setStdinText(pStdin);
  setShowStdin((!$m.eq(pStdin, "")));
  setHasRun(false);
  setResGpuCmds([]);
  }
  async function runCodeNow() {
  let ev = (await $m.eval(codeText, stdinText));
  let outClean = $m.call(shPg, "stripAnsiCodes", [$m.get(ev, "output")]);
  setResOutput(outClean);
  setResError($m.get(ev, "isError"));
  setResDuration(formatPgMs($m.get(ev, "durationMs")));
  setResGpuCmds($m.get(ev, "gpuCommands"));
  setHasRun(true);
  if ($m.get(ev, "hasUi")) {
    setOutputTab("gpu");
  } else {
    setOutputTab("console");
  }
  }
  function appendTemplate(snippet) {
  setCodeText(((codeText + "\n") + snippet));
  }
  function copyCode() {
  $m.call($m.get($m.native, "clipboard"), "write", [codeText]);
  setCopied(true);
  }
  let presetBtns = [];
  let pi = 0;
  while ((pi < $m.size(presets))) {
    let p = $m.idx(presets, pi);
    let isAct = $m.eq(selectedId, $m.get(p, "id"));
    $m.call(presetBtns, "push", [$m.h("button", { "key": pi, "className": (isAct ? "px-3 py-1.5 rounded-lg bg-amber-500 text-zinc-950 font-semibold text-xs shrink-0" : "px-3 py-1.5 rounded-lg bg-zinc-900 border border-zinc-800 text-zinc-300 text-xs shrink-0"), "onClick": (() => {
  return selectPreset(p);
}) }, $m.get(p, "name"))]);
    (pi = (pi + 1));
  }
  return $m.h("div", { "className": "flex flex-col gap-4 w-full max-w-6xl pb-10" }, $m.h("div", { "className": "flex flex-row items-center justify-between gap-3 flex-wrap border-b border-zinc-800 pb-4" }, $m.h("div", { "className": "flex flex-col gap-1" }, $m.h("h1", { "className": "text-2xl font-bold text-zinc-100" }, "WebAssembly Interactive Playground"), $m.h("p", { "className": "text-xs text-zinc-400" }, "Edit MEMElang code live and execute directly in the pure - MEMElang WebAssembly sandbox .")), $m.h("div", { "className": "flex flex-row items-center gap-2 flex-wrap" }, $m.h("button", { "className": "px-3 py-1.5 rounded-lg bg-zinc-900 border border-zinc-700 text-xs text-zinc-200", "onClick": (() => {
  return setShowStdin((!showStdin));
}) }, (showStdin ? "Hide Stdin" : "Stdin Input")), $m.h("button", { "className": "inline-flex flex-row items-center gap-1.5 px-3 py-1.5 rounded-lg bg-zinc-900 border border-zinc-700 text-xs text-zinc-200", "onClick": copyCode }, $m.h(Icon, { "name": (copied ? "check" : "copy"), "size": 13, "color": (copied ? "#34d399" : "#e4e4e7") }), $m.h("span", null, (copied ? "Copied!" : "Copy Code"))), $m.h("button", { "className": "inline-flex flex-row items-center gap-1.5 px-4 py-1.5 rounded-lg bg-emerald-500 text-zinc-950 font-bold text-xs", "onClick": runCodeNow }, $m.h(Icon, { "name": "play", "size": 13, "color": "#09090b" }), $m.h("span", null, "Run in Wasm")))), $m.h("div", { "className": "flex flex-col gap-2" }, $m.h("span", { "className": "font-mono text-xs text-zinc-400" }, (("Recipes (" + $m.str($m.size(presets))) + ") - Scroll horizontally or tap a recipe:")), $m.h("div", { "className": "w-full overflow-x-auto flex flex-row items-center gap-2 pb-2" }, presetBtns)), $m.h("div", { "className": "flex flex-row items-center gap-2 flex-wrap" }, $m.h("span", { "className": "font-mono text-xs text-zinc-500" }, "Quick Insert :"), $m.h("button", { "className": "px-2 py-1 rounded bg-zinc-900 border border-zinc-800 font-mono text-xs text-sky-300", "onClick": (() => {
  return appendTemplate($m.call(shPg, "formatSnippetForMode", ["pakad x = 42;\ndikha x;", syntaxMode]));
}) }, ($m.eq(syntaxMode, "en") ? "+ let" : "+ pakad")), $m.h("button", { "className": "px-2 py-1 rounded bg-zinc-900 border border-zinc-800 font-mono text-xs text-amber-300", "onClick": (() => {
  return appendTemplate($m.call(shPg, "formatSnippetForMode", ["jugaad greet(name) {\n    ye_le \"Namaste \" + name;\n}\ndikha greet(\"Duniya\");", syntaxMode]));
}) }, ($m.eq(syntaxMode, "en") ? "+ fn" : "+ jugaad")), $m.h("button", { "className": "px-2 py-1 rounded bg-zinc-900 border border-zinc-800 font-mono text-xs text-emerald-300", "onClick": (() => {
  return appendTemplate($m.call(shPg, "formatSnippetForMode", ["bula \"platform\";\ndikha platform.info();", syntaxMode]));
}) }, "+ platform"), $m.h("button", { "className": "px-2 py-1 rounded bg-zinc-900 border border-zinc-800 font-mono text-xs text-fuchsia-300", "onClick": (() => {
  return appendTemplate($m.call(shPg, "formatSnippetForMode", ["bula \"permissions\";\ndikha permissions.check(\"camera\");", syntaxMode]));
}) }, "+ permissions"), $m.h("button", { "className": "px-2 py-1 rounded bg-zinc-900 border border-zinc-800 font-mono text-xs text-amber-400", "onClick": (() => {
  return appendTemplate($m.call(shPg, "formatSnippetForMode", ["jugaad MiniCard() {\n    ye_le <div className=\"p-4 rounded-xl bg-slate-900 border border-indigo-500 text-white\">Live GPU Card</div>;\n}\nui.mount(MiniCard);", syntaxMode]));
}) }, "+ ui . mount")), (showStdin ? $m.h("div", { "className": "flex flex-col gap-1.5" }, $m.h("span", { "className": "font-mono text-xs text-zinc-400" }, "Standard Input ( for pooch ( ) calls ) :"), $m.h("input", { "className": "w-full px-3 py-2 rounded-lg bg-zinc-900 border border-zinc-700 font-mono text-xs text-zinc-100", "placeholder": "Type stdin lines separated by Enter...", "value": stdinText, "onChange": ((ev) => {
  return setStdinText($m.get($m.get(ev, "target"), "value"));
}) })) : null), $m.h("div", { "className": "flex flex-col lg:flex-row gap-4 w-full" }, $m.h("div", { "className": "flex-1 flex flex-col rounded-xl border border-zinc-800 bg-zinc-950 overflow-hidden" }, $m.h("div", { "className": "px-4 py-2.5 bg-zinc-900 border-b border-zinc-800 flex flex-row items-center justify-between gap-2" }, $m.h("div", { "className": "flex flex-row items-center gap-2" }, $m.h("span", { "className": "font-mono text-xs text-zinc-300 font-medium" }, "main . meme - Live Editor"), $m.h("span", { "className": ($m.eq(syntaxMode, "en") ? "px-1.5 py-0.5 rounded bg-sky-950 border border-sky-800 font-mono text-xs text-sky-300" : "px-1.5 py-0.5 rounded bg-amber-950 border border-amber-800 font-mono text-xs text-amber-300") }, ($m.eq(syntaxMode, "en") ? "English" : "Fun Hindi"))), $m.h("span", { "className": "font-mono text-xs text-zinc-500" }, ($m.str($m.size(codeText)) + " chars"))), $m.h("textarea", { "className": "w-full h-80 p-3 bg-zinc-950 font-mono text-xs text-zinc-100", "value": codeText, "placeholder": "Write MEMElang code here...", "onChange": ((ev) => {
  return setCodeText($m.get($m.get(ev, "target"), "value"));
}) })), $m.h("div", { "className": "flex-1 flex flex-col rounded-xl border border-zinc-800 bg-zinc-950 overflow-hidden" }, $m.h("div", { "className": "px-4 py-2.5 bg-zinc-900 border-b border-zinc-800 flex flex-row items-center justify-between gap-2" }, $m.h("div", { "className": "flex flex-row items-center gap-2" }, $m.h("button", { "className": ($m.eq(outputTab, "console") ? "px-2.5 py-1 rounded bg-zinc-800 text-xs font-semibold text-zinc-100" : "px-2.5 py-1 rounded text-xs text-zinc-400"), "onClick": (() => {
  return setOutputTab("console");
}) }, "Console"), $m.h("button", { "className": ($m.eq(outputTab, "gpu") ? "px-2.5 py-1 rounded bg-amber-500 text-xs font-semibold text-zinc-950" : "px-2.5 py-1 rounded bg-zinc-950 border border-zinc-800 text-xs text-amber-300"), "onClick": (() => {
  return setOutputTab("gpu");
}) }, (("Direct GPU UI (" + $m.str($m.size(resGpuCmds))) + ")"))), $m.h("span", { "className": "font-mono text-xs text-zinc-400" }, (hasRun ? (((resError ? "Error • " : "Done • ") + resDuration) + "ms") : "Idle"))), $m.h("div", { "className": "p-4 flex flex-col gap-3 min-h-[320px]" }, ($m.eq(outputTab, "gpu") ? $m.h("canvas", { "className": "w-full h-72 rounded-xl border border-zinc-800 bg-zinc-950", "onPaint": ((painter, bounds) => {
  return paintPreviewCommands(painter, bounds, resGpuCmds);
}) }) : (hasRun ? $m.h("div", { "className": "flex flex-col gap-2" }, (resError ? $m.h("div", { "className": "p-3 rounded-lg bg-rose-950/40 border border-rose-800 text-xs text-rose-200 font-semibold" }, "Meme Diagnostic Alert") : null), $m.h("div", { "className": (resError ? "p-3 rounded-lg bg-rose-950/20 border border-rose-900 font-mono text-xs text-rose-200" : "p-3 rounded-lg bg-zinc-900 border border-zinc-800 font-mono text-xs text-emerald-300") }, ((!$m.eq(resOutput, "")) ? resOutput : "(Execution completed with no stdout output)"))) : $m.h("div", { "className": "p-6 text-center flex flex-col gap-2" }, $m.h("span", { "className": "font-mono text-sm text-zinc-300" }, "Ready to run MEMElang in WebAssembly"), $m.h("span", { "className": "text-xs text-zinc-500" }, "Pick a recipe above or edit main . meme , then click Run in Wasm ."))))))));
  }
  const __ns = { PlaygroundView, formatPgMs, paintPreviewCommands };
  $mod_9.__cache = __ns;
  return __ns;
}

// Module: ErrorsView.meme
function $mod_10() {
  if ($mod_10.__cache) return $mod_10.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  let useState; useState = $m.ui["useState"]; 
  let Icon; Icon = $m.icons["Icon"]; 
  meme = $m.meme;
  const _mt0 = $mod_6();
  let shErr = _mt0;
  function ErrorsView(props) {
  let errorsList = $m.get(props, "errorsList");
  let syntaxMode = ((!$m.eq($m.get(props, "syntaxMode"), null)) ? $m.get(props, "syntaxMode") : "hi");
  let onOpenPlayground = $m.get(props, "onOpenPlayground");
  let searchTerm;
  let setSearchTerm;
  [searchTerm, setSearchTerm] = $m.iter(useState(""));
  let activeCat;
  let setActiveCat;
  [activeCat, setActiveCat] = $m.iter(useState("All"));
  let testedTitle;
  let setTestedTitle;
  [testedTitle, setTestedTitle] = $m.iter(useState(""));
  let testedOutput;
  let setTestedOutput;
  [testedOutput, setTestedOutput] = $m.iter(useState(""));
  let catOpts = ["All", "Arithmetic", "Syntax", "Runtime", "OOP", "Ownership & Borrowing", "Lexer", "Parser", "Type Error"];
  let qLower = $m.call(searchTerm, "to_lower", []);
  let filtered = [];
  let ei = 0;
  while ((ei < $m.size(errorsList))) {
    let item = $m.idx(errorsList, ei);
    let matchCat = ($m.eq(activeCat, "All") || $m.eq($m.get(item, "category"), activeCat));
    let matchSearch = true;
    if ((!$m.eq(qLower, ""))) {
      let tMatch = ($m.call($m.call($m.get(item, "title"), "to_lower", []), "find", [qLower]) >= 0);
      let qMatch = ($m.call($m.call($m.get(item, "quote"), "to_lower", []), "find", [qLower]) >= 0);
      let sMatch = ($m.call($m.call($m.get(item, "scenario"), "to_lower", []), "find", [qLower]) >= 0);
      let oMatch = ($m.call($m.call($m.get(item, "origin"), "to_lower", []), "find", [qLower]) >= 0);
      (matchSearch = (((tMatch || qMatch) || sMatch) || oMatch));
    }
    if ((matchCat && matchSearch)) {
      $m.call(filtered, "push", [item]);
    }
    (ei = (ei + 1));
  }
  async function triggerLive(errObj, dispCode) {
  let r = (await $m.eval(dispCode));
  setTestedTitle($m.get(errObj, "title"));
  setTestedOutput($m.call(shErr, "stripAnsiCodes", [$m.get(r, "output")]));
  }
  let catButtons = [];
  let ci = 0;
  while ((ci < $m.size(catOpts))) {
    let cname = $m.idx(catOpts, ci);
    let isAct = $m.eq(activeCat, cname);
    $m.call(catButtons, "push", [$m.h("button", { "key": ci, "className": (isAct ? "px-3 py-1 rounded-full bg-amber-500 text-zinc-950 font-semibold text-xs" : "px-3 py-1 rounded-full bg-zinc-900 border border-zinc-800 text-zinc-300 text-xs"), "onClick": (() => {
  return setActiveCat(cname);
}) }, cname)]);
    (ci = (ci + 1));
  }
  let cards = [];
  let fi = 0;
  while ((fi < $m.size(filtered))) {
    let errObj = $m.idx(filtered, fi);
    let dispCode = $m.call(shErr, "formatSnippetForMode", [$m.get(errObj, "code"), syntaxMode]);
    let showLive = ($m.eq(testedTitle, $m.get(errObj, "title")) && (!$m.eq(testedOutput, "")));
    $m.call(cards, "push", [$m.h("div", { "key": fi, "className": "rounded-xl border border-zinc-800 bg-zinc-900/50 p-5 flex flex-col gap-3" }, $m.h("div", { "className": "flex flex-row items-center justify-between gap-2 flex-wrap" }, $m.h("span", { "className": "px-2 py-0.5 rounded bg-zinc-800 border border-zinc-700 font-mono text-xs text-amber-300" }, $m.get(errObj, "category")), $m.h("span", { "className": "text-xs text-zinc-400" }, $m.get(errObj, "origin"))), $m.h("h2", { "className": "text-base font-bold text-zinc-100" }, $m.get(errObj, "title")), $m.h("span", { "className": "font-mono text-xs text-zinc-400" }, ("Trigger: " + $m.get(errObj, "scenario"))), $m.h("div", { "className": "p-3 rounded-lg bg-zinc-950 border border-zinc-800 text-xs text-zinc-200" }, (("\"" + $m.get(errObj, "quote")) + "\"")), $m.h("div", { "className": "p-3 rounded-lg bg-zinc-950 border border-zinc-800 overflow-x-auto" }, $m.call(shErr, "renderHighlightedCode", [dispCode])), (showLive ? $m.h("div", { "className": "p-3 rounded-lg bg-rose-950/30 border border-rose-900 font-mono text-xs text-rose-200" }, testedOutput) : null), $m.h("div", { "className": "flex flex-row items-center gap-2" }, $m.h("button", { "className": "flex-1 inline-flex flex-row items-center justify-center gap-1.5 py-2 px-3 rounded-lg bg-zinc-100 text-zinc-950 font-semibold text-xs", "onClick": (async () => {
  return (await triggerLive(errObj, dispCode));
}) }, $m.h(Icon, { "name": "zap", "size": 13, "color": "#09090b" }), $m.h("span", null, "Trigger Error Live")), $m.h("button", { "className": "inline-flex flex-row items-center gap-1.5 py-2 px-3 rounded-lg bg-zinc-800 border border-zinc-700 text-zinc-200 text-xs", "onClick": (() => {
  return onOpenPlayground(dispCode);
}) }, $m.h(Icon, { "name": "terminal", "size": 13, "color": "#e4e4e7" }), $m.h("span", null, "Playground"))))]);
    (fi = (fi + 1));
  }
  return $m.h("div", { "className": "flex flex-col gap-5 max-w-6xl w-full pb-12" }, $m.h("div", { "className": "flex flex-col gap-2 border-b border-zinc-800 pb-5" }, $m.h("div", { "className": "flex flex-row items-center gap-2 flex-wrap" }, $m.h("span", { "className": "px-2.5 py-1 rounded bg-zinc-900 border border-zinc-700 font-mono text-xs text-amber-400" }, "The Diagnostic Catalog"), $m.h("span", { "className": ($m.eq(syntaxMode, "en") ? "px-2 py-1 rounded bg-sky-950 border border-sky-800 font-mono text-xs text-sky-300" : "px-2 py-1 rounded bg-amber-950 border border-amber-800 font-mono text-xs text-amber-300") }, ($m.eq(syntaxMode, "en") ? "English Syntax" : "Fun Hindi Syntax"))), $m.h("h1", { "className": "text-2xl font-bold text-zinc-100" }, (("Iconic Meme Errors (" + $m.str($m.size(filtered))) + ")")), $m.h("p", { "className": "text-sm text-zinc-400" }, "MEMElang replaces cryptic compiler failures with iconic Bollywood & internet memes . Trigger any error live in WebAssembly ."), $m.h("div", { "className": "pt-2 flex flex-col gap-3" }, $m.h("input", { "className": "w-full max-w-md px-3.5 py-2 rounded-lg bg-zinc-900 border border-zinc-700 text-sm text-zinc-100", "placeholder": "Search meme errors (e.g. baburao, divide)...", "value": searchTerm, "onChange": ((ev) => {
  return setSearchTerm($m.get($m.get(ev, "target"), "value"));
}) }), $m.h("div", { "className": "flex flex-row items-center gap-2 flex-wrap" }, catButtons))), $m.h("div", { "className": "grid grid-cols-1 md:grid-cols-2 gap-4" }, cards));
  }
  const __ns = { ErrorsView };
  $mod_10.__cache = __ns;
  return __ns;
}

// Module: App.meme
function $mod_11() {
  if ($mod_11.__cache) return $mod_11.__cache;
  let ui = $m.ui, native = $m.native, platform = $m.platform, permissions = $m.permissions, meme = $m.meme, tijori = $m.tijori;
  let useState; useState = $m.ui["useState"]; let useRouter; useRouter = $m.ui["useRouter"]; 
  let ScrollArea; ScrollArea = $m.shadcn["ScrollArea"]; let Popover; Popover = $m.shadcn["Popover"]; 
  let Icon; Icon = $m.icons["Icon"]; 
  native = $m.native;
  platform = $m.platform;
  meme = $m.meme;
  const _mt0 = $mod_0();
  let catsMod = _mt0;
  const _mt1 = $mod_1();
  let coreMod = _mt1;
  const _mt2 = $mod_2();
  let stdlibMod = _mt2;
  const _mt3 = $mod_3();
  let platMod = _mt3;
  const _mt4 = $mod_4();
  let presetsMod = _mt4;
  const _mt5 = $mod_5();
  let errsMod = _mt5;
  const _mt6 = $mod_8();
  let docViewMod = _mt6;
  const _mt7 = $mod_9();
  let pgViewMod = _mt7;
  const _mt8 = $mod_10();
  let errViewMod = _mt8;
  let DocContentComp = $m.get(docViewMod, "DocContentView");
  let PlaygroundComp = $m.get(pgViewMod, "PlaygroundView");
  let ErrorsComp = $m.get(errViewMod, "ErrorsView");
  let DOCS_CATEGORIES = $m.call(catsMod, "getCategories", []);
  let DOCS_CORE = $m.call(coreMod, "getSectionsCore", []);
  let DOCS_STDLIB = $m.call(stdlibMod, "getSectionsStdlib", []);
  let DOCS_PLATFORM = $m.call(platMod, "getSectionsPlatform", []);
  let PLAYGROUND_RECIPES = $m.call(presetsMod, "getPlaygroundPresets", []);
  let ERROR_ITEMS = $m.call(errsMod, "getErrorsCatalog", []);
  function collectAllSections() {
  let all = [];
  let i = 0;
  while ((i < $m.size(DOCS_CORE))) {
    $m.call(all, "push", [$m.idx(DOCS_CORE, i)]);
    (i = (i + 1));
  }
  (i = 0);
  while ((i < $m.size(DOCS_STDLIB))) {
    $m.call(all, "push", [$m.idx(DOCS_STDLIB, i)]);
    (i = (i + 1));
  }
  (i = 0);
  while ((i < $m.size(DOCS_PLATFORM))) {
    $m.call(all, "push", [$m.idx(DOCS_PLATFORM, i)]);
    (i = (i + 1));
  }
  return all;
  }
  let ALL_SECTIONS = collectAllSections();
  function getTrackForSection(secId) {
  let ci = 0;
  while ((ci < $m.size(DOCS_CATEGORIES))) {
    let cat = $m.idx(DOCS_CATEGORIES, ci);
    let sids = $m.get(cat, "sectionIds");
    let j = 0;
    while ((j < $m.size(sids))) {
      if ($m.eq($m.idx(sids, j), secId)) {
        return ((!$m.eq($m.get(cat, "track"), null)) ? $m.get(cat, "track") : "core");
      }
      (j = (j + 1));
    }
    (ci = (ci + 1));
  }
  return "core";
  }
  function getTrackLabel(trackId) {
  if ($m.eq(trackId, "platform")) {
    return "UI, Native & Platform";
  }
  return "Core Language";
  }
  function resolveRouteState(routePath, routeQuery, fallbackSection) {
  if ($m.eq(routePath, "/playground")) {
    return {"tab": "playground", "section": fallbackSection};
  }
  if ($m.eq(routePath, "/errors")) {
    return {"tab": "errors", "section": fallbackSection};
  }
  if ($m.call(routePath, "startsWith", ["/docs/"])) {
    let secId = $m.call(routePath, "slice", [6, $m.size(routePath)]);
    if ((!$m.eq(secId, ""))) {
      return {"tab": "docs", "section": secId};
    }
  }
  if ((((!$m.eq(routeQuery, null)) && $m.call(routeQuery, "has", ["section"])) && (!$m.eq($m.idx(routeQuery, "section"), "")))) {
    return {"tab": "docs", "section": $m.idx(routeQuery, "section")};
  }
  return {"tab": "docs", "section": fallbackSection};
  }
  function SidebarPanel(sections, activeSecId, activeTrack, onSelectTrack, searchQuery, setSearchQuery, onSelectSection) {
  let qLow = $m.call(searchQuery, "to_lower", []);
  let findSectionById = ((sid) => {
  let k = 0;
  while ((k < $m.size(sections))) {
    if ($m.eq($m.get($m.idx(sections, k), "id"), sid)) {
      return $m.idx(sections, k);
    }
    (k = (k + 1));
  }
  return null;
});
  let navNodes = [];
  if ((!$m.eq(qLow, ""))) {
    let matches = [];
    let si = 0;
    while ((si < $m.size(sections))) {
      let s = $m.idx(sections, si);
      let tOk = ($m.call($m.call($m.get(s, "title"), "to_lower", []), "find", [qLow]) >= 0);
      let dOk = ($m.call($m.call($m.get(s, "description"), "to_lower", []), "find", [qLow]) >= 0);
      let idOk = ($m.call($m.call($m.get(s, "id"), "to_lower", []), "find", [qLow]) >= 0);
      if (((tOk || dOk) || idOk)) {
        let trk = getTrackForSection($m.get(s, "id"));
        let trkBadge = ($m.eq(trk, "platform") ? "[UI/Platform] " : "[Core] ");
        $m.call(matches, "push", [$m.h("button", { "key": si, "className": ($m.eq(activeSecId, $m.get(s, "id")) ? "w-full text-left px-3 py-2 rounded-lg bg-zinc-800 text-zinc-100 text-xs font-semibold" : "w-full text-left px-3 py-2 rounded-lg text-zinc-400 text-xs"), "onClick": (() => {
  return onSelectSection($m.get(s, "id"));
}) }, (trkBadge + $m.get(s, "title")))]);
      }
      (si = (si + 1));
    }
    $m.call(navNodes, "push", [$m.h("div", { "key": 0, "className": "flex flex-col gap-1" }, $m.h("span", { "className": "px-3 py-1 font-mono text-xs text-zinc-400 font-semibold" }, (("SEARCH RESULTS (" + $m.str($m.size(matches))) + ")")), (($m.size(matches) > 0) ? $m.h("div", { "className": "flex flex-col gap-1" }, matches) : $m.h("span", { "className": "px-3 py-2 text-xs text-zinc-500" }, "No matching sections")))]);
  } else {
    let ci = 0;
    while ((ci < $m.size(DOCS_CATEGORIES))) {
      let cat = $m.idx(DOCS_CATEGORIES, ci);
      let catTrack = ((!$m.eq($m.get(cat, "track"), null)) ? $m.get(cat, "track") : "core");
      if ((!$m.eq(catTrack, activeTrack))) {
        (ci = (ci + 1));
        continue;
      }
      let sids = $m.get(cat, "sectionIds");
      let catBtns = [];
      let j = 0;
      while ((j < $m.size(sids))) {
        let secObj = findSectionById($m.idx(sids, j));
        if ((!$m.eq(secObj, null))) {
          let isAct = $m.eq(activeSecId, $m.get(secObj, "id"));
          $m.call(catBtns, "push", [$m.h("button", { "key": j, "className": (isAct ? "w-full text-left px-3 py-1.5 rounded-lg bg-zinc-800 text-zinc-100 text-xs font-semibold" : "w-full text-left px-3 py-1.5 rounded-lg text-zinc-400 text-xs"), "onClick": (() => {
  return onSelectSection($m.get(secObj, "id"));
}) }, $m.get(secObj, "title"))]);
        }
        (j = (j + 1));
      }
      $m.call(navNodes, "push", [$m.h("div", { "key": ci, "className": "flex flex-col gap-1 pb-3" }, $m.h("span", { "className": "px-3 py-1 text-xs font-semibold text-amber-400" }, $m.get(cat, "name")), $m.h("div", { "className": "flex flex-col gap-0.5" }, catBtns))]);
      (ci = (ci + 1));
    }
  }
  return $m.h("div", { "className": "flex flex-col gap-3" }, $m.h("div", { "className": "flex flex-col gap-1" }, $m.h("span", { "className": "px-1 font-mono text-xs text-zinc-500" }, "Documentation Sections"), $m.h("div", { "className": "flex flex-row gap-1 bg-zinc-900 p-1 rounded-xl border border-zinc-800" }, $m.h("button", { "className": ($m.eq(activeTrack, "core") ? "flex-1 px-2 py-1.5 rounded-lg bg-amber-500 text-zinc-950 font-semibold text-xs text-center" : "flex-1 px-2 py-1.5 rounded-lg text-zinc-400 text-xs text-center"), "onClick": (() => {
  return onSelectTrack("core");
}) }, "Core Language"), $m.h("button", { "className": ($m.eq(activeTrack, "platform") ? "flex-1 px-2 py-1.5 rounded-lg bg-amber-500 text-zinc-950 font-semibold text-xs text-center" : "flex-1 px-2 py-1.5 rounded-lg text-zinc-400 text-xs text-center"), "onClick": (() => {
  return onSelectTrack("platform");
}) }, "UI & Platform"))), $m.h("div", { "className": "flex flex-row items-center gap-2" }, $m.h("input", { "className": "flex-1 px-3 py-2 rounded-lg bg-zinc-900 border border-zinc-800 text-xs text-zinc-100", "placeholder": "Search docs (e.g. jugaad)...", "value": searchQuery, "onChange": ((ev) => {
  return setSearchQuery($m.get($m.get(ev, "target"), "value"));
}) }), ((!$m.eq(searchQuery, "")) ? $m.h("button", { "className": "px-2 py-2 rounded-lg bg-zinc-800 text-xs text-zinc-300", "onClick": (() => {
  return setSearchQuery("");
}) }, "X") : null)), $m.h("div", { "className": "flex flex-col gap-1" }, navNodes));
  }
  function App() {
  let router = useRouter();
  let initLang = ((((!$m.eq($m.get(router, "query"), null)) && $m.call($m.get(router, "query"), "has", ["lang"])) && $m.eq($m.idx($m.get(router, "query"), "lang"), "en")) ? "en" : "hi");
  let selectedSecId;
  let setSelectedSecId;
  [selectedSecId, setSelectedSecId] = $m.iter(useState("intro"));
  let searchQuery;
  let setSearchQuery;
  [searchQuery, setSearchQuery] = $m.iter(useState(""));
  let playgroundCode;
  let setPlaygroundCode;
  [playgroundCode, setPlaygroundCode] = $m.iter(useState(""));
  let mobileMenuOpen;
  let setMobileMenuOpen;
  [mobileMenuOpen, setMobileMenuOpen] = $m.iter(useState(false));
  let syntaxMode;
  let setSyntaxMode;
  [syntaxMode, setSyntaxMode] = $m.iter(useState(initLang));
  let langMenuOpen;
  let setLangMenuOpen;
  [langMenuOpen, setLangMenuOpen] = $m.iter(useState(false));
  let routeInfo = resolveRouteState($m.get(router, "path"), $m.get(router, "query"), selectedSecId);
  let activeTab = $m.get(routeInfo, "tab");
  let effectiveSecId = $m.get(routeInfo, "section");
  let activeTrack = getTrackForSection(effectiveSecId);
  let activeTrackLabel = getTrackLabel(activeTrack);
  let scr = $m.call($m.get($m.native, "device"), "screen", []);
  let isCompact = (($m.get(scr, "width") > 0) && ($m.get(scr, "width") < 820));
  let curIndex = 0;
  let idxSearch = 0;
  while ((idxSearch < $m.size(ALL_SECTIONS))) {
    if ($m.eq($m.get($m.idx(ALL_SECTIONS, idxSearch), "id"), effectiveSecId)) {
      (curIndex = idxSearch);
      break;
    }
    (idxSearch = (idxSearch + 1));
  }
  let currentSection = $m.idx(ALL_SECTIONS, curIndex);
  let prevSection = ((curIndex > 0) ? $m.idx(ALL_SECTIONS, (curIndex - 1)) : null);
  let nextSection = (((curIndex + 1) < $m.size(ALL_SECTIONS)) ? $m.idx(ALL_SECTIONS, (curIndex + 1)) : null);
  function navigateToSection(secId) {
  setSelectedSecId(secId);
  setMobileMenuOpen(false);
  setLangMenuOpen(false);
  $m.call(router, "navigate", [("/docs/" + secId)]);
  }
  function navigateToTrack(trkId) {
  let targetSec = ($m.eq(trkId, "platform") ? "react-jsx-ui" : "intro");
  setSelectedSecId(targetSec);
  setSearchQuery("");
  setMobileMenuOpen(false);
  setLangMenuOpen(false);
  $m.call(router, "navigate", [("/docs/" + targetSec)]);
  }
  function openPlaygroundWithCode(code) {
  setPlaygroundCode(code);
  setMobileMenuOpen(false);
  setLangMenuOpen(false);
  $m.call(router, "navigate", ["/playground"]);
  }
  function goTab(tabName) {
  setMobileMenuOpen(false);
  setLangMenuOpen(false);
  if ($m.eq(tabName, "playground")) {
    $m.call(router, "navigate", ["/playground"]);
  } else if ($m.eq(tabName, "errors")) {
    $m.call(router, "navigate", ["/errors"]);
  } else {
    $m.call(router, "navigate", [("/docs/" + effectiveSecId)]);
  }
  }
  function selectSyntaxMode(nextMode) {
  setSyntaxMode(nextMode);
  setLangMenuOpen(false);
  }
  return $m.h("div", { "className": "h-screen w-full bg-zinc-950 text-zinc-100 flex flex-col overflow-hidden" }, $m.h("div", { "className": "shrink-0 border-b border-zinc-800 bg-zinc-950 px-4 py-3 flex flex-row items-center justify-between gap-3 flex-wrap" }, $m.h("div", { "className": "flex flex-row items-center gap-3" }, ((isCompact && $m.eq(activeTab, "docs")) ? $m.h("button", { "className": "px-2.5 py-1.5 rounded-lg bg-zinc-900 border border-zinc-700 text-xs font-semibold text-zinc-200", "onClick": (() => {
  return setMobileMenuOpen((!mobileMenuOpen));
}) }, (mobileMenuOpen ? "Close Menu" : "Topics")) : null), $m.h("button", { "className": "flex flex-row items-center gap-2 bg-transparent p-0", "onClick": (() => {
  return navigateToSection("intro");
}) }, $m.h("span", { "className": "px-2.5 py-1 rounded-lg bg-zinc-900 border border-zinc-700 font-bold text-amber-400 text-sm" }, "M"), $m.h("span", { "className": "font-bold text-base text-zinc-100" }, "MEMElang")), ((!isCompact) ? $m.h("span", { "className": "px-2 py-0.5 rounded bg-zinc-900 border border-zinc-800 font-mono text-xs text-emerald-400" }, (("100% Pure .meme (" + $m.call($m.platform, "os", [])) + ")")) : null)), $m.h("div", { "className": "flex flex-row items-center gap-1.5 bg-zinc-900 p-1 rounded-xl border border-zinc-800" }, $m.h("button", { "className": ($m.eq(activeTab, "docs") ? "px-3 py-1.5 rounded-lg bg-zinc-800 text-zinc-100 text-xs font-semibold" : "px-3 py-1.5 rounded-lg text-zinc-400 text-xs"), "onClick": (() => {
  return goTab("docs");
}) }, "Documentation"), $m.h("button", { "className": ($m.eq(activeTab, "playground") ? "inline-flex flex-row items-center gap-1.5 px-3 py-1.5 rounded-lg bg-zinc-800 text-zinc-100 text-xs font-semibold" : "inline-flex flex-row items-center gap-1.5 px-3 py-1.5 rounded-lg text-zinc-400 text-xs"), "onClick": (() => {
  return goTab("playground");
}) }, $m.h(Icon, { "name": "terminal", "size": 14, "color": ($m.eq(activeTab, "playground") ? "#f4f4f5" : "#a1a1aa") }), $m.h("span", null, "Playground & REPL")), $m.h("button", { "className": ($m.eq(activeTab, "errors") ? "inline-flex flex-row items-center gap-1.5 px-3 py-1.5 rounded-lg bg-zinc-800 text-zinc-100 text-xs font-semibold" : "inline-flex flex-row items-center gap-1.5 px-3 py-1.5 rounded-lg text-zinc-400 text-xs"), "onClick": (() => {
  return goTab("errors");
}) }, $m.h(Icon, { "name": "alert-triangle", "size": 14, "color": ($m.eq(activeTab, "errors") ? "#f4f4f5" : "#a1a1aa") }), $m.h("span", null, "Meme Errors"))), $m.h("div", { "className": "flex flex-row items-center gap-2" }, $m.h("button", { "className": ($m.eq(syntaxMode, "en") ? "px-3 py-1.5 rounded-lg bg-sky-950 border border-sky-700 font-mono text-xs font-semibold text-sky-200" : "px-3 py-1.5 rounded-lg bg-amber-950 border border-amber-700 font-mono text-xs font-semibold text-amber-200"), "onClick": (() => {
  return setLangMenuOpen((!langMenuOpen));
}) }, (("Syntax: " + ($m.eq(syntaxMode, "en") ? "English" : "Fun Hindi")) + " v")), ((!isCompact) ? $m.h("button", { "className": "px-3 py-1.5 rounded-lg bg-zinc-100 text-zinc-950 font-semibold text-xs", "onClick": (() => {
  return goTab("playground");
}) }, "Try Live") : null))), (langMenuOpen ? $m.h(Popover, { "className": "fixed top-14 right-4 z-50 w-64 rounded-xl border border-zinc-700 bg-zinc-900 p-2 flex flex-col gap-1.5 shadow-2xl" }, $m.h("span", { "className": "px-2 py-1 font-mono text-xs text-zinc-400" }, "Code Syntax Mode"), $m.h("button", { "className": ($m.eq(syntaxMode, "hi") ? "w-full text-left px-3 py-2 rounded-lg bg-amber-500 text-zinc-950 text-xs font-semibold" : "w-full text-left px-3 py-2 rounded-lg bg-zinc-950 border border-zinc-800 text-zinc-200 text-xs"), "onClick": (() => {
  return selectSyntaxMode("hi");
}) }, "Fun Hindi (pakad, jugaad, dekh)"), $m.h("button", { "className": ($m.eq(syntaxMode, "en") ? "w-full text-left px-3 py-2 rounded-lg bg-sky-500 text-zinc-950 text-xs font-semibold" : "w-full text-left px-3 py-2 rounded-lg bg-zinc-950 border border-zinc-800 text-zinc-200 text-xs"), "onClick": (() => {
  return selectSyntaxMode("en");
}) }, "English (let, fn, if, print)")) : null), ($m.eq(activeTab, "docs") ? $m.h("div", { "className": "flex-1 min-h-0 flex flex-row overflow-hidden" }, ((!isCompact) ? $m.h(ScrollArea, { "className": "w-72 shrink-0 h-full overflow-y-auto border-r border-zinc-800 p-4 bg-zinc-950" }, SidebarPanel(ALL_SECTIONS, effectiveSecId, activeTrack, navigateToTrack, searchQuery, setSearchQuery, navigateToSection)) : null), $m.h(ScrollArea, { "key": ("doc-" + effectiveSecId), "className": "flex-1 h-full overflow-y-auto p-6" }, (isCompact ? $m.h("div", { "className": "flex flex-row gap-1 bg-zinc-900 p-1 rounded-xl border border-zinc-800 mb-4" }, $m.h("button", { "className": ($m.eq(activeTrack, "core") ? "flex-1 px-2 py-1.5 rounded-lg bg-amber-500 text-zinc-950 font-semibold text-xs text-center" : "flex-1 px-2 py-1.5 rounded-lg text-zinc-400 text-xs text-center"), "onClick": (() => {
  return navigateToTrack("core");
}) }, "Core Language"), $m.h("button", { "className": ($m.eq(activeTrack, "platform") ? "flex-1 px-2 py-1.5 rounded-lg bg-amber-500 text-zinc-950 font-semibold text-xs text-center" : "flex-1 px-2 py-1.5 rounded-lg text-zinc-400 text-xs text-center"), "onClick": (() => {
  return navigateToTrack("platform");
}) }, "UI & Platform")) : null), ((isCompact && mobileMenuOpen) ? $m.h("div", { "className": "mb-6 p-4 rounded-xl border border-zinc-800 bg-zinc-900" }, SidebarPanel(ALL_SECTIONS, effectiveSecId, activeTrack, navigateToTrack, searchQuery, setSearchQuery, navigateToSection)) : null), $m.h(DocContentComp, { "section": currentSection, "prevSection": prevSection, "nextSection": nextSection, "trackLabel": activeTrackLabel, "syntaxMode": syntaxMode, "onNavigateSection": navigateToSection, "onOpenPlayground": openPlaygroundWithCode }))) : ($m.eq(activeTab, "playground") ? $m.h(ScrollArea, { "key": "pg-scroll", "className": "flex-1 h-full overflow-y-auto p-6 animate-fade-in" }, $m.h(PlaygroundComp, { "presets": PLAYGROUND_RECIPES, "initialCode": playgroundCode, "syntaxMode": syntaxMode })) : $m.h(ScrollArea, { "key": "err-scroll", "className": "flex-1 h-full overflow-y-auto p-6 animate-fade-in" }, $m.h(ErrorsComp, { "errorsList": ERROR_ITEMS, "syntaxMode": syntaxMode, "onOpenPlayground": openPlaygroundWithCode })))), $m.h("div", { "className": "shrink-0 border-t border-zinc-800 bg-zinc-950 px-4 py-2.5 flex flex-row items-center justify-between gap-2 flex-wrap" }, $m.h("span", { "className": "text-xs text-zinc-400" }, "MEMElang v1 . 0 • Built 100 % in Pure MEMElang , C + + 20 & WebAssembly"), $m.h("span", { "className": "font-mono text-xs text-amber-400" }, "Wah bete mauj kardi !")));
  }
  $m.call($m.ui, "mount", [App]);
  $m.call($m.ui, "show", ["MEMElang Docs & Playground"]);
  const __ns = { SidebarPanel, resolveRouteState, getTrackLabel, getTrackForSection, collectAllSections, DOCS_PLATFORM, ERROR_ITEMS, DOCS_CORE, DocContentComp, ALL_SECTIONS, ErrorsComp, DOCS_STDLIB, App, DOCS_CATEGORIES, PlaygroundComp, PLAYGROUND_RECIPES };
  $mod_11.__cache = __ns;
  return __ns;
}

$mod_11();
