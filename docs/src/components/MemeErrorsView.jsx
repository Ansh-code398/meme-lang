import React, { useState } from 'react';
import { AlertTriangle, Play, Search, Terminal, Loader2 } from 'lucide-react';
import { runMemeCode } from '../wasm/memeRunner';

const ERROR_CATALOG = [
  {
    title: "Babu Bhaiya Divide By Zero",
    scenario: "Division by 0 (e.g. 10 / 0)",
    quote: "Kya coder banega re tu? 0 se divide karke brahmand khatam karega kya!",
    origin: "Hera Pheri (Babu Bhaiya)",
    category: "Arithmetic",
    code: `pakad a = 10;
pakad b = 0;
dikha a / b;`
  },
  {
    title: "Babu Bhaiya Modulo By Zero",
    scenario: "Modulo by 0 (e.g. 15 % 0)",
    quote: "0 se modulo (%) karne chale ho? Maths teacher ko heart attack aa jayega!",
    origin: "School Trauma / Hera Pheri",
    category: "Arithmetic",
    code: `pakad x = 15;
dikha x % 0;`
  },
  {
    title: "Chhoti Bachi Ho Kya (Missing Semicolon)",
    scenario: "Statement without trailing semicolon ';'",
    quote: "Chhoti bachi ho kya?! Line ke end mein ';' lagana bhool gaye!",
    origin: "Tiger Shroff (Heropanti)",
    category: "Syntax",
    code: `pakad a = 42
dikha a;`
  },
  {
    title: "Ashneer Grover Doglapan",
    scenario: "Invalid left-hand side in assignment (e.g. (x) = 5)",
    quote: "Ye sab doglapan hai! '=' ke left side mein sirf seedha variable ya property aata hai, koi equation ya bracket nahi!",
    origin: "Ashneer Grover (Shark Tank India)",
    category: "Syntax",
    code: `pakad x = 10;
(x) = 20;`
  },
  {
    title: "Control Uday Control",
    scenario: "Using single '=' inside an if condition",
    quote: "Control Uday, control! 'dekh' mein '=' (assign) nahi, '==' (compare) lagta hai!",
    origin: "Majnu Bhai / Uday Shetty (Welcome)",
    category: "Syntax",
    code: `pakad x = 5;
dekh x = 10 {
    dikha "Condition met";
}`
  },
  {
    title: "Rasode Mein Kaun Tha (Undeclared Read)",
    scenario: "Referencing an undeclared identifier",
    quote: "Rasode mein kaun tha?! Ye '<name>' bina bataye kahan se aaya? Pehle 'pakad <name>;' se declare kar!",
    origin: "Kokilaben (Saath Nibhaana Saathiya)",
    category: "Runtime",
    code: `dikha rasode_ki_rashi;`
  },
  {
    title: "Chuna Laga Diya (Undeclared Assignment)",
    scenario: "Assigning to a variable that wasn't declared with pakad",
    quote: "Chuna laga diya! '<name>' naam ka koi variable hi nahi hai. Pehle 'pakad <name>;' likh!",
    origin: "Panchayat / Indian Memes",
    category: "Runtime",
    code: `chuna = 500;`
  },
  {
    title: "String Khol Ke Chhod Diya",
    scenario: "Unterminated string literal",
    quote: "😒 String khol ke chhod diya... band tera baap karega?",
    origin: "Classic Desi Parent Scolding",
    category: "Lexer",
    code: `pakad s = "khula reh gaya;
dikha s;`
  },
  {
    title: "Moye Moye (EOF Mid-Expression)",
    scenario: "Code cuts off abruptly mid-expression",
    quote: "Moye Moye! Expression adhoori chhod ke code beech mein hi khatam kar diya.",
    origin: "Viral Moye Moye Meme",
    category: "Parser",
    code: `pakad x = 10 +`
  },
  {
    title: "Chi Sasura (Alien Symbol)",
    scenario: "Typing unmapped symbols like @ or $",
    quote: "😭 Chi sasura! Ye kaisa alien symbol ghusa diye ho code mein: @",
    origin: "Gangs of Wasseypur",
    category: "Lexer",
    code: `pakad x = @123;`
  },
  {
    title: "Paisa Double Overflow",
    scenario: "64-bit integer addition overflow",
    quote: "25 din mein paisa double karte-karte integer overflow ho gaya! Aukaat ke bahar chala gaya jod.",
    origin: "Anuradha / Phir Hera Pheri",
    category: "Arithmetic",
    code: `// Max 64-bit int overflow
pakad big = 9223372036854775807;
dikha big + 1;`
  },
  {
    title: "Delulu is Not the Solulu",
    scenario: "Multiplying a string by a negative number",
    quote: "Delulu is not the solulu! String ko negative baar kaise multiply karoge?",
    origin: "Gen-Z Pop Culture",
    category: "Arithmetic",
    code: `pakad str = "swag" * -3;`
  },
  {
    title: "Seb Ko Santre Se Mat Tolo",
    scenario: "Comparing incompatible types (e.g. 10 < 'hello')",
    quote: "Seb ko santre se mat tolo! 'int' aur 'string' ko compare nahi kar sakte!",
    origin: "Desi Idiom",
    category: "Type Error",
    code: `dekh 10 < "hello" {
    dikha "Kamaal hai";
}`
  },
  {
    title: "Beta Tumse Na Ho Payega",
    scenario: "Calling a non-function value (e.g. 42())",
    quote: "Beta, tumse na ho payega! '42' koi jugaad (function) thodi hai jo call kar rahe ho!",
    origin: "Ramadhir Singh (Gangs of Wasseypur)",
    category: "Runtime",
    code: `pakad x = 42;
x();`
  },
  {
    title: "Khopdi Tod Saale Ka (Missing OOP Property)",
    scenario: "Accessing a non-existent method or property on an object",
    quote: "Khopdi tod saale ka! 'Influencer' biradari ke paas 'humble' naam ka kuch nahi hai!",
    origin: "Babu Bhaiya (Hera Pheri)",
    category: "OOP",
    code: `biradari Influencer {
    jugaad shuru(naam) {
        apun.naam = naam;
    }
}
pakad i = Influencer("Orry");
dikha i.humble;`
  },
  {
    title: "Fake Nepotism",
    scenario: "Inheriting from a primitive value instead of a class",
    quote: "Fake nepotism! 'fake_parent' koi biradari (class) nahi hai, isko inherit kaise karoge?",
    origin: "Bollywood Industry Debate",
    category: "OOP",
    code: `pakad fake_parent = 42;
biradari Starkid nepo fake_parent {
}`
  },
  {
    title: "Gaddari Korbe (Arity Mismatch)",
    scenario: "Calling a function with wrong number of arguments",
    quote: "Gaddari korbe! Jugaad ne 2 arguments maange the, aur tumne 1 chipka diye!",
    origin: "Viral Bengali Meme",
    category: "Runtime",
    code: `jugaad jod(a, b) {
    ye_le a + b;
}
jod(10);`
  },
  {
    title: "Pehli Fursat Mein Nikal (Use After Move)",
    scenario: "Reading a heap variable after transferring its ownership",
    quote: "Pehli fursat mein nikal! 'a' ki value move ho chuki hai! Ownership transfer ke baad use karna mana hai!",
    origin: "Hindustani Bhau",
    category: "Ownership & Borrowing",
    code: `pakad a = "Baburao ka khazana";
pakad b = a;
dikha a;`
  },
  {
    title: "Doglapan (Mutating While Borrowed)",
    scenario: "Attempting to modify a variable while an immutable borrow (udhaar) is active",
    quote: "Doglapan! 'x' abhi udhaar (borrowed) pe hai, isko modify nahi kar sakte!",
    origin: "Ashneer Grover (Shark Tank India)",
    category: "Ownership & Borrowing",
    code: `pakad x = "shuru";
pakad r = udhaar x;
x = "badal diya";`
  },
  {
    title: "Beech Mein Mat Bolo (Exclusive Borrow Access)",
    scenario: "Accessing owner directly while exclusively borrowed (udhaar_badal)",
    quote: "Beech mein mat bolo! 'x' abhi kisi aur ko udhaar diya hua hai!",
    origin: "TV News Debate Meme",
    category: "Ownership & Borrowing",
    code: `pakad x = "Pavitra data";
pakad m = udhaar_badal x;
dikha x;`
  },
  {
    title: "Ek Mawaali Ek Time Pe (Borrow Conflict)",
    scenario: "Attempting a mutable borrow while a shared borrow is active",
    quote: "Ek mawaali ek time pe ek hi jagah! 'x' already udhaar (borrowed) pe hai, exclusive borrow nahi mil sakta!",
    origin: "Welcome Movie",
    category: "Ownership & Borrowing",
    code: `pakad x = "Khandaani haar";
pakad r = udhaar x;
pakad m = udhaar_badal x;`
  },
  {
    title: "Baburao Ka Chashma (Dangling Reference Escape)",
    scenario: "Returning or escaping a reference to a temporary local variable",
    quote: "Baburao ka chashma! Local variable 'local_var' ka udhaar wapas nahi bhej sakte, scope khatam hote hi mar jayega!",
    origin: "Hera Pheri (Baburao)",
    category: "Ownership & Borrowing",
    code: `jugaad leaky() {
    pakad local_var = "Temporary secret";
    ye_le udhaar local_var;
}
pakad r = leaky();`
  }
];

