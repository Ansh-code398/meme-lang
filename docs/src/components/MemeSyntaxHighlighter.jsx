import React from 'react';

/**
 * High-performance, zero-dependency MEMElang syntax highlighter.
 * Tokenizes MEMElang source code into React syntax elements with Tailwind classes.
 */

const KEYWORDS = new Set([
  'pakad', 'le', 'let', 'const',
  'jugaad', 'kar', 'fn', 'def', 'kaam', 'ye_le', 'wapas', 'de', 'return',
  'biradari', 'vibe', 'class', 'sancha', 'roop', 'nepo', 'extends',
  'dekh', 'agar', 'elif', 'if', 'warna', 'nahito', 'else',
  'jabtak', 'while', 'ghoom', 'har', 'for', 'mein', 'in', 'bas', 'break', 'chal', 'continue',
  'panga', 'try', 'lafda', 'sambhal', 'catch', 'pakka', 'hamesha', 'finally',
  'chillao', 'fek', 'throw',
  'bula', 'chura', 'lao', 'shamil', 'import', 'jaise', 'as',
  'avtar', 'kism', 'enum', 'jaanch', 'parakh', 'match', 'baaki', 'default'
]);

const TANTRIK = new Set([
  '__jod__', '__ghata__', '__guna__', '__bhaag__', '__bacha__',
  '__barabar__', '__chota__', '__bada__',
  '__la__', '__daal__', '__rakh__',
  '__dikha__', '__size__', '__chala__', '__call__',
  '__add__', '__sub__', '__mul__', '__div__', '__mod__',
  '__eq__', '__lt__', '__gt__', '__getitem__', '__setitem__',
  '__str__', '__repr__', '__len__',
  'shuru', 'paida', 'alvida'
]);

const TRANSFORMS = new Set([
  'sab_pe', 'badal_sab', 'badal', 'map', 'chhaan', 'chhan', 'filter', 'filter_kar',
  'nichod', 'kul', 'jod', 'sameto', 'reduce', 'line_mein', 'saja', 'chhaant', 'sort',
  'har_ek', 'each', 'forEach', 'pakad_ek', 'dhund', 'find', 'koi', 'some', 'any', 'sab', 'saare', 'every', 'all',
  'chala_de',
  'daal', 'nikal', 'push', 'pop', 'append', 'ghusa', 'dhakka', 'laat_maar', 'nikaal',
  'ulta', 'reverse', 'ek_kar', 'unique', 'seedha', 'flatten',
  'gino', 'count', 'kahan', 'index_of', 'indexOf', 'tukde', 'chunk', 'tukka', 'sample', 'feko', 'shuffle',
  'pehla', 'first', 'aakhri', 'last', 'khali_hai', 'is_empty', 'isEmpty',
  'akshar', 'char_at', 'charAt', 'akshar_list', 'chars',
  'shuru_hota', 'starts_with', 'startsWith', 'khatam_hota', 'ends_with', 'endsWith',
  'duhrao', 'repeat', 'aage_bhar', 'pad_start', 'padStart', 'piche_bhar', 'pad_end', 'padEnd',
  'number_hai', 'is_digit', 'isDigit', 'akshar_hai', 'is_alpha', 'isAlpha',
  'space_hai', 'is_space', 'isSpace', 'bada_hai', 'is_upper', 'isUpper', 'chota_hai', 'is_lower', 'isLower',
  'laal', 'red', 'hara', 'green', 'peela', 'yellow', 'neela', 'blue',
  'gulabi', 'pink', 'magenta', 'asmani', 'cyan', 'safed', 'white', 'kala', 'black',
  'mota', 'bold', 'halka', 'dim', 'tedha', 'italic', 'line', 'underline',
  'rang_hata', 'strip_ansi',
  'jodo', 'hatao', 'chhodo', 'has', 'hai_kya', 'keys', 'chaabi', 'values', 'maal', 'nakal', 'clone', 'copy',
  'rakh', 'set', 'get', 'post', 'put', 'delete', 'route', 'request', 'chala',
  'khamba', 'column', 'katar', 'row', 'likh', 'text', 'dabba', 'badge', 'puch', 'button', 'faasla', 'divider', 'click', 'dabao'
]);

const BUILTINS = new Set([
  'dikha', 'flex', 'print', 'bol', 'pooch', 'bata', 'input',
  'systumm', 'sys', 'os', 'cli', 'app',
  'tijori', 'vault', 'db', 'dhaba', 'server', 'api', 'ui', 'view', 'gui',
  'table', 'dikha_mez', 'box', 'dikha_dabba', 'clear', 'saaf_kar',
  'useState', 'useEffect', 'useMemo', 'useRef', 'state_bana', 'bridge',
  'code_de', 'ord', 'akshar_bana', 'chr',
  'kya_hai', 'size', 'len', 'length', 'lambai', 'nakal', 'bina_point', 'point_wala', 'kundli',
  'gol', 'upar_gol', 'neeche_gol', 'vargmool', 'ghaat', 'kalam',
  'chhota', 'bada', 'ab', 'ruk', 'so_ja'
]);

