const vscode = require('vscode');

/**
 * Autocomplete items definitions for MEMElang
 */
const KEYWORDS = [
  { label: 'pakad', detail: 'Variable declaration', doc: 'Declare a new variable with lexical scoping (e.g. `pakad x = 10;`)' },
  { label: 'jugaad', detail: 'Function declaration', doc: 'Define a named function or anonymous closure (e.g. `jugaad calc(a, b = 0) { ... }`)' },
  { label: 'ye_le', detail: 'Return statement', doc: 'Return a value from a function (e.g. `ye_le result;`)' },
  { label: 'biradari', detail: 'Class declaration', doc: 'Define a new class/blueprint (e.g. `biradari Khiladi { ... }`)' },
  { label: 'nepo', detail: 'Class inheritance', doc: 'Inherit from another biradari (e.g. `biradari Beta nepo Baap`)' },
  { label: 'khud', detail: 'Instance reference (this)', doc: 'Current instance pointer inside methods (e.g. `khud.balance`)' },
  { label: 'apun', detail: 'Instance alias (self)', doc: 'Desi alias for current instance (e.g. `apun.naam`)' },
  { label: 'se', detail: 'Super class reference', doc: 'Call superclass methods (e.g. `se.shuru(...)`)' },
  { label: 'dekh', detail: 'If condition', doc: 'Branch control flow if condition is truthy (e.g. `dekh runs > 50 { ... }`)' },
  { label: 'warna', detail: 'Else condition', doc: 'Fallback branch when dekh condition is false (e.g. `warna { ... }`)' },
  { label: 'jabtak', detail: 'While loop', doc: 'Loop repeatedly while condition is truthy (e.g. `jabtak i < 10 { ... }`)' },
  { label: 'ghoom', detail: 'For loop', doc: 'Iterate over ranges or collections (e.g. `ghoom pakad x mein list { ... }`)' },
  { label: 'mein', detail: 'In keyword', doc: 'Membership operator in ghoom loop (e.g. `mein arr`)' },
  { label: 'bas', detail: 'Break statement', doc: 'Break out of the current loop immediately' },
  { label: 'chal', detail: 'Continue statement', doc: 'Skip to the next iteration of the loop' },
  { label: 'panga', detail: 'Try block', doc: 'Begin a safe block to catch runtime errors (e.g. `panga { ... }`)' },
  { label: 'lafda', detail: 'Catch block', doc: 'Catch exceptions thrown in panga block (e.g. `lafda err { ... }`)' },
  { label: 'pakka', detail: 'Finally block', doc: 'Always executes regardless of error (e.g. `pakka { ... }`)' },
  { label: 'chillao', detail: 'Throw error', doc: 'Throw a custom meme error exception (e.g. `chillao "Galat baat!";`)' },
  { label: 'bula', detail: 'Module import', doc: 'Import a module or file (e.g. `bula "hisab" jaise math;`)' },
  { label: 'jaise', detail: 'Import alias (as)', doc: 'Alias imported module identifier' },
  { label: 'avtar', detail: 'Enum declaration', doc: 'Define an enumeration with variants (e.g. `avtar Disha { UTTAR, DAKSHIN }`)' },
  { label: 'jaanch', detail: 'Pattern match (switch)', doc: 'Match value against enum variants or patterns (e.g. `jaanch dir { ... }`)' },
  { label: 'baaki', detail: 'Default pattern / Variadic rest', doc: 'Default match case or variadic rest parameter (*baaki)' },
  { label: 'han', detail: 'Boolean true', doc: 'Truthy boolean constant (synonym: real, sahi)' },
  { label: 'nhi', detail: 'Boolean false / Logical NOT', doc: 'Falsy boolean constant or prefix logical not' },
  { label: 'nalla', detail: 'Null / Monostate', doc: 'Empty, uninitialized or missing value constant (null)' }
];

const TYPES = [
  { label: 'ank', detail: 'Type: 64-bit integer', doc: 'Primitive 64-bit signed integer type (e.g. `pakad x: ank = 42;`)' },
  { label: 'point', detail: 'Type: 64-bit float', doc: 'Primitive double-precision floating point type (e.g. `pakad pi: point = 3.14;`)' },
  { label: 'bol', detail: 'Type: String', doc: 'Primitive UTF-8 string text type (e.g. `pakad name: bol = "MEME";`)' },
  { label: 'sach', detail: 'Type: Boolean', doc: 'Primitive boolean truth value type (`han` / `nhi`)' },
  { label: 'nalla', detail: 'Type: Null / Void', doc: 'Null or void return type constant' },
  { label: 'soochi', detail: 'Type: List array', doc: 'Generic dynamic list collection type (e.g. `soochi[ank]`)' },
  { label: 'list', detail: 'Type: List array alias', doc: 'Generic dynamic list collection type (e.g. `list[ank]`)' },
  { label: 'khata', detail: 'Type: Hash map', doc: 'Generic key-value map collection type (e.g. `khata[bol, ank]`)' },
  { label: 'map', detail: 'Type: Hash map alias', doc: 'Generic key-value map collection type (e.g. `map[bol, ank]`)' },
  { label: 'har_kism', detail: 'Type: Dynamic any', doc: 'Dynamic escape hatch type that bypasses static type checks' },
  { label: 'any', detail: 'Type: Dynamic any alias', doc: 'Dynamic escape hatch type' }
];

