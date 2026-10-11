#!/usr/bin/env bash
# 22_render_1080p.sh — 出 1920x1080 版（桌面舞台：OpenCode + 终端 + fxtk 窗口）。
#
# 与 20_render.sh 的区别: 分辨率 1920x1080、截图用 JPEG q96（1080p 下 PNG 太慢: 1.8fps vs 更快的 JPEG），
# 编码仍是 crf 14。
set -euo pipefail
cd "$(dirname "$0")/../../.."
PV=mv/pv; OUT=$PV/out; BUILD=$PV/build1080; FPS=30; CHUNK=${CHUNK:-600}
TOTAL=${TOTAL:-$(python3 -c "print(int(217.33*$FPS))")}
export MV_W=1920 MV_H=1080
mkdir -p "$OUT" "$BUILD/parts"
python3 "$PV/scripts/03_assets.py"

if ! curl -s -o /dev/null -m 2 http://127.0.0.1:8901/web/mv.html; then
  ( cd "$PV" && setsid nohup python3 -m http.server 8901 --bind 127.0.0.1 >/tmp/mvhttp.log 2>&1 < /dev/null & sleep 1.5 )
fi

: > "$BUILD/parts/list.txt"
idx=0
for ((i=0; i<TOTAL; i+=CHUNK)); do
  n=$(( TOTAL - i < CHUNK ? TOTAL - i : CHUNK ))
  t0=$(python3 -c "print($i/$FPS)"); t1=$(python3 -c "print(($i+$n)/$FPS)")
  id=$(printf %02d $idx)
  d="$BUILD/parts/chunk$id"; rm -rf "$d"; mkdir -p "$d"
  echo "== chunk $id: frames $i..$((i+n-1))  t=$t0..$t1"
  MV_URL=http://127.0.0.1:8901/web/mv.html node "$PV/web/render.mjs" "$d" "$FPS" "$t0" "$t1" 96
  ffmpeg -hide_banner -loglevel error -y -framerate "$FPS" -i "$d/%05d.jpg" \
         -c:v libx264 -preset medium -crf 14 -pix_fmt yuv420p "$BUILD/parts/part$id.mp4"
  ( cd "$BUILD/parts" && ls part*.mp4 | sed "s|^|file '|; s|$|'|" > list.txt )
  rm -rf "$d"
  idx=$((idx+1))
done

ffmpeg -hide_banner -loglevel error -y -f concat -safe 0 -i "$BUILD/parts/list.txt" -c copy "$OUT/film_silent_1080p.mp4"
# 成片音轨: source_aligned.m4a = 4.89s 前导静音 + source.m4a + 尾部补到 217.34s。
# 前导 4.89s 让片源第一个音符(0.11s)正好落在成片 t=5.0s(冷开场结束/切主界面那一刻);
# 生成命令与实测见 mv/pv/ALIGN.md。不要换回 source_pad.m4a(那条没有前导静音, 音乐会提前 5 秒起)。
[ -f "$PV/audio/source_aligned.m4a" ] || { echo "缺少 $PV/audio/source_aligned.m4a（见 mv/pv/ALIGN.md 的生成命令）"; exit 1; }
ffmpeg -hide_banner -loglevel error -y -i "$OUT/film_silent_1080p.mp4" -i "$PV/audio/source_aligned.m4a" \
       -c:v copy -c:a aac -b:a 192k -shortest "$OUT/world_execute_me_1080p.mp4"
ls -la "$OUT"