const OOP_SELF = new Set(['khud', 'apun', 'apna', 'this', 'self', 'se', 'super']);

const COMPONENTS = new Set([
  'Card', 'CardHeader', 'CardTitle', 'CardContent', 'Button', 'Badge', 'Input', 'Separator', 'Alert'
]);

const CONSTANTS = new Set([
  'han', 'real', 'sahi', 'true', 'yes',
  'nhi', 'fake', 'galat', 'false', 'no', 'na',
  'nalla', 'shunya', 'null', 'nil', 'none', 'khali'
]);

const TYPES = new Set([
  'ank', 'bina_point', 'sankhya', 'int',
  'point', 'point_wala', 'dashamlav', 'float', 'double',
  'bol', 'shabd', 'vakya', 'baat', 'string', 'str',
  'sach', 'faisla', 'vibe_check', 'bool',
  'nalla', 'kuch_nhi', 'khali', 'void', 'null',
  'har_kism', 'joker', 'sab', 'kuch_bhi', 'any',
  'soochi', 'tolli', 'gang', 'list',
  'khata', 'kundli', 'map'
]);

export function highlightMemeCode(code) {
  if (!code) return [];

  const tokens = [];
  let i = 0;
  const len = code.length;

  while (i < len) {
    const ch = code[i];

    // 1. Single-line comment
    if (ch === '/' && code[i + 1] === '/') {
      let start = i;
      while (i < len && code[i] !== '\n') i++;
      tokens.push({ type: 'comment', value: code.slice(start, i) });
      continue;
    }

    // 2. Multi-line comment
    if (ch === '/' && code[i + 1] === '*') {
      let start = i;
      i += 2;
      while (i < len && !(code[i] === '*' && code[i + 1] === '/')) i++;
      if (i < len) i += 2;
      tokens.push({ type: 'comment', value: code.slice(start, i) });
      continue;
    }

    // 3. F-Strings (f"..." or f'...')
    if ((ch === 'f' || ch === 'F') && (code[i + 1] === '"' || code[i + 1] === "'")) {
      const quote = code[i + 1];
      let start = i;
      i += 2; // skip f and opening quote
      const parts = [{ type: 'fstring-prefix', value: code.slice(start, i) }];

      let litStart = i;
      while (i < len && code[i] !== quote) {
        if (code[i] === '\\') {
          i += 2;
          continue;
        }
        if (code[i] === '{' && code[i + 1] === '{') {
          i += 2;
          continue;
        }
        if (code[i] === '{') {
          if (i > litStart) {
            parts.push({ type: 'string', value: code.slice(litStart, i) });
          }
          let exprStart = i;
          let depth = 1;
          i++; // skip '{'
          while (i < len && depth > 0) {
            if (code[i] === '{') depth++;
            else if (code[i] === '}') depth--;
            else if (code[i] === '"' || code[i] === "'") {
              const innerQ = code[i++];
              while (i < len && code[i] !== innerQ) {
                if (code[i] === '\\') i++;
                i++;
              }
            }
            i++;
          }
          parts.push({ type: 'fstring-expr', value: code.slice(exprStart, i) });
          litStart = i;
          continue;
        }
        i++;
      }

      if (i > litStart) {
        parts.push({ type: 'string', value: code.slice(litStart, i) });
      }
      if (i < len && code[i] === quote) {
        parts.push({ type: 'fstring-quote', value: quote });
        i++; // skip closing quote
      }
      tokens.push({ type: 'fstring', parts });
      continue;
    }

    // 4. Regular Strings ("..." or '...')
    if (ch === '"' || ch === "'") {
      const quote = ch;
      let start = i;
      i++;
      while (i < len && code[i] !== quote) {
        if (code[i] === '\\') i += 2;
        else i++;
      }
      if (i < len) i++; // skip closing quote
      tokens.push({ type: 'string', value: code.slice(start, i) });
      continue;
    }

    // 5. Numbers (int / float)
    if (ch >= '0' && ch <= '9') {
      let start = i;
      while (i < len && code[i] >= '0' && code[i] <= '9') i++;
      if (code[i] === '.' && i + 1 < len && code[i + 1] >= '0' && code[i + 1] <= '9') {
        i++; // skip '.'
        while (i < len && code[i] >= '0' && code[i] <= '9') i++;
      }
      tokens.push({ type: 'number', value: code.slice(start, i) });
      continue;
    }

    // 6. Arrow operators `=>` and `->`
    if ((ch === '=' || ch === '-') && code[i + 1] === '>') {
      tokens.push({ type: 'arrow', value: code.slice(i, i + 2) });
      i += 2;
      continue;
    }

    // 7. Operators / Punctuation
    if ('+-*/%!=<>'.includes(ch)) {
      let start = i;
      if ((ch === '=' && code[i + 1] === '=') ||
          (ch === '!' && code[i + 1] === '=') ||
          (ch === '<' && code[i + 1] === '=') ||
          (ch === '>' && code[i + 1] === '=')) {
        i += 2;
      } else {
        i++;
      }
      tokens.push({ type: 'operator', value: code.slice(start, i) });
      continue;
    }

    // 8. Identifiers / Keywords / Tantrik
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch === '_') {
      let start = i;
      while (i < len && (
        (code[i] >= 'a' && code[i] <= 'z') ||
        (code[i] >= 'A' && code[i] <= 'Z') ||
        (code[i] >= '0' && code[i] <= '9') ||
        code[i] === '_'
      )) {
        i++;
      }
      const word = code.slice(start, i);

      // Check if preceded by dot '.' (e.g. .sab_pe, .chhaan)
      let isPrecededByDot = false;
      let checkIdx = start - 1;
      while (checkIdx >= 0 && (code[checkIdx] === ' ' || code[checkIdx] === '\t')) {
        checkIdx--;
      }
      if (checkIdx >= 0 && code[checkIdx] === '.') {
        isPrecededByDot = true;
      }

      if (TANTRIK.has(word)) {
        tokens.push({ type: 'tantrik', value: word });
      } else if (isPrecededByDot && TRANSFORMS.has(word)) {
        tokens.push({ type: 'transform', value: word });
      } else if (TRANSFORMS.has(word)) {
        tokens.push({ type: 'transform', value: word });
      } else if (TYPES.has(word)) {
        tokens.push({ type: 'type', value: word });
      } else if (KEYWORDS.has(word)) {
        tokens.push({ type: 'keyword', value: word });
      } else if (OOP_SELF.has(word)) {
        tokens.push({ type: 'self', value: word });
      } else if (BUILTINS.has(word)) {
        tokens.push({ type: 'builtin', value: word });
      } else if (COMPONENTS.has(word) || /^[A-Z][a-zA-Z0-9_]*$/.test(word)) {
        tokens.push({ type: 'component', value: word });
      } else if (CONSTANTS.has(word)) {
        tokens.push({ type: 'constant', value: word });
      } else {
        tokens.push({ type: 'identifier', value: word });
      }
      continue;
    }

    // 9. Everything else (whitespace, braces, semicolons)
    tokens.push({ type: 'plain', value: ch });
    i++;
  }

  return tokens;
}

