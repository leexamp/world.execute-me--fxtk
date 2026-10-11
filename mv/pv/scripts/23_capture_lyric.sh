#!/usr/bin/env bash
# ============================================================================
# 23_capture_lyric.sh —— 43 个手写歌词场景 → 每场景 900 帧 确定性离线渲染
#
# 为什么不是旧办法(ffmpeg -f x11grab 抓 Xephyr 的屏):
#   抓屏和应用是两套时钟: 抓到的帧里大量是上一帧的重复(实测旧 43 场景 7740 帧,
#   相邻完全相同 1839 对 = 23.9%; 最惨的 satisfaction/ready/protection 是 175/180),
#   而且只有 180 帧 = 3.0s —— 长句(t1-t0 最长 ≈13s)后半段只能冻在最后一帧, 像卡死。
#   现在让应用自己按固定 tick 逐帧落盘(lyric.c 的 FX_DUMP/FX_FRAMES):
#   第 k 帧渲染用的就是 tick=k, 与真实时间、vsync 抖动完全无关, 严格 1 tick/帧;
#   900 帧 = 15.0s $60fps, 覆盖全曲最长句。
#
# 为什么还要 X: sokol_app 在本仓库只有 X11/GLX 后端(third_party/sokol/sokol_app.h),
#   没有 EGL/headless —— 实测 env -u DISPLAY 直接 abort(核心转储)。所以退回 Xephyr
#   虚拟 X: 它只负责提供一个 GL 上下文, 画面不经过它抓取, 每个 worker 一个 :9x。
#
# 用法:
#   bash mv/pv/scripts/23_capture_lyric.sh                 # 全部 43 场景
#   ONLY="sine travel" bash mv/pv/scripts/23_capture_lyric.sh
#   JOBS=6 Q=3 SCALE=960:540 FRAMES=900 bash mv/pv/scripts/23_capture_lyric.sh
# 环境变量: FRAMES JOBS Q SCALE ONLY START_DISP
# 只杀自己起的进程(并发环境友好): 绝不 pkill -x Xephyr / pkill -x lyric_sim。
# ============================================================================
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
APP=$ROOT/mv/pv/lyric_app
OUT=$ROOT/mv/pv/footage/lyric
FR=$ROOT/mv/pv/footage/lyric_frames
LOG=$FR/.capture.log
FRAMES=${FRAMES:-900}
JOBS=${JOBS:-6}
Q=${Q:-3}
SCALE=${SCALE:-960:540}
ONLY=${ONLY:-}
START_DISP=${START_DISP:-91}
KEEP_PNG=${KEEP_PNG:-0}          # 1 = 保留 /tmp/lydump/<name> 里的中转 PNG(体积超限时可换质量重编码)

# 场景名/顺序直接从 lyric.c 的 SCENES[] 抽, 避免两边手工维护不一致
mapfile -t NAMES < <(grep -o '{ "[a-z0-9]*", sc_' "$APP/lyric.c" | sed 's/{ "//; s/", sc_//')
if [ "${#NAMES[@]}" -ne 43 ]; then echo "!! 从 lyric.c 解析到 ${#NAMES[@]} 个场景, 期望 43, 中止" >&2; exit 1; fi

SEL=()
if [ -n "$ONLY" ]; then
  for k in $ONLY; do
    if [[ "$k" =~ ^[0-9]+$ ]]; then SEL+=("$k|${NAMES[$k]}"); else
      for i in "${!NAMES[@]}"; do [ "${NAMES[$i]}" = "$k" ] && SEL+=("$i|$k"); done
    fi
  done
else
  for i in "${!NAMES[@]}"; do SEL+=("$i|${NAMES[$i]}"); done
