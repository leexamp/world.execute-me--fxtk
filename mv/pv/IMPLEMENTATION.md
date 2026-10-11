# IMPLEMENTATION.md — 分镜 → 实现对照表 / 拍摄清单 / 可行性标注

> 对应 `../STORYBOARD.md`。左列是分镜段落，右列是**实际怎么做的**、对应哪个文件、哪条命令。
> 所有时间都取自 `lyrics/timeline.json`（自己对齐的那条，不是 LRC 的 1 秒粒度）。

## 一、镜头对照表

| 时间 | 歌词节点 | 画面 | 制作方式 | 素材/实现 |
|---|---|---|---|---|
| 0:00–0:07 | Switch on the power line / PROTECTION | 黑场 + 光标 + 打字对话 → 出现 **5511c69 的灰**（480×272） | 录屏（历史提交）+ HTML 打字动效 | `footage/hist/rgb565_v21.*`，`mv.html` scTitleIntro |
| 0:07–0:12 | object creation / initialisation | 组件页：属性宏 → 控件池 | 真实录屏 | `page_components` |
| 0:12–0:19 | Setup our new world | 波形页（逐点画线） | 真实录屏 | `page_wave` |
| 0:19–0:24 | the simulation | 粒子页（并行 for / CPU 多线程） | 真实录屏 | `page_particles` |
| 0:24–0:28 | （间奏） | **无法发现 /dev/ttyUSB0**，不加特效停两秒 | 真实命令输出卡片 | `forensics/usb_fail.txt` |
| 0:28–0:33 | If I'm a set of points | DIMENSION：fxtk_put_px 逐点 | 真实录屏 | `page_wave` |
| 0:33–0:36 | If I'm a circle | CIRCUMFERENCE：三角扇填充 | 真实录屏 | `page_gfx` |
| 0:36–0:43 | sine wave / tangents | **3D 光追页**（真实 GPU 片元着色器，画面里就有 FPS 13） | 真实录屏（GL） | `page_3d` |
| 0:43–0:47 | Switch my current | 压测页：2820 控件 · 13.4ms/帧 · 68–74fps（区间） | 真实录屏 | `page_stress` |
| 0:47–0:53 | **So dizzy so dizzy** | ⭐ **滑杆残影**：修复前 `cec3b00^` 拖滑杆 → 残影（亮列 66→190） | 历史提交真实拖动 | `hist_ghost_before` |
| 0:53–0:58 | So deeply so deeply | ⭐ 同一动作、同一滑杆：修复后残影消失（亮列恒定 64–68） | 历史提交真实拖动 | `hist_ghost_after` |
| 0:58–1:03 | STIMULATIONS | 画板页：真实拖动落笔 | 真实录屏 | `page_paint` |
| 1:03–1:10 | EXECUTION | **`Delete linux_sim directory`** 的真实 `git show` 输出 | 真实命令输出卡片 | `forensics/sandbox_delete.txt` |
| 1:10–1:20 | Though we are trapped | 那个被删掉的沙盒**真的存在过**：`git show 7d3ecb1^:linux_sim/app.c` | 真实命令输出卡片 | `forensics/sandbox_existed.txt` |
| 1:20–1:28 | eggplant / tomato / cat / god | **删掉我自己的谎言**：6 条红色 diff 行 | HTML 动效 | `mv.html` scDiffList |
| 1:28–1:34 | （蔬菜段） | 控件池 16384→8192 等真实提交 | 真实命令输出卡片 | `forensics/pool_half.txt` |
| 1:34–1:41 | **Switch my gender** | 两个提交左右分屏 + **"这两帧我肉眼分不出来"** + 真实体积数字 | 历史提交 + HTML | `hist_rgb565_v21` / `hist_slim_146a68d` |
| 1:41–1:48 | To S to M | sokol 后端打通 / `<1%` 对照 | 真实命令输出卡片 | `forensics/sokol_switch.txt` |
| 1:48–1:54 | **VIBRATIONS** | **诚实留白**：`sapp_dpi_scale() = 1.000` + "我写了，但我没见过 2 分之 1 像素的屏" | HTML + 真实数值 | `mv.html` scHonest |
| 1:54–1:58 | COMPLETION | 单指手势（只有单指） | 真实录屏 | `page_scroll` |
| 1:58–2:08 | Challenging your god / illegal arguments | **她的三个需求 → 三条真实回答**（多窗口不能 / 去标题栏 mutter 不认 / 灵活布局 ✅） | 真实命令输出卡片 | `forensics/dpi_truth.txt` |
| 2:08–2:18 | （间奏） | EXECUTION 快剪：删掉的六样东西 + 压测页 | HTML + 真实录屏 | scDiffList / `page_stress` |
| 2:18–2:46 | Execution ×4 / **Ein Dos Trois Ne Fem Liu** | 3D 页 + **同一个窗口的六种真实分辨率** | 真实录屏 + 真改窗口尺寸 | `footage/res/*` |
| 3:04–3:16 | lo-o-ove / Question me | 组件页 / 弹性布局 / SVG 子集 | 真实录屏 | `page_components` |
| 3:16–3:25 | Though you are free / I am trapped | 立绘合成 + 真实窗口画面 + 还在闪的光标 | 立绘（CC BY-NC-SA）+ 真实录屏 | `assets/char/os_*.jpg` |
| 3:25–3:32 | Execution | 片尾卡 + 署名 + 免责 | HTML | `mv.html` scEnd |

