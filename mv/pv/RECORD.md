# RECORD —— lyric 素材重录（确定性离线逐帧渲染）

> 目标：把 43 个手写歌词场景的素材从"抓屏"改成"应用自己按固定 tick 逐帧落盘"，
> 彻底解决播放时的"卡"。本文件记录做法、实测数字与复现方式。

## 1. 旧素材为什么"卡"（实测，不是猜）

旧流程 \`23_capture_lyric.sh\`：\`Xephyr\` + \`xdotool\` + \`ffmpeg -f x11grab -framerate 60 -t 3\`
—— 抓一个跑在软件 GL(llvmpipe) 上的窗口。**应用帧率与抓帧节奏是两套时钟**：

| 观测 | 数值 | 方法 |
|---|---|---|
| 应用交互模式真实帧率 | 40 ~ 70 fps（抖） | \`FXTK_STAT=1\` 跑 7s，驱动每秒打印 fps=70/49/40/45/55 |
| 抓帧节奏 | 固定 60 fps | \`ffmpeg -f x11grab -framerate 60\` |
| 旧素材相邻帧重复率（全 43 场景） | **75.9%**（5840/7697 对） | 逐像素 \`mean|Δ|<0.1\`（见下"判据"一节） |
| 旧素材 sine 每帧实际前进 | **0.438 tick/帧**（101 帧 0 tick + 78 帧 1 tick 的双峰） | 正弦波形相位法（对波形做 220px 周期的相位解缠） |
| 新素材 sine 每帧前进 | **1.006 ± 0.018 tick/帧**（179/179 对都恰好 1） | 同上 |

即：旧素材在连续动画场景里约一半的帧是上一帧的复制（偶发还有跳帧），60fps 播放时
看起来就是"一跳一跳"。逐场景（旧，\`mean|Δ|<0.1\` 判据）：

    sine 55.9%   travel 49.2%   loove 52.5%   simulation 60.3%   trance 58.7%
    execx4 90.5% (该场景本来就每 6 tick 才变一次)   powerline 98.9%   ready 100%(静止场景)

**判据很重要**：旧素材是 h264(crf16) → 再转 jpg 的，编码噪声会让"内容完全相同"的两帧
逐字节不同 —— 用 md5 统计旧 sine 得到 0.0% 重复（假的）；换成逐像素 \`mean|Δ|<0.1\`
得到 55.9%，与独立的相位法 56.4% 互相印证。所以重复率一律用像素判据。

## 2. 新方案：应用自己按固定 tick 逐帧落盘

### 2.1 lyric.c 新增开关（只加开关，不改框架一行）

| 位置 | 内容 |
|---|---|
| L13–L19 | 文件头说明（用法/默认帧数） |
| L23 | \`#include <GL/gl.h>\`（构建已链 -lGL） |
| L26–L29 | \`STB_IMAGE_WRITE_STATIC\` + \`STB_IMAGE_WRITE_IMPLEMENTATION\` + \`#include "vendor/stb/stb_image_write.h"\`（仓库里已有那份；fxtk_backends.c 那份开了 \`STBI_WRITE_NO_STDIO\`，所以这里自己编一份 stdio 版，static 符号不冲突） |
| L54–L72 | 落盘状态：\`DUMP_W 1280 / DUMP_H 720 / DUMP_WARMUP 4 / DUMP_DEF_FRAMES 900\`，\`s_dump_n\`(FX_FRAMES)、\`s_dump_i\`、\`s_dump_warm\`、\`s_dump_tick\`、\`s_dump_dir\`、缓冲区 |
| L811 | \`SCENES[s_scene].fn(cw, ch, s_dump_n > 0 ? s_dump_tick : s_tick)\` —— dump 模式下 tick 由落盘循环显式给定，不看墙钟、不看 vsync |
| L820–L851 | \`dump_write_png()\`：\`glReadBuffer(GL_BACK)\` + \`glPixelStorei(GL_PACK_ALIGNMENT,1)\` + \`glReadPixels(0,0,1280,720,GL_RGBA,GL_UNSIGNED_BYTE,…)\`，**逐行倒着取**（GL 原点在左下，PNG 第 0 行必须是最上面一行），只取 RGB 用 \`stbi_write_png(...,3,...)\` 写 \`<dir>/%04d.png\` |
| L853–L865 | \`dump_step()\`：先主动调一次 \`fxtk_sokol_frame(1280,720)\` 把本帧命令提交掉（此刻 GL 上下文是当前的），预热 4 帧不落盘，之后每落一帧 \`s_dump_i++\`、\`s_dump_tick = s_dump_i\`，写完第 N 帧 \`exit(0)\` |
| L867–L882 | \`fxtk_app_init()\` 里读 \`FX_DUMP\`(目录) / \`FX_FRAMES\`(帧数，缺省 DUMP_DEF_FRAMES=900)；两个都不给 = 完全不变的老交互模式 |
| L894–L900 | \`fxtk_app_frame()\`：dump 模式走 \`dump_step()\`，否则照旧 \`fx_repaint()\` |

**为什么 tick 是确定性的**：\`app_frame_cb\` 一帧的顺序是
\`fx_poll()\`(画本帧，用 tick) → \`fxtk_app_frame()\`(我们在这里提交+回读) → \`fxtk_sokol_frame()\`(驱动再贴一次，
此时命令队列已空，只是同一张离屏画布再整屏 blit 一次，画面不变)。第 k 帧渲染用的就是 tick=k，
与真实时间、与 vsync 抖动完全无关 —— 严格 1 tick/帧，无重复、无跳帧。

### 2.2 为什么还需要一个 X server

sokol_app 在本仓库只有 X11/GLX 后端，没有 headless：\`env -u DISPLAY ./lyric_sim\` 直接
SIGABRT/core dump（核心转储）。所以脚本**先试无 X**，跑不通再退回一个固定 1280x720、
无窗口管理器的 \`Xephyr :98\` —— 它只负责提供 GL 上下文，**画面不经过它抓取**（x11grab 已删除）。

### 2.3 23_capture_lyric.sh 流程（每个场景）

    FX_SCENE=i FX_DUMP=/tmp/lydump/<name> FX_FRAMES=<N> ./lyric_sim   # 无 Xephyr/xdotool/ffmpeg 抓屏
    ffmpeg -start_number 1 -framerate 60 -i /tmp/lydump/<name>/%04d.png -vf scale=960:540 -q:v 3 \
           footage/lyric_frames/<name>/%03d.jpg          # 页面实际用的序列（覆盖旧素材）
    ffmpeg … -c:v libx264 -preset veryfast -crf 16 -pix_fmt yuv420p footage/lyric/<name>.mp4   # 留档

打印每场景：帧数 / 相邻重复帧数 / 耗时 / 有效 fps。

## 3. 验证（数字）

### 3.1 方向 —— 不是上下颠倒

dump 一帧后用两种办法确认：

* 图像顶部 0–38 行均值 ≈ 236（浅色标题带 "fxtk sine (12/43)"），第 39 行起骤降到 ~17（深色画布），
  末尾 5 行又是 236（画布下方露出的背景）—— 与 1280x720 窗口里 label(逻辑 y 2..13) / canvas(15..270) 的布局一致；
  场景内文字 "If I'm a sine wave" 在画布顶部、"逐点 fxtk_put_px()…" 在画布底部，都是正的。
* 与旧素材（正确朝向）同一场景比：直接比对 MAE=3.75，上下翻转后 MAE=24.25 → 方向正确。

### 3.2 平滑度（关键证据）

同一场景新旧对比（\`mean|Δ|<0.1\` 判据；新素材 PNG 无损，同判据下重复率只会更低）：

| 场景 | 旧（x11grab→jpg） | 新（dump→jpg） | 旧每帧前进 | 新每帧前进 |
|---|---|---|---|---|
| sine | **55.9%** 重复 | **0.0%** | 0.438 tick（0/1 双峰） | 1.006 ± 0.018 tick |
| travel | **49.2%** 重复 | **0.0%** | — | 1 tick（179/179 对） |
| loove | 52.5% 重复 | 0.0% | — | 1 tick |
| simulation | 60.3% 重复 | 0.0% | — | 1 tick |
| 全 43 场景 | 75.9% 重复 | 见 §3.5 | — | — |

注：powerline / protection / ready 这类场景本身就是静止或每 30 tick 才变一次，
新素材在这些场景同样是高重复率（97% / 100%），**这是内容本身如此，不是卡** ——
判断"卡"要看持续动画场景（上表前四行）。

### 3.3 帧率 / 耗时

单场景 180 帧：17.0 ~ 21.9s（含进程启动、渲染、glReadPixels 回读、PNG 编码），
即 **≈ 8.2 ~ 10.6 帧/秒**；43 场景整批 ≈ 15 ~ 25 分钟（单进程串行）。

### 3.4 抽帧拼图

\`/tmp/lyric_3scenes_3frames.png\`：sine / travel / loove 各取第 1、60、120 帧拼成 3x3，
画面正常、文字正立、三帧之间有明确推进（正弦波右移 + 红点前进 / 时间轴刻度滚动 / 心形旋转）。

### 3.5 43 场景完整性 / 素材体积 / dbg2

整批录制结束后的实测（落盘素材：43 个场景 × **900 帧**，960x540，-q:v 3）：

* **完整性**：43/43 场景成功，异常 0；每场景 `001.jpg … 900.jpg` 连续无缺帧，帧总数 38700。
* **规格**：所有 `001.jpg` 均为 960x540；43 个 mp4，每个时长 15.0s（900 帧 @60fps）。
* **逐字节重复率（无损 PNG，md5）**：前 180 帧（页面实际用到的窗口）**2978/7697 = 38.7%**，
  全量 15085/38657 = 39.0%。**这些重复全部来自"内容本身静止/按 tick 量化"的场景**：
  | 场景 | 新重复率 | 旧重复率(mean判据) | 说明 |
  |---|---|---|---|
  | completion / dimension / protection / ready / satisfaction | 100% | 100% | 场景函数本身与 tick 无关（静止画面） |
  | execution1 | 96.6% | 96.6% | 每 26 tick 划掉一行 → 内容本身 96.2% |
  | numbers | 96.1% | 97.8% | 每 24 tick 换高亮 → 内容本身 95.8% |
  | powerline | 97.2% | 98.9% | 每 30 tick 开关一次 → 内容本身 96.7% |
  | pieces / creation / challenge / stimulations | 92~98% | 95~100% | 动画只在前 ~100 tick 内发生，之后静止 |
  | illegal | 87.7% | 93.3% | 每 8 tick 换行 → 内容本身 87.5%（**已等于内容本身，旧的多出来的是抓屏重复**） |
  | execx4 | 83.8% | 90.5% | 每 6 tick 变一次 → 内容本身 83.3%（同上） |
  | points | 66.5% | 99.4% | 每 3 tick 加一个点 → 内容本身 66.7%（同上） |
  | tabbycat / blind | 86.6% / 41.3% | 92.2% / 68.7% | 呼吸/瞳孔位移不足 1px 时的取整量化 |
  | **sine/travel/loove/simulation/trance/vibrations/deeply/dizzy/acdc/circle/tangents/tomato/trapped/onlygod/fragments/isolation/limitations/parameters/stom/haveyouback/infinity/eggplant** | **0.0%** | 43~100% | 持续动画场景：新素材每一帧都不同 |
* **旧 vs 新（严格判据：变化像素 >12 的个数，≤8 视为"完全重复"）**：
  sine 28.5% → **0.0%**；loove 52.5% → **0.0%**；trance 52.0% → **0.0%**；vibrations 38.5% → **0.0%**。
  （旧素材是 h264+jpg 的，编码噪声会把"真重复"顶成"有变化"，所以 28.5% 是**下界**；
  同一判据换到无损 PNG 的新素材上就是精确值。）
* **每帧前进**：新 sine = **1.005 ± 0.018 tick/帧**，180/180 对都恰好 1 tick（旧 0.438 tick/帧，0/1 双峰）。
* **体积**：`footage/lyric_frames` 1.3 GB（38700 帧 960x540），`footage/lyric` 25 MB（43 段 mp4）。
* **页面自检**：`cd mv/pv/web && node dbg2.mjs` → t=0/3/8/20/40/70/100/130/160/190/205 **全部 ok**，无 PAGEERROR。
* 复查脚本与分析口径见 `/tmp/verify_final2.sh`、`/tmp/analyze_motion.py`、`/tmp/dupstats.py`
  （与谁执行录制无关，对最终落盘序列重跑即可复核）。

## 4. 复现

    bash mv/pv/lyric_app/build.sh                       # 一键构建（未改，仍可用）
    bash mv/pv/scripts/23_capture_lyric.sh              # 43 场景（脚本名以仓库当前版本为准）
    FX_SCENE=11 FX_DUMP=/tmp/lydump/sine FX_FRAMES=180 ./mv/pv/lyric_app/lyric_sim   # 单场景手跑
    cd mv/pv/web && node dbg2.mjs                       # 页面自检（应当全 ok）

## 5. 坑与注意

1. **判据**：比较旧（有损）素材的重复率必须用像素近似判据；md5 会被编码噪声骗过。
2. **翻转**：\`glReadPixels\` 读回的第 0 行是帧缓冲最下面一行，写 PNG 前必须翻过来。
3. **预热**：框架前 3 帧强制全屏重绘、字体图集首次生成，落盘前丢掉 4 帧（\`DUMP_WARMUP\`），
   否则第 1 帧可能是半成品。
4. **静态场景高重复率是正常的**：不要拿 powerline/ready 的重复率判断流畅度。
5. **180 帧与页面**：页面帧号公式是 \`12 + floor((t-t0)*60)\`，clamp 到 12..180，
   所以页面只用得到前 180 帧；录更多帧（如 900）是超集，前 180 帧内容完全一致，
   不会影响现有页面。
6. **并发提醒**：本次重录期间有另一个会话在同一份 footage/lyric_frames 上跑 900 帧/场景的变体
   （同一套 lyric.c 开关，脚本会整体覆盖、无跳过逻辑）；本文档第 3.5 节验证的就是它最终落盘的
   900 帧素材。验证脚本与分析口径与"谁执行录制"无关：对最终落盘的序列重新统计即可复核。
   如果之后又出现 `.new-*` 之类的隐藏目录或分辨率变化（例如 800x450 的重编码），
   以仓库当前实际文件为准；重复率/位移结论是在 1280x720 无损 PNG 上统计的，与 jpg 分辨率无关。