export default function MemeErrorsView({ onOpenInPlayground }) {
  const [searchTerm, setSearchTerm] = useState('');
  const [runningIdx, setRunningIdx] = useState(null);
  const [errorOutputs, setErrorOutputs] = useState({});

  const handleTestError = async (idx, code) => {
    setRunningIdx(idx);
    try {
      const res = await runMemeCode(code);
      setErrorOutputs((prev) => ({ ...prev, [idx]: res.output }));
    } catch (e) {
      setErrorOutputs((prev) => ({ ...prev, [idx]: e.message }));
    } finally {
      setRunningIdx(null);
    }
  };

  const filtered = ERROR_CATALOG.filter((item) => {
    if (!searchTerm) return true;
    const term = searchTerm.toLowerCase();
    return (
      item.title.toLowerCase().includes(term) ||
      item.quote.toLowerCase().includes(term) ||
      item.origin.toLowerCase().includes(term) ||
      item.scenario.toLowerCase().includes(term)
    );
  });

  return (
    <div className="max-w-6xl mx-auto px-4 lg:px-8 py-8">
      {/* Header */}
      <div className="mb-8">
        <div className="inline-flex items-center gap-1.5 px-2.5 py-1 rounded-md text-xs font-mono font-medium bg-zinc-900 text-zinc-300 border border-zinc-700/60 mb-3">
          <AlertTriangle className="w-3.5 h-3.5 text-amber-400" />
          The Diagnostic Catalog
        </div>
        <h1 className="text-3xl font-bold tracking-tight text-zinc-100 mb-3">
          Iconic Meme Errors
        </h1>
        <p className="text-zinc-400 text-sm max-w-2xl leading-relaxed">
          MEMElang replaces unhelpful C++ & compiler errors with iconic dialogues from Hera Pheri, Welcome, Shark Tank, and Wasseypur. Click <strong className="text-zinc-200">Trigger Error Live</strong> on any card to execute it via WebAssembly in your browser.
        </p>

        {/* Search Filter with WCAG AA Contrast */}
        <div className="relative mt-6 max-w-md">
          <Search className="w-4 h-4 text-zinc-400 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none" />
          <input
            type="text"
            placeholder="Search meme errors (e.g. babu bhaiya, divide)..."
            value={searchTerm}
            onChange={(e) => setSearchTerm(e.target.value)}
            className="w-full bg-zinc-900 border border-zinc-700 rounded-lg pl-10 pr-4 py-2 text-sm text-zinc-100 placeholder:text-zinc-400 focus:outline-none focus:border-zinc-500 transition-colors"
          />
        </div>
      </div>

      {/* Grid of Error Cards */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-5">
        {filtered.map((item, idx) => (
          <div
            key={idx}
            className="rounded-lg border border-zinc-800 bg-zinc-900/40 p-5 flex flex-col justify-between hover:border-zinc-750 transition-colors shadow-sm"
          >
            <div>
              <div className="flex items-center justify-between gap-2 mb-2">
                <span className="text-xs font-mono px-2 py-0.5 rounded bg-zinc-800 text-zinc-300 border border-zinc-700/50">
                  {item.category}
                </span>
                <span className="text-xs font-medium text-zinc-400">
                  {item.origin}
                </span>
              </div>

              <h2 className="text-base font-semibold text-zinc-100 mb-1">
                {item.title}
              </h2>

              <div className="text-xs text-zinc-400 mb-3 font-mono">
                Trigger: <span className="text-zinc-300">{item.scenario}</span>
              </div>

              {/* The Meme Quote Banner */}
              <div className="p-3.5 rounded-md bg-zinc-950 border border-zinc-800 text-zinc-200 text-xs leading-relaxed mb-4">
                "{item.quote}"
              </div>
            </div>

            <div>
              {/* Trigger Code */}
              <div className="bg-zinc-950 rounded-md p-3 text-xs font-mono text-zinc-300 border border-zinc-850 mb-3 overflow-x-auto">
                <pre><code>{item.code}</code></pre>
              </div>

              {/* Execution result if tested */}
              {errorOutputs[idx] && (
                <div className="mb-3 p-3 rounded-md bg-rose-950/20 border border-rose-900/40 text-xs font-mono text-rose-200 whitespace-pre-wrap leading-relaxed">
                  {errorOutputs[idx]}
                </div>
              )}

              {/* Action Buttons */}
              <div className="flex items-center gap-2">
                <button
                  onClick={() => handleTestError(idx, item.code)}
                  disabled={runningIdx === idx}
                  className="flex-1 flex items-center justify-center gap-1.5 py-1.5 px-3 rounded-md bg-zinc-100 hover:bg-white text-zinc-950 font-semibold text-xs transition-colors shadow-sm disabled:opacity-50"
                >
                  {runningIdx === idx ? (
                    <Loader2 className="w-3.5 h-3.5 animate-spin" />
                  ) : (
                    <Play className="w-3.5 h-3.5 fill-current" />
                  )}
                  <span>{runningIdx === idx ? 'Triggering...' : 'Trigger Error Live'}</span>
                </button>

                {onOpenInPlayground && (
                  <button
                    onClick={() => onOpenInPlayground(item.code)}
                    className="p-1.5 rounded-md bg-zinc-800 hover:bg-zinc-700 text-zinc-300 hover:text-white border border-zinc-700/60 transition-colors text-xs"
                    title="Open in Playground"
                  >
                    <Terminal className="w-4 h-4 text-zinc-400" />
                  </button>
                )}
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
