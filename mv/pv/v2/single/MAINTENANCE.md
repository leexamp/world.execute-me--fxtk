# MAINTENANCE.md — world.execute(me); 多窗口 MV 维护手册

面向"以后还要改它/重录它"的人。结构说明、数据表格式、后端职责、已知坑、回归命令、成片合成、发布与隐私规则，都在这里。

> 配套文档：[`BUILD.md`](BUILD.md)（编译/运行/录屏）；[`../../README.md`](../../README.md)、[`../../CONTEXT.md`](../../CONTEXT.md)、[`../../HISTORY.md`](../../HISTORY.md) 是 v1 MV 的史料，不是 v2.5 程序的说明。

---

## 1. 目录与职责

| 路径 | 职责 |
|---|---|
| `movie.c` | 全部应用逻辑：时间轴、6 窗口创建/编排、81 句歌词构图、生成艺术、片尾 |
| `script.inc` | 左栏台词表 `SCR[]`（纯文本，改台词只动这里） |
| `BUILD.md` | 编译/运行/画面结构/录屏 |
| `MAINTENANCE.md` | 本文件 |
| `movie` / `world_execute_me.exe` | 构建产物，不入库 |
| `../../../../drivers/fxtk_win.[ch]` | 多窗口 façade（后端无关；应用只依赖它） |
| `../../../../drivers/fxtk_sokol_mw.c` | 多窗口 GL 后端：X11 GLX + Win32 WGL |
| `../../../../drivers/fxtk_sokol_driver.c` | fxtk 的 sokol 驱动；`FXTK_MW` 模式支持每窗口画布/帧缓冲缓存 |
| `../../../../drivers/fxtk_mw.h` / `fxtk_mw_gl.c` | 后端内部接口 / 单窗口 GL 入口 |

---

## 2. 时间轴与数据表

### 2.1 `LYR[]`（歌词）
`{ float t0, t1; const char *s; }`，81 条有效行 + 结束哨兵。
- 时间单位：秒，`t0/t1` 来自歌曲对齐；
- 改歌词文本只改 `s`；改时间会同时影响"切镜边界"和"每句画面"，务必重新预演。

### 2.2 `LART[]`（每句歌词的构图，81 行）
`{ motif, p1, p2, p3, bg }` 五个字段，与 `LYR[]` 一一对齐（有编译期长度断言 `lart_size_check`，对不上会编译失败）。
- `motif`：构图编号（见 `draw_motif()` 的 `case`）；
- `p1/p2/p3`：该句独有参数（数量/速度/相位/配色）；
- `bg`：背景纹样编号（`draw_bg()`：实底/网格/点阵/扫描线/横带/十字/斜线/星点）。
- 设计原则：**不共用同一组 (bg, motif)**；同一母题只用在语义相关处（如茄子→养分）。

### 2.3 `CHAPTERS[]` / `SCENE_MASK[]`（段落与窗口开关）
- 25 个段落标签（现在是"版本里程碑"文案）；
- `SCENE_MASK[ch]` 位掩码控制窗口开关：bit0 台词、bit1 终端、bit2 画面、bit3 章节、bit4 歌词、bit5 通知；
- 代码里额外强制 **台词/歌词/画面** 三窗常开；`Ein` 段强制六窗全开。

### 2.4 改台词
只改 `script.inc`；句数变化会影响台词窗初始高度（`g_edit_h` 自适应），不影响时间轴。

---

## 3. 画面与动画系统

- 所有绘制都走 `rf/tx/ln/FC/DC/DA/DRR/FRR/FE/FTR` 等封装；内部统一乘 **`g_sc`**（1280×720 设计稿 → 屏幕等比缩放）。
- 新增一个"每句歌词的构图"：在 `draw_motif()` 里加 `case`，再在 `LART[]` 改对应行的 motif/参数。
- 新增一个"每句的独有实物"：建议写独立 `obj_xxx()` 函数（参考 `obj_eggplant/obj_tomato/obj_cat`），只使用基础图元与 SDF 圆角填充，不画人。
- 常用动画：
  - 每句切画面：窗口轻推 `g_nudge` + 画面内容缩放 `g_zoom`（窗口级，不做画布仿射）；
  - 70s/110s 重排：`layout()` 用 `u1/u2` 缓动，加一次短缩脉冲 `g_lp`；
  - Ein 段：`ein_active()` 时间窗 → 强制六窗、改尺寸 380×240、3×2 网格、进出闪白（`g_flash`）。
