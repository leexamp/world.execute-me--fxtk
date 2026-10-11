#!/usr/bin/env python3
"""fxtk_studio.py — 在嵌套 X (Xephyr) 里驱动真实 fxtk 窗口并录制画面。

【为什么要这层】
  直接录 DISPLAY=:1 会录到用户桌面: 窗口位置/装饰/分辨率都不可控, 还可能录进别人的窗口。
  Xephyr 提供一个 1280x720 的干净 X 服务器; 无窗口管理器 => 窗口 1:1 铺满屏幕, 没有装饰。
  于是"录到的像素"就是"应用画的像素"。

【诚实性】
  这里录到的每一帧都来自真实 fxtk 渲染管线 (sokol/GL 或 SDL 驱动) 的真实输出。
  没有任何一帧是后来画的/伪造的。xdotool 送的是真实 X 事件。
"""
from __future__ import annotations
import os, re, signal, subprocess, time, shutil, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]          # fxtk-pc
PV = ROOT / "mv" / "pv"


class Studio:
    def __init__(self, display=":99", size=(1280, 720), workdir=None, logdir=None):
        self.display = display
        self.w, self.h = size
        self.workdir = Path(workdir) if workdir else (PV / "build" / "studio")
        self.workdir.mkdir(parents=True, exist_ok=True)
        self.logdir = Path(logdir) if logdir else (PV / "build" / "logs")
        self.logdir.mkdir(parents=True, exist_ok=True)
        self.xephyr = None
        self.app = None
        self.app_name = None

    # ---------- X 服务器 ----------
    def env(self):
        e = dict(os.environ)
        e["DISPLAY"] = self.display
        return e

    def start_x(self):
        # 【坑】如果同一个 display 上还有上一台 Xephyr 活着, 新的一定起不来(锁文件),
        # 而 xdpyinfo 会连上"老的"那台 —— 于是你以为在 480x272 上录, 实际录的是 1280x720
        # 屏幕正中央裁下来的一块。这次真踩到了, 所以这里必须杀掉同名 display 上的残留。
        self._kill_display()
        d = self.display.lstrip(":")
        for p in Path("/tmp").glob(f".X{d}-lock"):
            try:
                p.unlink()
            except OSError:
                pass
        log = open(self.logdir / "xephyr.log", "w")
        self.xephyr = subprocess.Popen(
            ["Xephyr", self.display, "-screen", f"{self.w}x{self.h}", "-no-host-grab", "-br"],
            stdin=subprocess.DEVNULL, stdout=log, stderr=log, start_new_session=True)
        for _ in range(80):
            out = subprocess.run(["xdpyinfo"], env=self.env(), capture_output=True, text=True)
            if out.returncode == 0:
                # 必须核对尺寸: 连上了但尺寸不对, 说明连的是上一次那台
                m = re.search(r"dimensions:\s+(\d+)x(\d+)", out.stdout)
                if m and (int(m.group(1)), int(m.group(2))) == (self.w, self.h):
                    return
                if m:
                    raise RuntimeError(f"display {self.display} 尺寸是 {m.group(1)}x{m.group(2)}, 不是 {self.w}x{self.h}")
            time.sleep(0.1)
        raise RuntimeError("Xephyr 起不来")

    def _kill_display(self):
        try:
            out = subprocess.run(["pgrep", "-a", "Xephyr"], capture_output=True, text=True).stdout
        except Exception:
            return
        for line in out.splitlines():
            if re.search(r"Xephyr\s+" + re.escape(self.display) + r"\b", line):
                try:
                    os.kill(int(line.split()[0]), signal.SIGTERM)
                except Exception:
                    pass
        time.sleep(0.5)

    def start_app(self, binary, extra_env=None, args=(), name=None):
        env = self.env()
        env.update(extra_env or {})
        log = open(self.logdir / f"app-{name or 'app'}.log", "w")
        self.app = subprocess.Popen([str(binary), *args], cwd=str(Path(binary).parent),
                                    env=env, stdin=subprocess.DEVNULL,
                                    stdout=log, stderr=log, start_new_session=True)
        self.app_name = name or str(binary)
        self.wait_window()
        return self.app

    def wait_window(self, timeout=20.0):
        t0 = time.time()
        while time.time() - t0 < timeout:
            out = subprocess.run(["xdotool", "search", "--onlyvisible", "--name", "."],
                                 env=self.env(), capture_output=True, text=True).stdout.split()
            if out:
                time.sleep(0.8)      # 等首帧画完
                return out[-1]
            time.sleep(0.2)
        raise RuntimeError("应用窗口没出现")

    # ---------- 输入 ----------
    def _xd(self, *args):
        subprocess.run(["xdotool", *[str(a) for a in args]], env=self.env(),
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)

    def move(self, x, y, dur=0.0):
        self._xd("mousemove", "--sync", x, y)
        if dur: time.sleep(dur)

    def click(self, x, y, button=1, pause=0.25):
        # 先动到别处再移过来: 某些控件只认真实的位置变化事件
        self._xd("mousemove", "--sync", max(0, x - 120), max(0, y + 90))
        time.sleep(0.06)
        self._xd("mousemove", "--sync", x, y)
        time.sleep(0.08)
        self._xd("mousedown", button); time.sleep(0.05)
        self._xd("mouseup", button); time.sleep(pause)

    def key(self, k, pause=0.15):
        self._xd("key", "--clearmodifiers", k); time.sleep(pause)

    def type(self, text, delay=40, pause=0.2):
        self._xd("type", "--delay", delay, text); time.sleep(pause)

    def drag(self, x0, y0, x1, y1, steps=24, hold=0.30, button=1):
        self._xd("mousemove", "--sync", x0, y0); time.sleep(0.2)
        self._xd("mousedown", button); time.sleep(hold)
        for i in range(1, steps + 1):
            self._xd("mousemove", "--sync",
                     round(x0 + (x1 - x0) * i / steps), round(y0 + (y1 - y0) * i / steps))
            time.sleep(0.02)
        time.sleep(hold)
        self._xd("mouseup", button); time.sleep(0.15)

    def resize(self, w, h):
        wid = self.wait_window()
        self._xd("windowsize", wid, w, h)
        time.sleep(0.6)
        self.w, self.h = w, h

    # ---------- 录制 ----------
    def _grab_cmd(self, out, seconds=None, frames=None, fps=30, mouse=False, crf=16):
        cmd = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-y",
               "-f", "x11grab", "-framerate", str(fps),
               "-video_size", f"{self.w}x{self.h}", "-draw_mouse", "1" if mouse else "0",
               "-i", self.display]
        if seconds is not None:
            cmd += ["-t", f"{seconds:.3f}"]
        if frames is not None:
            cmd += ["-frames:v", str(frames)]
        if str(out).endswith(".png"):
            cmd += ["-frames:v", "1", str(out)]
        else:
            cmd += ["-c:v", "libx264", "-preset", "veryfast", "-crf", str(crf),
                    "-pix_fmt", "yuv420p", str(out)]
        return cmd

    def grab(self, out, mouse=False):
        out = str(out); Path(out).parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(self._grab_cmd(out, frames=1, mouse=mouse), check=True)

    def record(self, out, seconds, fps=30, mouse=False, crf=16):
        out = str(out); Path(out).parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(self._grab_cmd(out, seconds=seconds, fps=fps, mouse=mouse, crf=crf), check=True)
        return out

    def rec_start(self, out, fps=30, mouse=False, crf=14):
        """开始后台录制 (返回 Popen); 用 rec_stop 收尾。"""
        out = str(out); Path(out).parent.mkdir(parents=True, exist_ok=True)
        self._rec = subprocess.Popen(self._grab_cmd(out, fps=fps, mouse=mouse, crf=crf),
                                     stdin=subprocess.DEVNULL,
                                     stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        time.sleep(0.6)          # 等 x11grab 真正连上
        return self._rec

    def rec_stop(self):
        p = getattr(self, "_rec", None)
        if not p:
            return
        time.sleep(0.3)
        p.send_signal(signal.SIGINT)
        try:
            p.wait(timeout=10)
        except subprocess.TimeoutExpired:
            p.kill(); p.wait()
        self._rec = None

    # ---------- 清理 ----------
    def stop_app(self):
        if self.app and self.app.poll() is None:
            try: os.killpg(os.getpgid(self.app.pid), signal.SIGTERM)
            except ProcessLookupError: pass
            try: self.app.wait(timeout=5)
            except subprocess.TimeoutExpired:
                try: os.killpg(os.getpgid(self.app.pid), signal.SIGKILL)
                except ProcessLookupError: pass
        self.app = None

    def close(self):
        self.stop_app()
        if self.xephyr and self.xephyr.poll() is None:
            try: os.killpg(os.getpgid(self.xephyr.pid), signal.SIGTERM)
            except ProcessLookupError: pass
            try: self.xephyr.wait(timeout=5)
            except subprocess.TimeoutExpired:
                try: os.killpg(os.getpgid(self.xephyr.pid), signal.SIGKILL)
                except ProcessLookupError: pass
        self.xephyr = None

    def __enter__(self):
        self.start_x(); return self

    def __exit__(self, *a):
        self.close()


def ui_scale(w=1280, h=720):
    """432x272 设计画布 → 窗口的缩放。实测 1280x720 下 = 2.6 (页面截图量的)。"""
    if (w, h) == (1280, 720):
        return 2.6
    return min(w / 480.0, h / 272.0)


def tab_xy(i, n=12, w=1280, h=720):
    """fx_tab_new(title("波形,图形,...")) 的 12 个页签中心 (窗口坐标)。

    实测依据: 设计坐标 x∈[10,470] 的页签条在 1280x720 上落在 x∈[26,1222], 即 s=2.6;
    页签行中心 y=81。校准过程见 build/pages/test_*.png。
    """
    s = ui_scale(w, h)
    return round(s * (10 + 460 * (i + 0.5) / n)), round(s * 31)

