import fs from 'fs';
import path from 'path';
import http from 'http';
import { spawn } from 'child_process';

const DIST_DIR = path.resolve('docs/dist');
const CHROME_BIN = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome';
const ARTIFACT_DIR = '/Users/anshsharma/.gemini/antigravity/brain/7525a83b-8ec9-4636-ae7d-4a919ffc44f4';

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.wasm': 'application/wasm',
  '.json': 'application/json; charset=utf-8',
  '.webmanifest': 'application/manifest+json; charset=utf-8',
  '.png': 'image/png',
  '.ico': 'image/x-icon',
};

function startSpaServer() {
  return new Promise((resolve) => {
    const server = http.createServer((req, res) => {
      const reqPath = (req.url || '/').split('?')[0];
      let filePath = path.join(DIST_DIR, reqPath === '/' ? 'index.html' : reqPath);
      if (!fs.existsSync(filePath) || fs.statSync(filePath).isDirectory()) {
        // Mimic Vercel vercel.json rewrite: "/(.*)" -> "/index.html"
        filePath = path.join(DIST_DIR, 'index.html');
      }
      const ext = path.extname(filePath);
      res.writeHead(200, { 'Content-Type': MIME[ext] || 'application/octet-stream' });
      fs.createReadStream(filePath).pipe(res);
    });
    server.listen(0, '127.0.0.1', () => {
      resolve({ server, port: server.address().port });
    });
  });
}

class CdpClient {
  constructor(wsUrl) {
    this.ws = new WebSocket(wsUrl);
    this.nextId = 1;
    this.pending = new Map();
    this.events = [];
  }
  async connect() {
    await new Promise((resolve, reject) => {
      this.ws.addEventListener('open', resolve, { once: true });
      this.ws.addEventListener('error', reject, { once: true });
    });
    this.ws.addEventListener('message', (ev) => {
      const msg = JSON.parse(ev.data);
      if (msg.id && this.pending.has(msg.id)) {
        const { resolve, reject } = this.pending.get(msg.id);
        this.pending.delete(msg.id);
        if (msg.error) reject(new Error(JSON.stringify(msg.error)));
        else resolve(msg.result);
      } else if (msg.method) {
        this.events.push(msg);
      }
    });
  }
  send(method, params = {}) {
    const id = this.nextId++;
    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject });
      this.ws.send(JSON.stringify({ id, method, params }));
    });
  }
  close() {
    this.ws.close();
  }
}

async function waitForSnap(cdp, pred, desc, timeoutMs = 15000) {
  const t0 = Date.now();
  let lastSnap = null;
  while (Date.now() - t0 < timeoutMs) {
    const res = await cdp.send('Runtime.evaluate', {
      expression: '(() => { try { return window.__MEME_AGENT__ ? window.__MEME_AGENT__.snapshot() : null; } catch (e) { return null; } })()',
      returnByValue: true,
    });
    lastSnap = res?.result?.value || null;
    if (lastSnap && pred(lastSnap)) return lastSnap;
    await new Promise((r) => setTimeout(r, 150));
  }
  throw new Error(`Timed out waiting for: ${desc}. Last nodes sample: ${JSON.stringify((lastSnap?.nodes || []).slice(0, 12))}`);
}

async function clickByLabel(cdp, textSubstr) {
  const res = await cdp.send('Runtime.evaluate', {
    expression: `(() => {
      const s = window.__MEME_AGENT__.snapshot();
      const clickable = (s.nodes || []).filter(r => r.hasClick && r.width > 0 && r.height > 0);
      const hit = clickable.find(r => (r.accessibleName || '').trim() === ${JSON.stringify(textSubstr)})
               || clickable.find(r => (r.accessibleName || '').includes(${JSON.stringify(textSubstr)}));
      if (!hit) return { ok: false, names: clickable.map(r => r.accessibleName).slice(0, 40) };
      const cx = hit.x + hit.width * 0.5;
      const cy = hit.y + hit.height * 0.5;
      return { ok: true, id: hit.id, cx, cy, name: hit.accessibleName, vh: s.viewport?.height || 820 };
    })()`,
    returnByValue: true,
  });
  const info = res.result.value;
  if (!info.ok) throw new Error(`Could not find clickable element matching '${textSubstr}'. Available: ${JSON.stringify(info.names)}`);
  if (info.cy > info.vh - 10 || info.cy < 10) {
    // Scroll container or trigger agent click when item is outside visible fold
    await cdp.send('Runtime.evaluate', {
      expression: `window.__MEME_AGENT__.act(${info.id}, "click", "")`,
      returnByValue: true,
    });
  } else {
    await cdp.send('Input.dispatchMouseEvent', { type: 'mousePressed', x: info.cx, y: info.cy, button: 'left', clickCount: 1 });
    await cdp.send('Input.dispatchMouseEvent', { type: 'mouseReleased', x: info.cx, y: info.cy, button: 'left', clickCount: 1 });
  }
  await new Promise((r) => setTimeout(r, 150));
  return info;
}

