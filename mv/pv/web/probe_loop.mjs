// 独立探针: 确认帧号公式 = 任务规定, 且长句尾部走【循环段】而非冻帧。
// renderFrame(t): t<5 冷开场; 否则 renderMain(t-LYOFF), LYOFF=4.99。
// 期望值用页面同一套数据(web/assets.json 的 lyrics[].t0)独立算一遍。
import { chromium } from 'playwright-core';
import os from 'os';
import fs from 'fs';
const EXE = os.homedir() + '/.cache/ms-playwright/chromium-1148/chrome-linux/chrome';
const A = JSON.parse(fs.readFileSync(new URL('./assets.json', import.meta.url), 'utf8'));
const LY = A.lyrics, LYOFF = 4.99;
function lineT0(tm) { let k = LY[0].t0; for (const l of LY) if (tm >= l.t0) k = l.t0; return k; }
function expect(t) {
  const tm = Math.max(0, t - LYOFF);
  let i = 12 + Math.floor((tm - lineT0(tm)) * 60); if (i < 12) i = 12;
  const raw = i, loop = i > 900;
  if (loop) i = 300 + ((i - 300) % 600);
  return { i, raw, loop, tm, t0: lineT0(tm) };
}
const b = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox','--disable-gpu'] });
const p = await b.newPage({ viewport: { width: 1920, height: 1080 } });
p.on('pageerror', e => console.log('PAGEERROR:', e.message));
await p.goto('http://127.0.0.1:8901/web/mv.html');
await p.waitForTimeout(2000);
let bad = 0;
for (const t of process.argv.slice(2).map(Number)) {
  const err = await p.evaluate(async (tt) => { try { await window.renderFrame(tt); return null; } catch (e) { return e.message; } }, t);
  await p.waitForTimeout(500);
  const r = await p.evaluate(() => {
    const pick = (id) => { const e = document.getElementById(id); if (!e) return null;
      return { src: e.getAttribute('src') || '', op: getComputedStyle(e).opacity }; };
    const a = pick('cv'), c = pick('cv2');
    return (c && c.op === '1') ? c : a;
  });
  const src = r ? r.src : '';
  const m = /lyric_frames\/([a-z0-9]+)\/(\d{3})\.jpg/.exec(src);
  const name = m ? m[1] : '?', frame = m ? parseInt(m[2], 10) : -1;
  const e = expect(t);
  const ok = frame === e.i;
  if (!ok) bad++;
  console.log('renderFrame(' + t + ') t_main=' + e.tm.toFixed(2) + ' line.t0=' + e.t0 +
    ' -> ' + name + '/' + String(frame).padStart(3, '0') +
    '  期望#' + e.i + (e.loop ? ' [LOOP 300..900, 原始 i=' + e.raw + ']' : ' [HEAD]') + '  ' + (ok ? 'OK' : 'MISMATCH') +
    (err ? '  ERR:' + err : ''));
}
console.log(bad === 0 ? 'ALL-OK' : ('FAILED ' + bad));
await b.close();