- 结束段：196s 后只剩台词+歌词；208s 起台词向背景色混暗；204s 后歌词条淡出。

---

## 4. 后端：多真实窗口

- 应用只调 `fxtk_win_*`；后端 vtable 在 `drivers/fxtk_sokol_mw.c` 注册。
- 一个进程、**一个 GL 上下文**、N 个原生窗口：每帧对每个窗口 `make_current → 画 → SwapBuffers`。
- **必须关 vsync**：多窗口串行 `SwapBuffers` 若开 vsync 会互相等待，帧率被拖到 ~10fps（已在 `fxtk_mw_init` 里关闭）。
- 画布/帧缓冲按 **窗口尺寸** 缓存：X11/Windows 都缓存 24 份；窗口尺寸变化会新建缓存。**不要逐帧改窗口尺寸**（会把缓存打爆 → sokol 校验 panic/闪烁）。
- 窗口位置：
  - 应用坐标是 **1280×720 逻辑舞台**；`win_place()` 把"客户区左上角"放到设计坐标；
  - **Win32 需要扣标题栏偏移**（`fxtk_mw_client_offset` 返回偏移）；
  - **X11 不要扣偏移**（`XMoveWindow` 传的就是客户区位置，框架由 WM 负责）——这一条踩过坑：给 X11 也减偏移会让窗口乱飞、叠成黑块。
- 窗口标题是 UTF-8：Win32 用 `CreateWindowExW`+UTF-16 转换 + `DefWindowProcW`；X11 用 `_NET_WM_NAME`。
- 关闭任意窗口 = 退出（`WM_CLOSE` / `WM_DELETE_WINDOW`）；`Esc` 也退出。

---

## 5. 已知坑（改之前先看）

1. **闪屏**：常见来源是（a）逐帧改窗口尺寸把画布缓存打爆（已改为只在目标变化时 resize）；（b）每帧重复 `XMoveWindow/SetWindowPos`（已缓存位置，未变则不动）；（c）多窗口 vsync 串行（已关）。再出现闪，先查这三处。
2. **dt 钳制**：`update()` 里 `dt>0.2s` 会被钳到 1/60；如果机器帧率很低（<5fps），窗口动画会按"帧数"推进而不是真实时间，看起来卡在半路（Ein 段在低性能机器上尤其明显）。重录时建议 `WEE_STATIC=1` 或提前把窗口动画时长调短。
3. **Ein 六窗段**：必须六窗同时可见、3×2 网格；某扇窗没出现先查 `ein_active()` 的强制显示分支。
4. **标题栏遮挡**：布局留了约 42px 的垂直间隙给系统标题栏；改尺寸/位置时不要挤掉这个间隙。
5. **颜色格式**：画布/交换链用 `SG_PIXELFORMAT_RGBA8`；不要试图改 BGRA（曾在 Wine 上黑屏）。
6. **sokol 校验 panic**：多窗口缓存数组曾是 8 槽，连续 resize 会写越界（`color attachment view has wrong type`），现为 24 槽；不要改小。
7. **字幕/台词模糊**：文本在窗口画布上按物理像素渲染；`g_sc` 是屏幕等比系数，录像时用屏幕原生分辨率即可。

---

## 6. 回归检查（每次改完跑一遍）

```bash
# 0) 编译（见 BUILD.md），然后：
pkill -x Xvfb; Xvfb :99 -screen 0 1920x1080x24 & sleep 1
# 1) 全片 QA：必须 exit=0、无残留窗口
DISPLAY=:99 WEE_NODRIFT=1 WEE_FPS=6 WEE_DUR=212 nice -n 19 \
  ./movie > /tmp/qa.log 2>&1; echo "exit=$?"; tail -2 /tmp/qa.log
# 2) 关键点截图：0s / 40s / 70s 重排 / 110s 换肤 / 160s 六国语言 / 205s 结尾
for t in 0.5 40 70.2 110.2 160 205; do
  DISPLAY=:99 WEE_T0=$t WEE_DUR=212 WEE_FPS=10 nice -n 19 ./movie & sleep 1
  ffmpeg -v error -f x11grab -video_size 1920x1080 -framerate 10 -i :99 -t 0.3 -y /tmp/f_$t.mp4
  pkill -x movie; sleep 0.2
done
# 3) Windows 路径（有 Wine 时）
WINEPREFIX=$PWD/.wineprefix WINEDEBUG=-all DISPLAY=:99 WEE_T0=150 \
  wine explorer /desktop=film,1920x1080 ./world_execute_me.exe
```

