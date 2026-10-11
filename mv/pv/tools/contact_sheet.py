#!/usr/bin/env python3
"""contact_sheet.py — 把 12 个演示页各抓一张真图, 拼成一张联络表(用于选镜头)。

用法: python3 contact_sheet.py [宽 高] [步进]
"""
import sys, subprocess
from pathlib import Path
from PIL import Image, ImageDraw
sys.path.insert(0, str(Path(__file__).parent))
from fxtk_studio import Studio, tab_xy, ROOT

W = int(sys.argv[1]) if len(sys.argv) > 1 else 1280
H = int(sys.argv[2]) if len(sys.argv) > 2 else 720
OUT = ROOT / "mv" / "pv" / "build" / "pages" / f"{W}x{H}"
OUT.mkdir(parents=True, exist_ok=True)
PAGES = ["波形", "图形", "控件", "图片", "3D", "输入", "画板", "键鼠", "压测", "滚动", "组件", "粒子"]

with Studio(":99", (W, H)) as st:
    st.start_app(ROOT / "demo-main" / "fxtk_sim", name="contact")
    shots = []
    for i, nm in enumerate(PAGES):
        x, y = tab_xy(i)
        if W != 1280:
            x = round(10 + (470 - 10) * (i + 0.5) / 12 * W / 1280)
        st.click(x, y, pause=0.5)
        p = OUT / f"{i:02d}_{nm}.png"
        st.grab(p)
        shots.append((nm, p))
        print("captured", nm, p)
    st.stop_app()

cols, rows = 3, 4
tw, th = W // 3, H // 3
sheet = Image.new("RGB", (cols * tw, rows * th), "black")
d = ImageDraw.Draw(sheet)
for i, (nm, p) in enumerate(shots):
    im = Image.open(p).resize((tw, th))
    sheet.paste(im, ((i % cols) * tw, (i // cols) * th))
    d.text(((i % cols) * tw + 8, (i // cols) * th + 8), nm, fill=(255, 80, 80))
sheet.save(ROOT / "mv" / "pv" / "build" / "pages" / f"sheet_{W}x{H}.png")
print("sheet:", ROOT / "mv" / "pv" / "build" / "pages" / f"sheet_{W}x{H}.png")
