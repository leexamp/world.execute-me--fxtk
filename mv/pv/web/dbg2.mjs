import { chromium } from 'playwright-core';
import os from 'os';
const EXE = os.homedir() + '/.cache/ms-playwright/chromium-1148/chrome-linux/chrome';
const b = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox','--disable-gpu'] });
const p = await b.newPage({ viewport: { width: 1920, height: 1080 } });
p.on('pageerror', e => console.log('PAGEERROR:', e.message));
await p.goto('http://127.0.0.1:8901/web/mv.html');
await p.waitForTimeout(1500);
for (const t of [0, 3, 8, 20, 40, 70, 100, 130, 160, 190, 205]) {
  const r = await p.evaluate((tt) => { try { window.renderFrame(tt); return 'ok'; } catch (e) { return 'ERR: ' + e.message; } }, t);
  console.log('t=' + t, r);
}
await b.close();
