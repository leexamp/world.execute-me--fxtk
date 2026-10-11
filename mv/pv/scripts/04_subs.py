#!/usr/bin/env python3
"""04_subs.py — 从对齐后的时间轴导出 SRT / ASS 字幕 (给需要单独字幕轨的人)。"""
from __future__ import annotations
import json
from pathlib import Path

PV = Path(__file__).resolve().parents[1]
d = json.loads((PV / "lyrics" / "timeline.json").read_text(encoding="utf-8"))


def ts(t):
    h = int(t // 3600); m = int(t % 3600 // 60); s = t % 60
    return f"{h:02d}:{m:02d}:{s:06.3f}".replace(".", ",")


out = []
for i, l in enumerate(d["lines"], 1):
    out.append(f"{i}\n{ts(l['t0'])} --> {ts(min(l['t1'], l['t0']+6))}\n{l['text']}\n")
(PV / "out").mkdir(exist_ok=True)
(PV / "out" / "world_execute_me.srt").write_text("\n".join(out), encoding="utf-8")
print("SRT:", len(out), "行 ->", PV / "out" / "world_execute_me.srt")