const BUILTINS = [
  { label: 'dikha', detail: 'Builtin: Print to output', doc: 'Print expressions separated by space, ending with newline (e.g. `dikha "Hello", name;`)' },
  { label: 'pooch', detail: 'Builtin: Console / stdin input', doc: 'Prompt user for input and return string (e.g. `pakad name = pooch("Naam? ");`)' },
  { label: 'size', detail: 'Builtin: Length / size of collection', doc: 'Returns length of string, list, map, or custom object with __size__ (e.g. `size(arr)`)' },
  { label: 'kya_hai', detail: 'Builtin: Type name inspection', doc: 'Returns string representation of type (e.g. "number", "string", "list", "khata", "object")' },
  { label: 'kundli', detail: 'Builtin: Reflection inspector', doc: 'Returns map of object fields or list of class methods (e.g. `kundli(obj)` or `kundli(Class)`)' },
  { label: 'nakal', detail: 'Builtin: Deep clone', doc: 'Creates a deep copy clone of any value (e.g. `pakad copy = nakal(obj);`)' },
  { label: 'bina_point', detail: 'Builtin: Integer conversion', doc: 'Converts float, string, or boolean to integer (e.g. `bina_point("42")`)' },
  { label: 'point_wala', detail: 'Builtin: Float conversion', doc: 'Converts integer or string to double float (e.g. `point_wala("3.14")`)' },
  { label: 'ruk', detail: 'Builtin: Sleep', doc: 'Pause execution for N milliseconds (e.g. `ruk(500);`)' },
  { label: 'ab', detail: 'Builtin: Current timestamp', doc: 'Returns current Unix epoch time in milliseconds' }
];

const TRANSFORMS = [
  { label: 'sab_pe', detail: 'Transform: map(fn)', doc: 'Applies fn(item) or fn(item, idx) to each element and returns new Array (e.g. `arr.sab_pe(x => x * 2)`)' },
  { label: 'badal_sab', detail: 'Transform: map(fn) alias', doc: 'Alias for .sab_pe(fn)' },
  { label: 'chhaan', detail: 'Transform: filter(fn)', doc: 'Filters elements where fn(item) evaluates to truthy (e.g. `arr.chhaan(x => x % 2 == 0)`)' },
  { label: 'filter_kar', detail: 'Transform: filter(fn) alias', doc: 'Alias for .chhaan(fn)' },
  { label: 'nichod', detail: 'Transform: reduce(fn, init?)', doc: 'Folds list from left to right using acc = fn(acc, item) (e.g. `arr.nichod((acc, x) => acc + x, 0)`)' },
  { label: 'sameto', detail: 'Transform: reduce(fn, init?) alias', doc: 'Alias for .nichod(fn, init)' },
  { label: 'line_mein', detail: 'Transform: sort(comparator?)', doc: 'Sorts list in-place and returns it (uses __chota__ for custom instances)' },
  { label: 'chhaant', detail: 'Transform: sort alias', doc: 'Alias for .line_mein(comparator)' },
  { label: 'har_ek', detail: 'Transform: forEach(fn)', doc: 'Executes fn(item) for each element without building a new list' },
  { label: 'pakad_ek', detail: 'Transform: find(fn)', doc: 'Returns the first element matching predicate fn(item), or nalla if none match' },
  { label: 'chala_de', detail: 'Reflection: Dynamic method invoke', doc: 'Dynamically calls a method by string name (e.g. `obj.chala_de("method_naam", [args])`)' },
  { label: 'jodo', detail: 'List: append(val)', doc: 'Appends an element to the list in-place' },
  { label: 'hatao', detail: 'List: pop()', doc: 'Removes and returns the last element of the list' },
  { label: 'has', detail: 'Map: containsKey(key)', doc: 'Returns han if key exists in map, else nhi' },
  { label: 'keys', detail: 'Map: keys()', doc: 'Returns a list containing all keys in map' },
  { label: 'values', detail: 'Map: values()', doc: 'Returns a list containing all values in map' }
];

