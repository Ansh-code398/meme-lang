// MEMElang Complete Documentation Content

export const DOCS_SECTIONS = [
  {
    id: 'intro',
    title: 'Introduction & Philosophy',
    badge: 'Overview',
    description: 'Learn why MEMElang exists and how it transforms standard compiler misery into Desi meme humor.',
    content: [
      {
        type: 'text',
        text: `**MEMElang** is a dynamically typed, interpreted programming language written in clean C++20 and compiled to WebAssembly. It blends genuine modern language features (lexical closures, recursive descent parser with precedence climbing, class-based OOP with single inheritance, dynamic field binding, and short-circuit boolean evaluation) with unfiltered Desi internet culture and Bollywood meme energy.`
      },
      {
        type: 'callout',
        variant: 'info',
        title: 'Core Philosophy: No Cap, Pure Vibe',
        text: `Every programming error message you've ever hated ("Segmentation fault", "TypeError: undefined is not a function", "SyntaxError: Unexpected token") has been replaced with iconic dialogues from Hera Pheri, Shark Tank India, CID, Gangs of Wasseypur, and Indian meme culture.`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Language Feature Highlights'
      },
      {
        type: 'list',
        items: [
          '**Zero Dependencies Runtime**: Tree-walk interpreter built from scratch in C++20.',
          '**100% WebAssembly Compatible**: Compiles via Emscripten and runs natively in any browser with zero server roundtrips.',
          '**Full Object-Oriented Programming**: `biradari` classes, `nepo` inheritance, `apun` self-reference, and automatic `shuru` constructors.',
          '**Functional Goodies**: First-class functions (`jugaad`), lexical closures, and recursion.',
          '**Desi Arithmetic**: Built-in string multiplication (`"ha" * 5` -> `"hahahahaha"`), safe division, and 64-bit integer overflow checks.',
          '**Humorous Diagnostic Engine**: Every syntax and runtime trap yields an iconic meme error message instead of cryptic stack traces.'
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'First Taste: Hello MEMElang'
      },
      {
        type: 'code',
        title: 'hello_world.meme',
        code: `// Welcome to MEMElang!
pakad naam = "MEMElang";
dikha "Welcome to", naam;
dikha "Hasi ruki nahi: " + ("ha" * 6);`
      }
    ]
  },
  {
    id: 'quickstart',
    title: 'Quickstart & Installation',
    badge: 'Getting Started',
    description: 'How to install and run MEMElang on macOS, Linux, or directly in your browser.',
    content: [
      {
        type: 'text',
        text: `You can use MEMElang in three ways: natively via the CLI binary, interactively in the REPL, or right here in the web browser through WebAssembly.`
      },
      {
        type: 'heading',
        level: 2,
        text: '1. Build & Run Native CLI'
      },
      {
        type: 'text',
        text: `Clone the repository and compile using any C++20 compiler (` + '`clang++` or `g++`' + `):`
      },
      {
        type: 'terminal',
        commands: [
          'git clone https://github.com/your-username/memelang.git',
          'cd memelang',
          'make',
          './meme examples/influence.meme'
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '2. Interactive REPL'
      },
      {
        type: 'text',
        text: `Launch the interactive shell by running ` + '`./meme`' + ` with no arguments:`
      },
      {
        type: 'terminal',
        commands: [
          './meme',
          'meme > pakad swag = 100;',
          'meme > dikha swag * 2;',
          '200',
          'meme > bas;'
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '3. Run Inline Expressions from Terminal'
      },
      {
        type: 'terminal',
        commands: [
          './meme -e \'pakad a = 10; pakad b = 32; dikha "Jod:", a + b;\''
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '4. Global CLI Setup & Project Scaffolding'
      },
      {
        type: 'text',
        text: `Link the binary globally to run ` + '`meme`' + ` from any directory, then scaffold modern apps with built-in templates:`
      },
      {
        type: 'terminal',
        commands: [
          './meme setup',
          'meme naya my-app --template=react',
          'cd my-app'
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '5. Multiplatform Dev Server & Test Suite'
      },
      {
        type: 'text',
        text: `Run your application across platforms (Zero-WebView native macOS desktop, browser, or terminal) with instant hot-reload:`
      },
      {
        type: 'terminal',
        commands: [
          'meme dev src/App.meme --platform=desktop',
          'meme test tests'
        ]
      }
    ]
  },
  {
    id: 'variables',
    title: 'Variables & Lexical Scope',
    badge: 'Basics',
    description: 'Declaring variables with pakad, mutating values, and lexical scope rules.',
    content: [
      {
        type: 'text',
        text: `Variables in MEMElang are declared using the keyword ` + '`pakad`' + ` (or the tolerated alias ` + '`le`' + `). Uninitialized variables default to ` + '`nalla`' + ` (null).`
      },
      {
        type: 'code',
        title: 'variables.meme',
        code: `// Declare variables
pakad balance = 500;
pakad user = "Baburao";
pakad khali_variable; // Evaluates to 'nalla'

// Reassignment
balance = balance + 250;
dikha "User:", user, "| Paisa:", balance;
dikha "Khali kya hai?", khali_variable;`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Block Scoping'
      },
      {
        type: 'text',
        text: `MEMElang uses block scoping via braces ` + '`{ ... }`' + `. Inner blocks shadow outer declarations and can read/write enclosing scopes seamlessly:`
      },
      {
        type: 'code',
        title: 'scoping.meme',
        code: `pakad x = "global";
{
    pakad x = "local";
    dikha "Inside block:", x;
}
dikha "Outside block:", x;`
      },
      {
        type: 'callout',
        variant: 'warning',
        title: 'Undeclared Variable Errors',
        text: `Reading an undeclared variable triggers: **"Rasode mein kaun tha?! Ye '<name>' bina bataye kahan se aaya? Pehle 'pakad <name>;' se declare kar!"**`
      }
    ]
  },
  {
    id: 'types',
    title: 'Data Types & Literals',
    badge: 'Types',
    description: 'Dynamic typing, numbers, strings with repetition, booleans, and null.',
    content: [
      {
        type: 'text',
        text: `MEMElang is dynamically typed. Every value belongs to one of the following fundamental types:`
      },
      {
        type: 'table',
        headers: ['Type', 'Literal Example', 'Truthy / Falsy', 'Notes'],
        rows: [
          ['byte', '255b, 0xffb, 0b1010b', '0 is falsy, others truthy', '8-bit unsigned integer (`uint8_t`) with literal suffixes'],
          ['int32', 'int32(42), ank32(100)', '0 is falsy, others truthy', '32-bit fast integer ALU path with auto-promotion on overflow'],
          ['int / long', '42, -100, 0x1F, 0b101', '0 is falsy, others truthy', '64-bit signed integer (`long long`)'],
          ['float32', '3.14f, -0.5f', '0.0 is falsy, others truthy', '32-bit single-precision IEEE 754 float (`float`)'],
          ['float / double', '3.14, -0.5, 1e-4', '0.0 is falsy, others truthy', '64-bit double-precision IEEE 754 float (`double`)'],
          ['char', 'char("Namaste"), \'A\'', 'Empty or \'\\0\' is falsy', 'Single character primitive (`char`)'],
          ['string', '"namaste", \'bro\'', 'Empty "" is falsy', 'Supports string multiplication `"ha" * 3`'],
          ['bytes', 'b"Hello\\x00World"', 'Empty buffer is falsy', 'Raw binary buffer with hex decode, mutation & binary I/O'],
          ['set', '{1, 2, 3}, { "a", "b" }', 'Empty set() is falsy', 'Order-preserving O(1) hash set with union `+` & diff `-`'],
          ['tuple', '(1, "two", 3.0), (1,)', 'Empty () is falsy', 'Fixed-size immutable sequence with indexing and slicing'],
          ['list', '[1, 2, 3], ["a", "b"]', 'Empty [] is falsy', 'Dynamic growable list with spread, push, and pop'],
          ['map', '{"key": "value"}', 'Empty {} is falsy', 'Key-value hash map with destructuring and spread'],
          ['bool', 'real / han, cap / nhi', 'real is truthy, cap is falsy', 'Gen-Z: `real`/`cap`, Desi: `han`/`nhi`'],
          ['nalla', 'nalla / kuch_nhi', 'Always falsy', 'Null value ("unemployed / empty")'],
          ['function', 'jugaad add(a, b) { ... }', 'Always truthy', 'First-class callable entity'],
          ['object', 'Influencer("Orry")', 'Always truthy', 'Instance of a `biradari` class']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Extended Primitives: bytes, set & tuple'
      },
      {
        type: 'code',
        title: 'extended_primitives.meme',
        code: `// 1. Raw Bytes Buffer
pakad buf = b"Hello";
buf[0] = 104; // mutable: 'h'
dikha buf.to_string(); // "hello"
dikha buf.to_hex();    // "68656c6c6f"

// Static hex parsing & binary I/O
pakad decoded = bytes.from_hex("43686169"); // "Chai"
fs.write_bytes("drink.bin", decoded);
pakad readBack = fs.read_bytes("drink.bin");

// 2. Set with Deduplication & Algebraic Operations
pakad s = {1, 2, 2, 3}; // size: 3
s.add(4);
dikha s.has(2); // han
pakad unionSet = s + {4, 5, 6};
pakad diffSet = unionSet - {1, 2};

// 3. Tuples (Immutable Sequences)
pakad coords = (10, 20, 30);
dikha coords[0]; // 10
panga {
    coords[0] = 99; // Throws error: tuples are immutable!
} lafda (err) {
    dikha "Tuple cannot be modified!";
}`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Interactive Type Inspection with kya_hai()'
      },
      {
        type: 'code',
        title: 'types.meme',
        code: `dikha kya_hai(255b);     // byte
dikha kya_hai(int32(42)); // int32
dikha kya_hai(100);       // int
dikha kya_hai(3.14f);     // float32
dikha kya_hai(3.14);      // float
dikha kya_hai(char("A")); // char
dikha kya_hai(b"Data");   // bytes
dikha kya_hai({1, 2, 3}); // set
dikha kya_hai((1, 2, 3)); // tuple
dikha kya_hai("Sholay");  // string
dikha kya_hai(real);      // bool
dikha kya_hai(nalla);     // nalla`
      }
    ]
  },
  {
    id: 'operators',
    title: 'Operators & Short-Circuit Logic',
    badge: 'Operators',
    description: 'Arithmetic, comparison, logical short-circuiting, and string multiplication.',
    content: [
      {
        type: 'heading',
        level: 2,
        text: 'Arithmetic Operators'
      },
      {
        type: 'table',
        headers: ['Operator', 'Name', 'Example', 'Result'],
        rows: [
          ['+', 'Addition / Concatenation', '10 + 20 or "Paisa " + "Double"', '30 or "Paisa Double"'],
          ['-', 'Subtraction', '100 - 35', '65'],
          ['*', 'Multiplication / Repetition', '"Yo " * 3', '"Yo Yo Yo "'],
          ['/', 'Division (Safe)', '10 / 2', '5'],
          ['%', 'Modulo', '17 % 5', '2']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Logical Operators with Short-Circuiting'
      },
      {
        type: 'text',
        text: `Logical operators ` + '`aur`' + ` (AND) and ` + '`ya`' + ` (OR) perform short-circuit evaluation. If the left operand determines the outcome, the right operand is completely skipped:`
      },
      {
        type: 'code',
        title: 'short_circuit.meme',
        code: `// Safe short-circuit: Division by zero on right is never evaluated!
pakad safe_and = cap aur (10 / 0);
pakad safe_or = real ya (10 / 0);

dikha "Safe AND result:", safe_and;
dikha "Safe OR result:", safe_or;`
      }
    ]
  },
  {
    id: 'control-flow',
    title: 'Control Flow: dekh, warna, jabtak, ghuma',
    badge: 'Logic & Loops',
    description: 'Conditional statements and loop constructs with break and continue.',
    content: [
      {
        type: 'heading',
        level: 2,
        text: 'Conditionals: dekh & warna'
      },
      {
        type: 'code',
        title: 'conditionals.meme',
        code: `pakad score = 88;

dekh score >= 90 {
    dikha "Full marks! Sharma ji ke ladke jaisa.";
} warna dekh score >= 60 {
    dikha "First division! Thoda aur jugaad laga lete.";
} warna {
    dikha "Moye moye! Agli baar padh lena.";
}`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Loops: jabtak (While Loop)'
      },
      {
        type: 'code',
        title: 'while_loop.meme',
        code: `pakad count = 1;
jabtak count <= 5 {
    dikha "Count hai:", count;
    count = count + 1;
}`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Range Loops: ghuma (For Loop)'
      },
      {
        type: 'text',
        text: `Spin loops over integer ranges using ` + '`ghuma <var> se <start> se <end> [se <step>]`' + `:`
      },
      {
        type: 'code',
        title: 'range_loop.meme',
        code: `// Spin from 1 to 10 with step 2
ghuma i se 1 se 10 se 2 {
    dekh i == 5 {
        dikha "Mil gaya 5, aage badho!";
        aage_badh; // continue
    }
    dikha "Step number:", i;
}`
      }
    ]
  },
  {
    id: 'collections',
    title: 'Native Collections (Lists & Hash Maps)',
    badge: 'Data Structures',
    description: 'Dynamic Arrays (Lists `[]`), Hash Maps (`{}`), negative indexing, and `ghuma ... mein` iteration.',
    content: [
      {
        type: 'text',
        text: `MEMElang features native high-performance Collections with reference semantics (` + '`std::shared_ptr`' + `). Lists and Maps are mutable by default and changes persist across references.`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Dynamic Arrays (Lists)'
      },
      {
        type: 'text',
        text: `Define arrays with square brackets ` + '`[elem1, elem2, ...]`' + `. Arrays support positive (0-based) and negative indexing (` + '`arr[-1]`' + ` returns the last element).`
      },
      {
        type: 'code',
        title: 'arrays_demo.meme',
        code: `// Array literals & indexing
pakad gangs = ["Sardar", "Faizal", "Definite"];
dikha gangs[0];   // Sardar
dikha gangs[-1];  // Definite (negative indexing)

// Array mutation & index assignment
gangs[1] = "Perpendicular";
dikha gangs; // ["Sardar", "Perpendicular", "Definite"]

// Built-in Array methods
gangs.ghusa("Tangent");       // push element
pakad out = gangs.laat_maar(); // pop last element ("Tangent")
dikha gangs.hai_kya("Faizal"); // nhi
dikha gangs.size();           // 3
dikha gangs.chipka(" -> ");    // "Sardar -> Perpendicular -> Definite"`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Hash Maps (Dicts / Khata)'
      },
      {
        type: 'text',
        text: `Define key-value maps with curly braces ` + '`{ key: value, ... }`' + `. Access properties with bracket indexing ` + '`map["key"]`' + ` or object dot notation ` + '`map.key`' + `.`
      },
      {
        type: 'code',
        title: 'maps_demo.meme',
        code: `// Map literal
pakad villain = {
    "naam": "Mogambo",
    "dialogue": "Mogambo khush hua!",
    "power": 9000
};

// Bracket & dot notation access
dikha villain["naam"];      // Mogambo
dikha villain.dialogue;     // Mogambo khush hua!

// Map methods
dikha villain.chaabi();     // ["naam", "dialogue", "power"]
dikha villain.maal();       // ["Mogambo", "Mogambo khush hua!", 9000]
dikha villain.hai_kya("power"); // han
villain.hata("power");      // delete key`
      },
      {
        type: 'heading',
        level: 2,
        text: 'For-Each Iteration (ghuma ... mein)'
      },
      {
        type: 'text',
        text: `Iterate cleanly over Lists, Maps, or Strings using ` + '`ghuma item mein collection { ... }`' + `:`
      },
      {
        type: 'code',
        title: 'for_each_demo.meme',
        code: `// Iterate over array elements
ghuma role mein ["Hero", "Villain", "Comedian"] {
    dikha "Role:", role;
}

// Iterate over characters of a string
ghuma ch mein "MEME" {
    dikha ch;
}

// Iterate over keys of a map
pakad scores = {"Munna": 95, "Guddu": 88};
ghuma student mein scores {
    dikha student, "scored:", scores[student];
}`
      }
    ]
  },
  {
    id: 'pattern-matching',
    title: 'Enums (avtar) & Pattern Matching (jaanch)',
    badge: 'Control Flow',
    description: 'Type-safe Enums (`avtar`) and pattern matching (`jaanch`) with fallback guards (`baaki` / `warna`).',
    content: [
      {
        type: 'text',
        text: `Define type-safe enumeration constants with ` + '`avtar`' + ` (or ` + '`kism`' + `) and dispatch cleanly with ` + '`jaanch`' + ` (or ` + '`parakh`' + `) pattern matching.`
      },
      {
        type: 'code',
        title: 'enums_and_matching.meme',
        code: `// Define an Enum
avtar PaymentStatus {
    Pending,
    Approved,
    Rejected
}

pakad current = PaymentStatus.Approved;

// Pattern match with jaanch
jaanch current {
    PaymentStatus.Pending => {
        dikha "Paisa raste mein hai, intezaar karo!";
    }
    PaymentStatus.Approved => {
        dikha "Party shuru! Payment ho gaya!";
    }
    PaymentStatus.Rejected => {
        dikha "Khatam, tata, bye-bye! Payment reject!";
    }
    baaki => {
        dikha "Aayein? Ye kaisa status hai!";
    }
}`
      }
    ]
  },
  {
    id: 'functions',
    title: 'Functions (jugaad) & Closures',
    badge: 'Functions',
    description: 'Writing functions, recursive algorithms, and lexical environment capture.',
    content: [
      {
        type: 'text',
        text: `Every function in MEMElang is a ` + '`jugaad`' + `. Functions return values via ` + '`ye_le`' + ` (or ` + '`wapas`' + `). Functions are first-class citizens and capture their lexical scope:`
      },
      {
        type: 'code',
        title: 'functions.meme',
        code: `// Function declaration
jugaad jod(a, b) {
    ye_le a + b;
}

pakad ans = jod(15, 27);
dikha "15 + 27 =", ans;`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Recursion: Factorial'
      },
      {
        type: 'code',
        title: 'factorial.meme',
        code: `jugaad factorial(n) {
    dekh n <= 1 {
        ye_le 1;
    }
    ye_le n * factorial(n - 1);
}

dikha "Factorial of 6 is:", factorial(6);`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Lexical Closures'
      },
      {
        type: 'code',
        title: 'closures.meme',
        code: `jugaad createMultiplier(multiplier) {
    jugaad mult(val) {
        ye_le val * multiplier;
    }
    ye_le mult;
}

pakad triple = createMultiplier(3);
dikha "Triple of 9 is:", triple(9);`
      }
    ]
  },
  {
    id: 'oop',
    title: 'Object-Oriented Programming (biradari)',
    badge: 'OOP',
    description: 'Classes, inheritance with nepo, apun self-reference, and constructor shuru.',
    content: [
      {
        type: 'text',
        text: `MEMElang brings Desi culture to Object-Oriented Programming. Blueprints are defined with ` + '`biradari`' + `, inheritance uses ` + '`nepo`' + `, instances refer to themselves via ` + '`apun`' + `, and constructors are initialized with ` + '`shuru`' + `.`
      },
      {
        type: 'code',
        title: 'oop_inheritance.meme',
        code: `// Base community class
biradari AmeeroKiAulaad {
    jugaad shuru(naam, bank_balance) {
        apun.naam = naam;
        apun.balance = point_wala(bank_balance);
    }

    jugaad kothi_bangle_check() {
        dikha apun.naam, "ke paas hai Rs.", apun.balance;
    }
}

// Inherit using 'nepo'
biradari Influencer nepo AmeeroKiAulaad {
    jugaad show_swag() {
        dekh apun.balance > 50000.0 {
            dikha apun.naam, "says: Guys, welcome to my humble struggle vlog!";
            ye_le real;
        } warna {
            dikha "Paisa khatam, camera band!";
            ye_le cap;
        }
    }
}

pakad star = Influencer("Orry", "99999.50");
star.kothi_bangle_check(); // Inherited from AmeeroKiAulaad
star.show_swag();          // Defined on Influencer`
      }
    ]
  },
  {
    id: 'error-handling',
    title: 'Structured try / catch / finally / throw (panga / lafda)',
    badge: 'Universal Aliases & Structured Errors',
    description: 'Structured Error objects with .baat/.message, .kism/.type, .line, Galti() constructor, and full Python/JS keyword aliases.',
    content: [
      {
        type: 'text',
        text: `MEMElang provides robust, structured exception handling with full keyword aliases across WhatsApp Hinglish, Python, and JavaScript/TypeScript. Caught errors are rich Error objects with full backward compatibility (printing cleanly in ` + '`dikha`' + ` and supporting string methods).`
      },
      {
        type: 'table',
        headers: ['WhatsApp Hinglish', 'Universal Alias', 'Role / Purpose'],
        rows: [
          ['panga { ... }', 'try { ... } / koshish', 'Block of code that might fail or throw an exception'],
          ['lafda (err) { ... }', 'catch (err) { ... } / except (err)', 'Catches structured errors or runtime exceptions (optional parens or bare)'],
          ['pakka { ... }', 'finally { ... } / hamesha', 'Guaranteed execution block, runs whether an error occurred or not'],
          ['chillao expr', 'throw expr / raise expr / fek', 'Throws custom error string, Map, or Galti(msg, type) instance']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Structured Error Object Properties'
      },
      {
        type: 'table',
        headers: ['Property', 'Universal Alias', 'Description', 'Example'],
        rows: [
          ['err.baat', 'err.message', 'Human-readable error description string', '"Something went wrong"'],
          ['err.kism', 'err.type', 'Categorized error category', '"NetworkLafda", "FileLafda", "MathLafda", "UserLafda"'],
          ['err.line', 'err.line', 'Source code line number where exception was raised', '42']
        ]
      },
      {
        type: 'code',
        title: 'structured_errors.meme',
        code: `// Throwing structured error with Galti / Error
panga {
    chillao Galti("Database connection timed out", "NetworkLafda");
} lafda (err) {
    dikha "Caught error message:", err.baat;      // or err.message
    dikha "Error category:", err.kism;            // or err.type
    dikha "Source line:", err.line;
    dikha "Direct print:", err;                   // Formats cleanly as plain message string
} pakka {
    dikha "Connection cleaned up!";
}

// Full Python / JS keyword parity
try {
    raise Error("Invalid payload schema", "TypeLafda");
} catch (e) {
    dikha f"Type error: {e.message} (kind: {e.type})";
}`
      }
    ]
  },
  {
    id: 'async-await-promises',
    title: 'Async / Await (sabar) & Promises (Vaada)',
    badge: 'True Asynchrony',
    description: 'Non-blocking async task scheduling backed by C++ worker threads and Promises with Promise.all and Promise.race.',
    content: [
      {
        type: 'text',
        text: `MEMElang delivers non-blocking asynchronous execution with standard ` + '`async / await`' + ` semantics and WhatsApp Hinglish aliases (` + '`sabar`' + ` / ` + '`side_mein kaam`' + `). Async functions automatically return a ` + '`Vaada`' + ` (` + '`Promise`' + `).`
      },
      {
        type: 'table',
        headers: ['WhatsApp Hinglish', 'Universal Alias', 'Description'],
        rows: [
          ['async kaam fn() / side_mein kaam fn()', 'async def fn() / async fn()', 'Declares an asynchronous function returning a Promise'],
          ['sabar expr', 'await expr', 'Awaits a Promise, Thread handle, or async fetch request'],
          ['Vaada.de(val)', 'Promise.resolve(val)', 'Creates an instantly resolved Promise'],
          ['Vaada.radh(err)', 'Promise.reject(err)', 'Creates an instantly rejected Promise'],
          ['Vaada.sab([p1, p2])', 'Promise.all([p1, p2])', 'Runs promises in parallel and resolves to a list of all results'],
          ['Vaada.race([p1, p2])', 'Promise.race([p1, p2])', 'Resolves or rejects as soon as the first promise settles'],
          ['Vaada.ruk(ms)', 'Promise.sleep(ms)', 'Non-blocking asynchronous delay in milliseconds']
        ]
      },
      {
        type: 'code',
        title: 'async_demo.meme',
        code: `// 1. Asynchronous functions
async kaam fetchUserData(id) {
    sabar Vaada.ruk(50); // non-blocking 50ms pause
    ye_le {"id": id, "name": "Raju", "role": "Dev"};
}

// 2. Awaiting Promises with 'sabar' or 'await'
pakad user = sabar fetchUserData(101);
dikha f"Fetched user: {user.name} ({user.role})";

// 3. Parallel concurrency with Promise.all (Vaada.sab)
pakad p1 = fetchUserData(1);
pakad p2 = fetchUserData(2);
pakad allUsers = sabar Promise.all([p1, p2]);
dikha f"Total loaded users: {allUsers.lambai()}";

// 4. Promise method chaining (.then / .phir and .catch / .pakad)
fetchUserData(99)
    .then(u => dikha f"Chained user: {u.name}")
    .catch(err => dikha f"Lafda: {err.message}");`
      }
    ]
  },
  {
    id: 'fetch-http-client',
    title: 'Full-Featured fetch() & jaal (http) Client',
    badge: 'HTTP & Networking',
    description: 'REST client supporting GET, POST, PUT, PATCH, DELETE, custom headers, query params, Bearer auth, timeouts, and rich Response objects.',
    content: [
      {
        type: 'text',
        text: `The global ` + '`fetch(url, options)`' + ` (alias ` + '`mangwa`' + `) and ` + '`jaal`' + ` (alias ` + '`http`' + `) client provide native HTTP networking with Bearer authentication, auto-serialized JSON bodies, query parameter encoding, and deterministic 0ms offline mock testing via ` + '`mock://`' + ` URLs.`
      },
      {
        type: 'table',
        headers: ['Option / Property', 'Type', 'Description'],
        rows: [
          ['method', 'string', 'HTTP verb: "GET", "POST", "PUT", "PATCH", "DELETE", "HEAD", "OPTIONS" (default: "GET")'],
          ['headers', 'Map', 'Custom HTTP headers map (e.g. {"Content-Type": "application/json", "X-Custom": "val"})'],
          ['params', 'Map', 'Query parameters auto-encoded into ?k1=v1&k2=v2 URL query string'],
          ['auth / token', 'string', 'Bearer authorization token auto-injected into Authorization: Bearer <token>'],
          ['body / json', 'Map / List / string', 'Request payload (maps and lists are auto-serialized to JSON string)'],
          ['timeout', 'number', 'Timeout duration in milliseconds']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Response Object Properties & Methods'
      },
      {
        type: 'table',
        headers: ['Member', 'Type', 'Description'],
        rows: [
          ['res.status', 'int', 'HTTP status code (e.g. 200, 201, 404, 500)'],
          ['res.ok / res.sahi', 'bool', 'True if status code is in 200..299 range'],
          ['res.url', 'string', 'Final requested URL including encoded query parameters'],
          ['res.method', 'string', 'HTTP request method utilized'],
          ['res.headers', 'Map', 'Map of response headers'],
          ['res.body / res.raw', 'string', 'Raw response body text'],
          ['res.data / res.maal', 'Value', 'Parsed JSON map/list if body is JSON, otherwise raw string'],
          ['res.time_ms', 'int', 'Elapsed request duration in milliseconds'],
          ['res.json()', 'Callable', 'Method parsing response body into a Map / List'],
          ['res.text()', 'Callable', 'Method returning raw string body'],
          ['res.raise_for_status()', 'Callable', 'Throws a structured NetworkLafda if res.ok is false']
        ]
      },
      {
        type: 'code',
        title: 'http_fetch_demo.meme',
        code: `// Global fetch with POST, query params, Bearer auth, and JSON body
pakad res = sabar fetch("https://api.example.com/items", {
    "method": "POST",
    "params": {"category": "chai", "page": 1},
    "token": "secret_bearer_token_123",
    "body": {"name": "Masala Chai", "price": 20},
    "timeout": 5000
});

dekh res.ok {
    pakad item = res.json();
    dikha f"Item added in {res.time_ms}ms: {item.name}";
} warna {
    dikha f"Request failed with status {res.status}";
}

// Clean shorthand methods on jaal / http
pakad items = sabar jaal.get("mock://items", {"params": {"q": "chai"}});
pakad updated = sabar jaal.put("mock://items/1", {"price": 25});
pakad patched = sabar jaal.patch("mock://items/1", {"stock": 100});
pakad deleted = sabar jaal.delete("mock://items/1");`
      }
    ]
  },
  {
    id: 'concurrency-threads-mp',
    title: 'Multi-Threading (dhaga) & Multi-Processing (kaamgar)',
    badge: 'Hardware Concurrency',
    description: 'Hardware threads (dhaga.chala), Mutex locks (dhaga.tala), Go channels (dhaga.naali), parallel work-stealing pool (mp.map), and OS child processes (mp.spawn).',
    content: [
      {
        type: 'text',
        text: `MEMElang provides first-class primitives for hardware concurrency. Multi-threading is exposed via ` + '`dhaga`' + ` (` + '`thread`' + `), thread synchronization via recursive Mutex locks and Go-style channels, parallel data processing via ` + '`kaamgar`' + ` (` + '`mp`' + `), and OS subprocess execution.`
      },
      {
        type: 'table',
        headers: ['WhatsApp Hinglish', 'Universal Alias', 'Capability'],
        rows: [
          ['dhaga.chala(fn, ...args)', 'thread.spawn(fn, ...args)', 'Spawns an isolated hardware thread running fn with args'],
          ['t.jod() / sabar t', 't.join() / await t', 'Waits for thread execution and returns its result'],
          ['dhaga.sab([fn1, fn2])', 'thread.all([fn1, fn2])', 'Spawns and joins multiple threads concurrently, returning results list'],
          ['dhaga.tala()', 'thread.mutex()', 'Recursive mutex with .band() (lock), .khol() (unlock), and .surakshit(fn) (with_lock)'],
          ['dhaga.naali(capacity?)', 'thread.channel(capacity?)', 'Go-style channel with .bhej(val) (send), .le() (recv), and .band()'],
          ['dhaga.cores()', 'thread.cores()', 'Returns number of CPU hardware execution cores'],
          ['kaamgar.sab_pe(fn, list, cores?)', 'mp.map(fn, list, workers?)', 'Parallel work-stealing pool mapping across CPU cores'],
          ['kaamgar.alag(cmd, args?)', 'mp.spawn(cmd, args?)', 'Spawns OS child process with .ruk() (wait), .code(), and .out() (stdout)']
        ]
      },
      {
        type: 'code',
        title: 'concurrency_showcase.meme',
        code: `// 1. Spawning OS threads
pakad t = dhaga.chala((a, b) => a * b, 6, 7);
pakad product = sabar t; // or t.jod()
dikha f"Thread product: {product}"; // 42

// 2. Thread-safe Mutex lock
pakad lock = dhaga.tala();
pakad sharedCounter = 0;
lock.surakshit(() => {
    sharedCounter = sharedCounter + 1;
});

// 3. Go-style channel message passing
pakad ch = dhaga.naali();
dhaga.chala(() => {
    ch.bhej("packet_from_worker");
});
pakad msg = ch.le();
dikha f"Received channel message: {msg}";

// 4. Parallel work-stealing map across CPU cores
pakad numbers = [10, 20, 30, 40, 50];
pakad squares = mp.map(x => x * x, numbers);
dikha f"Parallel squares: {squares}";

// 5. Child processes
pakad proc = mp.spawn("echo 'Child process execution'");
pakad exitCode = proc.ruk();
dikha f"Process code {exitCode}: {proc.out()}";`
      }
    ]
  },
  {
    id: 'filesystem-paths',
    title: 'File System (file / fs) & Paths (rasta / path)',
    badge: 'File Management',
    description: 'Complete file operations, CSV table parsing/writing, JSON storage, recursive directory walking, and cross-platform path helpers.',
    content: [
      {
        type: 'text',
        text: `The ` + '`file`' + ` (` + '`fs`' + `) and ` + '`rasta`' + ` (` + '`path`' + `) standard libraries offer full file system management, automated CSV reading and writing into Lists of Maps, JSON serialization, recursive file discovery (glob), and cross-platform path manipulation.`
      },
      {
        type: 'table',
        headers: ['Category', 'WhatsApp Hinglish', 'Universal Alias', 'Description'],
        rows: [
          ['Read / Write', 'file.padh(p) / file.likh(p, s)', 'fs.read / fs.write', 'Reads full text file or overwrites file contents'],
          ['Append / Lines', 'file.jod(p, s) / file.lines(p)', 'fs.append / fs.read_lines', 'Appends text or reads file into a List of line strings'],
          ['JSON Storage', 'file.padh_json(p) / file.likh_json(p, v)', 'fs.read_json / fs.write_json', 'Reads and writes JSON structured files automatically'],
          ['CSV Engine', 'file.padh_csv(p) / file.likh_csv(p, rows)', 'fs.read_csv / fs.write_csv', 'Parses CSV into a List of Maps [{"col": val}] and writes back'],
          ['Copy / Move / Del', 'file.nakal(a, b) / file.hila(a, b)', 'fs.copy / fs.move / fs.remove', 'Recursively copies, renames/moves, or deletes files & directories'],
          ['Stats & Meta', 'file.kundli(p)', 'fs.stat(p)', 'Returns {"size", "is_file", "is_dir", "modified", "ext"} map'],
          ['Search & Glob', 'file.khoj(dir, "*.meme")', 'fs.glob(dir, pattern)', 'Recursively walks directories matching wildcard pattern'],
          ['Path Resolution', 'rasta.jod(a, b) / rasta.asli(p)', 'path.join / path.resolve', 'Joins path components or resolves canonical absolute path']
        ]
      },
      {
        type: 'code',
        title: 'fs_path_demo.meme',
        code: `// 1. Reading and writing text files
fs.write("notes.txt", "Pehli line\\nDusri line\\n");
pakad lines = fs.read_lines("notes.txt");
dikha f"Total lines: {lines.lambai()}";

// 2. CSV parsing into List of Maps
pakad staff = [
    {"id": "1", "name": "Raju", "role": "Fullstack Dev"},
    {"id": "2", "name": "Baburao", "role": "Team Lead"}
];
fs.write_csv("staff.csv", staff);
pakad loadedStaff = fs.read_csv("staff.csv");
dikha f"Staff member: {loadedStaff[0].name} ({loadedStaff[0].role})";

// 3. File statistics
pakad stat = fs.stat("staff.csv");
dikha f"File size: {stat.size} bytes, Modified: {stat.modified}";

// 4. Wildcard globbing
pakad memeFiles = fs.glob(".", "*.meme");
dikha f"Found meme scripts: {memeFiles.lambai()}";

// 5. Cross-platform path helpers
pakad full = path.join("workspace", "src", "main.meme");
dikha f"Basename: {path.basename(full)}"; // main.meme
dikha f"Stem: {path.stem(full)}";         // main
dikha f"Extension: {path.ext(full)}";      // .meme
dikha f"Dirname: {path.dirname(full)}";   // workspace/src

// Clean up
fs.remove("notes.txt");
fs.remove("staff.csv");`
      }
    ]
  },
  {
    id: 'modules',
    title: 'Module & File Imports (bula / jaise)',
    badge: 'Architecture',
    description: 'Multi-file modular architecture with `bula` and `jaise`, canonical caching, and circular import detection.',
    content: [
      {
        type: 'text',
        text: `Organize your project into multiple files using ` + '`bula "<path>" [jaise <alias>];`' + `. Modules run in isolated namespaces and export top-level variables, functions, and classes.`
      },
      {
        type: 'code',
        title: 'imports_demo.meme',
        code: `// Import with alias
bula "./math_utils.meme" jaise mu;
dikha mu.add(10, 20);

// Import without alias (auto-derives stem 'helpers')
bula "./helpers.meme";
dikha helpers.formatText("namaste");

// Circular import protection:
// If a imports b and b imports a, MEMElang aborts with:
// "Golmaal hai sab golmaal hai! Circular import pakda gaya: '<path>'"`
      }
    ]
  },
  {
    id: 'stdlib',
    title: 'Built-in Standard Library (hisab, waqt, json, file)',
    badge: 'Standard Library',
    description: 'Comprehensive built-in modules: `hisab` (Math), `waqt` (Time), `json` (Serialization), and `file` (File I/O).',
    content: [
      {
        type: 'text',
        text: `MEMElang includes built-in standard library modules directly available globally or via ` + '`bula`' + `:`
      },
      {
        type: 'heading',
        level: 2,
        text: '1. hisab (Math Module)'
      },
      {
        type: 'table',
        headers: ['Member', 'Description', 'Example'],
        rows: [
          ['hisab.pi', 'Mathematical constant pi (3.14159...)', 'hisab.pi'],
          ['hisab.e', 'Euler\'s constant e (2.71828...)', 'hisab.e'],
          ['hisab.gol(x)', 'Rounds x to nearest integer', 'hisab.gol(3.7) -> 4'],
          ['hisab.niche(x)', 'Floors x to integer', 'hisab.niche(3.9) -> 3'],
          ['hisab.upar(x)', 'Ceils x to integer', 'hisab.upar(3.1) -> 4'],
          ['hisab.jadd(x)', 'Square root of x', 'hisab.jadd(16) -> 4.0'],
          ['hisab.taqat(b, exp)', 'Power base^exp', 'hisab.taqat(2, 3) -> 8.0'],
          ['hisab.halka(a, b)', 'Minimum of a and b', 'hisab.halka(10, 20) -> 10'],
          ['hisab.bhaari(a, b)', 'Maximum of a and b', 'hisab.bhaari(10, 20) -> 20'],
          ['hisab.positive(x)', 'Absolute value |x|', 'hisab.positive(-42) -> 42'],
          ['hisab.tukka()', 'Random float in [0.0, 1.0)', 'hisab.tukka()'],
          ['hisab.tukka_num(min, max)', 'Random integer in [min, max]', 'hisab.tukka_num(1, 100)']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '2. waqt & so_ja (Time & Sleep)'
      },
      {
        type: 'code',
        title: 'time_demo.meme',
        code: `pakad t1 = waqt.ab(); // Unix timestamp in seconds
so_ja(0.1);           // Sleep for 100ms
pakad elapsed = waqt.ab() - t1;
dikha "Elapsed time:", elapsed;`
      },
      {
        type: 'heading',
        level: 2,
        text: '3. json (Serialization & Parsing)'
      },
      {
        type: 'code',
        title: 'json_demo.meme',
        code: `pakad hero = {
    "name": "Shaktimaan",
    "powers": ["Fly", "Spin", "Laser"],
    "alter_ego": "Pandit Gangadhar"
};

// Stringify to JSON
pakad serialized = json.likh(hero);
dikha "JSON String:", serialized;

// Parse back into MEMElang Objects & Lists
pakad parsed = json.samajh(serialized);
dikha "Powers count:", parsed.powers.size();`
      },
      {
        type: 'heading',
        level: 2,
        text: '4. file (File System I/O)'
      },
      {
        type: 'code',
        title: 'file_demo.meme',
        code: `// Check existence
dekh file.hai_kya("data.txt") {
    dikha "File content:", file.padh("data.txt");
} warna {
    file.likh("data.txt", "Initial entry\\n");
    file.jod("data.txt", "Appended log entry\\n");
    dikha "Created and wrote to data.txt!";
}`
      },
      {
        type: 'heading',
        level: 2,
        text: '5. systumm (OS & Process Control)'
      },
      {
        type: 'table',
        headers: ['Member / Method', 'English Alias', 'Description', 'Example'],
        rows: [
          ['systumm.chala(cmd)', 'sys.exec(cmd)', 'Executes shell command and returns output', 'systumm.chala("git status")'],
          ['systumm.env(key, def?)', 'os.env(key, def)', 'Reads environment variable', 'systumm.env("HOME")'],
          ['systumm.set_env(k, v)', 'os.set_env(k, v)', 'Sets environment variable', 'systumm.set_env("PORT", "8080")'],
          ['systumm.kahan()', 'os.cwd()', 'Returns current working directory', 'systumm.kahan()'],
          ['systumm.jao(path)', 'os.chdir(path)', 'Changes directory', 'systumm.jao("../")'],
          ['systumm.kaun()', 'os.platform()', 'Returns "mac", "linux", "windows", "wasm"', 'systumm.kaun()'],
          ['systumm.copy(text)', 'sys.copy(text)', 'Copies string to OS clipboard', 'systumm.copy("Secret")'],
          ['systumm.paste()', 'sys.paste()', 'Reads text from OS clipboard', 'systumm.paste()'],
          ['systumm.nikal(code?)', 'sys.exit(code)', 'Exits program with exit status code', 'systumm.nikal(0)']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '6. cli (Interactive Terminal Apps)'
      },
      {
        type: 'table',
        headers: ['Method', 'English Alias', 'Description', 'Example'],
        rows: [
          ['cli.args_samajh(args?)', 'cli.parse_args(args?)', 'Parses flags (--key=val, --flag, -f) into Map', 'cli.args_samajh()'],
          ['cli.chuno(prompt, opts)', 'cli.select(prompt, opts)', 'Displays numbered interactive choice selection', 'cli.chuno("Branch?", ["main", "dev"])'],
          ['cli.puch_han_na(prompt)', 'cli.confirm(prompt)', 'Prompts user with yes/no [han/na] confirmation', 'cli.puch_han_na("Deploy now?")'],
          ['cli.chhupa_puch(prompt)', 'cli.password(prompt)', 'Prompts password input without echo', 'cli.chhupa_puch("Token: ")'],
          ['cli.progress(cur, tot, msg?)', 'cli.progress(...)', 'Renders in-place terminal progress bar', 'cli.progress(50, 100, "Downloading")']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '7. app (Native Desktop GUI & Notifications)'
      },
      {
        type: 'table',
        headers: ['Method', 'English Alias', 'Description', 'Example'],
        rows: [
          ['app.alert(title, msg)', 'app.dikha_box(title, msg)', 'Native GUI popup alert dialog', 'app.alert("Ready", "Server online")'],
          ['app.puch_box(title, q, def?)', 'app.input_box(...)', 'Native GUI text input dialog', 'app.puch_box("Login", "Username: ")'],
          ['app.han_na_box(title, q)', 'app.confirm_box(...)', 'Native GUI Yes/No confirmation dialog', 'app.han_na_box("Exit", "Sure?")'],
          ['app.file_chuno(title?)', 'app.pick_file(title?)', 'Native system file picker dialog', 'app.file_chuno("Select Image")'],
          ['app.notify(title, msg)', 'app.notify(...)', 'Native OS desktop notification banner', 'app.notify("Done", "Build passed")'],
          ['app.khol(target)', 'app.open(target)', 'Opens URL or file with default system application', 'app.khol("https://google.com")']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '8. Rich Terminal Styling & Unicode UI Widgets'
      },
      {
        type: 'table',
        headers: ['Feature', 'Syntax / Method', 'Description'],
        rows: [
          ['Inline Color Tags', 'dikha "[hara bold]OK:[/] Done"', 'Terminal styling tags: [laal], [hara], [peela], [neela], [gulabi], [asmani], [safed], [kala], [mota], [line], [tedha], [halka], and [/] reset'],
          ['String Color Methods', '"OK".hara().mota()', 'Chainable ANSI methods: .laal(), .hara(), .peela(), .neela(), .gulabi(), .asmani(), .mota(), .line(), .rang_hata()'],
          ['Unicode Bordered Table', 'dikha.mez(data) / table(data)', 'Renders List of Maps or List of Lists as bordered Unicode table'],
          ['Unicode Box Panel', 'dikha.dabba(text, title?) / box(...)', 'Wraps text in rounded Unicode box container with optional title'],
          ['Screen Clear', 'dikha.saaf() / clear()', 'Clears terminal screen and resets cursor position']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '9. tijori (Local Persistent Document DB)'
      },
      {
        type: 'table',
        headers: ['Method', 'Aliases', 'Description', 'Example'],
        rows: [
          ['tijori.rakh(key, val)', 'tijori.set(...)', 'Saves a key-value pair to persistent local storage', 'tijori.rakh("user", {"name": "Raju"})'],
          ['tijori.nikal(key, default?)', 'tijori.get(...)', 'Retrieves value by key, or returns default if not found', 'tijori.nikal("user", khali)'],
          ['tijori.hai_kya(key)', 'tijori.has(...)', 'Returns han if key exists in storage, else nhi', 'tijori.hai_kya("user")'],
          ['tijori.hata(key)', 'tijori.delete(...)', 'Deletes a key from storage and syncs to disk', 'tijori.hata("user")'],
          ['tijori.chaabi()', 'tijori.keys()', 'Returns a list of all saved keys', 'tijori.chaabi()'],
          ['tijori.maal()', 'tijori.values()', 'Returns a list of all saved values', 'tijori.maal()'],
          ['tijori.sab()', 'tijori.all()', 'Returns entire database as a map', 'tijori.sab()'],
          ['tijori.saaf()', 'tijori.clear()', 'Clears all stored entries', 'tijori.saaf()']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '10. dhaba (FastAPI / Express-Style Server)'
      },
      {
        type: 'table',
        headers: ['Method', 'Description', 'Example'],
        rows: [
          ['dhaba.get(route, handler)', 'Registers a GET endpoint handler (supports `:param` routes)', 'dhaba.get("/orders", (req) => { de {"ok": han} })'],
          ['dhaba.post(route, handler)', 'Registers a POST endpoint handler with request body', 'dhaba.post("/order", (req) => { de req["body"] })'],
          ['dhaba.put(route, handler)', 'Registers a PUT endpoint handler', 'dhaba.put("/order/:id", (req) => { ... })'],
          ['dhaba.delete(route, handler)', 'Registers a DELETE endpoint handler', 'dhaba.delete("/order/:id", (req) => { ... })'],
          ['dhaba.request(method, path, body?, headers?)', 'Dispatches request programmatically (in-memory or testing)', 'dhaba.request("GET", "/orders")'],
          ['dhaba.chala(port, callback?)', 'Starts the HTTP server listener on given port', 'dhaba.chala(8080, () => dikha "Chalu!")'],
          ['dhaba.saaf()', 'Clears all registered server routes', 'dhaba.saaf()']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '11. ui (Declarative Cross-Platform UI Framework)'
      },
      {
        type: 'table',
        headers: ['Component / Method', 'Description', 'Example'],
        rows: [
          ['ui.khamba([children])', 'Vertical column layout container (alias ui.column)', 'ui.khamba([ui.likh("Header"), ui.button("OK")])'],
          ['ui.katar([children])', 'Horizontal row layout container (alias ui.row)', 'ui.katar([card1, card2])'],
          ['ui.likh(text, variant?)', 'Typography text node (variant: "heading", "subheading", "body", "caption")', 'ui.likh("☕ Tapri POS", "heading")'],
          ['ui.dabba(children, title?)', 'Bordered card / box container with optional title panel', 'ui.dabba([item1, badge], "Order #1")'],
          ['ui.badge(label, color?)', 'Status badge pill (color: "hara", "peela", "laal", "asmani", "gulabi")', 'ui.badge("Ready", "hara")'],
          ['ui.puch(placeholder)', 'Interactive text input field component', 'ui.puch("Enter customer name...")'],
          ['ui.button(label, onClick)', 'Action button with lambda callback handler', 'ui.button("Order Bhej", () => dikha "Bheja!")'],
          ['ui.faasla()', 'Divider / spacing element (alias ui.divider)', 'ui.faasla()'],
          ['ui.dikha(screen, title?)', 'Renders screen directly to the terminal with Unicode box alignment', 'ui.dikha(screen, "POS App")'],
          ['ui.html(screen, title?)', 'Exports declarative UI tree into HTML5 + Tailwind CSS markup', 'ui.html(screen, "Web View")'],
          ['ui.click(screen, label?)', 'Programmatically triggers a button click event handler (alias ui.dabao)', 'ui.click(screen, "Order Bhej")']
        ]
      }
    ]
  },
  {
    id: 'builtins',
    title: 'Standard Built-in Functions',
    badge: 'Standard Library',
    description: 'Type inspection, length checking, numeric conversions, and console inputs.',
    content: [
      {
        type: 'table',
        headers: ['Function', 'Aliases', 'Parameters', 'Return Type', 'Description'],
        rows: [
          ['kya_hai(val)', 'aukaat', 'Any value', 'string', 'Returns the type name: "int", "float", "string", "bool", "nalla", "function", "object"'],
          ['size(str)', 'kitna_lamba', 'string', 'int', 'Returns the character length of a string.'],
          ['bina_point(val)', 'pura_num', 'string, float, bool', 'int', 'Converts value to a whole 64-bit integer (`long long`).'],
          ['point_wala(val)', 'float_bana', 'string, int, bool', 'float', 'Converts value to a decimal floating point (`double`).'],
          ['bata(prompt)', 'pooch', 'string (optional)', 'string', 'Reads a line of text from console input with prompt.']
        ]
      },
      {
        type: 'code',
        title: 'builtins.meme',
        code: `pakad s = "Namaste Dosto";
dikha "String size:", size(s);
dikha "Type check:", kya_hai(s);

pakad num_str = "1234";
pakad num = bina_point(num_str) + 66;
dikha "Converted jod:", num;

pakad pi_str = "3.14159";
dikha "Decimal multiplied:", point_wala(pi_str) * 2.0;`
      }
    ]
  },
  {
    id: 'memory-safety',
    title: 'Rust-Style Ownership & Borrow Checker',
    badge: 'Memory Safety',
    description: 'Single ownership, moves for heap types, borrowing with udhaar and udhaar_badal, explicit nakal, and RAII alvida destructors.',
    content: [
      {
        type: 'text',
        text: `MEMElang brings **Rust-level memory safety** to Desi programming. Say goodbye to use-after-free, double drops, and data races! Values have single owners; assignments of heap types move ownership by default; references are checked at runtime with hilarious Bollywood errors.`
      },
      {
        type: 'heading',
        level: 2,
        text: '1. Single Ownership & Move Semantics'
      },
      {
        type: 'text',
        text: `Primitive types (\`int\`, \`float\`, \`bool\`, \`nalla\`) have \`Copy\` semantics. Heap and compound resources (\`string\`, \`biradari\` object instances, and closures) have **Move** semantics. Assigning or passing them transfers ownership permanently:`
      },
      {
        type: 'code',
        title: 'move_semantics.meme',
        code: `pakad a = "Baburao ka khazana";
pakad b = a; // Ownership transferred to b!

// Reading 'a' now throws a borrow-checker error:
// dikha a; // Error: Pehli fursat mein nikal! 'a' ki value move ho chuki hai!

dikha "Naya owner b:", b;

// Re-assigning re-initializes 'a':
a = "Naye sapne";
dikha "Reinitialized a:", a;`
      },
      {
        type: 'heading',
        level: 2,
        text: '2. Explicit Cloning with nakal()'
      },
      {
        type: 'text',
        text: `To duplicate a heap value without giving up ownership of the original, use \`nakal(val)\` (or method \`val.nakal()\`, aliases: \`clone\`, \`copy\`):`
      },
      {
        type: 'code',
        title: 'explicit_clone.meme',
        code: `pakad original = "Diamond Kohinoor";
pakad duplicate = nakal(original); // Cloned!

dikha "Original:", original; // Still valid!
dikha "Duplicate:", duplicate;

biradari Khazana {
    jugaad shuru(coins) {
        apun.coins = coins;
    }
}

pakad k1 = Khazana(100);
pakad k2 = k1.nakal(); // Deep clone of instance!
k2.coins = 500;

dikha "k1 coins:", k1.coins; // 100
dikha "k2 coins:", k2.coins; // 500`
      },
      {
        type: 'heading',
        level: 2,
        text: '3. Shared Borrowing (udhaar) vs Mutable Borrowing (udhaar_badal)'
      },
      {
        type: 'text',
        text: `Instead of moving ownership, you can lend references using **Aliasing XOR Mutability** rules:
- **\`udhaar x\`** (\`&x\`): Immutable shared borrow. Multiple shared borrows can coexist. \`x\` cannot be mutated or moved while borrowed!
- **\`udhaar_badal x\`** (\`&mut x\`): Exclusive mutable borrow. Only one can exist at a time; direct access to \`x\` is suspended until the borrow ends!`
      },
      {
        type: 'code',
        title: 'borrowing.meme',
        code: `pakad hero = "Munna Bhai";

// 1. Shared Borrowing:
pakad r1 = udhaar hero;
pakad r2 = udhaar hero;
dikha "Read via ref1:", r1;
dikha "Read via ref2:", r2;

// hero = "Circuit"; // Error: Doglapan! 'hero' abhi udhaar pe hai!

// 2. Mutable Borrowing in Scopes:
pakad score = 100;
{
    pakad m = udhaar_badal score;
    m = 250; // Mutate through reference!
    dikha "Inside scope modified score:", m;
}
// Block exited, borrow released!
dikha "Outside scope score:", score; // 250`
      },
      {
        type: 'heading',
        level: 2,
        text: '4. RAII Scope Destructors (alvida)'
      },
      {
        type: 'text',
        text: `Classes can define a destructor method \`jugaad alvida() { ... }\` (alias: \`drop\`). MEMElang automatically invokes this destructor when an instance's owner block scope ends!`
      },
      {
        type: 'code',
        title: 'raii_destructors.meme',
        code: `biradari FileLocker {
    jugaad shuru(filename) {
        apun.filename = filename;
        dikha "🔓 File opened:", apun.filename;
    }
    jugaad alvida() {
        dikha "🔒 File closed cleanly (RAII):", apun.filename;
    }
}

{
    pakad locker = FileLocker("confidential.docx");
    dikha "Doing important file processing...";
}
// When scope exits, alvida() is called automatically!
dikha "Scope ended cleanly!";`
      }
    ]
  },
  {
    id: 'meme-errors',
    title: 'Iconic Meme Error Catalog',
    badge: 'Diagnostics',
    description: 'Complete guide to all iconic Bollywood, Shark Tank, and Desi internet error messages in MEMElang.',
    content: [
      {
        type: 'text',
        text: `Say goodbye to boring stack traces. In MEMElang, compiler and runtime mistakes trigger legendary Indian internet quotes. Run any code below to trigger these errors live:`
      },
      {
        type: 'table',
        headers: ['Error Trigger', 'Meme Quote Message', 'Cultural Reference'],
        rows: [
          ['Unclosed string quote', '😒 String khol ke chhod diya... band tera baap karega?', 'Classic Indian Parent Scolding'],
          ['Illegal alien symbol (@, $)', '😭 Chi sasura! Ye kaisa alien symbol ghusa diye ho code mein: <symbol>', 'Gangs of Wasseypur'],
          ['Missing semicolon (;)', 'Chhoti bachi ho kya?! Line ke end mein \';\' lagana bhool gaye!', 'Tiger Shroff / Heropanti'],
          ['Invalid LHS assignment', 'Ye sab doglapan hai! \'=\' ke left side mein sirf seedha variable ya property aata hai!', 'Ashneer Grover / Shark Tank'],
          ['Accidental \'=\' in condition', 'Control Uday, control! \'<context>\' mein \'=\' nahi, \'==\' lagta hai!', 'Welcome Movie'],
          ['EOF mid-expression', 'Moye Moye! Expression adhoori chhod ke code beech mein hi khatam kar diya.', 'Viral Moye Moye Meme'],
          ['Undeclared variable read', 'Rasode mein kaun tha?! Ye \'<name>\' bina bataye kahan se aaya?', 'Saath Nibhaana Saathiya'],
          ['Undeclared variable assign', 'Chuna laga diya! \'<name>\' naam ka koi variable hi nahi hai.', 'Panchayat / Popular Meme'],
          ['Use after move', 'Pehli fursat mein nikal! \'<name>\' ki value move ho chuki hai! Ownership transfer ke baad use karna mana hai!', 'Hindustani Bhau'],
          ['Mutate while borrowed', 'Doglapan! \'<name>\' abhi udhaar (borrowed) pe hai, isko modify nahi kar sakte!', 'Ashneer Grover'],
          ['Direct access while mutably borrowed', 'Beech mein mat bolo! \'<name>\' abhi kisi aur ko udhaar diya hua hai!', 'Arnab Goswami Debate'],
          ['Borrow conflict / Aliasing', 'Ek mawaali ek time pe ek hi jagah! \'<name>\' already borrowed hai!', 'Welcome Movie'],
          ['Dangling reference escape', 'Baburao ka chashma! Local variable \'<name>\' ka udhaar wapas nahi bhej sakte!', 'Hera Pheri (Baburao)'],
          ['Integer overflow in +', '25 din mein paisa double karte-karte integer overflow ho gaya!', 'Phir Hera Pheri'],
          ['Negative string repeat', 'Delulu is not the solulu! String ko negative baar kaise multiply karoge?', 'Gen-Z Internet Slang'],
          ['Divide by zero', 'Kya coder banega re tu? 0 se divide karke brahmand khatam karega kya!', 'Hera Pheri (Babu Bhaiya)'],
          ['Type mismatch in comparison', 'Seb ko santre se mat tolo! \'<t1>\' aur \'<t2>\' ko compare nahi kar sakte!', 'Desi Idiom'],
          ['Calling non-callable', 'Beta, tumse na ho payega! \'<val>\' koi jugaad (function) thodi hai!', 'Gangs of Wasseypur (Ramadhir)'],
          ['Calling nalla (null)', 'Jugaad toh \'nalla\' (null) nikla! Khali cheez ko call kaise karoge?', 'Mumbai Slang'],
          ['Arity mismatch', 'Gaddari korbe! Jugaad ne <N> arguments maange the, aur tumne <M> chipka diye!', 'Viral Bengali Meme'],
          ['Missing object property', 'Khopdi tod saale ka! \'<class>\' biradari ke paas \'<prop>\' naam ka kuch nahi hai!', 'Hera Pheri (Babu Bhaiya)'],
          ['Non-class superclass', 'Fake nepotism! \'<parent>\' koi biradari (class) nahi hai!', 'Bollywood Nepotism Debate']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Try It Live: Intentional Error Demo'
      },
      {
        type: 'code',
        title: 'trigger_error.meme',
        code: `// Try running this to see Babu Bhaiya's zero division error!
pakad ans = 100 / 0;
dikha ans;`
      }
    ]
  },
  {
    id: 'gradual-typing',
    title: 'Gradual Typing & Static Analysis',
    badge: 'Type System',
    description: 'Combine dynamic prototyping flexibility with compile-time type verification using the native TypeChecker.',
    content: [
      {
        type: 'text',
        text: `MEMElang provides a **gradual typing system**. You can write code without type annotations for lightning-fast prototyping, or add static type annotations to variables, function parameters, and return types for compile-time safety and self-documenting code.`
      },
      {
        type: 'callout',
        variant: 'info',
        title: 'Zero Runtime Performance Penalty',
        text: `Static types are verified at compile-time by the C++ ` + '`TypeChecker`' + `. In production, verified code executes with zero runtime type checking overhead while giving you bulletproof contract guarantees.`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Core Type Keywords & Annotations'
      },
      {
        type: 'table',
        headers: ['Type Keyword', 'Alternative Hindi/English', 'Underlying C++ Type', 'Example'],
        rows: [
          ['sankhya / ank', 'int, number', 'int64_t', 'pakad age: sankhya = 25;'],
          ['point / dashamlav', 'float, double', 'double (IEEE-754)', 'pakad pi: point = 3.14159;'],
          ['bol / shabd', 'string, str', 'std::string (interned)', 'pakad name: bol = "Chai";'],
          ['sach_jhooth / sach', 'bool, boolean', 'bool', 'pakad isOnline: sach = han;'],
          ['suchi', 'list, array', 'std::vector<Value>', 'pakad items: suchi = [1, 2, 3];'],
          ['nakshe', 'map, dict', 'std::unordered_map', 'pakad user: nakshe = {"id": 1};'],
          ['nalla', 'null, void', 'std::monostate', 'pakad empty: nalla = nalla;'],
          ['har_kism', 'any, koi_bhi', 'dynamic Value', 'pakad flex: har_kism = 42;']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Typed Functions & Signatures'
      },
      {
        type: 'text',
        text: `Function signatures can specify parameter types and return type contracts using the ` + '`->`' + ` arrow syntax. If a function returns an incompatible type or receives illegal arguments, the compiler flags it before execution starts:`
      },
      {
        type: 'code',
        title: 'typed_functions.meme',
        code: `// Function with explicit argument and return type contracts
kaam calculate_tax(amount: point, rate: point) -> point {
    de amount * rate;
}

// Typed collections and variables
pakad bill: point = 1250.50;
pakad gstRate: point = 0.18;
pakad totalTax: point = calculate_tax(bill, gstRate);

dikha f"Total Tax Due: ₹{totalTax}";`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Static Checking via CLI'
      },
      {
        type: 'text',
        text: `You can run static analysis checks over your entire codebase using the compiler without executing the code:`
      },
      {
        type: 'terminal',
        commands: [
          'meme check src/App.meme',
          'meme check tests/'
        ]
      }
    ]
  },
  {
    id: 'react-jsx-ui',
    title: 'Declarative React JSX & UI Hooks',
    badge: 'Frontend UI',
    description: 'Build reactive cross-platform user interfaces using familiar React JSX syntax, useState, useEffect, and controlled components.',
    content: [
      {
        type: 'text',
        text: `MEMElang treats **React JSX as first-class syntax**. You can build modern, responsive applications using component hierarchies, reactive hooks, and Tailwind CSS utility classes without needing Babel, Webpack, or external transpilers.`
      },
      {
        type: 'heading',
        level: 2,
        text: '1. First-Class JSX Syntax & Tailwind Styling'
      },
      {
        type: 'text',
        text: `Standard HTML tags (` + '`<div>`' + `, ` + '`<button>`' + `, ` + '`<input>`' + `, ` + '`<span>`' + `) and custom components can be written directly inside expressions. Style elements using standard Tailwind CSS classes:`
      },
      {
        type: 'code',
        title: 'simple_jsx.meme',
        code: `card = (
    <div className="p-6 bg-slate-900 border border-slate-800 rounded-2xl shadow-xl">
        <h2 className="text-xl font-bold text-white mb-2">☕ Tapri Chai</h2>
        <p className="text-slate-400 text-sm mb-4">Fresh brewed cutting chai with adrak and elaichi</p>
        <button className="px-4 py-2 bg-amber-500 hover:bg-amber-400 text-slate-950 font-bold rounded-lg transition">
            Order Now - ₹15
        </button>
    </div>
);

ui.dikha(card, "Tapri Menu");`
      },
      {
        type: 'heading',
        level: 2,
        text: '2. State Management with useState'
      },
      {
        type: 'text',
        text: `Manage stateful reactivity with the ` + '`useState`' + ` hook. Use array destructuring ` + '`[state, setState]`' + ` to capture the value and updater function. Updating state triggers automatic re-renders across all active views:`
      },
      {
        type: 'code',
        title: 'counter_state.meme',
        code: `kaam Counter() {
    [count, setCount] = useState(0);

    de (
        <div className="p-6 bg-slate-900 rounded-2xl max-w-sm mx-auto text-center border border-slate-800">
            <h1 className="text-2xl font-bold text-white mb-2">Chai Counter</h1>
            <div className="text-4xl font-extrabold text-amber-400 my-4">{count}</div>
            <div className="flex gap-3 justify-center">
                <button
                    className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-white rounded-lg font-semibold"
                    onClick={() => setCount(c => c - 1)}
                >
                    - Ghatao
                </button>
                <button
                    className="px-4 py-2 bg-amber-500 hover:bg-amber-400 text-slate-950 rounded-lg font-bold"
                    onClick={() => setCount(c => c + 1)}
                >
                    + Jodo
                </button>
            </div>
        </div>
    );
}`
      },
      {
        type: 'heading',
        level: 2,
        text: '3. Lifecycle & Side Effects with useEffect'
      },
      {
        type: 'text',
        text: `Use ` + '`useEffect`' + ` to perform side-effects, such as loading data from local persistent storage (` + '`tijori`' + `) or syncing with remote APIs when the component mounts:`
      },
      {
        type: 'code',
        title: 'use_effect.meme',
        code: `kaam NotesApp() {
    [notes, setNotes] = useState([]);

    // Lifecycle hook: runs on mount to hydrate state from persistent vault
    useEffect(() => {
        pakad saved = tijori.nikal("notes_list");
        agar saved != nalla {
            setNotes(saved);
        }
    });

    de (
        <div className="p-6">
            <h1 className="text-white text-xl font-bold">Saved Notes ({notes.lambai()})</h1>
        </div>
    );
}`
      },
      {
        type: 'heading',
        level: 2,
        text: '4. Controlled Inputs & Two-Way Binding'
      },
      {
        type: 'text',
        text: `Inputs support seamless two-way state binding. Input values and keyboard events sync automatically with the reactive engine:`
      },
      {
        type: 'code',
        title: 'controlled_input.meme',
        code: `kaam NewNoteForm({ onAdd }) {
    [text, setText] = useState("");

    kaam handleSubmit() {
        agar text.lambai() > 0 {
            onAdd(text);
            setText("");
        }
    }

    de (
        <div className="flex gap-2">
            <input
                className="flex-1 bg-slate-800 text-white px-4 py-2 rounded-xl border border-slate-700 focus:outline-none focus:border-amber-500"
                placeholder="Naya note likho..."
                value={text}
                onChange={(e) => setText(e.target.value)}
            />
            <button
                className="px-5 py-2 bg-amber-500 hover:bg-amber-400 text-slate-950 font-bold rounded-xl"
                onClick={handleSubmit}
            >
                Save
            </button>
        </div>
    );
}`
      },
      {
        type: 'heading',
        level: 2,
        text: '5. Mounting the Application'
      },
      {
        type: 'text',
        text: `Call ` + '`ui.mount(<App />)`' + ` to launch the top-level root component. The runtime automatically configures the appropriate rendering backend (Native macOS Cocoa Window, Web Canvas / DOM, or Terminal UI) based on your run configuration:`
      },
      {
        type: 'code',
        title: 'mount_root.meme',
        code: `// Mount top-level application root
ui.mount(<App />, "ChaiVault Desktop");`
      }
    ]
  },
  {
    id: 'native-desktop',
    title: 'Zero-WebView Native Desktop Engine',
    badge: 'Native GUI',
    description: 'Hardware-accelerated 60 FPS native desktop rendering via Cocoa NSWindow and CoreGraphics without WebViews or Electron overhead.',
    content: [
      {
        type: 'text',
        text: `Traditional desktop frameworks like Electron bundle an entire Chromium browser and Node runtime with every application, resulting in 200MB+ memory usage, slow startup times, and sluggish performance. MEMElang completely rethinks desktop apps with a **Zero-WebView Native Engine**.`
      },
      {
        type: 'callout',
        variant: 'info',
        title: 'Sub-15MB RAM & Instant Launch',
        text: `MEMElang native desktop applications run on a pure Cocoa ` + '`NSWindow`' + ` via CoreGraphics Quartz. Startup takes less than 10 milliseconds, and memory consumption stays under 15MB.`
      },
      {
        type: 'heading',
        level: 2,
        text: 'C++ Tailwind v4 Layout Solver'
      },
      {
        type: 'text',
        text: `At the core of the native desktop engine is ` + '`TailwindEngine.h`' + `, a single-pass Flexbox and CSS Grid constraint solver written in modern C++20. It parses standard Tailwind utility classes directly and computes exact layout geometries without any browser engine:`
      },
      {
        type: 'list',
        items: [
          '**Display & Direction**: `flex`, `flex-col`, `flex-row`, `grid`, `grid-cols-2`, `grid-cols-3`',
          '**Alignment & Spacing**: `items-center`, `justify-between`, `justify-center`, `gap-2`, `gap-4`, `gap-6`',
          '**Dimensions & Box Model**: `w-full`, `max-w-md`, `h-screen`, `p-4`, `p-6`, `px-3`, `py-2`, `m-2`',
          '**Borders & Shapes**: `rounded-lg`, `rounded-2xl`, `rounded-full`, `border`, `border-slate-800`',
          '**Color Palette**: Full Tailwind palette (`bg-slate-950`, `bg-amber-500`, `text-white`, `text-slate-400`, etc.)',
          '**Elevation & Effects**: `shadow-sm`, `shadow-md`, `shadow-xl`, `shadow-2xl`'
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Pure Vector Display List Pipeline'
      },
      {
        type: 'text',
        text: `Once layout computation completes, the engine translates component trees into a compact display list of raw vector draw instructions executed directly by CoreGraphics Quartz:`
      },
      {
        type: 'table',
        headers: ['Vector Command', 'Parameters', 'Quartz Render Target'],
        rows: [
          ['FILL_ROUND_RECT', 'x, y, w, h, radius, color', 'CGContextFillPath with CGPathAddRoundedRect'],
          ['STROKE_RECT', 'x, y, w, h, radius, strokeWidth, color', 'CGContextStrokePath with specified border width'],
          ['DRAW_TEXT', 'x, y, text, fontSize, color, weight', 'CTLineDraw / CGContextShowGlyphsAtPoint']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Multiplatform Dev Server & Hot-Reload'
      },
      {
        type: 'text',
        text: `Launch the dev server with your desired target platform. The server features instantaneous state preservation and hot-reloads on file save:`
      },
      {
        type: 'terminal',
        commands: [
          '# Launch native macOS Cocoa desktop window',
          'meme dev src/App.meme --platform=desktop',
          '',
          '# Or launch web browser target with HTML5 DOM',
          'meme dev src/App.meme --platform=web',
          '',
          '# Or launch terminal rich ASCII preview',
          'meme dev src/App.meme --platform=terminal'
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Interactive Keyboard Shortcuts'
      },
      {
        type: 'text',
        text: `When running in dev mode, you can control targets and windows directly from the terminal without restarting the server:`
      },
      {
        type: 'table',
        headers: ['Key', 'Action', 'Description'],
        rows: [
          ['[d]', 'Desktop Window', 'Opens the hardware-accelerated Cocoa NSWindow'],
          ['[w]', 'Web Browser', 'Opens Google Chrome / Safari to the live web application'],
          ['[t]', 'Terminal Preview', 'Renders live Unicode ANSI widget frames in terminal'],
          ['[r]', 'Hot Reload', 'Forces an immediate recompilation and state reload'],
          ['[p]', 'Switch Platform', 'Cycles active default platform between desktop, web, terminal'],
          ['[q]', 'Quit Server', 'Cleanly shuts down all windows and terminates the server']
        ]
      }
    ]
  },
  {
    id: 'chameleon-bridge',
    title: 'Chameleon Universal Interop Bridge',
    badge: 'Universal Bridge',
    description: 'Call Python NumPy / Pandas, Node/npm packages, and Rust crates natively in MEMElang with zero configuration.',
    content: [
      {
        type: 'text',
        text: `Why choose between Python for scientific data science, Node/npm for modern cloud packages, and Rust for systems-grade cryptography? The **Chameleon Universal Interop Bridge** lets MEMElang import libraries from all three ecosystems seamlessly with zero glue code and zero configuration.`
      },
      {
        type: 'callout',
        variant: 'info',
        title: 'Universal Ecosystem Syntax',
        text: `Use the familiar ` + '`bula "<ecosystem>:<module>" jaise <alias>;`' + ` syntax. The Chameleon Sentinel automatically detects environment runtimes and marshals parameters across languages bidirectionally.`
      },
      {
        type: 'heading',
        level: 2,
        text: '1. Python Interop (NumPy & Pandas)'
      },
      {
        type: 'text',
        text: `Call Python packages like ` + '`numpy`' + `, ` + '`pandas`' + `, and ` + '`scipy`' + ` directly. Array operations, matrix math, and DataFrames operate with native syntax:`
      },
      {
        type: 'code',
        title: 'python_interop.meme',
        code: `// Direct Python NumPy vector math
bula "py:numpy" jaise np;

pakad vec = np.arange(10, 100, 10);
dikha "NumPy Vector:", vec;
dikha "Sum:", np.sum(vec);
dikha "Mean:", np.mean(vec);

pakad v1 = np.array([1, 2, 3]);
pakad v2 = np.array([4, 5, 6]);
dikha "Dot Product:", np.dot(v1, v2);

// Direct Python Pandas DataFrames
bula "py:pandas" jaise pd;

pakad df = pd.DataFrame([
    { "item": "Chai", "price": 20, "sold": 150 },
    { "item": "Bun Maska", "price": 40, "sold": 80 },
    { "item": "Maggi", "price": 50, "sold": 45 }
]);

dikha "Total Menu Items:", df.col("item").lambai();`
      },
      {
        type: 'heading',
        level: 2,
        text: '2. JavaScript & NPM Interop'
      },
      {
        type: 'text',
        text: `Tap into the npm registry for real-time messaging, WebSockets, or cloud databases:`
      },
      {
        type: 'code',
        title: 'npm_interop.meme',
        code: `// Realtime messaging via Ably
bula "npm:ably" jaise ably;

pakad client = ably.Realtime("API_KEY");
pakad channel = client.channels.get("orders");
channel.publish("new_order", {
    "item": "Kullad Chai",
    "qty": 3,
    "timestamp": systumm.ab()
});

// Cloud Document DB via Firebase
bula "npm:firebase" jaise fb;
pakad db = fb.firestore();
db.collection("orders").add({ "status": "pending", "total": 90 });`
      },
      {
        type: 'heading',
        level: 2,
        text: '3. Rust High-Performance Interop'
      },
      {
        type: 'text',
        text: `Leverage Rust's blistering speed for cryptographic hashing, UUID generation, or parallel iteration:`
      },
      {
        type: 'code',
        title: 'rust_interop.meme',
        code: `// Cryptographic operations & UUIDs
bula "rust:crypto" jaise crypto;

pakad secretHash = crypto.sha256("MEMElang_Ultra_Secret");
pakad traceId = crypto.uuid_v4();

dikha "SHA-256 Digest:", secretHash;
dikha "Trace UUID:", traceId;

// Parallel computing via Rayon
bula "rust:rayon" jaise rayon;
pakad dataset = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
pakad parallelSquares = rayon.par_map(dataset, x => x * x);
dikha "Parallel Squared:", parallelSquares;`
      },
      {
        type: 'heading',
        level: 2,
        text: 'Cross-Language Type Marshaling'
      },
      {
        type: 'table',
        headers: ['MEMElang Value', 'Python Type', 'JavaScript Type', 'Rust Type'],
        rows: [
          ['sankhya (42)', 'int', 'number', 'i64 / u64'],
          ['point (3.14)', 'float', 'number', 'f64'],
          ['bol ("namaste")', 'str', 'string', 'String / &str'],
          ['sach (han / real)', 'bool (True)', 'boolean (true)', 'bool'],
          ['suchi ([1, 2, 3])', 'list', 'Array', 'Vec<T>'],
          ['nakshe ({"a": 1})', 'dict', 'Object', 'HashMap<K, V>'],
          ['nalla (null)', 'None', 'null / undefined', 'Option::None']
        ]
      }
    ]
  },
  {
    id: 'tapri-package-manager',
    title: 'Tapri Package Manager, Workspaces & Storage',
    badge: 'Tooling & Storage',
    description: 'Manage project manifests (tapri.json), lockfiles, root path resolution, persistent local storage, and automated testing.',
    content: [
      {
        type: 'text',
        text: `**Tapri** is the official package manager, workspace orchestrator, and persistent runtime companion for MEMElang, named after India's ubiquitous roadside tea stalls where every conversation and idea comes together.`
      },
      {
        type: 'heading',
        level: 2,
        text: '1. tapri.json Project Manifest'
      },
      {
        type: 'text',
        text: `Every MEMElang project is defined by a ` + '`tapri.json`' + ` file detailing project scripts, entrypoints, and cross-ecosystem dependencies:`
      },
      {
        type: 'code',
        title: 'tapri.json',
        code: `{
  "name": "notes-desktop",
  "version": "1.0.0",
  "main": "src/App.meme",
  "scripts": {
    "dev": "meme dev src/App.meme --platform=desktop",
    "web": "meme dev src/App.meme --platform=web",
    "start": "meme src/App.meme",
    "test": "meme test tests"
  },
  "dependencies": {
    "py:numpy": "^1.24.0",
    "rust:crypto": "latest"
  }
}`
      },
      {
        type: 'heading',
        level: 2,
        text: '2. tapri.lock Determinism'
      },
      {
        type: 'text',
        text: `When dependencies are installed, Tapri computes exact versions and integrity hashes, writing them to ` + '`.meme_env/tapri.lock`' + `. This guarantees identical, deterministic builds across every developer machine and CI/CD environment.`
      },
      {
        type: 'heading',
        level: 2,
        text: '3. Tapri CLI Commands Reference'
      },
      {
        type: 'table',
        headers: ['Command', 'Purpose', 'Example'],
        rows: [
          ['meme naya <name>', 'Scaffold project with templates (react, api, fullstack)', 'meme naya my-app --template=react'],
          ['meme tapri init', 'Initialize tapri.json in current directory', 'meme tapri init'],
          ['meme tapri install', 'Resolve and install dependencies into .meme_env/', 'meme tapri install'],
          ['meme tapri add <pkg>', 'Add a package to tapri.json and install it', 'meme tapri add py:numpy'],
          ['meme tapri suchi', 'List all installed packages and versions', 'meme tapri suchi'],
          ['meme run <script>', 'Execute a script configured in tapri.json', 'meme run test'],
          ['meme setup', 'Link meme binary globally into system PATH', 'meme setup']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: '4. Root Path Resolution (@/ Aliases)'
      },
      {
        type: 'text',
        text: `Avoid brittle relative paths like ` + '`../../components/Header.meme`' + `. MEMElang automatically resolves the ` + '`@/`' + ` alias to the root directory where ` + '`tapri.json`' + ` resides:`
      },
      {
        type: 'code',
        title: 'root_imports.meme',
        code: `// Clean root-relative imports from any nested directory
bula "@/components/Header.meme" jaise Header;
bula "@/components/MetricCard.meme" jaise MetricCard;
bula "@/services/PythonService.meme" jaise PyService;`
      },
      {
        type: 'heading',
        level: 2,
        text: '5. Tijori: Persistent Local Storage Vault'
      },
      {
        type: 'text',
        text: `Applications often need persistent key-value and document storage that survives restarts without setting up heavy external databases. MEMElang includes **Tijori** (the Vault), a lightweight embedded database:`
      },
      {
        type: 'code',
        title: 'tijori_storage.meme',
        code: `// 1. Save data persistently to disk
tijori.rakh("user_preferences", {
    "theme": "dark",
    "fontSize": 14,
    "lastOpened": systumm.ab()
});

// 2. Retrieve saved data
pakad prefs = tijori.nikal("user_preferences");
agar prefs != nalla {
    dikha "Saved Theme:", prefs["theme"];
}

// 3. Remove a specific key
tijori.hatao("temporary_session");

// 4. Safely clear the vault
// tijori.saaf();`
      },
      {
        type: 'heading',
        level: 2,
        text: '6. Automated Testing Framework'
      },
      {
        type: 'text',
        text: `Write clean BDD-style unit and integration tests using ` + '`test(name, fn)`' + ` and ` + '`expect(val).toBe(...)`' + `. Run your entire test suite with ` + '`meme test`' + `:`
      },
      {
        type: 'code',
        title: 'app_test.meme',
        code: `// tests/notes_test.meme
test("Notes count parity", () => {
    pakad defaultNotes = [
        { "id": 1, "title": "Buy tea" },
        { "id": 2, "title": "Code MEMElang" }
    ];
    expect(defaultNotes.lambai()).toBe(2);
    expect(defaultNotes[0].title).toBe("Buy tea");
});

test("Immutability on note append", () => {
    pakad original = ["A", "B"];
    pakad updated = [...original, "C"];
    expect(updated.lambai()).toBe(3);
    expect(original.lambai()).toBe(2);
});`
      },
      {
        type: 'terminal',
        commands: [
          'meme test tests/',
          '✔ Tests passed: 2 | Failed: 0 (100% green)'
        ]
      }
    ]
  }
];

export const PLAYGROUND_PRESETS = [
  {
    id: 'chaivault-desktop',
    name: '🖥️ ChaiVault Native Desktop App (CoreGraphics 60 FPS)',
    description: 'Hardware-accelerated native macOS desktop notes app with Tailwind v4 layout, category filter, and persistent Tijori storage',
    code: `// ☕ ChaiVault Native Desktop App in MEMElang
bula "ui:shadcn" jaise { Button, Badge }

kaam App() {
    [notes, setNotes] = useState([
        { "id": 1, "title": "Compiler Optimizations", "category": "Dev", "priority": "High" },
        { "id": 2, "title": "Chai break at 5 PM", "category": "Personal", "priority": "Low" },
        { "id": 3, "title": "Zero-WebView GPU Profiling", "category": "Dev", "priority": "Urgent" }
    ]);
    [filter, setFilter] = useState("all");

    visibleNotes = notes.filter(n => filter == "all" || n.category == filter);

    de (
        <div className="p-8 bg-slate-950 text-white min-h-screen">
            <div className="max-w-4xl mx-auto">
                <div className="flex items-center justify-between pb-6 border-b border-slate-800">
                    <div>
                        <h1 className="text-3xl font-extrabold text-amber-400">☕ ChaiVault</h1>
                        <p className="text-sm text-slate-400">Native Cocoa NSWindow • 60 FPS CoreGraphics</p>
                    </div>
                    <div className="flex gap-2">
                        <button className="px-3 py-1 bg-slate-800 hover:bg-slate-700 rounded-lg text-xs" onClick={() => setFilter("all")}>All</button>
                        <button className="px-3 py-1 bg-amber-500/20 text-amber-300 rounded-lg text-xs" onClick={() => setFilter("Dev")}>Dev</button>
                        <button className="px-3 py-1 bg-emerald-500/20 text-emerald-300 rounded-lg text-xs" onClick={() => setFilter("Personal")}>Personal</button>
                    </div>
                </div>

                <div className="grid grid-cols-3 gap-4 mt-6">
                    {visibleNotes.map(n => (
                        <div key={n.id} className="p-4 bg-slate-900 border border-slate-800 rounded-2xl">
                            <span className="text-xs px-2 py-0.5 rounded bg-amber-500/20 text-amber-300 font-bold">{n.priority}</span>
                            <h3 className="text-lg font-bold mt-2">{n.title}</h3>
                            <p className="text-xs text-slate-500 mt-1">Category: {n.category}</p>
                        </div>
                    ))}
                </div>
            </div>
        </div>
    );
}

ui.mount(<App />, "ChaiVault Desktop");`
  },
  {
    id: 'shadcn-gpu-ui',
    name: '⚡ Zero-WebView Native GPU UI (JSX + Shadcn + Tailwind)',
    description: 'Direct GPU Canvas rendering powered by React JSX syntax, Shadcn components, and mathematical Tailwind CSS engine',
    code: `// ⚡ Zero-WebView Native GPU UI in MEMElang
bula "ui:shadcn" jaise { Card, Button, Badge, CardHeader, CardTitle, CardContent }

// 1. React State Hook with Array Destructuring
[orders, setOrders] = useState([
    {"item": "☕ Masala Chai", "price": 20, "badge": "Special"},
    {"item": "🍞 Bun Maska", "price": 40, "badge": "Hot"},
    {"item": "🍜 Street Maggi", "price": 50, "badge": "Popular"}
])

// 2. Pure React JSX Component Tree with Tailwind Utility Classes
appTree = (
    <Card className="p-6 bg-slate-900 border-slate-800 rounded-3xl w-full text-slate-100 shadow-2xl">
        <CardHeader className="flex flex-row items-center justify-between pb-4">
            <div>
                <CardTitle className="text-2xl font-bold tracking-tight text-white">
                    ☕ Tapri Point-Of-Sale
                </CardTitle>
                <div className="text-sm text-slate-400 mt-1">
                    Zero-WebView Direct GPU UI Engine • 60 FPS Canvas
                </div>
            </div>
            <Badge variant="outline" className="bg-emerald-950/60 text-emerald-400 border-emerald-700/50">
                🟢 Live Terminal
            </Badge>
        </CardHeader>

        <CardContent className="mt-4 flex flex-col gap-3">
            <div className="text-xs font-semibold uppercase tracking-wider text-slate-400">
                Current Active Orders
            </div>
            <div className="p-3 bg-slate-800/80 rounded-xl flex items-center justify-between border border-slate-700/50">
                <div>
                    <span className="font-medium text-white">☕ Masala Chai</span>
                    <span className="text-xs text-slate-400 ml-2">₹20</span>
                </div>
                <Badge variant="default" className="bg-amber-500 text-slate-950 font-bold">Ready</Badge>
            </div>
            <div className="p-3 bg-slate-800/80 rounded-xl flex items-center justify-between border border-slate-700/50">
                <div>
                    <span className="font-medium text-white">🍞 Bun Maska</span>
                    <span className="text-xs text-slate-400 ml-2">₹40</span>
                </div>
                <Badge variant="secondary">Cooking</Badge>
            </div>
            <div className="p-3 bg-slate-800/80 rounded-xl flex items-center justify-between border border-slate-700/50">
                <div>
                    <span className="font-medium text-white">🍜 Street Maggi</span>
                    <span className="text-xs text-slate-400 ml-2">₹50</span>
                </div>
                <Badge variant="destructive">Pending</Badge>
            </div>
            <Button variant="default" size="lg" className="w-full mt-2 bg-gradient-to-r from-amber-500 to-orange-500 font-bold text-slate-950">
                ⚡ Process Next Order
            </Button>
        </CardContent>
    </Card>
)

// 3. Mount to Direct GPU Canvas & Terminal
ui.dikha(appTree, "Tapri Direct GPU App")`
  },
  {
    id: 'ecosystem-bridge',
    name: '🌐 Universal Ecosystem Bridge (NumPy, Pandas & Crypto)',
    description: 'Call Python NumPy / Pandas and Rust cryptographic libraries natively in MEMElang with zero configuration',
    code: `// 🌐 Universal Ecosystem Bridge in MEMElang

// 1. Python NumPy Math & Vector Operations
bula "py:numpy" jaise np
numbers = np.arange(10, 110, 10)
dikha "[hara bold]NumPy Vector:[/] " + numbers
dikha "Sum = " + np.sum(numbers) + ", Mean = " + np.mean(numbers)

v1 = np.array([1, 2, 3])
v2 = np.array([4, 5, 6])
dikha "Dot Product (v1 • v2) = " + np.dot(v1, v2)

// 2. Python Pandas DataFrames
bula "py:pandas" jaise pd
sales = pd.DataFrame([
    {"item": "Chai", "price": 20, "sold": 150},
    {"item": "Samosa", "price": 15, "sold": 80},
    {"item": "Maggi", "price": 50, "sold": 45}
])
dikha "\\n[asmani bold]Pandas DataFrame Summary:[/] Total Rows = " + sales.col("item").lambai()

// 3. Rust Cryptography (SHA-256 & UUID v4)
bula "rust:crypto" jaise crypto
secretKey = crypto.sha256("MEMElang_Secret_2026")
traceId = crypto.uuid_v4()
dikha "\\n[peela bold]Rust SHA-256:[/] " + secretKey
dikha "[peela bold]Rust UUID v4:[/] " + traceId`
  },
  {
    id: 'tapri-pos-fullstack',
    name: '☕ Tapri POS Full-Stack (DB, Dhaba & UI)',
    description: 'Tijori (persistent local DB), Dhaba (FastAPI server), and Declarative Cross-Platform UI Framework',
    code: `// ☕ Tapri POS Full-Stack Application in MEMElang

// 1. Persistent Document Database (Tijori)
tijori.rakh("orders", [
    {"item": "Masala Chai", "price": 20, "status": "Ready"},
    {"item": "Bun Maska", "price": 40, "status": "Cooking"}
])

// 2. FastAPI-style Backend Server (Dhaba)
dhaba.get("/orders", (req) => {
    de {"ok": han, "maal": tijori.nikal("orders")}
})

// Test the Dhaba endpoint directly
res = dhaba.request("GET", "/orders")
dikha "[hara bold]HTTP " + res["status"] + " OK:[/] " + res["body"]

// 3. Declarative Cross-Platform UI Framework
screen = ui.khamba([
    ui.likh("☕ Tapri POS App", "heading"),
    ui.katar([
        ui.dabba([
            ui.likh("Masala Chai - ₹20"),
            ui.badge("Ready", "hara")
        ], "Order #1"),
        ui.dabba([
            ui.likh("Bun Maska - ₹40"),
            ui.badge("Cooking", "peela")
        ], "Order #2")
    ]),
    ui.puch("Naya order likho..."),
    ui.button("🔥 Order Bhej", () => {
        dikha "[hara mota]Naya order lag gaya![/]"
    })
])

// 4. Render to Terminal Screen
ui.dikha(screen, "Tapri Multi-Platform App")

// 5. Trigger Interactive Button Callback
ui.click(screen, "🔥 Order Bhej")`
  },
  {
    id: 'rich-terminal-stdlib',
    name: '🎨 Rich Terminal, UI Widgets & Stdlib',
    description: 'Inline style tags, Unicode tables (dikha.mez), box panels (dikha.dabba), string styling (.laal, .hara), systumm & cli',
    code: `// 🎨 Rich dikha styling, Unicode tables, boxes, and standard libraries!

// 1. Rich Inline Tags in dikha
dikha "[hara bold]✔ Success:[/] Server chalu ho gaya on [asmani line]http://localhost:8080[/]"
dikha "[laal]Error 404:[/] [peela]Resource gayab ho gaya![/]"

// 2. Chainable Color & Style Methods on Strings
status = "ONLINE".hara().mota()
dikha f"Database Status: {status}"
dikha "Raw plain text without ANSI: " + status.rang_hata()

// 3. Unicode Box Panels (dikha.dabba / box)
dikha.dabba("MEMElang v2.0\\nRich UI, Stdlib & WebAssembly Ready!", "🚀 Release Announcement")

// 4. Unicode Bordered Tables (dikha.mez / table)
tasks = [
    { task: "Lexer & Parser", status: "Done".hara(), priority: "High".laal() },
    { task: "Rich dikha & Widgets", status: "Done".hara(), priority: "High".laal() },
    { task: "Standard Libraries", status: "Done".hara(), priority: "Medium".peela() },
    { task: "Deploy to Production", status: "Ready".asmani(), priority: "Urgent".laal().mota() }
]
dikha.mez(tasks)

// 5. Expanded Array & String Methods
arr = [10, 20, 30]
arr.daal(40)
dikha "Pehla item:", arr.pehla(), "| Aakhri:", arr.aakhri()
dikha "Chunked by 2:", arr.tukde(2)

code = "42"
dikha "Padded ID:", code.aage_bhar(6, "0")
dikha "Is digit:", code.number_hai()

// 6. Systumm & CLI APIs
dikha "Running on platform:", systumm.kaun()
args = cli.args_samajh(["--mode=turbo", "--workers=4", "serve"])
dikha "CLI args parsed:", args`
  },
  {
    id: 'whatsapp-hinglish',
    name: '💬 WhatsApp Hinglish (Effortless & Clean)',
    description: 'Zero semicolons, no pakad/le boilerplate, kaam, de, agar/elif, har...mein, .chhan, .badal, .saja, .kul, .dhund',
    code: `// 💬 WhatsApp Hinglish MEMElang — Clean, Pythonic, Zero Keystroke Waste!

// 1. Python-style variable creation (No pakad / le needed) & optional semicolons
numbers = [5, 12, 8, 20, 3, 42, 17]

// 2. Short, natural keywords: 'kaam' (function) and 'de' (return)
kaam process_scores(arr) {
    // 3. Underscore-free collection methods: .chhan (filter), .badal (map), .saja (sort)
    big_doubled = arr
        .chhan(x => x >= 10)
        .badal(x => x * 2)
        .saja()

    de big_doubled
}

dikha "Processed Scores:", process_scores(numbers)

// 4. Clean conditionals with agar, elif, warna
kaam rank(score) {
    agar score >= 80 {
        de "Aura +10000 (Topper)"
    } elif score >= 33 {
        de "Pass ho gaye mauj karo"
    } warna {
        de "Raju, padhai likhai mein dhyan lagao"
    }
}

dikha rank(85)
dikha rank(12)

// 5. Har item mein list iteration
total = 0
har n mein [10, 20, 30] {
    total = total + n
}
dikha "Total sum:", total

// 6. Underscore-free .kul (reduce), .dhund (find), .koi (some), .sab (every)
dikha "Sum via .kul():", [1, 2, 3, 4, 5].kul((acc, x) => acc + x, 0)
dikha "First > 20:", [5, 12, 25, 30].dhund(x => x > 20)
dikha "Any even? (.koi):", [1, 3, 4, 7].koi(x => x % 2 == 0)
dikha "All positive? (.sab):", [2, 4, 6].sab(x => x > 0)`
  },
  {
    id: 'pythonic',
    name: '🐍 Pythonic Transforms & Tantrik Methods',
    description: 'Arrow lambdas, collection pipelines (.chhaan, .sab_pe, .nichod), Tantrik dunders (__jod__, __dikha__), F-strings, and destructuring',
    code: `// 🐍 Pythonic Superpowers in MEMElang

// 1. Arrow Lambdas & Collection Pipeline
pakad scores = [15, 42, 88, 23, 95, 60];
pakad toppers = scores
    .chhaan(s => s >= 50)
    .sab_pe(s => s + 5)
    .line_mein((a, b) => a > b); // Sort descending

dikha "Toppers bonus scores:", toppers;

// 2. Tantrik Operator Overloading & F-Strings
biradari Wallet {
    jugaad shuru(rupaye) {
        khud.bal = rupaye;
    }
    jugaad __jod__(other) {
        ye_le Wallet(khud.bal + other.bal);
    }
    jugaad __dikha__() {
        ye_le f"Wallet(₹{khud.bal})";
    }
}

pakad w1 = Wallet(500);
pakad w2 = Wallet(250);
pakad total = w1 + w2;
dikha f"Total money: {total}";

// 3. Array & Map Destructuring
pakad [first, second, *rest] = [10, 20, 30, 40, 50];
dikha f"first: {first}, second: {second}, rest: {rest}";

pakad {naam, city} = { "naam": "Gabbru", "city": "Delhi" };
dikha f"Player: {naam} from {city}";`
  },
  {
    id: 'hello',
    name: '👋 Hello World',
    description: 'Basic variables, printing, and string multiplication',
    code: `// Welcome to MEMElang WebAssembly Edition!
pakad welcome = "Namaste Sansaar!";
dikha welcome;

pakad laughter = "ha" * 6;
dikha "Desi laughter:", laughter;

pakad a = 25;
pakad b = 17;
dikha "25 + 17 =", a + b;`
  },
  {
    id: 'input',
    name: '💬 Console Input (pooch / bata)',
    description: 'Interactive and pre-fed standard input with pooch()',
    stdin: `Baburao Apte
Mumbai
50`,
    code: `// Interactive Input with pooch() / bata()
dikha "=== MEMElang Desi Survey ===";
pakad naam = pooch("Aapka naam kya hai? ");
pakad shahar = pooch("Kaunse shahar se ho? ");
pakad umar = pooch("Aapki umar kitni hai? ");

dikha "";
dikha "Swagat hai", naam, "bhai from", shahar + "!";
dikha "5 saal baad aapki umar hogi:", bina_point(umar) + 5;`
  },
  {
    id: 'fibonacci',
    name: '🌀 Fibonacci Recursion',
    description: 'Recursive algorithm with range loop generator',
    code: `// Recursive Fibonacci in MEMElang
jugaad fib(n) {
    dekh n <= 0 {
        ye_le 0;
    }
    dekh n == 1 {
        ye_le 1;
    }
    ye_le fib(n - 1) + fib(n - 2);
}

dikha "--- Pehle 8 Fibonacci Numbers ---";
ghuma i se 0 se 7 {
    dikha "fib(" + kya_hai(i) + " " + i + ") =", fib(i);
}`
  },
  {
    id: 'fizzbuzz',
    name: '⚡ FizzBuzz Challenge',
    description: 'Conditionals and modulo loops from 1 to 15',
    code: `// Classic FizzBuzz with Desi Memes
ghuma n se 1 se 15 {
    dekh (n % 15 == 0) {
        dikha n, ": 🔥 FizzBuzz (Full Masala)";
    } warna dekh (n % 3 == 0) {
        dikha n, ": Fizz";
    } warna dekh (n % 5 == 0) {
        dikha n, ": Buzz";
    } warna {
        dikha n;
    }
}`
  },
  {
    id: 'oop',
    name: '👑 OOP & Nepotism',
    description: 'Classes, nepo inheritance, and dynamic methods',
    code: `// Classes & Nepotism
biradari Actor {
    jugaad shuru(naam) {
        apun.naam = naam;
        apun.talent = 10;
    }

    jugaad dialogue() {
        dikha apun.naam, "bolta hai: Mere paas talent hai!";
    }
}

biradari StarKid nepo Actor {
    jugaad shuru(naam, launch_budget) {
        apun.naam = naam;
        apun.budget = point_wala(launch_budget);
    }

    jugaad interview() {
        dikha apun.naam, "says: Humne bohot struggle kiya hai, budget only Rs.", apun.budget;
    }
}

pakad kid = StarKid("Shanaya", "50000000.0");
kid.dialogue();  // Inherited from Actor
kid.interview(); // Defined on StarKid`
  },
  {
    id: 'errors',
    name: '🎭 Meme Error Showcase',
    description: 'Uncomment any line to trigger iconic meme error dialogues',
    code: `// Uncomment ONE line at a time to test meme errors:

// 1. Babu Bhaiya divide by zero:
dikha 100 / 0;

// 2. Rasode mein kaun tha (undeclared variable):
// dikha rasode_ki_rashi;

// 3. Ashneer Grover Doglapan error:
// (x) = 50;

// 4. Moye Moye unclosed string:
// pakad s = "khula chhod diya;

// 5. Gangs of Wasseypur (Beta tumse na ho payega):
// pakad num = 42; num();`
  },
  {
    id: 'borrow-checker',
    name: '🛡️ Rust-Style Borrow Checker',
    description: 'Single ownership, moves, udhaar (&), udhaar_badal (&mut), and RAII alvida()',
    code: `// 🛡️ MEMElang Rust-Style Memory Safety Showcase

// 1. Explicit Cloning with nakal()
pakad secret = "Tees Maar Khan Khazana";
pakad safe_copy = nakal(secret);
dikha "Safe copy cloned:", safe_copy;

// 2. Shared Borrowing (udhaar)
pakad hero = "Majnu Bhai";
pakad r1 = udhaar hero;
pakad r2 = udhaar hero;
dikha "Shared read 1:", r1;
dikha "Shared read 2:", r2;

// 3. Mutable Borrowing (udhaar_badal) in Scopes
pakad score = 10;
{
    pakad m = udhaar_badal score;
    m = 100;
    dikha "Mutated through exclusive borrow:", m;
}
dikha "Score after scope borrow release:", score;

// 4. RAII Destructors (alvida)
biradari SafeLocker {
    jugaad shuru(name) {
        apun.name = name;
        dikha "🔓 [RAII] Locker opened:", apun.name;
    }
    jugaad alvida() {
        dikha "🔒 [RAII] Locker auto-dropped and sealed:", apun.name;
    }
}

{
    pakad locker = SafeLocker("Swiss Vault");
    dikha "Vault is in active use...";
}
dikha "Scope finished, locker was cleanly dropped!";`
  },
  {
    id: 'collections-demo',
    name: '📦 Native Collections & Maps',
    description: 'Dynamic arrays, hash maps with dot notation, and ghuma ... mein iteration',
    code: `// 📦 Native Collections Showcase
pakad team = ["Kabir", "Bikram", "Guddu"];
team.ghusa("Bablu");
dikha "Initial team:", team;
dikha "Captain (negative index):", team[-1];

// Hash map with dot notation
pakad stats = {
    "runs": 85,
    "wickets": 3,
    "mvp": han
};
dikha "Player runs:", stats.runs;
dikha "All stat keys:", stats.chaabi();

// Iteration with ghuma ... mein
dikha "\\n--- Roster ---";
ghuma player mein team {
    dikha "Player:", player;
}`
  },
  {
    id: 'error-handling-demo',
    name: '🚨 Error Handling (panga / lafda)',
    description: 'panga/lafda/pakka try-catch-finally and chillao exception raising',
    code: `// 🚨 User-Level Error Handling Demo
jugaad withdraw(balance, amount) {
    dekh amount > balance {
        chillao {
            "code": 402,
            "msg": "Aukaat ke bahar! Bank mein itna paisa nahi hai!"
        };
    }
    ye_le balance - amount;
}

panga {
    pakad bal = 500;
    dikha "Balance after 200 withdrawal:", withdraw(bal, 200);
    dikha "Attempting 1000 withdrawal...";
    dikha withdraw(bal, 1000); // Throws!
} lafda err {
    dikha "❌ Lafda pakda gaya!";
    dikha "Error Code:", err.code;
    dikha "Error Msg:", err.msg;
} pakka {
    dikha "🏦 Transaction session closed cleanly (pakka block always runs)!";
}`
  },
  {
    id: 'enums-pattern-matching',
    name: '🎯 Enums & Pattern Matching',
    description: 'Type-safe avtar enums and jaanch pattern matching with baaki/warna guards',
    code: `// 🎯 Enums (avtar) & Pattern Matching (jaanch)
avtar OrderState {
    Cart,
    Placed,
    Shipped,
    Delivered
}

jugaad trackOrder(state) {
    jaanch state {
        OrderState.Cart => {
            dikha "🛒 Cart mein samaan pada hai, checkout karo!";
        }
        OrderState.Placed => {
            dikha "📦 Order place ho gaya, packing shuru!";
        }
        OrderState.Shipped => {
            dikha "🚚 Delivery boy raste mein hai!";
        }
        OrderState.Delivered => {
            dikha "🎉 Samaan mil gaya! Rating de do bro!";
        }
        baaki => {
            dikha "Aayein? Unknown status!";
        }
    }
}

trackOrder(OrderState.Shipped);
trackOrder(OrderState.Delivered);`
  },
  {
    id: 'stdlib-demo',
    name: '🧮 Built-in Stdlib (hisab, waqt, json)',
    description: 'Math, high-precision timing, and JSON serialization/deserialization',
    code: `// 🧮 Built-in Standard Library Showcase

// 1. Math (hisab)
dikha "Math PI:", hisab.pi;
dikha "Square root of 81:", hisab.jadd(81);
dikha "Round 7.8:", hisab.gol(7.8);
dikha "Random lucky number (1-100):", hisab.tukka_num(1, 100);

// 2. JSON Serialization
pakad player = {
    "name": "Virat",
    "century": 80,
    "active": han
};
pakad jsonStr = json.likh(player);
dikha "Serialized JSON:", jsonStr;

pakad decoded = json.samajh(jsonStr);
dikha "Decoded player name:", decoded.name;

// 3. Time (waqt)
pakad start = waqt.ab();
so_ja(0.05); // Sleep 50ms
dikha "Elapsed seconds:", waqt.ab() - start;`
  }
];
