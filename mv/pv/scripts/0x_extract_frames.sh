#!/usr/bin/env bash
# 0x_extract_frames.sh — 把录到的 mp4 拆成 JPEG 帧 (HTML 里用 <img> 逐帧播, 因为
# Playwright 的 Chromium 不带 H.264 解码器 —— 实测 canPlayType('video/mp4; avc1') 为空)。
set -euo pipefail
cd "$(dirname "$0")/../../.."   # 仓库根
FD="mv/pv/footage"; OUT="mv/pv/footage/frames"
mkdir -p "$OUT"
for f in "$FD"/*.mp4; do
  n=$(basename "$f" .mp4)
  case "$n" in *_raw|*preview) continue;; esac
  if [ -f "$OUT/$n/.done" ]; then echo "skip $n"; continue; fi
  rm -rf "$OUT/$n"; mkdir -p "$OUT/$n"
  ffmpeg -hide_banner -loglevel error -y -i "$f" -vf "fps=30,scale=1280:720" -q:v 3 "$OUT/$n/%04d.jpg"
  touch "$OUT/$n/.done"
  echo "$n $(ls "$OUT/$n" | grep -c jpg) frames"
done
du -sh "$OUT"
