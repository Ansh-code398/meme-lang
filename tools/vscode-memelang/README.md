# MEMElang VS Code Extension 🎭

Official Visual Studio Code extension providing **Syntax Highlighting**, **IntelliSense Autocomplete**, and **Code Snippets** for **MEMElang** (`.meme` files).

## Features

- 🎨 **Rich Syntax Highlighting**:
  - Full TextMate grammar covering variables (`pakad`), functions (`jugaad`), lambdas (`=>`), control flow (`dekh`, `warna`, `jabtak`, `ghoom`), and modules (`bula`).
  - **Tantrik Methods** highlighting (`__jod__`, `__ghata__`, `__guna__`, `__bhaag__`, `__bacha__`, `__barabar__`, `__chota__`, `__la__`, `__daal__`, `__dikha__`, `__size__`, `__chala__`).
  - **F-String Interpolation** highlighting (`f"Hello {name}"` with nested expression scopes).
  - **Collection Transforms** (`.sab_pe`, `.chhaan`, `.nichod`, `.line_mein`, `.har_ek`, `.pakad_ek`, `.chala_de`).
- ⚡ **IntelliSense & Autocomplete**:
  - Context-aware autocomplete for all MEMElang keywords, builtins, and functional transforms.
  - Type `.` to immediately see Array collection methods (`.sab_pe`, `.chhaan`, `.nichod`, `.line_mein`, etc.).
  - Type `__` to see all Tantrik dunder methods.
  - Hover documentation for all keywords and builtins.
- ✂️ **Productivity Snippets**:
  - `jugaad` -> function declaration template
  - `lambda` -> `(x) => x * 2` arrow function
  - `biradari` -> class template with constructor `shuru`
  - `tantrik_cmp` -> `__barabar__` & `__chota__`
  - `tantrik_index` -> `__la__` & `__daal__`
  - `chhaan`, `sab_pe`, `nichod` -> collection transform pipelines
  - `destruct_arr`, `destruct_map` -> destructuring templates
- ⌨️ **Language Configuration**:
  - Auto-closing pairs for `()`, `[]`, `{}`, `""`, `''`, `f""`, `f''`.
  - Indentation rules and comment toggling (`//` and `/* */`).

## Installation

### Local Installation
Copy or symlink this folder to your VS Code extensions directory:

```bash
# macOS / Linux
ln -s "$(pwd)/tools/vscode-memelang" ~/.vscode/extensions/memelang

# Or package to .vsix
cd tools/vscode-memelang
npx @vscode/vsce package
code --install-extension memelang-1.0.0.vsix
```
