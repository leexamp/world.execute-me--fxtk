#!/usr/bin/env bash
# 20_render.sh —— 把 mv.html 逐帧渲染成成片。
#
# 为什么要分块: 6 千多帧 JPEG 一次全落盘要 1GB+。每块 600 帧(20 秒)渲染 → 编码 → 删帧,
# 最后 concat 无损拼接, 再挂上音轨。任一块渲染失败都不会污染已经出好的块。
set -euo pipefail
cd "$(dirname "$0")/../../.."
PV=mv/pv; OUT=$PV/out; BUILD=$PV/build; FPS=30; CHUNK=${CHUNK:-600}
TOTAL=${TOTAL:-$(python3 -c "print(int(212.33*$FPS))")}
mkdir -p "$OUT" "$BUILD/parts"
python3 "$PV/scripts/03_assets.py"

if ! curl -s -o /dev/null -m 2 http://127.0.0.1:8900/web/mv.html; then
  ( cd "$PV" && setsid python3 -m http.server 8900 --bind 127.0.0.1 >/tmp/mvhttp.log 2>&1 & sleep 1.5 )
fi

: > "$BUILD/parts/list.txt"
idx=0
for ((i=0; i<TOTAL; i+=CHUNK)); do
  n=$(( TOTAL - i < CHUNK ? TOTAL - i : CHUNK ))
  t0=$(python3 -c "print($i/$FPS)")
  t1=$(python3 -c "print(($i+$n)/$FPS)")
  id=$(printf %02d $idx)
  d="$BUILD/parts/chunk$id"; rm -rf "$d"; mkdir -p "$d"
  echo "== chunk $id: frames $i..$((i+n-1))  t=$t0..$t1"
  # 用 PNG 而不是 JPEG: 截图这一环不要再叠一次有损编码（"糊"的一半来自这里）
  MV_URL=http://127.0.0.1:8900/web/mv.html node "$PV/web/render.mjs" "$d" "$FPS" "$t0" "$t1" 97 --png
  ffmpeg -hide_banner -loglevel error -y -framerate "$FPS" -pattern_type glob -i "$d/*.png" \
         -c:v libx264 -preset medium -crf 14 -pix_fmt yuv420p "$BUILD/parts/part$id.mp4"
  echo "file '$(cd "$(dirname "$BUILD/parts/part$id.mp4")" && pwd)/part$id.mp4'" >> "$BUILD/parts/list.txt"
  rm -rf "$d"
  idx=$((idx+1))
done

ffmpeg -hide_banner -loglevel error -y -f concat -safe 0 -i "$BUILD/parts/list.txt" -c copy "$OUT/film_silent.mp4"
[ -f "$PV/audio/source.m4a" ] || { echo "缺少 $PV/audio/source.m4a（歌曲自备，见 CREDITS.md）"; exit 1; }
ffmpeg -hide_banner -loglevel error -y -i "$OUT/film_silent.mp4" -i "$PV/audio/source.m4a" \
       -c:v copy -c:a aac -b:a 192k -shortest "$OUT/world_execute_me_fxtk_720p.mp4"
ls -la "$OUT"
