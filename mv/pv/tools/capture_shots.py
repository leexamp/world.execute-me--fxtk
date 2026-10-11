#!/usr/bin/env python3
"""capture_shots.py — 用真实 fxtk 窗口录制"动起来"的素材。

每个镜头 = 一次真实的窗口会话 (Xephyr + sokol/GL + xdotool 真实 X 事件) + ffmpeg x11grab。
产物是 mp4 (H.264, 30fps, 1280x720)。没有一帧是补出来的。

用法:
  python3 capture_shots.py [镜头名 ...]     # 不给名字就跑全部
  python3 capture_shots.py --list
"""
from __future__ import annotations
import sys, time, json
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import numpy as np
from PIL import Image
from fxtk_studio import Studio, tab_xy, ui_scale, ROOT

OUT = ROOT / "mv" / "pv" / "footage"
APP = ROOT / "demo-main" / "fxtk_sim"
W, H = 1280, 720
PAGES = ["波形", "图形", "控件", "图片", "3D", "输入", "画板", "键鼠", "压测", "滚动", "组件", "粒子"]


def sgn(i):
    return tab_xy(i, 12, W, H)


def goto(st, i, pause=0.5):
    st.click(*sgn(i), pause=pause)


def clip(name, st, seconds, action, mouse=False, warm=0.0):
    p = OUT / f"{name}.mp4"
    st.grab(OUT / f"{name}.still.png")          # 每个镜头留一张静帧 (HTML 里当封面用)
    st.rec_start(p, fps=30, mouse=mouse)
    if warm:
        time.sleep(warm)
    action()
    st.rec_stop()
    print("  ->", p.name)


def main(names):
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "pages.json").write_text(json.dumps(PAGES, ensure_ascii=False), encoding="utf-8")
    todo = names or ["page_wave", "page_gfx", "page_3d", "page_particles", "page_stress",
                     "page_scroll", "page_input", "page_paint", "page_components", "ui_hover"]
    with Studio(":99", (W, H)) as st:
        for name in todo:
            print("=== 录制", name)
            st.start_app(APP, name=name)
            try:
                if name == "page_wave":
                    goto(st, 0); clip(name, st, 8, lambda: time.sleep(8))
                elif name == "page_gfx":
                    goto(st, 1); clip(name, st, 8, lambda: (st.drag(760, 560, 980, 560, steps=30), time.sleep(4)))
                elif name == "page_3d":
                    goto(st, 4); clip(name, st, 10, lambda: time.sleep(10))
                elif name == "page_particles":
                    goto(st, 11); clip(name, st, 10, lambda: (st.drag(380, 590, 1000, 590, steps=30), time.sleep(6)))
                elif name == "page_stress":
                    goto(st, 8); clip(name, st, 10, lambda: time.sleep(10))
                elif name == "page_scroll":
                    goto(st, 9); clip(name, st, 8, lambda: (st.move(640, 400), st._xd("click", 5) or None, time.sleep(1),
                                                           [ (st._xd("click", 5), time.sleep(0.4)) for _ in range(4) ], time.sleep(3)))
                elif name == "page_input":
                    goto(st, 5); clip(name, st, 8, lambda: (st.click(700, 300), st.type("hello fxtk 2026", delay=90), time.sleep(3)))
                elif name == "page_paint":
                    goto(st, 6)
                    def paint():
                        st.drag(200, 220, 900, 480, steps=60, hold=0.1)
                        st.drag(900, 220, 300, 520, steps=60, hold=0.1)
                        time.sleep(2)
                    clip(name, st, 8, paint)
                elif name == "page_components":
                    goto(st, 10)
                    def comps():
                        for x, y in [(120, 180), (300, 300), (500, 420), (700, 300), (900, 200)]:
                            st.move(x, y, dur=0.5)
                        st.click(200, 600, pause=0.6); time.sleep(2)
                    clip(name, st, 9, comps, mouse=True)
                elif name == "ui_hover":
                    goto(st, 2)
                    def hov():
                        for i in range(9):
                            st.move(200 + (i % 3) * 150, 200 + (i // 3) * 120, dur=0.45)
                        time.sleep(1)
                    clip(name, st, 8, hov, mouse=True)
                else:
                    print("  !! 未知镜头", name)
            finally:
                st.stop_app()
    print("完成")


if __name__ == "__main__":
    if "--list" in sys.argv:
        print("page_wave page_gfx page_3d page_particles page_stress page_scroll page_input page_paint page_components ui_hover")
    else:
        main([a for a in sys.argv[1:] if not a.startswith("-")])