export default function MemeSyntaxHighlighter({ code, className = '' }) {
  const tokens = highlightMemeCode(code);

  return (
    <span className={`font-mono leading-relaxed whitespace-pre ${className}`}>
      {tokens.map((token, idx) => {
        switch (token.type) {
          case 'comment':
            return <span key={idx} className="text-zinc-500 italic">{token.value}</span>;
          case 'keyword':
            return <span key={idx} className="text-purple-400 font-semibold">{token.value}</span>;
          case 'type':
            return <span key={idx} className="text-teal-400 font-semibold">{token.value}</span>;
          case 'tantrik':
            return <span key={idx} className="text-amber-300 font-bold">{token.value}</span>;
          case 'transform':
            return <span key={idx} className="text-cyan-400 font-medium">{token.value}</span>;
          case 'builtin':
            return <span key={idx} className="text-sky-400 font-medium">{token.value}</span>;
          case 'component':
            return <span key={idx} className="text-amber-300 font-bold">{token.value}</span>;
          case 'self':
            return <span key={idx} className="text-rose-400 font-medium">{token.value}</span>;
          case 'constant':
            return <span key={idx} className="text-orange-400 font-semibold">{token.value}</span>;
          case 'number':
            return <span key={idx} className="text-emerald-400">{token.value}</span>;
          case 'string':
            return <span key={idx} className="text-lime-300">{token.value}</span>;
          case 'arrow':
            return <span key={idx} className="text-amber-400 font-bold">{token.value}</span>;
          case 'operator':
            return <span key={idx} className="text-pink-400">{token.value}</span>;
          case 'fstring':
            return (
              <span key={idx} className="text-lime-300">
                {token.parts.map((p, pIdx) => {
                  if (p.type === 'fstring-expr') {
                    return (
                      <span key={pIdx} className="text-zinc-100 font-normal">
                        <span className="text-amber-400 font-bold">&#123;</span>
                        <MemeSyntaxHighlighter code={p.value.slice(1, -1)} />
                        <span className="text-amber-400 font-bold">&#125;</span>
                      </span>
                    );
                  }
                  return <span key={pIdx}>{p.value}</span>;
                })}
              </span>
            );
          default:
            return <span key={idx}>{token.value}</span>;
        }
      })}
    </span>
  );
}
