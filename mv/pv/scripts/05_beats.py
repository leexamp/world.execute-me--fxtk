#!/usr/bin/env python3
"""05_beats.py — 从音轨里量出速度, 生成节拍网格 (供"按拍切镜"用)。

做法: 频谱通量 onset 包络 → 自相关找周期 → 用包络能量挑相位。
不猜 BPM, 全部从这条音轨量出来。
"""
from __future__ import annotations
import json, wave
from pathlib import Path
import numpy as np

PV = Path(__file__).resolve().parents[1]
WAV = PV / "build" / "song.wav"
OUT = PV / "lyrics" / "beats.json"


def envelope(path):
    with wave.open(str(path), "rb") as w:
        sr = w.getframerate(); n = w.getnframes(); ch = w.getnchannels()
        raw = np.frombuffer(w.readframes(n), dtype=np.int16).astype(np.float32)
    if ch > 1: raw = raw.reshape(-1, ch).mean(axis=1)
    raw /= 32768.0
    nfft, hop = 1024, 256
    win = np.hanning(nfft).astype(np.float32)
    frames = 1 + (len(raw) - nfft) // hop
    mag = np.empty((frames, nfft // 2 + 1), dtype=np.float32)
    for i in range(frames):
        mag[i] = np.abs(np.fft.rfft(raw[i*hop:i*hop+nfft] * win))
    flux = np.concatenate([[0.0], np.maximum(0.0, np.diff(mag, axis=0)).sum(axis=1)])
    k = 15
    base = np.convolve(flux, np.ones(k)/k, mode="same")
    return flux / (base + 1e-6), hop / sr


def main():
    env, hop_s = envelope(WAV)
    e = env - env.mean()
    ac = np.correlate(e, e, "full")[len(e)-1:]
    lo, hi = int(0.30 / hop_s), int(1.20 / hop_s)          # 50–200 BPM
    lag = lo + int(np.argmax(ac[lo:hi]))
    bpm = 60.0 / (lag * hop_s)
    # 倍频校正: 落在 80–180 区间内
    while bpm < 80: bpm *= 2
    while bpm > 180: bpm /= 2
    period = 60.0 / bpm
    # 相位: 在 [0, period) 内找让网格能量最大的偏移
    best_off, best_score = 0.0, -1.0
    for off in np.arange(0, period, 0.005):
        idx = ((np.arange(off, len(env)*hop_s, period)) / hop_s).astype(int)
        idx = idx[idx < len(env)]
        s = float(env[idx].mean())
        if s > best_score: best_score, best_off = s, float(off)
    beats = np.arange(best_off, len(env) * hop_s, period)
    data = {"bpm": round(bpm, 2), "period": round(period, 4), "offset": round(best_off, 3),
            "beats": [round(float(b), 3) for b in beats], "downbeat_hint": 4}
    OUT.write_text(json.dumps(data), encoding="utf-8")
    print(f"BPM {bpm:.1f}  周期 {period:.3f}s  相位 {best_off:.3f}s  节拍数 {len(beats)}  → {OUT}")


if __name__ == "__main__":
    main()
