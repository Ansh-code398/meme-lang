import React, { useState, useEffect } from 'react';
import Navbar from './components/Navbar';
import Sidebar from './components/Sidebar';
import DocContent from './components/DocContent';
import Playground from './components/Playground';
import MemeErrorsView from './components/MemeErrorsView';
import { DOCS_SECTIONS } from './data/docsContent';
import { initMemeWasm } from './wasm/memeRunner';

export default function App() {
  const [activeTab, setActiveTab] = useState('docs');
  const [activeSectionId, setActiveSectionId] = useState(DOCS_SECTIONS[0].id);
  const [searchQuery, setSearchQuery] = useState('');
  const [wasmStatus, setWasmStatus] = useState('loading');
  const [playgroundCode, setPlaygroundCode] = useState(null);

  useEffect(() => {
    // Eagerly initialize WebAssembly module
    initMemeWasm()
      .then(() => {
        setWasmStatus('ready');
      })
      .catch((err) => {
        console.error('Wasm init failed:', err);
        setWasmStatus('error');
      });
  }, []);

  const handleOpenInPlayground = (code) => {
    setPlaygroundCode(code);
    setActiveTab('playground');
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };

  const currentSectionIndex = DOCS_SECTIONS.findIndex((s) => s.id === activeSectionId);
  const currentSection = DOCS_SECTIONS[currentSectionIndex] || DOCS_SECTIONS[0];
  const prevSection = currentSectionIndex > 0 ? DOCS_SECTIONS[currentSectionIndex - 1] : null;
  const nextSection = currentSectionIndex < DOCS_SECTIONS.length - 1 ? DOCS_SECTIONS[currentSectionIndex + 1] : null;

  return (
    <div className="min-h-screen bg-zinc-950 text-zinc-100 flex flex-col font-sans selection:bg-amber-500/20 selection:text-amber-300">
      {/* Top Navbar */}
      <Navbar
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        wasmStatus={wasmStatus}
      />

      {/* Main Content Area based on Tab */}
      <main className="flex-1 flex flex-col">
        {activeTab === 'docs' && (
          <div className="flex-1 flex flex-col lg:flex-row max-w-7xl mx-auto w-full">
            <Sidebar
              sections={DOCS_SECTIONS}
              activeSection={activeSectionId}
              onSelectSection={(id) => {
                setActiveSectionId(id);
                window.scrollTo({ top: 0, behavior: 'smooth' });
              }}
              searchQuery={searchQuery}
              setSearchQuery={setSearchQuery}
            />
            <div className="flex-1 w-full min-w-0">
              <DocContent
                section={currentSection}
                onOpenInPlayground={handleOpenInPlayground}
                onNavigateSection={(id) => {
                  setActiveSectionId(id);
                  window.scrollTo({ top: 0, behavior: 'smooth' });
                }}
                prevSection={prevSection}
                nextSection={nextSection}
              />
            </div>
          </div>
        )}

        {activeTab === 'playground' && (
          <Playground initialCode={playgroundCode} />
        )}

        {activeTab === 'errors' && (
          <MemeErrorsView onOpenInPlayground={handleOpenInPlayground} />
        )}
      </main>

      {/* Minimal Footer */}
      <footer className="border-t border-zinc-800 bg-zinc-950 py-6 text-center text-xs text-zinc-400">
        <p>
          MEMElang v1.0 • Built with C++20, WebAssembly & React • 
          <span className="text-zinc-200 ml-1 font-medium">Wah bete mauj kardi!</span>
        </p>
      </footer>
    </div>
  );
}
