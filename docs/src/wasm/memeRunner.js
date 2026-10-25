import createMemeModule from './meme.js';
import memeWasmUrl from './meme.wasm?url';

let memeModuleInstance = null;
let memeModulePromise = null;

/**
 * Initializes and caches the MEMElang WebAssembly runtime module.
 */
export function initMemeWasm() {
  if (memeModuleInstance) {
    return Promise.resolve(memeModuleInstance);
  }
  if (memeModulePromise) {
    return memeModulePromise;
  }

  memeModulePromise = createMemeModule({
    locateFile: (path) => {
      if (path.endsWith('.wasm')) {
        return memeWasmUrl;
      }
      return new URL(`/wasm/${path}`, window.location.href).href;
    },
    print: (text) => console.log('[MEMElang Wasm Out]:', text),
    printErr: (text) => console.warn('[MEMElang Wasm Err]:', text)
  })
    .then((module) => {
      memeModuleInstance = module;
      return module;
    })
    .catch((err) => {
      console.error('Failed to instantiate WebAssembly module:', err);
      memeModulePromise = null;
      throw err;
    });

  return memeModulePromise;
}

/**
 * Executes MEMElang source code directly inside the browser using WebAssembly.
 * @param {string} sourceCode
 * @param {string} [stdinText='']
 * @returns {Promise<{ output: string, isError: boolean, durationMs: string, timestamp: string }>}
 */
export async function runMemeCode(sourceCode, stdinText = '') {
  const Module = await initMemeWasm();
  const runCode = Module.cwrap('run_meme_code', 'string', ['string', 'string']);

  const startTime = performance.now();
  let output = '';
  try {
    output = runCode(sourceCode, stdinText || '');
  } catch (err) {
    output = `\n❌ WebAssembly Execution Error:\n${err.message || err}\n`;
  }
  const endTime = performance.now();
  const durationMs = (endTime - startTime).toFixed(2);

  const isError =
    output.includes('❌ Gadbad ho gayi:') ||
    output.includes('❌ WebAssembly Execution Error') ||
    output.includes('❌ Anjaan gadbad');

  let gpuCommands = null;
  let hitRegions = [];
  try {
    const getLastGpuCmds = Module.cwrap('get_last_gpu_commands', 'string', []);
    const gpuRaw = getLastGpuCmds();
    if (gpuRaw && gpuRaw.trim().startsWith('[')) {
      gpuCommands = JSON.parse(gpuRaw);
    }
  } catch (e) {
    console.debug('Failed to parse GPU commands:', e);
  }

  try {
    const getLastHitRegions = Module.cwrap('get_last_hit_regions', 'string', []);
    const hitRaw = getLastHitRegions ? getLastHitRegions() : null;
    if (hitRaw && hitRaw.trim().startsWith('[')) {
      hitRegions = JSON.parse(hitRaw);
    }
  } catch (e) {
    console.debug('Failed to parse hit regions:', e);
  }

  return {
    output,
    isError,
    durationMs,
    gpuCommands,
    hitRegions,
    timestamp: new Date().toLocaleTimeString()
  };
}

/**
 * Dispatches a UI hit-test event (click, change) into the active MEMElang component runtime.
 * @param {number} nodeId
 * @param {string} [eventType='click']
 * @param {string} [value='']
 * @returns {Promise<{ gpuCommands: Array|null, hitRegions: Array }>}
 */
export async function dispatchUiEvent(nodeId, eventType = 'click', value = '') {
  const Module = await initMemeWasm();
  const dispatch = Module.cwrap('wasm_dispatch_ui_event', 'string', ['number', 'string', 'string']);
  let gpuCommands = null;
  let hitRegions = [];
  try {
    const gpuRaw = dispatch(nodeId, eventType, value);
    if (gpuRaw && gpuRaw.trim().startsWith('[')) {
      gpuCommands = JSON.parse(gpuRaw);
    }
    const getLastHitRegions = Module.cwrap('get_last_hit_regions', 'string', []);
    const hitRaw = getLastHitRegions ? getLastHitRegions() : null;
    if (hitRaw && hitRaw.trim().startsWith('[')) {
      hitRegions = JSON.parse(hitRaw);
    }
  } catch (e) {
    console.error('Failed to dispatch UI event in WebAssembly:', e);
  }
  return { gpuCommands, hitRegions };
}

/**
 * Returns the engine version string from WebAssembly.
 * @returns {Promise<string>}
 */
export async function getMemeVersion() {
  const Module = await initMemeWasm();
  const getVersion = Module.cwrap('get_meme_version', 'string', []);
  return getVersion();
}
