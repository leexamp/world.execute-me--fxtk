#!/usr/bin/env bash
# build.sh — 编译”手写歌词场景”的 fxtk 应用（MV 里 fxtk 窗口的画面来源）。
#
# 只读地引用仓库里的框架源码（components/fxtk + drivers），不改动仓库任何文件。
set -euo pipefail
cd "$(dirname "$0")"
REPO=${REPO:-../../..}
CORE="$REPO/components/fxtk/fxtk.c $REPO/components/fxtk/fxtk_draw.c $REPO/components/fxtk/fxtk_widgets.c \
      $REPO/components/fxtk/fxtk_effects.c $REPO/components/fxtk/fxtk_extra.c $REPO/components/fxtk/fxtk_backends.c \
      $REPO/components/fxtk/fxtk_par.c $REPO/components/fxtk/fxtk_gesture.c $REPO/components/fxtk/fxtk_flex.c $REPO/components/fxtk/fxtk_svg.c"
DRV="$REPO/drivers/fxtk_app_sokol.c $REPO/drivers/fxtk_sokol_driver.c $REPO/drivers/fxtk_font_stb.c"
gcc -O2 -w -I"$REPO/components/fxtk" -I"$REPO/drivers" -I"$REPO/third_party/sokol" \
    lyric.c $DRV $CORE -o lyric_sim \
    -lX11 -lXi -lXcursor -lGL -ldl -lpthread -lm
echo "构建完成: $(pwd)/lyric_sim"
echo "运行: FX_SCENE=12 ./lyric_sim   (0..42)"