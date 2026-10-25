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
          ['int', '42, -100', '0 is falsy, others truthy', '64-bit signed integer (`long long`)'],
          ['float', '3.14, -0.5', '0.0 is falsy, others truthy', 'IEEE 754 double precision (`double`)'],
          ['string', '"namaste", \'bro\'', 'Empty "" is falsy', 'Supports string multiplication `"ha" * 3`'],
          ['bool', 'real / han, cap / nhi', 'real is truthy, cap is falsy', 'Gen-Z: `real`/`cap`, Desi: `han`/`nhi`'],
          ['nalla', 'nalla / kuch_nhi', 'Always falsy', 'Null value ("unemployed / empty")'],
          ['function', 'jugaad add(a, b) { ... }', 'Always truthy', 'First-class callable entity'],
          ['object', 'Influencer("Orry")', 'Always truthy', 'Instance of a `biradari` class']
        ]
      },
      {
        type: 'heading',
        level: 2,
        text: 'Interactive Type Inspection with kya_hai()'
      },
      {
        type: 'code',
        title: 'types.meme',
        code: `dikha kya_hai(100);
dikha kya_hai(3.14);
dikha kya_hai("Sholay");
dikha kya_hai(real);
dikha kya_hai(nalla);`
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
    title: 'User-Level Error Handling (panga / lafda)',
    badge: 'Exceptions',
    description: 'Catch runtime errors with `panga`, handle with `lafda`, guarantee execution with `pakka`, and throw with `chillao`.',
    content: [
      {
        type: 'text',
        text: `MEMElang provides robust exception handling inspired by modern systems languages. Errors can be raised with ` + '`chillao`' + ` and caught using ` + '`panga`' + ` / ` + '`lafda`' + ` / ` + '`pakka`' + ` blocks.`
      },
      {
        type: 'code',
        title: 'try_catch_finally.meme',
        code: `jugaad divide(a, b) {
    dekh b == 0 {
        chillao "0 se divide karne ki koshish mat kar!";
    }
    ye_le a / b;
}

panga {
    dikha "Dividing 10 by 2:", divide(10, 2);
    dikha "Dividing 10 by 0:", divide(10, 0);
} lafda err {
    dikha "Lafda pakda gaya:", err;
} pakka {
    dikha "Cleanup complete: pakka (finally) hamesha chalta hai!";
}`
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
  }
];

export const PLAYGROUND_PRESETS = [
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
