import React, { useState, useEffect, useRef } from 'react';
import {
  Play, RotateCcw, Copy, Check, Terminal, Loader2,
  AlertTriangle, MessageSquare, Sparkles, Zap
} from 'lucide-react';
import { runMemeCode, dispatchUiEvent } from '../wasm/memeRunner';
import { PLAYGROUND_PRESETS } from '../data/docsContent';
import MemeSyntaxHighlighter from './MemeSyntaxHighlighter';

const AUTOCOMPLETE_ITEMS = [
  // Keywords
  { label: 'pakad', type: 'Keyword', insert: 'pakad ', detail: 'Variable declaration', desc: 'pakad x = value;' },
  { label: 'jugaad', type: 'Keyword', insert: 'jugaad ', detail: 'Function declaration', desc: 'jugaad fn(a, b) { ye_le a + b; }' },
  { label: 'ye_le', type: 'Keyword', insert: 'ye_le ', detail: 'Return statement', desc: 'ye_le result;' },
  { label: 'biradari', type: 'Keyword', insert: 'biradari ', detail: 'Class declaration', desc: 'biradari Name { ... }' },
  { label: 'nepo', type: 'Keyword', insert: 'nepo ', detail: 'Inheritance', desc: 'biradari Child nepo Parent' },
  { label: 'khud', type: 'Keyword', insert: 'khud.', detail: 'this pointer', desc: 'khud.property' },
  { label: 'apun', type: 'Keyword', insert: 'apun.', detail: 'self pointer', desc: 'apun.property' },
  { label: 'se', type: 'Keyword', insert: 'se.', detail: 'super pointer', desc: 'se.method()' },
  { label: 'dekh', type: 'Keyword', insert: 'dekh ', detail: 'If condition', desc: 'dekh condition { ... }' },
  { label: 'warna', type: 'Keyword', insert: 'warna {', detail: 'Else branch', desc: 'warna { ... }' },
  { label: 'jabtak', type: 'Keyword', insert: 'jabtak ', detail: 'While loop', desc: 'jabtak condition { ... }' },
  { label: 'ghoom', type: 'Keyword', insert: 'ghoom pakad item mein list {', detail: 'For loop', desc: 'ghoom pakad x mein arr { ... }' },
  { label: 'panga', type: 'Keyword', insert: 'panga {', detail: 'Try block', desc: 'panga { ... } lafda err { ... }' },
  { label: 'lafda', type: 'Keyword', insert: 'lafda err {', detail: 'Catch block', desc: 'lafda err { ... }' },
  { label: 'pakka', type: 'Keyword', insert: 'pakka {', detail: 'Finally block', desc: 'pakka { ... }' },
  { label: 'chillao', type: 'Keyword', insert: 'chillao ', detail: 'Throw error', desc: 'chillao "Error msg";' },
  { label: 'bula', type: 'Keyword', insert: 'bula "', detail: 'Module import', desc: 'bula "hisab" jaise math;' },
  { label: 'avtar', type: 'Keyword', insert: 'avtar ', detail: 'Enum definition', desc: 'avtar Color { RED, GREEN }' },
  { label: 'jaanch', type: 'Keyword', insert: 'jaanch ', detail: 'Pattern match', desc: 'jaanch val { ... }' },
  { label: 'han', type: 'Constant', insert: 'han', detail: 'Boolean true', desc: 'Truth value (han/real)' },
  { label: 'nhi', type: 'Constant', insert: 'nhi', detail: 'Boolean false', desc: 'False value (nhi/fake)' },
  { label: 'nalla', type: 'Constant', insert: 'nalla', detail: 'Null value', desc: 'Nil / null constant' },

  // Builtins
  { label: 'dikha', type: 'Builtin', insert: 'dikha ', detail: 'Print to console', desc: 'dikha a, b;' },
  { label: 'dikha.mez', type: 'Builtin', insert: 'dikha.mez()', detail: 'Unicode Table', desc: 'dikha.mez(listOfMapsOrLists)' },
  { label: 'dikha.dabba', type: 'Builtin', insert: 'dikha.dabba("", "")', detail: 'Unicode Box Panel', desc: 'dikha.dabba(text, title)' },
  { label: 'systumm', type: 'Module', insert: 'systumm.', detail: 'OS & System APIs', desc: 'systumm.kaun(), systumm.env(), systumm.copy()' },
  { label: 'cli', type: 'Module', insert: 'cli.', detail: 'Interactive CLI tools', desc: 'cli.args_samajh(), cli.chuno(), cli.progress()' },
  { label: 'app', type: 'Module', insert: 'app.', detail: 'Native GUI & Notifications', desc: 'app.alert(), app.notify(), app.khol()' },
  { label: 'tijori', type: 'Module', insert: 'tijori.', detail: 'Persistent Document DB', desc: 'tijori.rakh(k, v), tijori.nikal(k), tijori.hai_kya(k)' },
  { label: 'dhaba', type: 'Module', insert: 'dhaba.', detail: 'FastAPI / Express Server', desc: 'dhaba.get(r, cb), dhaba.post(r, cb), dhaba.request(m, p)' },
  { label: 'ui', type: 'Module', insert: 'ui.', detail: 'Declarative UI Framework', desc: 'ui.khamba(), ui.katar(), ui.dabba(), ui.likh(), ui.button(), ui.dikha()' },
  { label: 'kaam', type: 'Keyword', insert: 'kaam ', detail: 'Function (WhatsApp Hinglish)', desc: 'kaam fn(x) { de x * 2; }' },
  { label: 'de', type: 'Keyword', insert: 'de ', detail: 'Return (WhatsApp Hinglish)', desc: 'de result;' },
  { label: 'agar', type: 'Keyword', insert: 'agar ', detail: 'If (WhatsApp Hinglish)', desc: 'agar cond { ... }' },
  { label: 'har', type: 'Keyword', insert: 'har ', detail: 'For-each (WhatsApp Hinglish)', desc: 'har item mein list { ... }' },
  { label: 'pooch', type: 'Builtin', insert: 'pooch("")', detail: 'Input prompt', desc: 'pakad s = pooch("Name: ");' },
  { label: 'code_de', type: 'Builtin', insert: 'code_de("")', detail: 'Char to ASCII (ord)', desc: 'code_de("A") -> 65' },
  { label: 'akshar_bana', type: 'Builtin', insert: 'akshar_bana(65)', detail: 'ASCII to Char (chr)', desc: 'akshar_bana(65) -> "A"' },
  { label: 'size', type: 'Builtin', insert: 'size()', detail: 'Length / Size', desc: 'size(collection)' },
  { label: 'kya_hai', type: 'Builtin', insert: 'kya_hai()', detail: 'Type inspection', desc: 'kya_hai(val)' },
  { label: 'kundli', type: 'Builtin', insert: 'kundli()', detail: 'Reflection inspector', desc: 'kundli(obj)' },
  { label: 'nakal', type: 'Builtin', insert: 'nakal()', detail: 'Deep clone', desc: 'nakal(obj)' },
  { label: 'bina_point', type: 'Builtin', insert: 'bina_point()', detail: 'Convert to int', desc: 'bina_point("42")' },
  { label: 'point_wala', type: 'Builtin', insert: 'point_wala()', detail: 'Convert to float', desc: 'point_wala("3.14")' },

  // Transforms (Collection Methods)
  { label: 'sab_pe', type: 'Transform', insert: 'sab_pe(x => )', detail: 'Map array', desc: '.sab_pe(item => item * 2)' },
  { label: 'chhaan', type: 'Transform', insert: 'chhaan(x => )', detail: 'Filter array', desc: '.chhaan(item => item > 0)' },
  { label: 'nichod', type: 'Transform', insert: 'nichod((acc, x) => acc + x, 0)', detail: 'Reduce array', desc: '.nichod((acc, x) => acc + x, init)' },
  { label: 'line_mein', type: 'Transform', insert: 'line_mein()', detail: 'Sort in-place', desc: '.line_mein() or .line_mein((a, b) => a > b)' },
  { label: 'har_ek', type: 'Transform', insert: 'har_ek(x => )', detail: 'ForEach loop', desc: '.har_ek(item => ...)' },
  { label: 'pakad_ek', type: 'Transform', insert: 'pakad_ek(x => )', detail: 'Find element', desc: '.pakad_ek(item => item.id == 1)' },
  { label: 'chala_de', type: 'Transform', insert: 'chala_de("", [])', detail: 'Dynamic invoke', desc: '.chala_de("method_naam", [args])' },
  { label: 'jodo', type: 'Transform', insert: 'jodo()', detail: 'Append item', desc: '.jodo(element)' },
  { label: 'hatao', type: 'Transform', insert: 'hatao()', detail: 'Pop item', desc: '.hatao()' },
  { label: 'keys', type: 'Transform', insert: 'keys()', detail: 'Map keys', desc: '.keys()' },
  { label: 'values', type: 'Transform', insert: 'values()', detail: 'Map values', desc: '.values()' },

  // Tantrik Methods
  { label: '__jod__', type: 'Tantrik', insert: '__jod__(other) {\n        ye_le khud.val + other.val;\n    }', detail: 'Operator +', desc: 'jugaad __jod__(other)' },
  { label: '__ghata__', type: 'Tantrik', insert: '__ghata__(other) {\n        ye_le khud.val - other.val;\n    }', detail: 'Operator -', desc: 'jugaad __ghata__(other)' },
  { label: '__guna__', type: 'Tantrik', insert: '__guna__(other) {\n        ye_le khud.val * other.val;\n    }', detail: 'Operator *', desc: 'jugaad __guna__(other)' },
  { label: '__bhaag__', type: 'Tantrik', insert: '__bhaag__(other) {\n        ye_le khud.val / other.val;\n    }', detail: 'Operator /', desc: 'jugaad __bhaag__(other)' },
  { label: '__barabar__', type: 'Tantrik', insert: '__barabar__(other) {\n        ye_le khud.val == other.val;\n    }', detail: 'Operator ==', desc: 'jugaad __barabar__(other)' },
  { label: '__chota__', type: 'Tantrik', insert: '__chota__(other) {\n        ye_le khud.val < other.val;\n    }', detail: 'Operator <', desc: 'jugaad __chota__(other)' },
  { label: '__la__', type: 'Tantrik', insert: '__la__(key) {\n        ye_le khud.data[key];\n    }', detail: 'Read obj[k]', desc: 'jugaad __la__(key)' },
  { label: '__daal__', type: 'Tantrik', insert: '__daal__(key, val) {\n        khud.data[key] = val;\n    }', detail: 'Write obj[k]=v', desc: 'jugaad __daal__(key, val)' },
  { label: '__dikha__', type: 'Tantrik', insert: '__dikha__() {\n        ye_le f"Object({khud.val})";\n    }', detail: 'Stringify format', desc: 'jugaad __dikha__()' },
  { label: '__size__', type: 'Tantrik', insert: '__size__() {\n        ye_le size(khud.data);\n    }', detail: 'size(obj)', desc: 'jugaad __size__()' },
  { label: '__chala__', type: 'Tantrik', insert: '__chala__(...args) {\n        ye_le 42;\n    }', detail: 'Callable obj()', desc: 'jugaad __chala__(...)' },
  { label: 'shuru', type: 'Tantrik', insert: 'shuru() {\n        \n    }', detail: 'Constructor', desc: 'jugaad shuru(...)' }
];