const TANTRIK_METHODS = [
  { label: '__jod__', detail: 'Tantrik: Overload +', doc: 'Invoked by `a + b`. Must accept (other) and return result.' },
  { label: '__ghata__', detail: 'Tantrik: Overload -', doc: 'Invoked by `a - b`. Must accept (other) and return result.' },
  { label: '__guna__', detail: 'Tantrik: Overload *', doc: 'Invoked by `a * b`. Must accept (other) and return result.' },
  { label: '__bhaag__', detail: 'Tantrik: Overload /', doc: 'Invoked by `a / b`. Must accept (other) and return result.' },
  { label: '__bacha__', detail: 'Tantrik: Overload %', doc: 'Invoked by `a % b` (modulo). Must accept (other) and return result.' },
  { label: '__barabar__', detail: 'Tantrik: Overload ==', doc: 'Invoked by `a == b` (and auto-derives `!=`). Must return boolean.' },
  { label: '__chota__', detail: 'Tantrik: Overload <', doc: 'Invoked by `a < b` (and auto-derives `>`, `<=`, `>=`). Also drives .line_mein() sorting.' },
  { label: '__bada__', detail: 'Tantrik: Overload >', doc: 'Invoked by `a > b` when explicitly defined.' },
  { label: '__la__', detail: 'Tantrik: Indexing read obj[key]', doc: 'Invoked by `obj[key]`. Must accept (key) and return value.' },
  { label: '__daal__', detail: 'Tantrik: Indexing write obj[key] = val', doc: 'Invoked by `obj[key] = val`. Must accept (key, val).' },
  { label: '__rakh__', detail: 'Tantrik: Indexing write alias', doc: 'Alias for __daal__(key, val).' },
  { label: '__dikha__', detail: 'Tantrik: String conversion', doc: 'Invoked by `dikha obj`, `"str" + obj`, and `f"{obj}"`. Must return string.' },
  { label: '__size__', detail: 'Tantrik: Length measurement', doc: 'Invoked by `size(obj)`. Must return integer length.' },
  { label: '__chala__', detail: 'Tantrik: Function call obj(...)', doc: 'Invoked when calling object instance like a function: `obj(args...)`.' },
  { label: '__call__', detail: 'Tantrik: Function call alias', doc: 'Alias for __chala__(args...).' },
  { label: 'shuru', detail: 'OOP: Constructor', doc: 'Constructor initializer called on class instantiation: `Class(args)`' },
  { label: 'alvida', detail: 'OOP: RAII Destructor', doc: 'Destructor called automatically when object scope ends' }
];

function activate(context) {
  // Provider for general completions
  const completionProvider = vscode.languages.registerCompletionItemProvider(
    'memelang',
    {
      provideCompletionItems(document, position, token, context) {
        const linePrefix = document.lineAt(position).text.substr(0, position.character);

        // If typing after a dot '.', suggest functional collection transforms & methods
        if (linePrefix.endsWith('.')) {
          return TRANSFORMS.map((m) => {
            const item = new vscode.CompletionItem(m.label, vscode.CompletionItemKind.Method);
            item.detail = m.detail;
            item.documentation = new vscode.MarkdownString(m.doc);
            return item;
          });
        }

        // If typing double underscore '__', suggest Tantrik methods
        if (linePrefix.endsWith('__') || linePrefix.includes('jugaad __')) {
          return TANTRIK_METHODS.map((m) => {
            const item = new vscode.CompletionItem(m.label, vscode.CompletionItemKind.Method);
            item.detail = m.detail;
            item.documentation = new vscode.MarkdownString(m.doc);
            return item;
          });
        }

        // If typing after a colon ':', suggest types
        if (linePrefix.trimEnd().endsWith(':')) {
          return TYPES.map((t) => {
            const item = new vscode.CompletionItem(t.label, vscode.CompletionItemKind.TypeParameter);
            item.detail = t.detail;
            item.documentation = new vscode.MarkdownString(t.doc);
            return item;
          });
        }

        const items = [];

        // Add Types
        for (const t of TYPES) {
          const item = new vscode.CompletionItem(t.label, vscode.CompletionItemKind.TypeParameter);
          item.detail = t.detail;
          item.documentation = new vscode.MarkdownString(t.doc);
          items.push(item);
        }

        // Add Keywords
        for (const k of KEYWORDS) {
          const item = new vscode.CompletionItem(k.label, vscode.CompletionItemKind.Keyword);
          item.detail = k.detail;
          item.documentation = new vscode.MarkdownString(k.doc);
          items.push(item);
        }

        // Add Builtins
        for (const b of BUILTINS) {
          const item = new vscode.CompletionItem(b.label, vscode.CompletionItemKind.Function);
          item.detail = b.detail;
          item.documentation = new vscode.MarkdownString(b.doc);
          items.push(item);
        }

        // Add Tantrik Methods
        for (const t of TANTRIK_METHODS) {
          const item = new vscode.CompletionItem(t.label, vscode.CompletionItemKind.Function);
          item.detail = t.detail;
          item.documentation = new vscode.MarkdownString(t.doc);
          items.push(item);
        }

        // Add Transforms
        for (const tr of TRANSFORMS) {
          const item = new vscode.CompletionItem(tr.label, vscode.CompletionItemKind.Method);
          item.detail = tr.detail;
          item.documentation = new vscode.MarkdownString(tr.doc);
          items.push(item);
        }

        return items;
      }
    },
    '.', '_', ':' // Trigger completion characters
  );

  // Provider for Hover Documentation
  const hoverProvider = vscode.languages.registerHoverProvider('memelang', {
    provideHover(document, position, token) {
      const range = document.getWordRangeAtPosition(position);
      if (!range) return null;
      const word = document.getText(range);

      const all = [...TYPES, ...KEYWORDS, ...BUILTINS, ...TRANSFORMS, ...TANTRIK_METHODS];
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

  context.subscriptions.push(completionProvider, hoverProvider);
}

function deactivate() {}

module.exports = {
  activate,
  deactivate
};
