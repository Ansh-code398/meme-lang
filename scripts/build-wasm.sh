#!/usr/bin/env bash
set -e

# Determine directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Check if emsdk exists in tools/emsdk
if [ -d "$PROJECT_ROOT/tools/emsdk" ]; then
    echo "📦 Sourcing local emsdk environment..."
    export EMSDK_QUIET=1
    source "$PROJECT_ROOT/tools/emsdk/emsdk_env.sh" > /dev/null 2>&1
fi

if ! command -v em++ &> /dev/null; then
    echo "❌ Error: em++ (Emscripten) not found in PATH."
    echo "Please install or activate emsdk first:"
    echo "  ./tools/emsdk/emsdk install latest && ./tools/emsdk/emsdk activate latest"
    exit 1
fi

echo "🚀 Building MEMElang WebAssembly module..."

OUTPUT_DIR="$PROJECT_ROOT/docs/src/wasm"
PUBLIC_DIR="$PROJECT_ROOT/docs/public/wasm"
mkdir -p "$OUTPUT_DIR"
mkdir -p "$PUBLIC_DIR"

em++ -std=c++20 -O3 -fexceptions \
    -s WASM=1 \
    -s EXPORT_ES6=1 \
    -s EXPORTED_FUNCTIONS='["_run_meme_code", "_get_meme_version", "_get_last_gpu_commands", "_get_last_hit_regions", "_wasm_dispatch_ui_event", "_malloc", "_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "UTF8ToString", "stringToUTF8"]' \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createMemeModule" \
    -s ALLOW_MEMORY_GROWTH=1 \
    "$PROJECT_ROOT/src/wasm/meme_wasm.cpp" \
    "$PROJECT_ROOT/src/interpreter/Interpreter.cpp" \
    "$PROJECT_ROOT/src/types/TypeChecker.cpp" \
    -o "$OUTPUT_DIR/meme.js"

cp "$OUTPUT_DIR/meme.wasm" "$PUBLIC_DIR/meme.wasm"

echo "✅ WebAssembly build succeeded!"
echo "   ESM Wrapper: $OUTPUT_DIR/meme.js"
echo "   Binary Wasm: $PUBLIC_DIR/meme.wasm"

