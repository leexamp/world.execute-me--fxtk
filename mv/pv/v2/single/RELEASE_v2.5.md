# world.execute(me); — 桌面多窗口 MV v2.5（示例程序）

> 这不是网页 MV，而是一个**单文件 C 程序**：运行后在你的桌面上创建 **6 个真实系统窗口**，
> 按歌词时间轴切换 81 句专属画面、移动/缩放窗口、跑 GPU 光线步进。

## 下载

| 平台 | 文件 | 二进制大小 |
|---|---|---|
| Linux x86_64 | `world-execute-me-mv-v2.5-linux-x86_64.tar.gz` | 318,320 B |
| Windows x64 | `world-execute-me-mv-v2.5-windows-x64.zip` | 514,560 B |

两个包都附带 `BUILD.md` / `MAINTENANCE.md`；同页的 `SHA256SUMS.txt` 是本页附件的校验和。

## 运行

- **Linux**：解压后 `chmod +x movie-linux-x86_64 && ./movie`
- **Windows**：解压后双击 `world_execute_me-windows-x64.exe`
- 依赖：OpenGL 3.3；Linux 需 X11（XWayland 可以），Windows 自带 opengl32；**默认静音**
- 1080p 屏上舞台 1280×720 按 1.5× 铺满；其他分辨率等比缩放居中
- **关闭任意一扇窗 / Esc = 退出**（多窗口示例的关窗语义，正常设计）
- 环境变量：`WEE_T0=130` 从 130s 预览；`WEE_FPS=30` 限帧；`WEE_NODRIFT=1` 关漂移；
  `WEE_STATIC=1` 关所有窗口动画（录屏最稳）；`WEE_AUDIO=file.mp3` 显式开声音

## 构建参数（-Os）

- Linux：`-Os -ffunction-sections -fdata-sections -Wl,--gc-sections -s`
- Windows：`-Os -Wl,--gc-sections -s`
  （MinGW 下 `-ffunction-sections` 与 `-s` 组合会产出约 17 MB 未剥离产物，已避开）
- 体积对比 `-O2`：Linux 759,096 → **318,320 B**；Windows 1,125,128 → **514,560 B**
- 完整命令见 `mv/pv/v2/single/BUILD.md`

## 源码 / 文档

- 源码：本仓库 `mv/pv/v2/single/`（`movie.c` + `script.inc`）
- 编译/运行/录屏：[BUILD.md](https://github.com/leexamp/world.execute-me--fxtk/blob/feat/v2.5/mv/pv/v2/single/BUILD.md)
- 维护手册（数据表/动画/后端/坑位/成片合成）：[MAINTENANCE.md](https://github.com/leexamp/world.execute-me--fxtk/blob/feat/v2.5/mv/pv/v2/single/MAINTENANCE.md)
- 框架文档：fxtk-pc [docs/multi-window.md](https://github.com/leexamp/fxtk-pc/blob/main/docs/multi-window.md) ·
  [docs/window-manager.md](https://github.com/leexamp/fxtk-pc/blob/main/docs/window-manager.md)

## 已知限制

- 多窗口后端为实验性能力：X11 only、主屏、最多 8 窗口、鼠标/键盘事件未转发给控件层、真 HiDPI 未验证；
- 默认静音，需要声音请显式 `WEE_AUDIO=...`；
- 视频成片不在本 Release 内：仓库 `mv/pv/out/world_execute_me_final_1080p.mp4`（1920×1080 / 60fps / 3:40）。

---
本项目是 fxtk 的真实示例：所有窗口、文字、图形均由 fxtk 绘制；6 个窗口是**真实系统窗口**，不是后期合成。
