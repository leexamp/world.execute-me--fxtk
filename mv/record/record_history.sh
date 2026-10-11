#!/usr/bin/env bash
# record_history.sh — 历史镜头录制辅助
#
# 【为什么需要它】
#   分镜里有十几个镜头要回到历史提交去录。手工 checkout 既容易搞乱工作区,
#   又容易在"编不过"的时候含糊过去。这个脚本把两件事钉死:
#     ① 永远用【独立 worktree】, 绝不动你的工作分支
#     ② 编不过就如实报错, 绝不静默跳过
#
# 用法:
#   ./record_history.sh list                    # 列出可录的历史镜头
#   ./record_history.sh build cec3b00^          # 在 worktree 里编译某个提交
#   ./record_history.sh run   cec3b00^          # 编译并运行(然后手动录屏)
#   ./record_history.sh show  cec3b00           # 看提交详情(录"终端取证"镜头用)
#   ./record_history.sh clean                   # 清掉所有临时 worktree

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WT_BASE="${TMPDIR:-/tmp}/fxtk-hist"

c_info() { printf '\033[36m%s\033[0m\n' "$*"; }
c_ok()   { printf '\033[32m%s\033[0m\n' "$*"; }
c_err()  { printf '\033[31m%s\033[0m\n' "$*" >&2; }

# 素材表: 提交|标签|说明  (与 HISTORY.md 保持同步)
MILESTONES=(
  "fe7a2c5|史前|Initial commit(只有 README)"
  "6b1140d|RGB565|RGB565 时代代码首次进来"
  "a65b15d|RGB565|2.0版本主要更新"
  "5511c69|RGB565|v2.1更新(含 0x0841 暗色)"
  "7d3ecb1|伏笔|Delete linux_sim directory ★结尾揭示用"
  "146a68d|瘦身|发布产物 Linux 154->101KB / Windows 801->183KB"
  "f189a6d|性能|2816 控件 13.25 -> 9.31 ms/帧"
  "33b7136|sokol|P2.1+P2.2 sokol 后端打通(<1% 对照)"
  "70d8b96|sokol|3D 页接 GPU 光追"
  "cec3b00|真bug|滑杆残影修复 ★★★全片最有力镜头"
  "9a633b3|Windows|单 exe 无 DLL 达标"
  "6d81e01|sokol|sokol 转默认后端(SDL2 降遗留)"
  "34e9f29|输入法|中文输入法打通(XIM)"
  "815e8ae|bench|软件 108fps vs sokol 59fps"
  "f910ceb|自我否证|实测推翻'整画布重绘是根源'"
  "4837d08|瘦身|控件池 16384->8192"
  "004abc0|重构|驱动独立到 drivers/ + your_app/"
)

cmd_list() {
    c_info "可录制的历史镜头 (完整说明见 HISTORY.md)"
    printf '%-10s %-10s %s\n' "提交" "标签" "说明"
    printf '%-10s %-10s %s\n' "----------" "----------" "--------------------------------------------"
    for m in "${MILESTONES[@]}"; do
        IFS='|' read -r h tag desc <<<"$m"
        printf '%-10s %-10s %s\n' "$h" "$tag" "$desc"
    done
    echo
    c_info "★ 头号镜头: 滑杆残影"
    echo "   修复前: ./record_history.sh run 'cec3b00^'   # 拖滑杆 → 留下一串残影"
    echo "   修复后: ./record_history.sh run  cec3b00     # 同样操作 → 残影消失"
}

cmd_show() {
    local ref="$1"
    c_info "=== $ref 详情 (录'终端取证'镜头用这个输出) ==="
    git -C "$REPO_ROOT" show --stat "$ref"
}

cmd_build() {
    local ref="$1"
    local wt="$WT_BASE"
    c_info "=== 准备 worktree @ $ref ==="
    git -C "$REPO_ROOT" worktree remove --force "$wt" 2>/dev/null || true
    rm -rf "$wt"
    # 用 detached worktree, 不会创建/移动任何分支
    git -C "$REPO_ROOT" worktree add --detach "$wt" "$ref"

    c_info "=== 编译 (demo-main) ==="
    if [ ! -d "$wt/demo-main" ]; then
        c_err "!! 该提交没有 demo-main/ 目录 —— 这个时代可能还没有这个结构"
        c_err "!! 如实记录, 不要伪造。可以试更晚的提交。"
        exit 1
    fi
    ( cd "$wt/demo-main" && make )
    c_ok "编译成功: $wt/demo-main/fxtk_sim"
    echo "$wt"
}

cmd_run() {
    local ref="$1"
    local wt
    wt="$(cmd_build "$ref" | tail -1)"
    c_info "=== 启动 (开始录屏; Ctrl+C 结束) ==="
    c_info "    提示: 统一用 1280x720 窗口, 60fps 录制"
    ( cd "$wt/demo-main" && ./fxtk_sim )
}

cmd_clean() {
    c_info "=== 清理临时 worktree ==="
    git -C "$REPO_ROOT" worktree list
    git -C "$REPO_ROOT" worktree remove --force "$WT_BASE" 2>/dev/null || true
    rm -rf "$WT_BASE"
    git -C "$REPO_ROOT" worktree prune
    c_ok "已清理"
}

case "${1:-}" in
    list)  cmd_list ;;
    show)  cmd_show "${2:?用法: record_history.sh show <提交>}" ;;
    build) cmd_build "${2:?用法: record_history.sh build <提交>}" >/dev/null ;;
    run)   cmd_run "${2:?用法: record_history.sh run <提交>}" ;;
    clean) cmd_clean ;;
    *)
        echo "用法: $0 {list|show <提交>|build <提交>|run <提交>|clean}"
        echo
        echo "  list     列出可录制的历史镜头"
        echo "  show     看提交详情(录终端取证镜头)"
        echo "  build    在独立 worktree 里编译(不动你的分支)"
        echo "  run      编译并运行"
        echo "  clean    清理临时 worktree"
        exit 1
        ;;
esac
