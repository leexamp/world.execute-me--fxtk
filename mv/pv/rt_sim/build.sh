#!/usr/bin/env bash
# 为"最初的 fxtk"（fxtk-rt@48c2ef9，ESP32 专用、原本没有模拟器）编译一个 PC 模拟器。
#
# 原始代码在 orig/ 里，直接从 git 历史取出、一个字没改；
# 硬件层被 stubs/ 与 sim_driver.c 替换掉（见 sim_driver.c 顶部说明）。
set -euo pipefail
cd "$(dirname "$0")"
O=orig/components/fxtk
gcc -O2 -g -pthread -std=gnu11 \
    -Istubs -I"$O" -Iorig/main \
    "$O/fxtk.c" "$O/fxtk_draw.c" "$O/fxtk_font.c" "$O/fxtk_widgets.c" \
    "$O/cn_gray.c" "$O/ascii_gray16.c" \
    orig/main/app.c \
    sim_driver.c \
    -o fxtk_rt_sim \
    -lSDL2 -lm -pthread
echo "构建完成: $(pwd)/fxtk_rt_sim"
