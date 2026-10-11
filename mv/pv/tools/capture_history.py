#!/usr/bin/env python3
"""capture_history.py — 录历史提交的真实画面。

用法: python3 capture_history.py <二进制路径> <输出名> <动作> [秒数]
动作: still | drag_slider | idle | hover
历史提交在 /tmp/fxtk-hist-* 的独立 worktree 里编译 (不动工作分支)。
"""
from __future__ import annotations
import sys, time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from fxtk_studio import Studio, ROOT

OUT = ROOT / "mv" / "pv" / "footage" / "hist"
W, H = 1280, 720


def main(binary, name, action, seconds=8.0, size="480x272"):
    global W, H
    W, H = (int(x) for x in size.split("x"))
    OUT.mkdir(parents=True, exist_ok=True)
    with Studio(":99", (W, H)) as st:
        st.start_app(Path(binary), name=name)
        st.grab(OUT / f"{name}.still.png")
        if action == "still":
            print("still ->", OUT / f"{name}.still.png"); return
        p = OUT / f"{name}.mp4"
        st.rec_start(p, fps=30, mouse=(action == "drag_slider"))
        if action == "drag_slider":
            # 页面0 的滑杆在【设计坐标】pixel("60,194","292,212") 上;
            # 480x272 的老构建没有 UI 缩放, 所以直接用设计坐标: y=203, x 100..250
            y = 229; x0, x1 = 139, 330
            st.drag(x0, y, x1, y, steps=40, hold=0.4)
            time.sleep(1.0)
            st.drag(x1, y, x0 + 20, y, steps=40, hold=0.4)
            time.sleep(max(0.5, float(seconds) - 5))
        elif action == "hover":
            for i in range(6):
                st.move(200 + i * 150, 400, dur=0.6)
        else:
            time.sleep(float(seconds))
        st.rec_stop()
        print("clip ->", p)


if __name__ == "__main__":
    main(*sys.argv[1:])
