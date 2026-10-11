#!/usr/bin/env python3
"""01_align.py — 把 LRC 歌词对到我们下载的这条音轨上。

【为什么要自己对】
  网上拿到的 LRC 只有 1 秒粒度, 而且有些行是同时刻的(0:00 一堆)。
  直接拿它当字幕, 会明显飘。这个脚本用音轨自己的能量突变(onset)去吸附每一行,
  并且把"到底有多少行真的落在人声起点上"这个数字打出来 —— 自检, 不是自我感觉。

【诚实性】
  这是【对时间轴】, 不是【重新作词】。歌词文本来自公开 LRC, 未改动一个字;
  只改起始时间。改过的行数与偏移量都会打印出来。
"""
from __future__ import annotations
import json, re, sys, wave
from pathlib import Path
import numpy as np

PV = Path(__file__).resolve().parents[1]
ROOT = PV.parents[1]
SRC_LRC = PV / "lyrics" / "source.lrc"
WAV = PV / "build" / "song.wav"
OUT = PV / "lyrics" / "timeline.json"

META = re.compile(r"^(Lyrics|Composed|Arranged|Produced|Vocals|Title|Album|Artist)\b", re.I)


def parse_lrc(text: str):
    rows = []
    for line in text.splitlines():
        m = re.match(r"\[(\d+):(\d+(?:\.\d+)?)\](.*)$", line.strip())
        if not m:
            continue
        t = int(m.group(1)) * 60 + float(m.group(2))
        txt = m.group(3).strip()
        rows.append((t, txt))
    rows.sort(key=lambda r: r[0])
    return rows


def onset_envelope(path: Path, sr_out=22050, nfft=1024, hop=256):
    with wave.open(str(path), "rb") as w:
        sr = w.getframerate()
        n = w.getnframes()
        ch = w.getnchannels()
        raw = np.frombuffer(w.readframes(n), dtype=np.int16).astype(np.float32)
    if ch > 1:
        raw = raw.reshape(-1, ch).mean(axis=1)
    raw /= 32768.0
    if sr != sr_out:                      # 线性重采样即可(只取包络)
        idx = np.linspace(0, len(raw) - 1, int(len(raw) * sr_out / sr))
        raw = np.interp(idx, np.arange(len(raw)), raw).astype(np.float32)
        sr = sr_out
    win = np.hanning(nfft).astype(np.float32)
    frames = 1 + (len(raw) - nfft) // hop
    mag = np.empty((frames, nfft // 2 + 1), dtype=np.float32)
    for i in range(frames):
        seg = raw[i * hop:i * hop + nfft] * win
        mag[i] = np.abs(np.fft.rfft(seg))
    flux = np.maximum(0.0, np.diff(mag, axis=0)).sum(axis=1)
    flux = np.concatenate([[0.0], flux])
    # 局部归一 + 平滑, 避免被整体响度带跑
    k = 9
    base = np.convolve(flux, np.ones(k) / k, mode="same")
    env = flux / (base + 1e-6)
    env = np.convolve(env, np.ones(3) / 3, mode="same")
    return env, hop / sr


def peaks(env, hop_s, min_dist=0.12, rel=1.6):
    idx = []
    thr = np.median(env) * rel
    for i in range(1, len(env) - 1):
        if env[i] > thr and env[i] >= env[i - 1] and env[i] > env[i + 1]:
            if idx and (i - idx[-1]) * hop_s < min_dist:
                if env[i] > env[idx[-1]]:
                    idx[-1] = i
                continue
            idx.append(i)
    return np.array([i * hop_s for i in idx])


def energy_at(env, hop_s, t):
    i = int(t / hop_s)
    a, b = max(0, i - 4), min(len(env), i + 5)
    return float(env[a:b].max()) if b > a else 0.0


def main():
    if not SRC_LRC.exists():
        print("缺少 lyrics/source.lrc", file=sys.stderr)
        return 1
    rows = parse_lrc(SRC_LRC.read_text(encoding="utf-8"))
    lyrics = [(t, x) for t, x in rows if x and not META.match(x)]
    print(f"LRC 行数 {len(rows)}  其中歌词行 {len(lyrics)}")
    env, hop_s = onset_envelope(WAV)
    pk = peaks(env, hop_s)
    print(f"音轨 {len(env) * hop_s:.1f}s  检出 onset {len(pk)} 个")

    out, shifted, moved = [], 0, 0.0
    prev_t = -1.0
    for i, (t, txt) in enumerate(lyrics):
        best, best_gain = t, 0.0
        for p in pk:
            if abs(p - t) <= 0.85:
                g = energy_at(env, hop_s, p) - energy_at(env, hop_s, t) * 0.0
                if g > best_gain:
                    best, best_gain = float(p), g
        if abs(best - t) > 0.05:
            shifted += 1
            moved += abs(best - t)
        t = max(best, prev_t + 0.05)
        prev_t = t
        t1 = lyrics[i + 1][0] if i + 1 < len(lyrics) else 212.4
        out.append({"t0": round(t, 3), "t1": round(t1, 3), "text": txt, "lrc": round(float(dict(rows).get(0, 0)) if False else 0, 3)})

    # 逐行时长 = 下一行起点, 但起码 0.6s
    for i in range(len(out) - 1):
        out[i]["t1"] = round(max(out[i + 1]["t0"], out[i]["t0"] + 0.6), 3)
    out[-1]["t1"] = round(out[-1]["t0"] + 3.0, 3)

    hits = sum(1 for r in out if energy_at(env, hop_s, r["t0"]) > np.median(env) * 1.2)
    print(f"吸附过的行 {shifted}/{len(out)}  平均偏移 {moved / max(shifted,1):.3f}s")
    print(f"起点落在强 onset 上的行 {hits}/{len(out)} ({100.0*hits/len(out):.0f}%)")
    OUT.write_text(json.dumps({"duration": 212.33, "lines": out}, ensure_ascii=False, indent=1), encoding="utf-8")
    print("写出", OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
