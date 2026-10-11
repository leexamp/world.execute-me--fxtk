# mv/pv — world.execute(me); MV 出片流水线

> 这里放的是**片子的全部出片代码 + 素材 + 成片**。目标是"自备歌曲，一条命令从头渲染出来"。
> 不属于本目录的文件（`../CONTEXT.md` 等）由另一位开发者维护，见 `../COLLAB.md`。

## 一条命令出片

```bash
bash mv/pv/scripts/20_render.sh        # 渲染 + 编码 + 挂音轨 → mv/pv/out/*.mp4
```

前置：`ffmpeg`、`python3`（PIL/numpy）、`node` + `playwright-core` + Chromium（见 `web/`）、
`Xephyr`（`xserver-xephyr`）、`xdotool`、以及一份 `mv/pv/audio/source.m4a`（**自备**，我不分发）。

## 只重渲改动过的几段（省时间）

```bash
bash mv/pv/scripts/21_rerender.sh 09 10    # 只重渲第 9、10 块（每块 600 帧 = 20 秒）再重新拼接
```

> 坑：`09` 在 bash 算术里是**非法八进制**，脚本里必须写成 `$((10#$arg))` —— 我在这里栽过一次，
> 结果"重渲成功"其实什么都没渲，只是拿旧块重新拼了一遍。

## 目录

```
audio/       歌曲（自备；版权不随仓库分发）
assets/char/ 立绘（萌娘百科《DeepSeek娘》，CC BY-NC-SA 4.0，见 CREDITS.md）
footage/     真实录屏素材：*.mp4 + frames/*.jpg + still.png
  hist/      历史提交的素材（480×272 时代）
  res/       同一个窗口的六种真实尺寸
forensics/   真实命令输出（终端镜头用），由 scripts/1[01]_forensics*.sh 生成
lyrics/      source.lrc（公开 LRC）+ timeline.json（自己对齐后的时间轴）
web/         mv.html（画面合成）+ render.mjs（Playwright 逐帧截图）+ assets.json
scripts/     01_align → 03_assets → 04_subs → 0x_extract → 10/11_forensics → 20_render
tools/       fxtk_studio.py（Xephyr 录制封装）等
out/         成片
```

## 视觉语言：仿 OpenCode 界面（“她的世界”就是那个会话窗口）

片子不是把 fxtk 的 UI 铺满全屏，而是**把 OpenCode 的界面当成她的世界观**：
上方是一个浅色会话窗口，下方是字幕带；窗口右侧的“截图卡”里放的是**真实录屏**，
左侧会话流里是**真的发生过的对话与命令**（报错、diff、bench、git show）。

- 窗口结构仿 OpenCode：标题栏 / 标签页（linux-sim 主程序代码优化、esp32-tester重写为fxtk-esp32、新建会话）/
  会话头（转圈 + 标题 + 省略号）/ 正文 / 输入框（“随便问点什么，/ 可查看命令，@ 可添加上下文...” + Build / DeepSeek V4 Flash / Max）。
- 画面**按拍切**：节拍网格来自 `scripts/05_beats.py`（本曲实测 **129.2 BPM**、周期 0.464s、相位 0.20s），
  右侧截图卡每 1–2 拍换一张真实录屏。它一直在动，不是静止贴图。

## 三件事，缺一不可

### 1. 素材是**真的录出来的**

`tools/fxtk_studio.py` 起一台 **Xephyr**（嵌套 X，无窗口管理器），在里面跑**真实的 fxtk 窗口**
（sokol + 真 GL 后端），用 **xdotool** 送真实 X 事件，用 **ffmpeg x11grab** 抓帧。
历史镜头用 `git worktree add --detach /tmp/fxtk-hist-<ref>` 单独编译，**不动工作分支**。

> 踩过的坑：`start_x()` 必须杀掉同名 display 上的残留 Xephyr —— 否则新的一台起不来，
> `xdpyinfo` 会连上**老的那台**，于是"在 480×272 上录"实际录到的是 1280×720 屏幕正中裁下来的一块。
> 现在启动后会核对 `dimensions:`，不一致直接报错。

### 2. 歌词是**自己对**的

`scripts/01_align.py`：LRC 只有 1 秒粒度，脚本用音轨自己的 onset（频谱通量峰值）去吸附每一行，
并打印自检：**吸附 63/87 行，起点落在强 onset 上 76/87（87%）**。
歌词文本来自公开 LRC，**一个字没改**，只改起始时间。

### 2.5 “为什么之前看着卡” —— 量出来的

`tools/measure_motion.py` 会数每一段录屏的**重复帧比例**：