fi
[ ${#SEL[@]} -gt 0 ] || { echo "没有选中任何场景"; exit 1; }

mkdir -p "$OUT" "$FR"
: > "$LOG"
MYXE=(); MYSIM=()
cleanup() {
  local p
  for p in "${MYSIM[@]:-}"; do [ -n "$p" ] && kill "$p" 2>/dev/null; done
  for p in "${MYXE[@]:-}"; do [ -n "$p" ] && kill "$p" 2>/dev/null; done
  return 0
}
trap cleanup EXIT

# ---- 后端探测: 先试无 X, 不行再 Xephyr ----
XE_ON=""
pd=$(mktemp -d)
if timeout 30 env -u DISPLAY -u WAYLAND_DISPLAY FX_SCENE=0 FX_FRAMES=1 FX_DUMP="$pd" "$APP/lyric_sim" >/dev/null 2>&1 && [ -s "$pd/0001.png" ]; then
  echo "[后端] 无 X 直接跑通 —— 不需要 Xephyr"
else
  echo "[后端] 无 X 跑不通(sokol_app 硬依赖 XOpenDisplay, 实测 SIGABRT) -> 退回 Xephyr"
  XE_ON=1
  for w in $(seq 0 $((JOBS-1))); do
    d=$((START_DISP+w)); rm -f /tmp/.X$d-lock
    Xephyr :$d -screen 1300x760 -nolisten tcp -no-host-grab >/tmp/xe$d.log 2>&1 &
    MYXE+=("$!")
  done
  sleep 3.5
  RDY=""
  for w in $(seq 0 $((JOBS-1))); do
    d=$((START_DISP+w)); ok=0
    for t in 1 2 3 4 5 6 7 8; do
      rm -rf "$pd"; mkdir -p "$pd"
      DISPLAY=:$d FX_SCENE=0 FX_FRAMES=1 FX_DUMP="$pd" "$APP/lyric_sim" >/dev/null 2>&1
      [ -s "$pd/0001.png" ] && { ok=1; break; }
      sleep 1
    done
    RDY="$RDY :$d=$ok"
  done
  echo "[后端] Xephyr 就绪:$RDY"
fi
rm -rf "$pd"

# ---- 单场景流水线 ----
run_one() {                     # $1=序号 $2=名字 $3=display(空=无X)
  local i=$1 n=$2 d=$3
  local tmp=/tmp/lydump/$n t0 t1 sec got rc nj kb sp
  rm -rf "$tmp" "$FR/$n" "$OUT/$n.mp4"; mkdir -p "$tmp" "$FR/$n"
  t0=$(date +%s.%N)
  DISPLAY="$d" FX_SCENE=$i FX_DUMP="$tmp" FX_FRAMES=$FRAMES "$APP/lyric_sim" >/dev/null 2>"$tmp/.sim.log" &
  sp=$!
  MYSIM+=("$sp"); wait "$sp"; rc=$?
  t1=$(date +%s.%N); sec=$(echo "$t1 - $t0" | bc)
  got=$(ls "$tmp"/*.png 2>/dev/null | wc -l)
  if [ "$rc" != "0" ] || [ "$got" != "$FRAMES" ]; then
    echo "FAIL $n scene=$i rc=$rc png=$got/$FRAMES (日志 $tmp/.sim.log)" | tee -a "$LOG"; return 1
  fi
  ffmpeg -hide_banner -loglevel error -y -framerate 60 -start_number 1 -i "$tmp/%04d.png" \
    -vf "scale=$SCALE" -q:v "$Q" "$FR/$n/%03d.jpg" || { echo "FAIL $n jpg" | tee -a "$LOG"; return 1; }
  ffmpeg -hide_banner -loglevel error -y -framerate 60 -start_number 1 -i "$tmp/%04d.png" \
    -c:v libx264 -preset veryfast -crf 16 -pix_fmt yuv420p -r 60 "$OUT/$n.mp4" || { echo "FAIL $n mp4" | tee -a "$LOG"; return 1; }
  nj=$(ls "$FR/$n"/*.jpg 2>/dev/null | wc -l); kb=$(du -sk "$FR/$n" | cut -f1)
  [ "$KEEP_PNG" = "1" ] || rm -rf "$tmp"
  printf "%s\t%s\t%s\t%s\t%s\n" "$i" "$n" "$nj" "$sec" "$kb" >> "$LOG"
  printf "[%2d] %-14s %3d 帧  %6.1fs  %5.1f fps有效  %6dKB\n" "$i" "$n" "$nj" "$sec" \
    "$(echo "$FRAMES/$sec" | bc -l)" "$kb"
}

worker() {                      # $1=worker号 $2=display
  local w=$1 d=$2 k
  for ((k=w; k<${#SEL[@]}; k+=JOBS)); do
    [ -z "${SEL[$k]:-}" ] && continue
    IFS='|' read -r i n <<< "${SEL[$k]}"
    run_one "$i" "$n" "$d"
  done
}
echo "[录制] 场景 ${#SEL[@]} 个 x $FRAMES 帧  JOBS=$JOBS  jpg=$SCALE -q:v$Q  mp4=crf16/60fps"
T0=$(date +%s.%N)
for w in $(seq 0 $((JOBS-1))); do worker "$w" "$( [ -n "$XE_ON" ] && echo ":$((START_DISP+w))" )" & done
wait
T1=$(date +%s.%N)

# ---- 汇总 ----
echo "---------------------------------------------"
TOTF=0; TOTKB=0; OK=0; BAD=0
for s in "${SEL[@]}"; do
  IFS='|' read -r i n <<< "$s"
  c=$(ls "$FR/$n"/*.jpg 2>/dev/null | wc -l)
  kb=$(du -sk "$FR/$n" 2>/dev/null | cut -f1); kb=${kb:-0}
  TOTF=$((TOTF+c)); TOTKB=$((TOTKB+kb))
  if [ "$c" = "$FRAMES" ]; then OK=$((OK+1)); else BAD=$((BAD+1)); echo "!! $n 只有 $c 帧"; fi
done
MKB=$(du -sk "$OUT" | cut -f1)
echo "场景 $OK 个满 $FRAMES 帧 / 异常 $BAD 个"
echo "总耗时 $(echo "$T1 - $T0" | bc)s"
if [ "$TOTF" -gt 0 ]; then
  echo "帧总数 $TOTF   帧体积 $((TOTKB/1024))MB   单帧均值 $((TOTKB*1024/TOTF))B(含块对齐)"
  echo "mp4 体积 $((MKB/1024))MB   素材合计 $(((TOTKB+MKB)/1024))MB"
fi
echo "中转 PNG: /tmp/lydump/<name>/  KEEP_PNG=$KEEP_PNG"
echo "完成"
