#!/usr/bin/env bash
# 21_rerender.sh — 只重渲指定区间（改动某几个镜头时用，不必整片重来）。
# 用法: bash 21_rerender.sh 09 10      # 重渲 chunk09/10 并重新拼接
set -euo pipefail
cd "$(dirname "$0")/../../.."
PV=mv/pv; BUILD=$PV/build; OUT=$PV/out; FPS=30; CHUNK=600
TOTAL=$(python3 -c "print(int(212.33*$FPS))")
if ! curl -s -o /dev/null -m 2 http://127.0.0.1:8900/web/mv.html; then
  ( cd "$PV" && setsid python3 -m http.server 8900 --bind 127.0.0.1 >/tmp/mvhttp.log 2>&1 & sleep 1.5 )
fi
for arg in "$@"; do
  idx=$(( 10#$arg ))      # 坑: bash 把 09 当八进制, 09 是非法数字 —— 必须显式按十进制读
  i=$(( idx * CHUNK ))
  n=$(( TOTAL - i < CHUNK ? TOTAL - i : CHUNK ))
  t0=$(python3 -c "print($i/$FPS)"); t1=$(python3 -c "print(($i+$n)/$FPS)")
  id=$(printf %02d "$idx"); d="$BUILD/parts/chunk$id"; rm -rf "$d"; mkdir -p "$d"
  echo "== re-render chunk $id  t=$t0..$t1"
  MV_URL=http://127.0.0.1:8900/web/mv.html node "$PV/web/render.mjs" "$d" "$FPS" "$t0" "$t1" 97 --png
  ffmpeg -hide_banner -loglevel error -y -framerate "$FPS" -pattern_type glob -i "$d/*.png" \
         -c:v libx264 -preset medium -crf 14 -pix_fmt yuv420p "$BUILD/parts/part$id.mp4"
  rm -rf "$d"
done
( cd "$BUILD/parts" && ls part*.mp4 | sed "s|^|file '|; s|$|'|" > list.txt )
ffmpeg -hide_banner -loglevel error -y -f concat -safe 0 -i "$BUILD/parts/list.txt" -c copy "$OUT/film_silent.mp4"
ffmpeg -hide_banner -loglevel error -y -i "$OUT/film_silent.mp4" -i "$PV/audio/source.m4a" \
       -c:v copy -c:a aac -b:a 192k -shortest "$OUT/world_execute_me_fxtk_720p.mp4"
echo "已重出:"; ls -la "$OUT"
