#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
24_cover.py — MV 封面重制（第二版：安静 / 暗 / 悬念）

底图 = 真实渲染帧 mv/pv/footage/lyric_frames/newworld/850.jpg
       （neworld 场景第 850 帧, 60fps -> t = 849/60 = 14.150s）
只做: 裁剪 / 压暗 / 调色 / 辉光 / 叠加极小文字。
不伪造、不重绘窗口内部画面；绝不出现 "fxtk"。

产出 mv/pv/out/cover.jpg(1920x1080) / cover_1280.jpg(1280x720) / cover_thumb.jpg(480x270)
"""
import os, hashlib
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT   = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
FRAME  = os.path.join(ROOT, 'footage/lyric_frames/newworld/850.jpg')
WHALE  = os.path.join(ROOT, 'assets/char/maid_whale.png')
OUT    = os.path.join(ROOT, 'out')
FONT_MONO = '/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'
FONT_CJK  = '/usr/share/fonts/opentype/noto/NotoSansCJK-Light.ttc'

W, H = 1920, 1080
# ---- 屏幕(亮窗): 只取矩形边框以内、去掉标题栏与状态栏 --------------------
CROP = (60, 62, 843, 495)          # 960x540 帧内坐标 (l,t,r,b), 含边框+30px 内边距
WIN_W, WIN_H = 780, 431            # 缩放后尺寸
WIN_L, WIN_TOP = 990, 170          # 左上角
FEATHER = 18

# ---- 鲸鱼娘剪影 ----------------------------------------------------------
WHALE_H = 648
WHALE_CX = 575
WHALE_BOTTOM = 1080

def gblur(a, r):
    if r <= 0: return a
    im = Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), 'L')
    return np.asarray(im.filter(ImageFilter.GaussianBlur(r)), dtype=np.float32)

def shift(a, dx, dy):
    out = np.zeros_like(a)
    h, w = a.shape
    xs0, xs1 = max(0, dx), min(w, w + dx)
    ys0, ys1 = max(0, dy), min(h, h + dy)
    xd0, xd1 = max(0, -dx), min(w, w - dx)
    yd0, yd1 = max(0, -dy), min(h, h - dy)
    out[yd0:yd1, xd0:xd1] = a[ys0:ys1, xs0:xs1]
    return out

def key_whale(path, target_h):
    """白底抠图: 近白候选 -> 从四边连通淹没 -> 其余为前景。返回 (RGB float, alpha float 0..1)。"""
    im = Image.open(path).convert('RGB')
    w, h = im.size
    nw = max(1, round(w * target_h / h))
    im = im.resize((nw, target_h), Image.LANCZOS)
    a = np.asarray(im).astype(np.int16)
    cand = a.min(2) >= 235
    reach = np.zeros_like(cand)
    reach[0, :] = cand[0, :]; reach[-1, :] = cand[-1, :]
    reach[:, 0] = cand[:, 0]; reach[:, -1] = cand[:, -1]
    while True:
        new = reach.copy()
        new[1:, :] |= reach[:-1, :]; new[:-1, :] |= reach[1:, :]
        new[:, 1:] |= reach[:, :-1]; new[:, :-1] |= reach[:, 1:]
        new &= cand
        if new.sum() == reach.sum(): break
        reach = new
    alpha = (~reach).astype(np.float32)
    # 轻微羽化边缘
    alpha = gblur(alpha * 255.0, 0.8) / 255.0
    return np.asarray(im).astype(np.float32), alpha

def draw_text_layer(size, items):
    """items: list of dict(text,x,y,font,size,fill,alpha,spacing)"""
    lay = Image.new('RGBA', size, (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    for it in items:
        f = ImageFont.truetype(it['font'], it['size'], index=it.get('index', 0))
        x, y = it['x'], it['y']
        col = it['fill'] + (it['alpha'],)
        sp = it.get('spacing', 0)
        if sp:
            for ch in it['text']:
                d.text((x, y), ch, font=f, fill=col)
                x += d.textlength(ch, font=f) + sp
        else:
            d.text((x, y), it['text'], font=f, fill=col)
    return np.asarray(lay).astype(np.float32)

def main():
    src_md5 = hashlib.md5(open(FRAME, 'rb').read()).hexdigest()

    # ---------- 1. 底图 = 真实帧的裁剪 ----------
    frame = Image.open(FRAME).convert('RGB')
    wi = frame.crop(CROP).resize((WIN_W, WIN_H), Image.LANCZOS)
    win = np.asarray(wi).astype(np.float32)
    win = np.clip(win * 1.28, 0, 255)                 # 提亮, 让点阵与边框成为唯一光源

    # ---------- 2. 极暗底 ----------
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    bg = np.zeros((H, W, 3), np.float32)
    bg[..., 0] = 2.0; bg[..., 1] = 3.0; bg[..., 2] = 6.0

    # ---------- 3. 贴亮窗 ----------
    top, left = WIN_TOP, WIN_L
    x = np.arange(WIN_W); y = np.arange(WIN_H)
    mx = np.clip(np.minimum(x, WIN_W - 1 - x) / FEATHER, 0, 1); mx = mx * mx * (3 - 2 * mx)
    my = np.clip(np.minimum(y, WIN_H - 1 - y) / FEATHER, 0, 1); my = my * my * (3 - 2 * my)
    mask = np.outer(my, mx)[..., None]
    # 四角轻微收敛, 屏幕像被黑暗吞掉一部分
    cx, cy = WIN_W * 0.5, WIN_H * 0.5
    rr = (((xx[top:top+WIN_H, left:left+WIN_W] - left - cx) / cx) ** 2 +
          ((yy[top:top+WIN_H, left:left+WIN_W] - top - cy) / cy) ** 2)
    dim = np.clip(1.10 - 0.42 * rr, 0.45, 1.0)[..., None]
    win_c = win * dim
    region = bg[top:top+WIN_H, left:left+WIN_W]
    region[:] = region * (1 - mask) + win_c * mask

    # ---------- 4. 辉光: 亮窗是唯一光源, 向四周溢出 ----------
    lum = np.zeros((H, W), np.float32)
    lum[top:top+WIN_H, left:left+WIN_W] = win_c.max(2) * mask[..., 0]
    bp = np.clip(lum - 38.0, 0, 255)
    g_t = gblur(bp, 20); g_m = gblur(bp, 72); g_w = gblur(bp, 210)
    bg += g_t[..., None] * np.array([0.52, 0.86, 1.00], np.float32)
    bg += g_m[..., None] * np.array([0.24, 0.50, 0.82], np.float32) * 0.85
    bg += g_w[..., None] * np.array([0.10, 0.22, 0.44], np.float32) * 0.90

    # ---------- 5. 鲸鱼娘: 白底抠图 -> 极暗剪影 + 背光轮廓 ----------
    wrgb, walpha = key_whale(WHALE, WHALE_H)
    wh, ww = walpha.shape
    wl = int(WHALE_CX - ww / 2); wt = int(WHALE_BOTTOM - wh)
    # 她身后的方向性包裹光(光在右上), 先加到底上, 再压剪影
    wrap = gblur(walpha * 255.0, 46) / 255.0
    grad = (xx / W) * 0.65 + ((H - yy) / H) * 0.35
    wrap = wrap * np.clip(grad[wt:wt+wh, wl:wl+ww] * 1.4 - 0.15, 0, 1) * 0.10
    sub = bg[wt:wt+wh, wl:wl+ww]
    sub += wrap[..., None] * np.array([0.45, 0.78, 1.00], np.float32)
    # 剪影本体: 由原图亮度决定极暗的层次
    pl = wrgb.mean(2)
    val = 3.0 + (pl / 255.0) ** 1.40 * 19.0
    body = np.stack([val * 0.60, val * 0.78, val * 1.00], -1)
    # 她左边的黑暗吞掉她, 只有靠近光的一侧浮出
    gx = np.clip((np.arange(ww)[None, :] / ww) * 1.25 - 0.18, 0, 1).astype(np.float32)
    gy = np.clip(1.0 - np.clip((np.arange(wh)[:, None] / wh - 0.62) / 0.42, 0, 1) * 0.80, 0.20, 1.0).astype(np.float32)
    body *= (0.30 + 0.70 * gx)[..., None] * gy[..., None]
    # 右上轮廓光(唯一的受光面)
    rim = np.clip(walpha - shift(walpha, 8, -8), 0, 1)
    rim = gblur(rim * 255.0, 1.5) / 255.0
    body += rim[..., None] * np.array([0.55, 0.86, 1.00], np.float32) * 92.0
    rim2 = np.clip(walpha - shift(walpha, 20, -20), 0, 1)
    rim2 = gblur(rim2 * 255.0, 7.0) / 255.0
    body += rim2[..., None] * np.array([0.40, 0.66, 0.95], np.float32) * 26.0
    al = walpha[..., None]
    sub[:] = sub * (1 - al) + body * al

    # ---------- 6. 极小的文字: 一行代码 + 一行中文 ----------
    txt = draw_text_layer((W, H), [
        dict(text='world.execute(me);', x=1002, y=626, font=FONT_MONO, size=31,
             fill=(146, 198, 222), alpha=150),
        dict(text='\u5979\u5b9e\u73b0\u4e86\u81ea\u5df1\u7684\u4e16\u754c', x=1004, y=678,
             font=FONT_CJK, size=25, index=0, fill=(126, 146, 163), alpha=125, spacing=5),
    ])
    # 光标: 屏幕自己在呼吸
    cur = draw_text_layer((W, H), [
        dict(text='_', x=1300, y=622, font=FONT_MONO, size=34, fill=(196, 238, 255), alpha=205),
    ])
    bg = bg * (1 - txt[..., 3:] / 255.0) + txt[..., :3] * (txt[..., 3:] / 255.0)
    bg = bg * (1 - cur[..., 3:] / 255.0) + cur[..., :3] * (cur[..., 3:] / 255.0)

    # ---------- 7. 尘埃与暗角 ----------
    rng = np.random.default_rng(20261011)
    dust = np.zeros((H, W), np.float32)
    for _ in range(190):
        px = int(rng.normal(1330, 420)); py = int(rng.normal(430, 330))
        if not (0 <= px < W and 0 <= py < H): continue
        dust[py, px] = rng.uniform(40, 200)
    dust = gblur(dust, 1.1)
    bg += dust[..., None] * np.array([0.55, 0.85, 1.00], np.float32)

    vx, vy = 0.64, 0.42
    vig = 1.0 - 0.55 * np.clip((((xx / W - vx) ** 2 + (yy / H - vy) ** 2) * 2.4), 0, 1)
    bg *= vig[..., None]

    # ---------- 8. 颗粒 + 收尾 ----------
    bg += rng.normal(0, 1.7, (H, W, 1)).astype(np.float32)
    img = Image.fromarray(np.clip(bg, 0, 255).astype(np.uint8), 'RGB')

    os.makedirs(OUT, exist_ok=True)
    paths = []
    img.save(os.path.join(OUT, 'cover.jpg'), quality=93, subsampling=0, optimize=True)
    paths.append('cover.jpg')
    img.resize((1280, 720), Image.LANCZOS).save(os.path.join(OUT, 'cover_1280.jpg'),
                                                quality=93, subsampling=0, optimize=True)
    paths.append('cover_1280.jpg')
    img.resize((480, 270), Image.LANCZOS).save(os.path.join(OUT, 'cover_thumb.jpg'),
                                               quality=91, subsampling=0, optimize=True)
    paths.append('cover_thumb.jpg')

    print('src frame :', FRAME)
    print('src md5   :', src_md5)
    print('crop      :', CROP, '->', (WIN_W, WIN_H))
    for p in paths:
        fp = os.path.join(OUT, p)
        with Image.open(fp) as im:
            print('%-16s %dx%d  %d bytes  md5=%s' % (p, im.width, im.height,
                  os.path.getsize(fp), hashlib.md5(open(fp, 'rb').read()).hexdigest()))

if __name__ == '__main__':
    main()