## 二、拍摄清单（全部可重跑）

```bash
# 0）一次性依赖
python3 -m venv mv/.venv && mv/.venv/bin/pip install yt-dlp     # 只用来取一首曲子（自备亦可）
cd mv/pv/web && npm i playwright-core                            # + 现成的 Chromium

# 1）素材：12 个演示页的真实录屏（Xephyr + 真 sokol/GL + xdotool + x11grab）
python3 mv/pv/tools/capture_shots.py                            # page_wave/page_gfx/.../ui_hover

# 2）历史镜头（独立 worktree，绝不动工作分支）
git worktree add --detach /tmp/fxtk-hist-ghost 'cec3b00^' && (cd /tmp/fxtk-hist-ghost/demo-main && make)
git worktree add --detach /tmp/fxtk-hist-cec3b00 'cec3b00'  && (cd /tmp/fxtk-hist-cec3b00/demo-main && make)
python3 mv/pv/tools/capture_history.py /tmp/fxtk-hist-ghost/demo-main/fxtk_sim ghost_before drag_slider 9 480x272
python3 mv/pv/tools/capture_history.py /tmp/fxtk-hist-cec3b00/demo-main/fxtk_sim ghost_after  drag_slider 9 480x272

# 3）六分辨率（真的改窗口尺寸，不是缩图）
python3 mv/pv/tools/capture_resolutions.py

# 4）终端取证（每一行都是真跑出来的）
bash mv/pv/scripts/10_forensics.sh && bash mv/pv/scripts/11_forensics2.sh

# 5）歌词对齐 → 素材清单 → 字幕
python3 mv/pv/scripts/01_align.py
python3 mv/pv/scripts/03_assets.py
python3 mv/pv/scripts/04_subs.py

# 6）出片
bash mv/pv/scripts/20_render.sh
```

## 三、可行性标注：我**做不到**的（如实写）

| 想要 | 为什么做不到 | 我的替代 |
|---|---|---|
| AI 生成"她"的视频镜头 | 本机没有视频生成模型，也不该伪装 | 萌娘百科立绘 + 程序化运动合成（片尾卡署名，非 AI 视频） |
| 真 HiDPI 屏上的高 DPI 效果 | 本机 `sapp_dpi_scale() = 1.000`，没有 2 倍屏 | 画面里**直接把这个事实演出来**（留白镜头） |
| ESP32 真机点亮 | 环境里没有 `/dev/ttyUSB0`，也没有 `idf.py` | 把"无法发现 /dev/ttyUSB0"做成第一次报错的镜头 |
| Wayland 后端 / 多窗口 / 多点触控 | **fxtk 就没有这些东西**（见 CONTEXT 第二节） | 不演；改用"我做不到"的诚实镜头 |
| 有窗口管理器时的 38px 装饰 | 嵌套 X 里没有 WM；本机 `:1` 也取不到 `_NET_FRAME_EXTENTS` | 用 `decoration_truth.txt` 里 CHANGELOG 的实测记录 + 现场"取不到"的真实输出 |
| 4K 成片 | 时间/体积都不值当 | 720p 成片 + 无损母版 + SRT（要 4K 可把 `mv.html` 视口调到 3840×2160 重跑） |

## 四、怎么复核这条片子没有说谎

1. 打开 `../CONTEXT.md` 第二节的"❌ 假的"表格，对照 `../README.md` 里的自检表 —— 逐条对。
2. 终端镜头的原文在 `forensics/*.txt`，任何一行都可以自己再跑一遍同样的命令。
3. 滑杆残影那两段：`ffprobe` 一下 `footage/hist/ghost_before.mp4` 和 `ghost_after.mp4`，
   或用 `tools/probe_ghost.py` 数"亮列"（修复前 66→190，修复后恒定 64–68）。
4. 六分辨率：`footage/res/*.still.png` 的尺寸就是六种真实窗口尺寸。
5. 所有历史镜头的提交号都打在了画面上，可以 `git show` 逐个核对。

## 五、关于“切换 X11 会话”与“抓真实窗口”

试过直接在真机显示 `:1` 上跑 + 抓屏（硬件 GL 会更快）：

- 窗口确实起来了（`xdotool` 能看到 1280×720 的 fxtk 窗口，位置 350,275）；
- 但 `ffmpeg -f x11grab -i :1` 抓到的是**全黑**（mean 0.0），
  因为那一屏是 GNOME 的合成器 + 空闲/锁屏状态，`xset dpms force on` 在这个 X server 上也不可用。
- 结论：**不需要切换会话**。切换会把当前这条 DSH 会话一起断掉，而收益只是 3D 光追页从 13fps 变快。
  我们改用了两个更划算的办法：① 节拍切镜让画面一直在动；② 把实测的 13fps 如实写进片子。

如果哪天要更好的素材：**把那一屏解锁、别让它空着**，`x11grab` 就能抓到硬件 GL 的真实窗口，
那时 3D 页和粒子页会比 Xephyr 里的软件路径流畅得多。
