import React from 'react';
import CodeBlock from './CodeBlock';
import { Info, AlertTriangle, ArrowRight, ArrowLeft } from 'lucide-react';

export default function DocContent({ section, onOpenInPlayground, onNavigateSection, prevSection, nextSection }) {
  if (!section) return null;

  return (
    <article className="max-w-[72ch] mx-auto px-4 lg:px-10 py-8 lg:py-12">
      {/* Page Header */}
      <header className="mb-8 pb-6 border-b border-zinc-800">
        <h1 className="text-3xl font-bold tracking-tight text-zinc-100 mb-3">
          {section.title}
        </h1>
        {section.description && (
          <p className="text-base text-zinc-400 leading-relaxed">
            {section.description}
          </p>
        )}
      </header>

      {/* Body Content */}
      <div className="space-y-6 text-sm lg:text-base text-zinc-300 leading-relaxed">
        {section.content?.map((item, idx) => {
          if (item.type === 'text') {
            return (
              <p 
                key={idx} 
                className="leading-relaxed text-zinc-300" 
                dangerouslySetInnerHTML={{ 
                  __html: item.text
                    .replace(/\*\*(.*?)\*\*/g, '<strong class="text-zinc-100 font-semibold">$1</strong>')
                    .replace(/`([^`]+)`/g, '<code class="px-1.5 py-0.5 rounded bg-zinc-800/80 text-amber-300 font-mono text-xs border border-zinc-700/40">$1</code>') 
                }} 
              />
            );
          }

          if (item.type === 'heading') {
            // Guarantee semantic heading hierarchy (h2 for main headings, never skipping to h3 directly from h1)
            const HeadingTag = item.level === 3 ? 'h3' : 'h2';
            return (
              <HeadingTag
                key={idx}
                className={
                  item.level === 3
                    ? "text-lg font-semibold text-zinc-100 mt-6 mb-2"
                    : "text-xl font-semibold text-zinc-100 mt-10 mb-3 pt-6 border-t border-zinc-800/80 first:border-t-0"
                }
              >
                {item.text}
              </HeadingTag>
            );
          }

          if (item.type === 'code') {
            return (
              <CodeBlock
                key={idx}
                title={item.title}
                code={item.code}
                onOpenInPlayground={onOpenInPlayground}
              />
            );
          }

          if (item.type === 'callout') {
            const isWarning = item.variant === 'warning';
            return (
              <aside
                key={idx}
                className={`my-6 p-4 rounded-lg border bg-zinc-900/60 flex gap-3.5 items-start ${
                  isWarning
                    ? 'border-rose-900/40 text-rose-200'
                    : 'border-zinc-800 text-zinc-300'
                }`}
              >
                {isWarning ? (
                  <AlertTriangle className="w-5 h-5 text-rose-400 shrink-0 mt-0.5" />
                ) : (
                  <Info className="w-5 h-5 text-amber-400 shrink-0 mt-0.5" />
                )}
                <div className="text-sm">
                  {item.title && (
                    <div className="font-semibold text-zinc-100 mb-1">
                      {item.title}
                    </div>
                  )}
                  <div 
                    className="text-zinc-300 leading-relaxed"
                    dangerouslySetInnerHTML={{ 
                      __html: item.text
                        .replace(/\*\*(.*?)\*\*/g, '<strong class="text-zinc-100 font-semibold">$1</strong>')
                        .replace(/`([^`]+)`/g, '<code class="px-1.5 py-0.5 bg-zinc-800 rounded font-mono text-xs text-amber-300 border border-zinc-700/50">$1</code>') 
                    }} 
                  />
                </div>
              </aside>
            );
          }

          if (item.type === 'table') {
            return (
              <div key={idx} className="my-6 overflow-x-auto rounded-lg border border-zinc-800 bg-zinc-900/30">
                <table className="w-full text-left text-sm">
                  <thead className="bg-zinc-900/90 border-b border-zinc-800 text-zinc-400 font-mono text-xs uppercase tracking-wider">
                    <tr>
                      {item.headers.map((h, i) => (
                        <th key={i} className="px-4 py-3 font-semibold">{h}</th>
                      ))}
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-zinc-800/60">
                    {item.rows.map((row, rIdx) => (
                      <tr key={rIdx} className="hover:bg-zinc-900/50 transition-colors">
                        {row.map((cell, cIdx) => (
                          <td key={cIdx} className="px-4 py-3 text-zinc-300">
                            {cIdx === 0 ? (
                              <code className="text-amber-300 font-mono font-medium bg-zinc-800/80 px-2 py-0.5 rounded border border-zinc-700/50 text-xs">
                                {cell}
                              </code>
                            ) : (
                              cell
                            )}
                          </td>
                        ))}
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            );
          }

          if (item.type === 'list') {
            return (
              <ul key={idx} className="my-4 space-y-2 text-zinc-300 text-sm lg:text-base leading-relaxed pl-2">
                {item.items.map((li, lIdx) => (
                  <li key={lIdx} className="flex items-start gap-2.5">
                    <span className="text-zinc-500 select-none font-bold mt-0.5">•</span>
                    <span 
                      dangerouslySetInnerHTML={{ 
                        __html: li
                          .replace(/\*\*(.*?)\*\*/g, '<strong class="text-zinc-100 font-semibold">$1</strong>')
                          .replace(/`([^`]+)`/g, '<code class="px-1.5 py-0.5 rounded bg-zinc-800/80 text-amber-300 font-mono text-xs border border-zinc-700/40">$1</code>') 
                      }} 
                    />
                  </li>
                ))}
              </ul>
            );
          }

          if (item.type === 'terminal') {
            return (
              <div key={idx} className="my-4 rounded-lg border border-zinc-800 bg-zinc-950 p-4 font-mono text-xs text-zinc-300">
                <div className="text-xs text-zinc-400 mb-2 font-mono pb-2 border-b border-zinc-850">
                  Terminal
                </div>
                {item.commands.map((cmd, cIdx) => (
                  <div key={cIdx} className="flex items-center gap-2 py-0.5">
                    <span className="text-zinc-500 select-none">$</span>
                    <span className="text-emerald-400 font-medium">{cmd}</span>
                  </div>
                ))}
              </div>
            );
          }

          return null;
        })}
      </div>

      {/* Footer Navigation (Prev / Next) */}
      <footer className="mt-12 pt-6 border-t border-zinc-800 flex items-center justify-between gap-4">
        {prevSection ? (
          <button
            onClick={() => onNavigateSection(prevSection.id)}
            className="flex items-center gap-2 px-4 py-2.5 rounded-lg border border-zinc-800 hover:border-zinc-700 bg-zinc-900/60 text-zinc-300 hover:text-zinc-100 text-xs font-medium transition-colors"
          >
            <ArrowLeft className="w-3.5 h-3.5" />
            <div className="text-left">
              <span className="block text-xs text-zinc-400">Previous</span>
              <span>{prevSection.title}</span>
            </div>
          </button>
        ) : <div />}

        {nextSection && (
          <button
            onClick={() => onNavigateSection(nextSection.id)}
            className="flex items-center gap-2 px-4 py-2.5 rounded-lg border border-zinc-700 hover:border-zinc-600 bg-zinc-900 text-zinc-200 hover:text-white text-xs font-medium transition-colors ml-auto"
          >
            <div className="text-right">
              <span className="block text-xs text-zinc-400">Next</span>
              <span>{nextSection.title}</span>
            </div>
            <ArrowRight className="w-3.5 h-3.5" />
          </button>
        )}
      </footer>
    </article>
  );
}