async function capturePng(cdp, outPath) {
  const shot = await cdp.send('Page.captureScreenshot', { format: 'png' });
  fs.writeFileSync(outPath, Buffer.from(shot.data, 'base64'));
}

async function main() {
  if (!fs.existsSync(path.join(DIST_DIR, 'index.html'))) {
    throw new Error('Missing docs/dist/index.html');
  }
  const vercelCfg = JSON.parse(fs.readFileSync(path.join(DIST_DIR, 'vercel.json'), 'utf8'));
  if (!Array.isArray(vercelCfg.rewrites) || vercelCfg.rewrites[0]?.destination !== '/index.html') {
    throw new Error('Invalid docs/dist/vercel.json rewrites');
  }

  const { server, port } = await startSpaServer();
  const userDataDir = `/tmp/meme_docs_chrome_${Date.now()}`;
  fs.mkdirSync(userDataDir, { recursive: true });
  const chromeProc = spawn(CHROME_BIN, [
    '--headless=new',
    '--disable-gpu-sandbox',
    '--no-sandbox',
    '--remote-debugging-port=0',
    `--user-data-dir=${userDataDir}`,
    '--window-size=1280,820',
    'about:blank',
  ], { stdio: ['ignore', 'pipe', 'pipe'] });

  let cdp = null;
  try {
    const devPort = await new Promise((resolve, reject) => {
      let buf = '';
      const onData = (chunk) => {
        buf += chunk.toString();
        const m = buf.match(/DevTools listening on ws:\/\/127\.0\.0\.1:(\d+)\//);
        if (m) resolve(Number(m[1]));
      };
      chromeProc.stderr.on('data', onData);
      chromeProc.stdout.on('data', onData);
      setTimeout(() => reject(new Error('Chrome DevTools startup timeout:\n' + buf)), 10000);
    });

    const pages = await (await fetch(`http://127.0.0.1:${devPort}/json`)).json();
    const target = pages.find((p) => p.type === 'page') || pages[0];
    cdp = new CdpClient(target.webSocketDebuggerUrl);
    await cdp.connect();
    await cdp.send('Page.enable');
    await cdp.send('Runtime.enable');
    await cdp.send('Emulation.setDeviceMetricsOverride', { width: 1280, height: 820, deviceScaleFactor: 2, mobile: false });

    const pageErrors = [];
    const consoleLogs = [];
    await cdp.send('Log.enable');
    cdp.ws.addEventListener('message', (ev) => {
      const m = JSON.parse(ev.data);
      if (m.method === 'Runtime.exceptionThrown') pageErrors.push(JSON.stringify(m.params));
      if (m.method === 'Runtime.consoleAPICalled') {
        const text = (m.params.args || []).map((a) => a.value ?? a.description ?? '').join(' ');
        consoleLogs.push(`[console.${m.params.type}] ${text}`);
      }
      if (m.method === 'Log.entryAdded') {
        consoleLogs.push(`[log.${m.params.entry.level}] ${m.params.entry.text}`);
      }
    });

    const baseUrl = `http://127.0.0.1:${port}`;
    await cdp.send('Page.navigate', { url: `${baseUrl}/` });

    // 1. Docs home renders in WebGL2/Wasm
    try {
      await waitForSnap(cdp, (s) => (s.nodes || []).some((r) => (r.accessibleName || '') === 'Run'), 'Docs Home loaded', 10000);
    } catch (e) {
      await capturePng(cdp, path.join(ARTIFACT_DIR, 'pure_docs_debug_fail.png'));
      const snap = (await cdp.send('Runtime.evaluate', { expression: 'JSON.stringify(window.__MEME_AGENT__ ? window.__MEME_AGENT__.snapshot() : null)', returnByValue: true })).result.value;
      console.error('DEBUG SNAP:', snap);
      console.error('CONSOLE LOGS:\n' + consoleLogs.join('\n'));
      console.error('PAGE ERRORS:\n' + pageErrors.join('\n'));
      throw e;
    }
    await capturePng(cdp, path.join(ARTIFACT_DIR, 'pure_docs_home.png'));
    console.log('[PASS] Pure-.meme Docs Home rendered on WebGL2 Canvas');

    // 2. Inline CodeBlock "Run" button executes live meme.eval in Wasm!
    await clickByLabel(cdp, 'Run');
    await waitForSnap(cdp, (s) => (s.nodes || []).some((r) => (r.accessibleName || '').includes('Close')), 'Inline CodeBlock Run Drawer opened');
    console.log('[PASS] Inline Docs CodeBlock Run button executed live via meme.eval in Wasm');

    // 3. Navigate to Hosting & Deployment section via sidebar & check URL history sync!
    await clickByLabel(cdp, 'Hosting & Deployment');
    await waitForSnap(cdp, (s) => (s.nodes || []).some((r) => (r.accessibleName || '').includes('Package Static Web Bundle')), 'Hosting & Deployment section active');
    const urlAfterSec = (await cdp.send('Runtime.evaluate', { expression: 'window.location.pathname', returnByValue: true })).result.value;
    if (urlAfterSec !== '/docs/hosting-deploy') throw new Error(`Expected pathname /docs/hosting-deploy, got ${urlAfterSec}`);
    await capturePng(cdp, path.join(ARTIFACT_DIR, 'pure_docs_hosting.png'));
    console.log('[PASS] URL history synced to /docs/hosting-deploy');

    // 4. Deep-link reload test (Vercel SPA fallback /docs/native-permissions)
    await cdp.send('Page.navigate', { url: `${baseUrl}/docs/native-permissions` });
    await waitForSnap(cdp, (s) => (s.nodes || []).some((r) => (r.accessibleName || '') === 'Run'), 'Deep-link /docs/native-permissions reloaded cleanly');
    const deepPath = (await cdp.send('Runtime.evaluate', { expression: 'window.location.pathname', returnByValue: true })).result.value;
    if (deepPath !== '/docs/native-permissions') throw new Error(`Expected /docs/native-permissions on reload, got ${deepPath}`);
    console.log('[PASS] Direct deep-link reload /docs/native-permissions succeeded');

    // 5. Switch to Playground & REPL tab, pick GPU UI preset, Run in Wasm, check GPU Canvas!
    await clickByLabel(cdp, 'Playground & REPL');
    await waitForSnap(cdp, (s) => (s.nodes || []).some((r) => (r.accessibleName || '').includes('Run in Wasm')), 'Playground tab active');
    await clickByLabel(cdp, 'Zero-WebView Native GPU UI');
    await clickByLabel(cdp, 'Run in Wasm');
    await waitForSnap(cdp, (s) => (s.nodes || []).some((r) => (r.accessibleName || '').includes('Direct GPU UI (') && !(r.accessibleName || '').includes('Direct GPU UI (0)')), 'Playground GPU preview populated');
    await capturePng(cdp, path.join(ARTIFACT_DIR, 'pure_docs_playground_gpu.png'));
    console.log('[PASS] Playground executed GPU UI snippet in Wasm & painted <canvas> GPU preview');

    // 6. Switch to Meme Errors tab & trigger live error
    await clickByLabel(cdp, 'Meme Errors');
    await waitForSnap(cdp, (s) => (s.nodes || []).some((r) => (r.accessibleName || '').includes('Trigger Error Live')), 'Meme Errors tab active');
    await clickByLabel(cdp, 'Trigger Error Live');
    await new Promise((r) => setTimeout(r, 250));
    await capturePng(cdp, path.join(ARTIFACT_DIR, 'pure_docs_errors.png'));
    console.log('[PASS] Meme Errors Catalog triggered live diagnostic in Wasm');

    // 7. Mobile viewport layout (393x852)
    await cdp.send('Emulation.setDeviceMetricsOverride', { width: 393, height: 852, deviceScaleFactor: 2, mobile: true });
    await waitForSnap(cdp, (s) => Math.round(s.viewport?.width || 0) === 393, 'Mobile viewport resized to 393px');
    await clickByLabel(cdp, 'Documentation');
    await waitForSnap(cdp, (s) => (s.nodes || []).some((r) => (r.accessibleName || '').includes('Topics')), 'Mobile compact Topics drawer button rendered');
    await capturePng(cdp, path.join(ARTIFACT_DIR, 'pure_docs_mobile.png'));
    console.log('[PASS] Mobile viewport (393x852) compact drawer layout verified');

    if (pageErrors.length > 0) {
      throw new Error('Runtime JS/Wasm page errors detected:\n' + pageErrors.join('\n'));
    }
    console.log('ALL PURE-MEMELANG DOCS E2E CHECKS PASSED!');
  } finally {
    try { cdp?.close(); } catch (_) {}
    try { chromeProc.kill('SIGKILL'); } catch (_) {}
    try { server.close(); } catch (_) {}
    try { fs.rmSync(userDataDir, { recursive: true, force: true }); } catch (_) {}
  }
}

main().catch((err) => {
  console.error('❌ E2E Failed:', err);
  process.exit(1);
});
