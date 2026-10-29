import React from 'react';
import { Search, X } from 'lucide-react';

const CATEGORIES = [
  {
    name: 'Getting Started',
    sectionIds: ['intro', 'quickstart']
  },
  {
    name: 'Language Basics',
    sectionIds: ['variables', 'types', 'operators', 'control-flow', 'collections', 'pattern-matching']
  },
  {
    name: 'Functions & Architecture',
    sectionIds: ['functions', 'oop', 'error-handling', 'modules']
  },
  {
    name: 'Async, Networking & Concurrency',
    sectionIds: ['async-await-promises', 'fetch-http-client', 'concurrency-threads-mp', 'filesystem-paths']
  },
  {
    name: 'Type System & Memory Safety',
    sectionIds: ['gradual-typing', 'memory-safety']
  },
  {
    name: 'Native UI & Reactivity',
    sectionIds: ['react-jsx-ui', 'native-desktop']
  },
  {
    name: 'Universal Interop',
    sectionIds: ['chameleon-bridge']
  },
  {
    name: 'Tooling & Package Manager',
    sectionIds: ['tapri-package-manager']
  },
  {
    name: 'Standard Library & Errors',
    sectionIds: ['stdlib', 'builtins', 'meme-errors']
  }
];

export default function Sidebar({ sections, activeSection, onSelectSection, searchQuery, setSearchQuery }) {
  const filteredSections = sections.filter((s) => {
    if (!searchQuery) return true;
    const query = searchQuery.toLowerCase();
    return s.title.toLowerCase().includes(query) || (s.description && s.description.toLowerCase().includes(query));
  });

  return (
    <aside className="w-full lg:w-68 shrink-0 border-r border-zinc-800 p-4 lg:p-6 bg-zinc-950 lg:sticky lg:top-[57px] lg:h-[calc(100vh-57px)] lg:overflow-y-auto">
      {/* Search Input with WCAG AA Contrast */}
      <div className="relative mb-5">
        <Search className="w-4 h-4 text-zinc-400 absolute left-3 top-1/2 -translate-y-1/2 pointer-events-none" />
        <input
          type="text"
          placeholder="Search docs (e.g. jugaad)..."
          value={searchQuery}
          onChange={(e) => setSearchQuery(e.target.value)}
          className="w-full bg-zinc-900 border border-zinc-800 rounded-lg pl-9 pr-8 py-2 text-sm text-zinc-100 placeholder:text-zinc-400 focus:outline-none focus:border-zinc-600 transition-colors"
        />
        {searchQuery && (
          <button
            onClick={() => setSearchQuery('')}
            className="absolute right-2.5 top-1/2 -translate-y-1/2 text-zinc-400 hover:text-zinc-200 p-0.5 rounded"
            title="Clear search"
          >
            <X className="w-3.5 h-3.5" />
          </button>
        )}
      </div>

      {/* Navigation Tree */}
      <nav className="space-y-4">
        {searchQuery ? (
          <div>
            <div className="px-3 py-1.5 text-xs font-semibold uppercase tracking-wider text-zinc-400">
              Search Results ({filteredSections.length})
            </div>
            <div className="space-y-0.5 mt-1">
              {filteredSections.map((section) => {
                const isActive = activeSection === section.id;
                return (
                  <button
                    key={section.id}
                    onClick={() => onSelectSection(section.id)}
                    className={`w-full text-left px-3 py-2 rounded-md text-sm transition-colors ${
                      isActive
                        ? 'bg-zinc-800 text-zinc-100 font-medium'
                        : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-900/60'
                    }`}
                  >
                    {section.title}
                  </button>
                );
              })}
              {filteredSections.length === 0 && (
                <div className="px-3 py-4 text-xs text-zinc-400">
                  No sections match "{searchQuery}"
                </div>
              )}
            </div>
          </div>
        ) : (
          CATEGORIES.map((category) => {
            const categorySections = sections.filter((s) => category.sectionIds.includes(s.id));
            if (categorySections.length === 0) return null;

            return (
              <div key={category.name}>
                <div className="px-3 py-1 text-xs font-semibold uppercase tracking-wider text-zinc-400">
                  {category.name}
                </div>
                <div className="space-y-0.5 mt-1">
                  {categorySections.map((section) => {
                    const isActive = activeSection === section.id;
                    return (
                      <button
                        key={section.id}
                        onClick={() => onSelectSection(section.id)}
                        className={`w-full text-left px-3 py-2 rounded-md text-sm transition-colors ${
                          isActive
                            ? 'bg-zinc-800 text-zinc-100 font-medium'
                            : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-900/60'
                        }`}
                      >
                        {section.title}
                      </button>
                    );
                  })}
                </div>
              </div>
            );
          })
        )}
      </nav>
    </aside>
  );
}
