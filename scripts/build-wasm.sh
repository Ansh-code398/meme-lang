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

OUTPUT_DIR="$PROJECT_ROOT/dist/wasm"
PUBLIC_DIR="$PROJECT_ROOT/dist/wasm"
mkdir -p "$OUTPUT_DIR"

em++ -std=c++20 -O3 -fexceptions \
    -s WASM=1 \
    -s EXPORT_ES6=1 \
    -s EXPORTED_FUNCTIONS='["_run_meme_code", "_meme_eval_json", "_get_meme_version", "_get_last_gpu_commands", "_get_last_hit_regions", "_wasm_dispatch_ui_event", "_meme_web_render", "_meme_web_get_mesh_vertices", "_meme_web_get_mesh_vertex_count", "_meme_web_get_mesh_indices", "_meme_web_get_mesh_index_count", "_meme_web_get_batches_json", "_meme_web_get_atlas_rgba", "_meme_web_get_atlas_version", "_meme_web_get_atlas_width", "_meme_web_get_atlas_height", "_meme_web_get_commands_json", "_meme_web_get_hit_regions_json", "_meme_web_get_agent_snapshot", "_meme_web_pointer_move", "_meme_web_pointer_down", "_meme_web_pointer_up", "_meme_web_scroll", "_meme_web_key_event", "_meme_web_tick", "_meme_web_set_clipboard", "_meme_web_get_clipboard", "_meme_web_agent_act", "_meme_web_set_route", "_meme_native_complete_async", "_meme_native_complete_perm_async", "_malloc", "_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "UTF8ToString", "stringToUTF8", "HEAPU8", "HEAPU32"]' \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createMemeModule" \
    -s STACK_SIZE=4194304 \
    -s INITIAL_MEMORY=33554432 \
    -s ALLOW_MEMORY_GROWTH=1 \
    "$PROJECT_ROOT/src/wasm/meme_wasm.cpp" \
    "$PROJECT_ROOT/src/interpreter/Interpreter.cpp" \
    "$PROJECT_ROOT/src/types/TypeChecker.cpp" \
    -o "$OUTPUT_DIR/meme.js"

echo "⚡ Building lazy MEMElang coprocessor (meme-eval.wasm)..."
em++ -std=c++20 -O3 -fexceptions \
    -s WASM=1 \
    -s EXPORT_ES6=1 \
    -s EXPORTED_FUNCTIONS='["_meme_eval_json", "_run_meme_code", "_get_last_gpu_commands", "_get_meme_version", "_malloc", "_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "UTF8ToString", "stringToUTF8"]' \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createMemeEvalModule" \
    -s STACK_SIZE=2097152 \
    -s INITIAL_MEMORY=16777216 \
    -s ALLOW_MEMORY_GROWTH=1 \
    "$PROJECT_ROOT/src/wasm/meme_wasm.cpp" \
    "$PROJECT_ROOT/src/interpreter/Interpreter.cpp" \
    "$PROJECT_ROOT/src/types/TypeChecker.cpp" \
    -o "$OUTPUT_DIR/meme-eval.js"

echo "✅ WebAssembly build succeeded!"
echo "   ESM Wrapper: $OUTPUT_DIR/meme.js"
echo "   Binary Wasm: $PUBLIC_DIR/meme.wasm"
echo "   Lazy Eval:   $OUTPUT_DIR/meme-eval.js + $OUTPUT_DIR/meme-eval.wasm"

