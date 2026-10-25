import React from 'react';
import { Terminal, BookOpen, AlertTriangle, Play, Sparkles } from 'lucide-react';

export default function Navbar({ activeTab, setActiveTab, wasmStatus }) {
  return (
    <header className="sticky top-0 z-50 border-b border-zinc-800 bg-zinc-950/85 backdrop-blur-md px-4 lg:px-8 py-3">
      <div className="max-w-7xl mx-auto flex items-center justify-between gap-4">
        {/* Brand */}
        <div 
          className="flex items-center gap-2.5 cursor-pointer select-none"
          onClick={() => setActiveTab('docs')}
        >
          <div className="w-8 h-8 rounded-lg bg-zinc-900 border border-zinc-700 flex items-center justify-center font-bold text-amber-400 text-base">
            M
          </div>
          <div className="flex items-center gap-2">
            <span className="font-semibold text-lg tracking-tight text-zinc-100">
              MEMElang
            </span>
            <span className="text-xs px-2 py-0.5 rounded-md bg-zinc-800 text-zinc-300 font-mono border border-zinc-700/60">
              v1.0 WASM
            </span>
          </div>
        </div>

        {/* Center Navigation Tabs */}
        <nav className="hidden md:flex items-center gap-1 bg-zinc-900/60 p-1 rounded-lg border border-zinc-800">
          <button
            onClick={() => setActiveTab('docs')}
            className={`flex items-center gap-2 px-3 py-1.5 rounded-md text-sm font-medium transition-colors ${
              activeTab === 'docs'
                ? 'bg-zinc-800 text-zinc-100 shadow-sm'
                : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-800/40'
            }`}
          >
            <BookOpen className="w-4 h-4 text-zinc-400" />
            Documentation
          </button>

          <button
            onClick={() => setActiveTab('playground')}
            className={`flex items-center gap-2 px-3 py-1.5 rounded-md text-sm font-medium transition-colors ${
              activeTab === 'playground'
                ? 'bg-zinc-800 text-zinc-100 shadow-sm'
                : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-800/40'
            }`}
          >
            <Terminal className="w-4 h-4 text-zinc-400" />
            Playground & REPL
          </button>

          <button
            onClick={() => setActiveTab('errors')}
            className={`flex items-center gap-2 px-3 py-1.5 rounded-md text-sm font-medium transition-colors ${
              activeTab === 'errors'
                ? 'bg-zinc-800 text-zinc-100 shadow-sm'
                : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-800/40'
            }`}
          >
            <AlertTriangle className="w-4 h-4 text-zinc-400" />
            Meme Errors
          </button>
        </nav>

        {/* Right Status & Actions */}
        <div className="flex items-center gap-3">
          {/* Wasm Status Badge - Clean & Static (No Pulse) */}
          <div className="hidden sm:flex items-center gap-2 px-2.5 py-1 rounded-md bg-zinc-900 border border-zinc-800 text-xs font-mono">
            {wasmStatus === 'ready' ? (
              <>
                <span className="w-2 h-2 rounded-full bg-emerald-500"></span>
                <span className="text-zinc-300 font-medium">Wasm Ready</span>
              </>
            ) : wasmStatus === 'error' ? (
              <>
                <span className="w-2 h-2 rounded-full bg-rose-500"></span>
                <span className="text-rose-400 font-medium">Wasm Offline</span>
              </>
            ) : (
              <>
                <span className="w-2 h-2 rounded-full bg-amber-500"></span>
                <span className="text-zinc-400 font-medium">Loading Wasm...</span>
              </>
            )}
          </div>

          <button
            onClick={() => setActiveTab('playground')}
            className="flex items-center gap-2 bg-zinc-100 hover:bg-white text-zinc-950 font-medium px-3 py-1.5 rounded-md text-xs transition-colors shadow-sm"
          >
            <Play className="w-3.5 h-3.5 fill-current" />
            <span>Playground</span>
          </button>
        </div>
      </div>
    </header>
  );
}