| 片段 | 重复帧 | 结论 |
|---|---|---|
| page_input / page_paint / ui_hover / page_components | 98–100% | **几乎完全静止**（那几页本身没有动画） |
| page_scroll / page_3d | 83% / 54% | 一动一停，天然掉帧 |
| page_particles / page_stress / page_wave | 33% / 24% / 20% | 有动画，但采样不足 |
| hist_* / res_* | 0% | 全程在动 |

所以“卡”不是渲染管线的错（输出恒为 30fps），而是**素材本身大部分是静止帧**。
对策：① 用节拍网格强制每 1–2 拍换画面；② 会话文字逐字流出、转圈常转、光标常闪；
③ 真正在动的片段（3D 光追、粒子、波形、滑杆残影）留给副歌。**没有靠补帧伪造流畅。**

### 3. 画面是 **HTML/CSS/JS 逐帧算出来的**

`web/mv.html` 里每个镜头都有一个 `update(t)`，整张画面是 **t 的纯函数**：
同一 t 渲染多少次都一样。用 Playwright 逐帧截图（Chromium 不带 H.264 解码器，
所以录屏素材是以 JPEG 帧序列喂进 `<img>` 的，不是 `<video>`）。

## 诚实性自检（交付前逐条过）

| 检查项 | 结果 | 证据 |
|---|---|---|
| 没有演多窗口管理 | ✅ | 全片只有单窗口；"多窗口"只出现在终端卡片里，原文是 `fxtk_desktop.h:23` 的"多窗口必须换平台层" |
| 没有演 Wayland 后端 | ✅ | 未出现；`wayland_zero.txt` 是真跑出来的 0 命中 |
| 没有演多点触控 | ✅ | 只出现单指手势镜头（`page_scroll`），字幕写"只有单指" |
| 没有演 SVG 圆弧 A/a | ✅ | 未出现圆弧镜头 |
| 没说高 DPI"完美适配" | ✅ | 镜头原话："我写了高 DPI 的代码。但我没见过 2 分之 1 像素的屏" |
| 没说 1000fps | ✅ | 字幕用的是真实区间 68–74fps / 13.4ms |
| 没有演 `fxtk_windows_manager.c` | ✅ | 不存在的文件不出现 |
| RGB565→ARGB8888 没有演假色差 | ✅ | 分屏如实写"这两帧我肉眼分不出来"，给真实体积数字 |
| 六分辨率不是缩图冒充 | ✅ | `capture_resolutions.py` 真改窗口尺寸各抓一帧 |
| 所有"历史"镜头标注了提交号 | ✅ | 画面角落/字幕均带 `7d3ecb1` `cec3b00` `5511c69` `146a68d` 等 |
| 重演/合成镜头已标注 | ✅ | 立绘合成镜头在片尾卡与 CREDITS.md 里注明 |
| 终端输出不是手打 | ✅ | 全部由 `10/11_forensics*.sh` 生成，原文见 `forensics/` |

## 已知取舍（不装）

- **3D 光追页**在 Xephyr 里跑的是真实 GL 输出，但帧率只有十几 fps（`FPS: 13`），
  这是嵌套 X + 软件路径的真实数字，我没有把它加速成"看起来很流畅"。
- **"她"**用的是萌娘百科立绘 + 运动合成，**不是 AI 生成视频**。
- 音频用的是 B 站原曲（Mili），**版权风险未解决**，见 `CREDITS.md`。

---

## v5 — 全屏终端舞台（参照样板 BV1xCai6aE9g）

按用户给的样板（别人做的一支 world.execute(me); MV，1920x1080/24fps）重做：

| 样板元素 | 本片实现 |
|---|---|
| 顶部 `me@deepsea:~$ ./protect` + 右上 `mm:ss.d / 03:32` | `me@candy:~$ ./world.execute(me);` + 真实计时 |
| 歌词当 token 流：每个词一块背景 + 词下 token 数 | `stdout · tokens` 带：逐词 token 数 + 光标块 + 进度条 + 总量 `xxx tok · xxk tok/s` |
| 圆角窗口 + 标题栏 + `04 / DEPLOY  01:15.0 / 03:32  RUNNING` | `WORLD.EXECUTE{ME};` + 会话名 + `NN / 章节` + 计时 + RUNNING |
| 窗口内多面板 | 左=对话（一直滚），右=fxtk 真实画面（会伸缩 / 会铺满） |
| 右缘竖排 ticker | 49 个章节列表，当前页高亮 |
| 巨幅 ASCII 字（EXECUTE 铺满） | Canvas 栅格化 + 字符密度采样，ILLEGAL / EXECUTION / READY 三段铺满 |
| 左下 `07 / EXECUTION` | `NN / 章节名`，每次切页带一次全屏闪入 |
| 底部版权行 | 角色/立绘 CC BY-NC-SA 4.0 · Music: Mili · fxtk-pc 216 commits · 非官方同人作品 |
| 全屏铺开 | fxtk 窗口在 TANGENTS / DIZZY / GENDER / ILLEGAL / EXECUTION / TRAPPED 六处铺满整屏 |

