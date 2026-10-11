#!/usr/bin/env bash
# 11_forensics2.sh — 第二批真实取证 (必须在对话里出现的那种"做不到")
set -uo pipefail
cd "$(dirname "$0")/../../.."
OUT="mv/pv/forensics"; mkdir -p "$OUT"
run() { name="$1"; shift; { echo "\$ $*"; eval "$@" 2>&1; } > "$OUT/$name.txt"; echo "$name $(wc -c < "$OUT/$name.txt")B"; }

run usb_fail        "ls -l /dev/ttyUSB0; echo '---'; idf.py flash 2>&1 | head -2"
run window_truth    "grep -n '多窗口\\|单窗口' components/fxtk/fxtk_desktop.h | head -4; echo '---'; grep -c 'sapp_run' third_party/sokol/sokol_app.h"
run wayland_zero    "grep -ci wayland third_party/sokol/sokol_app.h; grep -ci wayland third_party/sokol/sokol_gfx.h"
run decoration_truth "grep -n 'mutter\\|_MOTIF_WM_HINTS\\|38' CHANGELOG.md | grep -i '标题栏\\|装饰\\|frame' | head -4; echo '--- 现场实测 ---'; xprop -root _NET_FRAME_EXTENTS 2>&1 | head -2"
run touch_truth     "grep -n 'touch_read' components/fxtk/fxtk.h | head -3"
run svg_truth       "grep -n \"圆弧\\|不支持\\|A/a\" components/fxtk/fxtk_svg.c | head -5"
run flex_truth      "grep -n 'fx_flex_apply' components/fxtk/fxtk.h | head -3"
run dpi_truth       "echo '【她的三个需求 → 我的三条真实回答】'; echo; echo '1) 多窗口管理:'; grep -n '多窗口' components/fxtk/fxtk_desktop.h | head -2; echo; echo '2) 去除标题栏:'; grep -n 'mutter' CHANGELOG.md | head -2; echo; echo '3) 灵活布局:'; grep -n 'fx_flex_apply' components/fxtk/fxtk.h | head -2"
ls "$OUT" | wc -l
