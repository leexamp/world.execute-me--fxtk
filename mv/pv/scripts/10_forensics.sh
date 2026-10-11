#!/usr/bin/env bash
# 10_forensics.sh — 采集"终端取证"镜头用的真实命令输出。
# 原则: 这个目录里的每一个字节都是真跑出来的。不允许手打。
set -uo pipefail
cd "$(dirname "$0")/../../.."
OUT="mv/pv/forensics"; mkdir -p "$OUT"
run() { name="$1"; shift; { echo "\$ $*"; eval "$@" 2>&1; } > "$OUT/$name.txt"; echo "$name $(wc -c < "$OUT/$name.txt")B"; }

run git_count        "git log --oneline | wc -l"
run git_first        "git log --reverse --format='%h|%ad|%s' --date=short | head -4"
run git_last         "git log --format='%h|%ad|%s' --date=short | head -3"
run sandbox_delete   "git show --stat 7d3ecb1 | head -12"
run sandbox_existed  "git show 7d3ecb1^:linux_sim/app.c 2>/dev/null | head -3"
run deleted_dirs     "git log --diff-filter=D --name-only --format='%h %ad %s' --date=short -- linux_sim | head -8"
run ghost_fix        "git show cec3b00 --stat | head -8 && echo '---' && git show cec3b00 -s --format='%H%n%ad%n%s' --date=iso | head -3"
run size_now         "ls -l demo-main/fxtk_sim | awk '{print \$5, \$9}'; file demo-main/fxtk_sim | cut -c1-120"
run honesty_marks    "grep -c '\[ENV\]\|\[N/A\]' CHANGELOG.md; grep -n '\[ENV\]\|\[N/A\]' CHANGELOG.md | head -6"
run slim_commit      "git show --stat 146a68d | head -10"
run sokol_switch     "git show -s --format='%h %ad %s' --date=short 33b7136 6d81e01 9a633b3 34e9f29"
run perf_truth       "git show -s --format='%h %ad %s' --date=short f910ceb a2cdab3 f189a6d 815e8ae"
run pool_half        "git show -s --format='%h %ad %s' --date=short 4837d08 043175e"
run v25_ten          "git log --oneline main..feat/v2.5 | head -12"
run frame_extents    "xprop -root _NET_FRAME_EXTENTS 2>/dev/null || echo 'xprop: [N/A] 本会话取不到根窗口属性'"
run diff_del_lines   "git log --diff-filter=D --format='%h %s' --name-only | grep -v '^$' | head -20"
chmod -R a+r "$OUT"; ls "$OUT" | wc -l