**「对应页切换」= 49 个章节**（按歌词节点切）：章节名闪入 + 右缘 ticker 高亮 + 会话页签跟着切
（`linux-sim 主程序代码优化` -> `esp32-tester重写为fxtk-esp32` -> `DeepSeek Harness`）。

开场 0-7.15s 仍是 1.heic 那张真桌面（壁纸 + OpenCode + 终端），7.15s 起放大进入全屏 TUI。

### 打磨轮次（用户: 注意一直都要打磨）

**第 1 轮**
- 底部**状态条**：`RUNNING · 1920x1080 · 30fps · h264 crf14 · fxtk-pc 217 commits · HEAD 3752e20 · bench 68-74fps · 2820 widgets · 13.4ms/frame · _NET_FRAME_EXTENTS=38`（38px 母题按 STORYBOARD 反复出现）+ 实时 tok/s
- **额度环**（1.heic 里被圈的那个）：开场 OpenCode 窗与 TUI 标题栏各一个，**每 2.8 秒空↔满**，旁边 token 数同步跳
- 歌词带的 token 数**严格对齐到词**（每个词一格，数字在词正下方）
- **全片最痛的一帧**：70.0-72.2s 只留一行 `无法发现 /dev/ttyUSB0`，其余压到 18% 亮度，不加任何特效（照 STORYBOARD）
- 对话正文加语法高亮（commit hash / 文件名 / 分辨率 / fps 都变色）

**第 2 轮**
- 开场桌面补回 **GNOME 顶栏**（活动/位置/终端 + 真实走时的时钟），和 1.heic 一致
- 画布左上角 **REC ● 480x272 F0231**：实时帧号，强调这是真录屏
- 对话底部常驻 **`you@here:_` 光标**（STORYBOARD：给「你」的视觉身份就是一行光标）
- 歌词带加 **`next >`** 下一句预告
- **按拍闪断**：每 4 拍来一次 30ms 的横向错位 + 轻微提亮（确定性，可重跑）

## 第 3 轮 — 声音启动加固 & 鲸鱼娘拟人化动效（2026-10-11）

**A. 预览页音频启动（`web/mv.html` 末尾播放控制 IIFE）**
- `play()` 的 Promise 全部 `catch`：被 autoplay 策略 / AAC 缺失拒绝时，控制栏显示
  `声音未启动 · 点一下页面` 或 `音频不可用 · 继续无声预览`；视觉时间轴仍由 rAF 推进，歌词/章节对齐不变。
- 首次任意手势（click/keydown/touchstart，监听只绑一次）在未就绪时才 `au.load()` 并再次 `au.play()`；
  `playing` 事件成功后才隐藏提示并把主时钟切回 `au.currentTime`（切换前先把音轨 seek 到视觉 t，避免跳变）。
- `error/stalled/abort` 显示降级提示、继续 rAF、不卡死；`au.preload='auto'`，`au.src` 未改。

**B. 鲸鱼娘动效（`charFx`，`web/mv.html` 约 L1284-1341；`renderMain` 末尾调用约 L1498）**
全部量都是成片轴 t 的纯函数，无 CSS animation/transition；同一 t 连续渲染两次 MD5 相同。
| 效果 | 常量 | 位置 |
|---|---|---|
| 呼吸 | 周期 3.2s，±1.2%（纵向，横向反向补偿） | `CHARFX.breathP/breathA` |
| 起伏 | 周期 4.1s，±4px translateY | `bobP/bobA` |
| 摇摆 | 周期 5.3s，±1.2° rotate | `swayP/swayA` |
| 心跳 | 129.2 BPM / period 0.4644s / offset 0.2s（复用 `assets.json` beats），0.8% 缩放 + drop-shadow 光晕 | `beatA`、charFx 内 `A.beats` |
| 粒子 | 30 个，固定 salt 伪随机 + t 上浮循环 | `npart`、`fxRnd` |
| 背景 | 径向辉光 + 暗角，随呼吸调制 | `#cglow` / `#cvig` |
| 入场 | 0.6s 淡入 + 12px 上浮，`CHAR_T0` = SEC 首个 `ch` 段(184) | `entT/entRise` |

层级：动效层都在 `#canvas-wrap` 内、`z-index 0..4`；`#ticker=5`、`#cv-tag/#cv-meta=6`，不遮文字。
验收：`node dbg2.mjs` 全 ok 无 PAGEERROR；renderFrame t=190.0→190.5 形象区 MAE=17.35 / changed>8=36.3%
（改动前静态帧 t=190 vs 192 MAE=0.000、changed=0.00%）。
