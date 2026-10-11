#!/usr/bin/env bash
# 08_capture_rt.sh — 录"最初的 fxtk"（fxtk-rt@48c2ef9，ESP32 专用、原本没有模拟器）的真实运行。
#
# 画面来源: rt_sim/orig/ 里那份**一个字节没改**的原始代码 + 我写的 PC 模拟器 sim_driver.c。
# 驱动方式: Xephyr 嵌套 X + xdotool 真实鼠标事件（点标签页 / 拖滑条 / 按键盘）。
set -euo pipefail
cd "$(dirname "$0")/../../.."
PV=mv/pv; BIN=$PV/rt_sim/fxtk_rt_sim; OUT=$PV/footage; FR=$OUT/frames
D=:98
mkdir -p "$FR"
[ -x "$BIN" ] || { echo "先跑 bash mv/pv/rt_sim/build.sh"; exit 1; }

cleanup() { pkill -x fxtk_rt_sim 2>/dev/null || true; pkill -x Xephyr 2>/dev/null || true; }
trap cleanup EXIT
cleanup; sleep 0.5; rm -f /tmp/.X98-lock

Xephyr "$D" -screen 640x480 -nolisten tcp >/tmp/xephyr98.log 2>&1 &
sleep 2.5
DISPLAY=$D "$BIN" >/tmp/rt_sim.log 2>&1 &
sleep 3.5
export DISPLAY=$D
WID=$(xdotool search --name fxtk | head -1)
xdotool windowmove "$WID" 0 0; sleep 0.5

# 按必须"按住"一帧以上：这个 UI 每 16ms 轮询采样鼠标状态，
# 瞬时 click(按下即抬起) 会被整帧吞掉，什么都点不动。踩过一次。
tap() { xdotool mousemove --sync "$1" "$2"; xdotool mousedown 1; sleep 0.13; xdotool mouseup 1; sleep 0.6; }
drag() { xdotool mousemove --sync "$1" "$2"; xdotool mousedown 1; sleep 0.2; \
         for x in $(seq "$1" 12 "$3"); do xdotool mousemove --sync $x "$2"; sleep 0.05; done; \
         xdotool mousemove --sync "$3" "$2"; sleep 0.2; xdotool mouseup 1; sleep 0.3; }

REC_PID=
rec_start() { ffmpeg -hide_banner -loglevel error -y -f x11grab -framerate 30 -video_size 480x272 \
   -i "$D+0,0" -t "$2" -c:v libx264 -preset veryfast -crf 14 -pix_fmt yuv420p \
   "$OUT/rt_$1.mp4" >/dev/null 2>&1 & REC_PID=$!; }
rec_wait() { wait "$REC_PID"; }

echo "== rt_orig_pages: 波形 -> 图形 -> 控件 -> 波形"
rec_start rt_orig_pages 12
sleep 0.5; tap 239 38; sleep 2.2; tap 392 38; sleep 2.2; tap 86 38; sleep 1.5
rec_wait
echo "== rt_orig_ui: 拖速度滑条 + 波形开关 + 数字键盘"
rec_start rt_orig_ui 14
sleep 0.5; drag 176 209 330; sleep 0.8; tap 60 234; sleep 0.5; tap 60 234; sleep 0.5
tap 239 38; sleep 0.8; tap 96 96; tap 174 96; tap 252 96; sleep 0.8; tap 366 60; sleep 1.0
rec_wait

for n in rt_orig_pages rt_orig_ui; do
  rm -rf "$FR/$n"; mkdir -p "$FR/$n"
  ffmpeg -hide_banner -loglevel error -y -i "$OUT/rt_$n.mp4" -vf fps=30 -q:v 2 "$FR/$n/%04d.jpg"
  ffmpeg -hide_banner -loglevel error -y -ss 1.0 -i "$OUT/rt_$n.mp4" -frames:v 1 "$OUT/rt_$n.still.png"
  echo "  $n: $(ls "$FR/$n" | wc -l) 帧"
done
echo "完成 -> $OUT/rt_orig_*.mp4"