---

## 7. 成片合成（从两份录屏到 1080p）

两份素材：Linux 录屏（1920×1080，SimpleScreenRecorder）、Windows 录屏（1280×720，OBS；逻辑屏 1:1）。
**对齐方法**：用 110s 换肤闪白定位视频时间与歌曲时间的偏移。
- 本次素材：`O_LIN = 5.60s`，`O_WIN = 6.07s`（即 song_t = video_t - O）；
- 校验：拿任意一句歌词（如 60s 的 "If I can give you all the stimulations"）在两份素材里对帧，帧上播放器时间应都是 1:00。

剪辑规则（本次成片）：
1. 0–30s 用 Linux；
2. 30s 后**按歌词边界**在 Linux/Windows 之间交替（每次换源都在歌词起点切）；
3. `Ein Dos Trois Ne Fem Liu`（158.77–161.79s）**必须用 Windows**（六窗段 Linux 效果差）；
4. 结尾接 8s 片尾卡（声明 + 三个仓库地址）；
5. 音轨用仓库内原曲 `mv/audio/source.m4a`（212.33s），对齐 song_t=0；片尾补静音。

参考脚本（可重跑；`/tmp/mvcut/render.py` 思路）：
```
# 每段：[a,b) 按 O 换算源视频时间，抽段并统一到 1920x1080/60fps
ffmpeg -ss (a+O) -t (b-a) -i <source> -vf "scale=1920:1080:flags=lanczos,fps=60,setsar=1" \
  -c:v libx264 -preset veryfast -crf 18 -an seg_%03d.mp4
# 拼视频 + 补片尾 + 混音
ffmpeg -f concat -safe 0 -i concat.txt -c copy video_all.mp4
ffmpeg -i video_all.mp4 -i mv/audio/source.m4a \
  -filter_complex "[1:a]apad=pad_dur=9[a]" -map 0:v -map "[a]" \
  -c:v copy -c:a aac -b:a 192k -t 220.4 out.mp4
```

---

## 8. 发布与仓库

- 两个仓库分开：
  - **MV**：`leexamp/world.execute-me--fxtk`（分支 `feat/v2.5`，含 `mv/`）；
  - **fxtk-pc 框架**：`leexamp/fxtk-pc`（分支 `feat/multi-window`，**不含 mv**）。
- 框架改动只提交 `components/fxtk/` + `drivers/`；MV 改动只提交 `mv/` 与文档。
- **隐私规则**：不入库主机名、绝对路径、个人照片（`1.heic`）、聊天记录（`mv/pv/incoming/chatlog*`）、构建产物；
  `.gitignore` 已覆盖 `.Trash-0/.wineprefix/.dev_archive/.workbuddy/mv/1.heic/mv/pv/incoming/chatlog*` 与 v2.5 产物。
- 历史注意：`1.heic` 仍在旧的已推送提交 `f823e82` 里；要彻底从 GitHub 历史移除，需要 `git filter-repo` 后 force push。

---

## 9. 版本史对照（台词里的段落）

| 台词时间 | 段落 | 真实依据 |
|---|---|---|
| 0–29s | 远古 / fxtk-rt | ESP32 480×272、ST6201/GT911、SPI 40MHz→20MHz、0x0841、lvgl 放不下→自写 gfx 驱动 |
| 30–50s | v1.0 / v1.2 | 从 rt 搬上 PC；控件/画布/字体；驱动接口、控件树、自检 |
| 50–68s | v2.1 | RGB565→ARGB8888（`5511c69`）；同一天删 `linux_sim`（`7d3ecb1`） |
| 68s 第一声 execution | v2.3 | 控件池减半 16.4→14.1MB、字体缓存、61 项自检 |
| 82–105s | v2.4 | SDL→sokol、像素对照 <1%、`cec3b00` 滑杆残影修复 |
| 107–174s | v2.5（很短） | 多窗口后端、6 窗口编排、1080p 等比缩放、81 句画面；开发故事插在几声 execution 之间 |
| 190–212s | 结尾 | 从 fxtk-rt 到 v2.5；"窗口还开着" |
