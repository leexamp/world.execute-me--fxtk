#!/usr/bin/env python3
"""03_assets.py — 把素材清单编成 web/assets.json (HTML 只读这一个文件)。"""
from __future__ import annotations
import json
from pathlib import Path

PV = Path(__file__).resolve().parents[1]
ROOT = PV.parents[1]
WEB = PV / "web"
FD = PV / "footage"
FORE = PV / "forensics"

clips, stills = {}, {}
for d in sorted((FD / "frames").glob("*")):
    if not d.is_dir():
        continue
    n = len(list(d.glob("*.jpg")))
    if n:
        clips[d.name] = {"dir": f"../footage/frames/{d.name}", "n": n}
for p in sorted(FD.glob("*.still.png")):
    stills[p.stem.replace(".still", "")] = f"../footage/{p.name}"
for p in sorted((FD / "hist").glob("*.still.png")):
    stills["hist/" + p.stem.replace(".still", "")] = f"../footage/hist/{p.name}"
for p in sorted((FD / "res").glob("*.still.png")):
    stills["res/" + p.stem.replace(".still", "")] = f"../footage/res/{p.name}"

lyrics = json.loads((PV / "lyrics" / "timeline.json").read_text(encoding="utf-8"))
beats = {}
bp = PV / "lyrics" / "beats.json"
if bp.exists():
    beats = json.loads(bp.read_text(encoding="utf-8"))
fore = {p.stem: p.read_text(encoding="utf-8", errors="replace") for p in sorted(FORE.glob("*.txt"))}
char = {p.stem: f"../assets/char/{p.name}" for p in sorted((PV / "assets" / "char").glob("*"))
        if p.suffix.lower() in (".jpg", ".jpeg", ".png")}

out = {"fps": 30, "duration": lyrics["duration"], "clips": clips, "stills": stills,
       "lyrics": lyrics["lines"], "forensics": fore, "char": char, "beats": beats}
(WEB / "assets.json").write_text(json.dumps(out, ensure_ascii=False), encoding="utf-8")
print(f"clips={len(clips)} stills={len(stills)} lyrics={len(out['lyrics'])} forensics={len(fore)} char={len(char)}")
print("clips:", " ".join(f"{k}({v['n']})" for k, v in clips.items()))
print("stills:", " ".join(stills))
print("char:", " ".join(char))
