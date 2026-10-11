/**
 * render.mjs — 用 Playwright 逐帧截 mv.html。
 *
 * 每一帧都是 t 的纯函数: window.renderFrame(t) 把整张画面算到 t 时刻的状态,
 * 所以同一 t 渲染多少次都一样 —— 可重跑、可校对, 不依赖真实播放速度。
 *
 * 用法: node render.mjs <输出目录> <帧率> <起始秒> <结束秒> [质量] [--png]
 */
import { chromium } from 'playwright-core';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const [outdir, fpsS, t0S, t1S, qS] = process.argv.slice(2);
const PNG = process.argv.includes('--png');
const fps = Number(fpsS || 30), t0 = Number(t0S || 0), t1 = Number(t1S || 0.04);
const quality = Number(qS || 93);
const EXE = process.env.CHROME_PATH || (process.env.HOME + '/.cache/ms-playwright/chromium-1148/chrome-linux/chrome');

fs.mkdirSync(outdir, { recursive: true });
const browser = await chromium.launch({
  executablePath: EXE,
  args: ['--no-sandbox', '--disable-gpu', '--force-color-profile=srgb', '--hide-scrollbars',
         '--font-render-hinting=none', '--disable-lcd-text'],
});
/* 分辨率从 MV_W / MV_H 取（默认 1920x1080，和 reference 截图 1:1） */
const VW = Number(process.env.MV_W || 1920), VH = Number(process.env.MV_H || 1080);
const page = await browser.newPage({ viewport: { width: VW, height: VH }, deviceScaleFactor: 1 });
page.on('pageerror', e => { console.error('PAGEERROR', e.message); });
// 注意: file:// 下 Chromium 不允许 fetch() 本地 json (assets.json 会 "Failed to fetch"),
// 所以默认走本地 http 服务; 见 scripts/20_render.sh。
const URL_BASE = process.env.MV_URL || 'file://' + path.join(HERE, 'mv.html');
await page.goto(URL_BASE);
await page.waitForFunction(() => window.mvReady === true, null, { timeout: 60000 });

const total = Math.max(1, Math.round((t1 - t0) * fps));
const t_start = Date.now();
for (let i = 0; i < total; i++) {
  const t = t0 + i / fps;
  await page.evaluate((tt) => window.renderFrame(tt), t);
  const idx = String(i).padStart(5, '0');
  const name = PNG ? idx + '_t' + t.toFixed(2) + '.png' : idx + '.jpg';
  const p = path.join(outdir, name);
  await page.screenshot(PNG ? { path: p } : { path: p, type: 'jpeg', quality });
  if (i % 60 === 0) {
    const el = (Date.now() - t_start) / 1000;
    console.log('  ' + i + '/' + total + '  t=' + t.toFixed(2) + 's  ' + el.toFixed(1) + 's  ' + (i / Math.max(el, .001)).toFixed(1) + ' fps');
  }
}
console.log('done', total, 'frames ->', outdir);
await browser.close();
