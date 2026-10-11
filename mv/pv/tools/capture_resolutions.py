#!/usr/bin/env python3
"""capture_resolutions.py — "同一个窗口, 六种分辨率": 用真实窗口尺寸变化录。

做法: 在一台 2560x1440 的 Xephyr 上跑真实的 fxtk 窗口, 用 xdotool 改窗口大小,
每次抓窗口所在区域(0,0)。窗口内容的缩放/重排是 fxtk 自己做的 —— 不是我们缩的图。
"""
from __future__ import annotations
import sys, time, subprocess
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from fxtk_studio import Studio, ROOT

OUT = ROOT / "mv" / "pv" / "footage" / "res"
SIZES = [(480, 272), (800, 480), (1280, 720), (1920, 1080), (2560, 1440), (720, 1280)]


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    with Studio(":99", (2560, 1440)) as st:
        st.start_app(ROOT / "demo-main" / "fxtk_sim", name="resolutions")
        wid = st.wait_window()
        for (w, h) in SIZES:
            st._xd("windowsize", wid, w, h)
            time.sleep(1.2)
            # 从 (0,0) 抓 w x h —— 没有窗口管理器, 窗口就贴在原点
            cmd = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-f", "x11grab",
                   "-video_size", f"{w}x{h}", "-i", f"{st.display}+0,0"]
            if w * h > 1920 * 1080:
                cmd += ["-frames:v", "1", str(OUT / f"{w}x{h}.png")]
            else:
                cmd += ["-t", "1.2", "-r", "30", "-c:v", "libx264", "-preset", "veryfast", "-crf", "16",
                        "-pix_fmt", "yuv420p", str(OUT / f"{w}x{h}.mp4")]
            subprocess.run(cmd, check=True)
            print("captured", w, h, "->", (OUT / f"{w}x{h}.mp4").exists() or (OUT / f"{w}x{h}.png").exists())
        st.grab(OUT / "window_now.png")
    print("done")


if __name__ == "__main__":
    main()
