import React, { useState } from 'react';
import { Play, Copy, Check, Terminal, Loader2, Code2, X } from 'lucide-react';
import { runMemeCode } from '../wasm/memeRunner';
import MemeSyntaxHighlighter from './MemeSyntaxHighlighter';

export default function CodeBlock({ code, title = 'snippet.meme', onOpenInPlayground }) {
  const [copied, setCopied] = useState(false);
  const [isRunning, setIsRunning] = useState(false);
  const [result, setResult] = useState(null);
  const [showOutput, setShowOutput] = useState(false);

  const handleCopy = () => {
    navigator.clipboard.writeText(code);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const handleRun = async () => {
    setIsRunning(true);
    setShowOutput(true);
    try {
      const res = await runMemeCode(code);
      setResult(res);
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

  return (
    <div className="my-6 rounded-lg border border-zinc-800 bg-zinc-950 overflow-hidden shadow-sm">
      {/* Code Header Bar */}
      <div className="flex items-center justify-between px-4 py-2.5 bg-zinc-900 border-b border-zinc-800 text-xs text-zinc-400">
        <div className="flex items-center gap-2">
          <Code2 className="w-4 h-4 text-zinc-400" />
          <span className="font-mono text-zinc-300 font-medium">{title}</span>
        </div>

        <div className="flex items-center gap-2">
          {onOpenInPlayground && (
            <button
              onClick={() => onOpenInPlayground(code)}
              className="flex items-center gap-1.5 px-2.5 py-1 rounded bg-zinc-800 hover:bg-zinc-700 text-zinc-300 hover:text-white transition-colors text-xs font-medium border border-zinc-700/60"
              title="Open and edit in full playground"
            >
              <Terminal className="w-3.5 h-3.5 text-zinc-400" />
              <span className="hidden sm:inline">Playground</span>
            </button>
          )}

          <button
            onClick={handleCopy}
            className="flex items-center gap-1.5 px-2.5 py-1 rounded bg-zinc-800 hover:bg-zinc-700 text-zinc-300 hover:text-white transition-colors text-xs font-medium border border-zinc-700/60"
            title="Copy code"
          >
            {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5 text-zinc-400" />}
            <span className="hidden sm:inline">{copied ? 'Copied' : 'Copy'}</span>
          </button>

          <button
            onClick={handleRun}
            disabled={isRunning}
            className="flex items-center gap-1.5 px-3 py-1 rounded bg-zinc-100 hover:bg-white text-zinc-950 font-semibold transition-colors text-xs shadow-sm disabled:opacity-50"
          >
            {isRunning ? (
              <Loader2 className="w-3.5 h-3.5 animate-spin" />
            ) : (
              <Play className="w-3.5 h-3.5 fill-current" />
            )}
            <span>Run</span>
          </button>
        </div>
      </div>

      {/* Code Text Body */}
      <div className="p-4 overflow-x-auto text-sm font-mono text-zinc-200 leading-relaxed bg-zinc-950">
        <pre className="selection:bg-amber-500/20">
          <MemeSyntaxHighlighter code={code} />
        </pre>
      </div>

      {/* Interactive Wasm Output Drawer */}
      {showOutput && (
        <div className="border-t border-zinc-800 bg-zinc-900/90 p-4 transition-all">
          <div className="flex items-center justify-between pb-2 mb-2 border-b border-zinc-800 text-xs">
            <div className="flex items-center gap-2">
              <span className="font-mono text-zinc-300 flex items-center gap-1.5 font-medium">
                <Terminal className="w-3.5 h-3.5 text-amber-400" />
                WebAssembly Output
              </span>
              {result && (
                <span className={`text-xs font-mono px-2 py-0.5 rounded ${
                  result.isError 
                    ? 'bg-rose-950/40 text-rose-300 border border-rose-800/40' 
                    : 'bg-emerald-950/40 text-emerald-300 border border-emerald-800/40'
                }`}>
                  {result.isError ? 'Error' : 'Success'}
                </span>
              )}
            </div>

            <div className="flex items-center gap-3 text-zinc-400 text-xs font-mono">
              {result && <span>{result.durationMs}ms</span>}
              <button
                onClick={() => setShowOutput(false)}
                className="hover:text-zinc-200 text-zinc-400 p-0.5 rounded"
                title="Close output"
              >
                <X className="w-3.5 h-3.5" />
              </button>
            </div>
          </div>

          {isRunning ? (
            <div className="flex items-center gap-2 text-xs text-amber-400 py-3 font-mono">
              <Loader2 className="w-4 h-4 animate-spin" />
              Executing in WebAssembly runtime...
            </div>
          ) : result ? (
            <pre className={`text-xs font-mono p-3 rounded overflow-x-auto whitespace-pre-wrap leading-relaxed ${
              result.isError 
                ? 'bg-rose-950/20 text-rose-200 border border-rose-900/40' 
                : 'bg-zinc-950 text-emerald-300 border border-zinc-800'
            }`}>
              {result.output || '(No console output produced)'}
            </pre>
          ) : null}
        </div>
      )}
    </div>
  );
}