const ANSI_CLASSES = {
  '1': 'font-bold',
  '2': 'opacity-70',
  '3': 'italic',
  '4': 'underline',
  '30': 'text-zinc-400',
  '31': 'text-rose-400',
  '32': 'text-emerald-400',
  '33': 'text-amber-400',
  '34': 'text-sky-400',
  '35': 'text-fuchsia-400',
  '36': 'text-cyan-400',
  '37': 'text-zinc-100',
};

function renderAnsi(text) {
  if (!text) return null;
  if (!text.includes('\u001b[') && !text.includes('\x1b[')) {
    return text;
  }

  const parts = [];
  const regex = /\u001b\[([0-9;]*)m/g;
  let lastIndex = 0;
  let currentClasses = new Set();
  let match;

  while ((match = regex.exec(text)) !== null) {
    if (match.index > lastIndex) {
      const segment = text.substring(lastIndex, match.index);
      if (currentClasses.size > 0) {
        parts.push(
          <span key={parts.length} className={Array.from(currentClasses).join(' ')}>
            {segment}
          </span>
        );
      } else {
        parts.push(segment);
      }
    }

    const codes = match[1] ? match[1].split(';') : ['0'];
    for (const code of codes) {
      if (code === '0' || code === '') {
        currentClasses.clear();
      } else if (ANSI_CLASSES[code]) {
        currentClasses.add(ANSI_CLASSES[code]);
      }
    }
    lastIndex = regex.lastIndex;
  }

  if (lastIndex < text.length) {
    const segment = text.substring(lastIndex);
    if (currentClasses.size > 0) {
      parts.push(
        <span key={parts.length} className={Array.from(currentClasses).join(' ')}>
          {segment}
        </span>
      );
    } else {
      parts.push(segment);
    }
  }

  return parts;
}

export default function Playground({ initialCode }) {
  const [selectedPreset, setSelectedPreset] = useState(PLAYGROUND_PRESETS[0].id);
  const [code, setCode] = useState(initialCode || PLAYGROUND_PRESETS[0].code);
  const [stdin, setStdin] = useState('');
  const [showStdin, setShowStdin] = useState(false);
  const [isRunning, setIsRunning] = useState(false);
  const [result, setResult] = useState(null);
  const [copied, setCopied] = useState(false);
  const [syntaxHighlighting, setSyntaxHighlighting] = useState(true);
  const [activeOutputTab, setActiveOutputTab] = useState('console'); // 'console' | 'gpu'

  // Autocomplete state
  const [showAutocomplete, setShowAutocomplete] = useState(false);
  const [autocompleteList, setAutocompleteList] = useState([]);
  const [activeSuggestionIndex, setActiveSuggestionIndex] = useState(0);
  const [caretCoords, setCaretCoords] = useState({ top: 40, left: 20 });
  const [queryInfo, setQueryInfo] = useState({ prefix: '', start: 0, end: 0 });

  const textareaRef = useRef(null);
  const preRef = useRef(null);
  const canvasRef = useRef(null);

  useEffect(() => {
    if (activeOutputTab === 'gpu' && canvasRef.current && result?.gpuCommands && result.gpuCommands.length > 0) {
      const canvas = canvasRef.current;
      const ctx = canvas.getContext('2d');
      if (!ctx) return;

      const dpr = window.devicePixelRatio || 1;
      const rect = canvas.getBoundingClientRect();
      const cssW = rect.width || 800;
      const cssH = rect.height || 600;

      canvas.width = cssW * dpr;
      canvas.height = cssH * dpr;
      ctx.scale(dpr, dpr);

      // Clean viewport background
      ctx.fillStyle = '#09090b';
      ctx.fillRect(0, 0, cssW, cssH);

      // Execute Direct GPU draw calls
      for (const cmd of result.gpuCommands) {
        ctx.save();
        if (cmd.type === 'FILL_ROUND_RECT') {
          ctx.fillStyle = cmd.color || '#000000';
          ctx.globalAlpha = cmd.alpha !== undefined ? cmd.alpha : 1.0;
          ctx.beginPath();
          if (ctx.roundRect) {
            ctx.roundRect(cmd.x, cmd.y, cmd.width, cmd.height, cmd.radius || 0);
          } else {
            ctx.rect(cmd.x, cmd.y, cmd.width, cmd.height);
          }
          ctx.fill();
        } else if (cmd.type === 'STROKE_ROUND_RECT') {
          ctx.strokeStyle = cmd.color || '#ffffff';
          ctx.globalAlpha = cmd.alpha !== undefined ? cmd.alpha : 1.0;
          ctx.lineWidth = cmd.lineWidth || 1;
          ctx.beginPath();
          if (ctx.roundRect) {
            ctx.roundRect(cmd.x, cmd.y, cmd.width, cmd.height, cmd.radius || 0);
          } else {
            ctx.rect(cmd.x, cmd.y, cmd.width, cmd.height);
          }
          ctx.stroke();
        } else if (cmd.type === 'DRAW_TEXT') {
          ctx.fillStyle = cmd.color || '#f8fafc';
          ctx.font = `${cmd.fontWeight || 400} ${cmd.fontSize || 14}px system-ui, -apple-system, sans-serif`;
          ctx.textBaseline = 'top';
          ctx.fillText(cmd.text, cmd.x, cmd.y);
        }
        ctx.restore();
      }
    }
  }, [activeOutputTab, result?.gpuCommands]);

  const handleCanvasClick = async (e) => {
    if (!canvasRef.current || !result?.hitRegions || result.hitRegions.length === 0) return;
    const rect = canvasRef.current.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    for (let i = result.hitRegions.length - 1; i >= 0; i--) {
      const region = result.hitRegions[i];
      if (x >= region.x && x <= region.x + region.width && y >= region.y && y <= region.y + region.height) {
        if (region.hasClick) {
          const res = await dispatchUiEvent(region.id, 'click', '');
          if (res?.gpuCommands) {
            setResult((prev) => ({
              ...prev,
              gpuCommands: res.gpuCommands,
              hitRegions: res.hitRegions
            }));
          }
          break;
        } else if (region.hasChange) {
          const inputVal = window.prompt("Naya input likho:", region.inputValue || "");
          if (inputVal !== null) {
            const res = await dispatchUiEvent(region.id, 'change', inputVal);
            if (res?.gpuCommands) {
              setResult((prev) => ({
                ...prev,
                gpuCommands: res.gpuCommands,
                hitRegions: res.hitRegions
              }));
            }
          }
          break;
        }
      }
    }
  };

  const handleCanvasMouseMove = (e) => {
    if (!canvasRef.current || !result?.hitRegions) return;
    const rect = canvasRef.current.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    let cursor = 'default';
    for (let i = result.hitRegions.length - 1; i >= 0; i--) {
      const region = result.hitRegions[i];
      if (x >= region.x && x <= region.x + region.width && y >= region.y && y <= region.y + region.height) {
        cursor = region.cursor || 'pointer';
        break;
      }
    }
    canvasRef.current.style.cursor = cursor;
  };

  useEffect(() => {
    if (initialCode) {
      setCode(initialCode);
      setSelectedPreset('custom');
    }
  }, [initialCode]);

  const handleSelectPreset = (presetId) => {
    setSelectedPreset(presetId);
    const found = PLAYGROUND_PRESETS.find((p) => p.id === presetId);
    if (found) {
      setCode(found.code);
      setStdin(found.stdin || '');
      if (found.stdin) {
        setShowStdin(true);
      }
      setResult(null);
      setShowAutocomplete(false);
    }
  };

  const handleRun = async () => {
    if (isRunning) return;
    setIsRunning(true);
    setShowAutocomplete(false);
    try {
      const res = await runMemeCode(code, stdin);
      setResult(res);
      if (res.gpuCommands && res.gpuCommands.length > 0) {
        setActiveOutputTab('gpu');
      } else {
        setActiveOutputTab('console');
      }
    } catch (err) {
      setResult({
        output: `Runner error: ${err.message || err}`,
        isError: true,
        durationMs: '0.00',
        timestamp: new Date().toLocaleTimeString()
      });
    } finally {
      setIsRunning(false);
    }
  };

  // Synchronize scrolling between Textarea and Background Pre
  const handleScroll = (e) => {
    if (preRef.current) {
      preRef.current.scrollTop = e.target.scrollTop;
      preRef.current.scrollLeft = e.target.scrollLeft;
    }
  };

  // Check and update autocomplete suggestions on text change or cursor movement
  const checkAutocomplete = (text, cursorPos) => {
    if (!cursorPos || cursorPos < 0) {
      setShowAutocomplete(false);
      return;
    }

    const textBefore = text.slice(0, cursorPos);
    const lines = textBefore.split('\n');
    const currentLineIdx = lines.length - 1;
    const currentLineText = lines[currentLineIdx];

    // Detect trigger: word, dot '.', or double underscore '__'
    const wordMatch = currentLineText.match(/(\.?[a-zA-Z0-9_]+|\.)$/);
    if (!wordMatch) {
      setShowAutocomplete(false);
      return;
    }

    const matchedStr = wordMatch[1];
    let query = matchedStr;
    const isDotTrigger = matchedStr.startsWith('.');
    if (isDotTrigger) {
      query = matchedStr.slice(1);
    }

    const isTantrikTrigger = matchedStr.startsWith('__');

    let matches = [];
    if (isDotTrigger) {
      matches = AUTOCOMPLETE_ITEMS.filter((item) =>
        item.type === 'Transform' && item.label.toLowerCase().startsWith(query.toLowerCase())
      );
    } else if (isTantrikTrigger) {
      matches = AUTOCOMPLETE_ITEMS.filter((item) =>
        item.type === 'Tantrik' && item.label.toLowerCase().startsWith(query.toLowerCase())
      );
    } else if (query.length >= 1) {
      matches = AUTOCOMPLETE_ITEMS.filter((item) =>
        item.label.toLowerCase().startsWith(query.toLowerCase())
      );
    }

    if (matches.length > 0) {
      // Approximate pixel coordinates
      const top = Math.min((currentLineIdx + 1) * 24 + 16 - (textareaRef.current?.scrollTop || 0), 380);
      const left = Math.min(Math.max((currentLineText.length - matchedStr.length) * 8.4 + 20 - (textareaRef.current?.scrollLeft || 0), 20), 400);

      setQueryInfo({
        prefix: matchedStr,
        start: cursorPos - matchedStr.length,
        end: cursorPos
      });
      setCaretCoords({ top: Math.max(top, 30), left });
      setAutocompleteList(matches.slice(0, 8));
      setActiveSuggestionIndex(0);
      setShowAutocomplete(true);
    } else {
      setShowAutocomplete(false);
    }
  };

  const applyAutocomplete = (item) => {
    if (!item) return;
    const start = queryInfo.start;
    const end = queryInfo.end;
    let insertText = item.insert || item.label;

    // If typing after dot '.' and item starts without dot
    if (queryInfo.prefix.startsWith('.') && !insertText.startsWith('.')) {
      insertText = '.' + insertText;
    }

    const newCode = code.slice(0, start) + insertText + code.slice(end);
    setCode(newCode);
    setShowAutocomplete(false);

    const newCursor = start + insertText.length;
    setTimeout(() => {
      if (textareaRef.current) {
        textareaRef.current.focus();
        textareaRef.current.selectionStart = textareaRef.current.selectionEnd = newCursor;
      }
    }, 0);
  };

  const handleChange = (e) => {
    const newText = e.target.value;
    setCode(newText);
    checkAutocomplete(newText, e.target.selectionStart);
  };

  const handleKeyDown = (e) => {
    // Autocomplete keyboard navigation
    if (showAutocomplete && autocompleteList.length > 0) {
      if (e.key === 'ArrowDown') {
        e.preventDefault();
        setActiveSuggestionIndex((prev) => (prev + 1) % autocompleteList.length);
        return;
      }
      if (e.key === 'ArrowUp') {
        e.preventDefault();
        setActiveSuggestionIndex((prev) => (prev - 1 + autocompleteList.length) % autocompleteList.length);
        return;
      }
      if (e.key === 'Enter' || e.key === 'Tab') {
        e.preventDefault();
        applyAutocomplete(autocompleteList[activeSuggestionIndex]);
        return;
      }
      if (e.key === 'Escape') {
        e.preventDefault();
        setShowAutocomplete(false);
        return;
      }
    }

    // Run shortcut: Ctrl+Enter or Cmd+Enter
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
      e.preventDefault();
      handleRun();
      return;
    }

    // Tab key support
    if (e.key === 'Tab') {
      e.preventDefault();
      const start = e.target.selectionStart;
      const end = e.target.selectionEnd;
      const newCode = code.substring(0, start) + '    ' + code.substring(end);
      setCode(newCode);
      setTimeout(() => {
        if (textareaRef.current) {
          textareaRef.current.selectionStart = textareaRef.current.selectionEnd = start + 4;
        }
      }, 0);
    }
  };

  const handleCopy = () => {
    navigator.clipboard.writeText(code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const lineCount = code.split('\n').length;
  const lineNumbers = Array.from({ length: Math.max(lineCount, 16) }, (_, i) => i + 1);

  return (
    <div className="max-w-7xl mx-auto px-4 lg:px-8 py-6 w-full">
      {/* Playground Header Bar */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4 mb-6 pb-4 border-b border-zinc-800">
        <div>
          <h1 className="text-2xl font-bold tracking-tight text-zinc-100 flex items-center gap-2.5">
            <Terminal className="w-6 h-6 text-amber-400" />
            Playground & WebAssembly REPL
          </h1>
          <p className="text-sm text-zinc-400 mt-1">
            Zero-latency browser execution with real-time Syntax Highlighting and IntelliSense Autocomplete.
          </p>
        </div>

        {/* Preset Selector */}
        <div className="flex items-center gap-2">
          <label className="text-xs font-medium text-zinc-400">Recipe:</label>
          <select
            value={selectedPreset}
            onChange={(e) => handleSelectPreset(e.target.value)}
            className="bg-zinc-900 border border-zinc-700 text-zinc-200 text-xs rounded-md px-3 py-1.5 focus:outline-none focus:border-zinc-500 font-medium cursor-pointer"
          >
            {PLAYGROUND_PRESETS.map((preset) => (
              <option key={preset.id} value={preset.id}>
                {preset.name}
              </option>
            ))}
            {selectedPreset === 'custom' && <option value="custom">Custom Code</option>}
          </select>
        </div>
      </div>

      {/* Main Grid: Code Editor on Left, Output on Right */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
        {/* Left: Code Editor Card */}
        <div className="rounded-lg border border-zinc-800 bg-zinc-950 overflow-hidden flex flex-col shadow-sm">
          {/* Editor Action Bar */}
          <div className="flex items-center justify-between px-4 py-2.5 bg-zinc-900 border-b border-zinc-800 text-xs text-zinc-400">
            <div className="flex items-center gap-2 font-mono text-xs">
              <span className="text-zinc-300 font-medium">main.meme</span>
              <span className="hidden sm:inline-flex items-center gap-1 text-[11px] text-amber-400 bg-amber-950/40 border border-amber-800/40 px-1.5 py-0.5 rounded">
                <Zap className="w-3 h-3" />
                IntelliSense On
              </span>
            </div>

            <div className="flex items-center gap-2">
              {/* Syntax Highlighting Toggle */}
              <button
                onClick={() => setSyntaxHighlighting(!syntaxHighlighting)}
                className={`flex items-center gap-1.5 px-2.5 py-1 rounded text-xs font-medium border transition-colors ${
                  syntaxHighlighting
                    ? 'bg-zinc-800 text-purple-300 border-zinc-700'
                    : 'bg-zinc-800/60 text-zinc-400 hover:text-zinc-200 border-zinc-700/60'
                }`}
                title="Toggle Syntax Highlighting"
              >
                <Sparkles className="w-3.5 h-3.5 text-purple-400" />
                <span className="hidden sm:inline">Syntax</span>
              </button>

              <button
                onClick={() => setShowStdin(!showStdin)}
                className={`flex items-center gap-1.5 px-2.5 py-1 rounded text-xs font-medium border transition-colors ${
                  showStdin
                    ? 'bg-zinc-800 text-amber-300 border-zinc-700'
                    : 'bg-zinc-800/60 text-zinc-400 hover:text-zinc-200 border-zinc-700/60'
                }`}
                title="Toggle standard input (stdin) for pooch() / bata()"
              >
                <MessageSquare className="w-3.5 h-3.5" />
                <span>Input (stdin)</span>
                {stdin.trim() && <span className="w-1.5 h-1.5 rounded-full bg-amber-400"></span>}
              </button>

              <button
                onClick={handleCopy}
                className="flex items-center gap-1 px-2.5 py-1 rounded bg-zinc-800 hover:bg-zinc-700 text-zinc-300 text-xs font-medium border border-zinc-700/60 transition-colors"
                title="Copy code"
              >
                {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5 text-zinc-400" />}
                <span className="hidden sm:inline">{copied ? 'Copied' : 'Copy'}</span>
              </button>

              <button
                onClick={() => {
                  const found = PLAYGROUND_PRESETS.find((p) => p.id === selectedPreset);
                  if (found) {
                    setCode(found.code);
                    setStdin(found.stdin || '');
                  } else {
                    setCode('');
                    setStdin('');
                  }
                  setResult(null);
                  setShowAutocomplete(false);
                }}
                className="flex items-center gap-1 px-2.5 py-1 rounded bg-zinc-800 hover:bg-zinc-700 text-zinc-300 text-xs font-medium border border-zinc-700/60 transition-colors"
                title="Reset code"
              >
                <RotateCcw className="w-3.5 h-3.5 text-zinc-400" />
                <span className="hidden sm:inline">Reset</span>
              </button>

              <button
                onClick={handleRun}
                disabled={isRunning}
                className="flex items-center gap-1.5 px-3.5 py-1 rounded bg-zinc-100 hover:bg-white text-zinc-950 font-semibold text-xs transition-colors shadow-sm disabled:opacity-50"
              >
                {isRunning ? (
                  <Loader2 className="w-3.5 h-3.5 animate-spin" />
                ) : (
                  <Play className="w-3.5 h-3.5 fill-current" />
                )}
                <span>Run</span>
                <span className="hidden md:inline text-xs opacity-60 ml-1 font-mono">(⌘Enter)</span>
              </button>
            </div>
          </div>

          {/* Collapsible Stdin Input Panel */}
          {showStdin && (
            <div className="p-3 bg-zinc-900 border-b border-zinc-800 text-xs">
              <div className="flex items-center justify-between mb-1.5 text-zinc-400">
                <span className="font-mono text-zinc-300">Standard Input (stdin)</span>
                <span className="text-zinc-400 text-xs">Pre-feed lines for pooch() / bata()</span>
              </div>
              <textarea
                value={stdin}
                onChange={(e) => setStdin(e.target.value)}
                placeholder="Type lines here to feed pooch() in order (e.g. Baburao&#10;50). If empty, pooch() will prompt you in browser dialog..."
                rows={3}
                className="w-full bg-zinc-950 border border-zinc-700 rounded p-2 text-zinc-100 font-mono text-xs placeholder:text-zinc-500 focus:outline-none focus:border-zinc-500"
              />
            </div>
          )}

          {/* Dual-Layer Editor: Syntax Highlighted Backdrop + Transparent Foreground Textarea */}
          <div className="flex flex-1 min-h-[460px] font-mono text-sm bg-zinc-950 relative overflow-hidden">
            {/* Line Numbers Column */}
            <div className="w-12 select-none py-4 text-right pr-3 text-zinc-600 bg-zinc-900/40 border-r border-zinc-800/80 leading-6 shrink-0 font-mono text-xs z-10">
              {lineNumbers.map((num) => (
                <div key={num}>{num}</div>
              ))}
            </div>

            {/* Code Editor Container */}
            <div className="relative flex-1 min-h-[460px] overflow-hidden">
              {/* Highlighted Pre Layer */}
              {syntaxHighlighting && (
                <pre
                  ref={preRef}
                  aria-hidden="true"
                  className="absolute inset-0 p-4 font-mono text-sm leading-6 whitespace-pre overflow-hidden pointer-events-none select-none bg-transparent m-0 border-0"
                >
                  <MemeSyntaxHighlighter code={code} />
                  {code.endsWith('\n') && '\n'}
                </pre>
              )}

              {/* Editable Textarea Layer */}
              <textarea
                ref={textareaRef}
                value={code}
                onChange={handleChange}
                onKeyDown={handleKeyDown}
                onScroll={handleScroll}
                onClick={(e) => checkAutocomplete(code, e.target.selectionStart)}
                spellCheck="false"
                className={`absolute inset-0 w-full h-full p-4 font-mono text-sm leading-6 whitespace-pre resize-none bg-transparent focus:outline-none ${
                  syntaxHighlighting
                    ? 'text-transparent caret-amber-400 selection:bg-amber-500/25 selection:text-transparent'
                    : 'text-zinc-100 placeholder:text-zinc-600 selection:bg-amber-500/20 selection:text-amber-300'
                }`}
                placeholder="// Write your MEMElang code here..."
              />

              {/* Floating Autocomplete Dropdown */}
              {showAutocomplete && autocompleteList.length > 0 && (
                <div
                  style={{
                    top: `${caretCoords.top}px`,
                    left: `${caretCoords.left}px`,
                  }}
                  className="absolute z-50 w-72 max-h-56 overflow-y-auto rounded-lg border border-zinc-700 bg-zinc-900/95 backdrop-blur-md shadow-2xl p-1 text-xs font-mono"
                >
                  <div className="px-2 py-1 text-[10px] text-zinc-500 border-b border-zinc-800 mb-1 flex items-center justify-between">
                    <span>MEMElang Suggestions</span>
                    <span>Tab / ↵ to insert</span>
                  </div>
                  {autocompleteList.map((item, idx) => (
                    <div
                      key={item.label}
                      onMouseDown={(e) => {
                        e.preventDefault();
                        applyAutocomplete(item);
                      }}
                      onMouseEnter={() => setActiveSuggestionIndex(idx)}
                      className={`flex items-center justify-between px-2.5 py-1.5 rounded cursor-pointer transition-colors ${
                        idx === activeSuggestionIndex
                          ? 'bg-amber-500/20 text-white border border-amber-500/40'
                          : 'text-zinc-300 hover:bg-zinc-800/80'
                      }`}
                    >
                      <div className="flex items-center gap-2 overflow-hidden">
                        <span className="font-semibold text-zinc-100">{item.label}</span>
                        <span className="text-[10px] text-zinc-400 truncate max-w-[110px]">{item.detail}</span>
                      </div>
                      <span className={`text-[9px] px-1.5 py-0.5 rounded font-bold uppercase tracking-wider shrink-0 ${
                        item.type === 'Keyword' ? 'bg-purple-950/80 text-purple-300 border border-purple-800/40' :
                        item.type === 'Tantrik' ? 'bg-amber-950/80 text-amber-300 border border-amber-800/40' :
                        item.type === 'Transform' ? 'bg-cyan-950/80 text-cyan-300 border border-cyan-800/40' :
                        item.type === 'Builtin' ? 'bg-sky-950/80 text-sky-300 border border-sky-800/40' :
                        'bg-zinc-800 text-zinc-400 border border-zinc-700'
                      }`}>
                        {item.type}
                      </span>
                    </div>
                  ))}
                </div>
              )}
            </div>
          </div>

          {/* Footer Bar */}
          <div className="px-4 py-2 bg-zinc-900 border-t border-zinc-800 flex items-center justify-between text-xs text-zinc-400 font-mono">
            <span>{lineCount} lines • MEMElang v1.0</span>
            <span className="text-zinc-400">Press <kbd className="px-1.5 py-0.5 rounded bg-zinc-800 text-zinc-300 font-mono border border-zinc-700">Ctrl</kbd> + <kbd className="px-1.5 py-0.5 rounded bg-zinc-800 text-zinc-300 font-mono border border-zinc-700">Enter</kbd> to execute</span>
          </div>
        </div>

        {/* Right: Output Console Card */}
        <div className="rounded-lg border border-zinc-800 bg-zinc-950 overflow-hidden flex flex-col shadow-sm">
          {/* Output Header Bar */}
          <div className="flex items-center justify-between px-3 py-2 bg-zinc-900 border-b border-zinc-800 text-xs text-zinc-400">
            <div className="flex items-center gap-1.5 font-mono">
              <button
                onClick={() => setActiveOutputTab('console')}
                className={`flex items-center gap-1.5 px-2.5 py-1 rounded text-xs transition-colors ${
                  activeOutputTab === 'console'
                    ? 'bg-zinc-800 text-zinc-100 font-semibold border border-zinc-700/60 shadow-xs'
                    : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                <Terminal className="w-3.5 h-3.5 text-zinc-400" />
                <span>Console</span>
              </button>

              <button
                onClick={() => setActiveOutputTab('gpu')}
                className={`flex items-center gap-1.5 px-2.5 py-1 rounded text-xs transition-colors ${
                  activeOutputTab === 'gpu'
                    ? 'bg-amber-500/20 text-amber-300 font-semibold border border-amber-500/40 shadow-xs'
                    : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                <Zap className="w-3.5 h-3.5 text-amber-400" />
                <span>⚡ Direct GPU UI</span>
                {result?.gpuCommands && result.gpuCommands.length > 0 && (
                  <span className="px-1.5 py-0.2 rounded-full text-[10px] bg-amber-500/30 text-amber-200 font-bold ml-0.5">
                    {result.gpuCommands.length}
                  </span>
                )}
              </button>

              {result && (
                <span className={`text-[10px] ml-2 px-2 py-0.5 rounded font-mono ${
                  result.isError 
                    ? 'bg-rose-950/40 text-rose-300 border border-rose-800/40' 
                    : 'bg-emerald-950/40 text-emerald-300 border border-emerald-800/40'
                }`}>
                  {result.isError ? 'Error' : 'Done'}
                </span>
              )}
            </div>

            <div className="flex items-center gap-3 text-zinc-400 text-xs font-mono">
              {result && <span>{result.durationMs}ms</span>}
              {result && (
                <button
                  onClick={() => setResult(null)}
                  className="hover:text-zinc-200 text-zinc-400 text-xs"
                >
                  Clear
                </button>
              )}
            </div>
          </div>

          {/* Output Body */}
          <div className="flex-1 p-4 overflow-auto font-mono text-sm min-h-[460px] bg-zinc-950">
            {activeOutputTab === 'gpu' ? (
              <div className="flex flex-col h-full space-y-3">
                <div className="flex items-center justify-between text-[11px] font-mono text-zinc-400 bg-zinc-900/60 border border-zinc-800/80 px-3 py-1.5 rounded-md">
                  <span className="flex items-center gap-1.5 text-amber-300 font-medium">
                    <Zap className="w-3.5 h-3.5 text-amber-400" />
                    Zero-WebView Direct GPU Pipeline
                  </span>
                  <span className="text-zinc-400">
                    {result?.gpuCommands && result.gpuCommands.length > 0
                      ? `${result.gpuCommands.length} Direct Draw Calls • 60 FPS Canvas`
                      : 'No UI mounted yet'}
                  </span>
                </div>
                <div className="flex-1 min-h-[420px] rounded-lg border border-zinc-800/80 bg-zinc-900/30 overflow-hidden relative flex items-center justify-center p-3">
                  {result?.gpuCommands && result.gpuCommands.length > 0 ? (
                    <canvas
                      ref={canvasRef}
                      onClick={handleCanvasClick}
                      onMouseMove={handleCanvasMouseMove}
                      className="w-full h-full max-w-[800px] max-h-[600px] rounded-xl shadow-2xl object-contain border border-zinc-800 bg-[#09090b]"
                    />
                  ) : (
                    <div className="flex flex-col items-center justify-center text-center p-8 text-zinc-500">
                      <Zap className="w-10 h-10 text-zinc-600 mb-2" />
                      <p className="text-xs font-medium text-zinc-300">No GPU UI draw commands emitted</p>
                      <p className="text-[11px] text-zinc-500 mt-1 max-w-sm">
                        Use JSX components styled with Tailwind classes or pick the <strong className="text-amber-400">⚡ Zero-WebView Native GPU UI</strong> recipe above and click Run!
                      </p>
                    </div>
                  )}
                </div>
              </div>
            ) : isRunning ? (
              <div className="flex flex-col items-center justify-center h-full text-zinc-500 gap-3 py-16">
                <Loader2 className="w-6 h-6 text-amber-400 animate-spin" />
                <p className="text-xs font-mono text-zinc-400">Evaluating in WebAssembly runtime...</p>
              </div>
            ) : result ? (
              <div className="space-y-3">
                {result.isError && (
                  <div className="p-3 rounded-md bg-rose-950/25 border border-rose-800/40 text-rose-300 flex items-start gap-2.5 text-xs">
                    <AlertTriangle className="w-4 h-4 text-rose-400 shrink-0 mt-0.5" />
                    <div>
                      <div className="font-semibold text-rose-200">Meme Diagnostic Alert</div>
                      <div className="text-xs text-rose-400/90 mt-0.5">
                        Your program encountered a syntax or runtime error!
                      </div>
                    </div>
                  </div>
                )}

                <pre className={`p-4 rounded-md leading-relaxed whitespace-pre-wrap overflow-x-auto ${
                  result.isError
                    ? 'bg-rose-950/20 text-rose-200 border border-rose-900/40'
                    : 'bg-zinc-900/80 text-emerald-300 border border-zinc-800'
                }`}>
                  {renderAnsi(result.output) || '(Execution completed with no output)'}
                </pre>
              </div>
            ) : (
              <div className="flex flex-col items-center justify-center h-full text-zinc-600 text-center py-16 space-y-2">
                <Terminal className="w-8 h-8 opacity-40 text-zinc-500" />
                <p className="text-xs text-zinc-400">No output yet. Write or pick a recipe and click <strong className="text-zinc-200">Run</strong>.</p>
                <p className="text-xs text-zinc-400">Output from <code className="text-zinc-400 font-mono">dikha</code>, <code className="text-zinc-400 font-mono">pooch()</code> input prompts, and diagnostics appear here.</p>
              </div>
            )}
          </div>

          {/* Output Status Footer */}
          <div className="px-4 py-2 bg-zinc-900 border-t border-zinc-800 text-xs font-mono text-zinc-400 flex items-center justify-between">
            <span>Status: {isRunning ? 'Running...' : result ? (result.isError ? 'Error reported' : 'Success') : 'Idle'}</span>
            {result?.timestamp && <span>Last run at {result.timestamp}</span>}
          </div>
        </div>
      </div>
    </div>
  );
}
