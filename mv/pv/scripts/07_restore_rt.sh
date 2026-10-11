#!/usr/bin/env bash
# 07_restore_rt.sh — 从 git 历史取出"最初的 fxtk"（fxtk-rt 最早那份上传），原样放到 rt_sim/orig/。
#
# 为什么需要它: 最早的 fxtk 只存在于 leexamp/fxtk-rt 这个 ESP32 工程里,
# 而且**没有 PC 模拟器** —— 要把它跑起来看, 只能自己补一层模拟器(见 sim_driver.c)。
#
# 可用环境变量 FXTK_RT_SRC 指定本地镜像位置(默认 /tmp/fxtk-rt.git)。
set -euo pipefail
cd "$(dirname "$0")/../../.."
RT=mv/pv/rt_sim
COMMIT=48c2ef9              # 2026-08-06 "Add files via upload" —— 最早有代码的那次提交
SRC="$(printenv FXTK_RT_SRC || true)"
[ -n "$SRC" ] || SRC=/tmp/fxtk-rt.git

if [ ! -d "$SRC" ]; then
  echo "没有本地镜像, 从 GitHub 取..."
  git clone --mirror https://github.com/leexamp/fxtk-rt "$SRC"
fi

rm -rf "$RT/orig"
mkdir -p "$RT/orig/components/fxtk" "$RT/orig/main"
for f in fxtk.c fxtk_draw.c fxtk_font.c fxtk_widgets.c fxtk_st6201.c fxtk_touch.c \
         cn_gray.c cn_gray.h ascii_gray16.c ascii_gray16.h fxtk.h fxtk_internal.h; do
  git -C "$SRC" show "$COMMIT:components/fxtk/$f" > "$RT/orig/components/fxtk/$f"
done
for f in app.c CMakeLists.txt; do
  git -C "$SRC" show "$COMMIT:main/$f" > "$RT/orig/main/$f"
done
for f in README.md LICENSE; do
  git -C "$SRC" show "$COMMIT:$f" > "$RT/orig/$f"
done

echo "已取出 fxtk-rt@$COMMIT -> $RT/orig"
echo "  原始提交: $(git -C "$SRC" log -1 --date=short --format=%ad%x20%an%x20%s $COMMIT)"
ls "$RT/orig/components/fxtk" | tr "\n" " "; echo