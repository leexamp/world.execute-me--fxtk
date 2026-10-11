# world.execute(me); — fxtk 复刻版 MV

> 用真实 fxtk v2.5 渲染器逐帧合成的一支 MV，含 PC 模拟器、歌词逐句对齐、TUI 舞台与终端叙事。
>
> An MV assembled frame-by-frame from the real fxtk v2.5 renderer: PC simulator, line-aligned lyrics, a TUI stage and terminal storytelling.

---

## 是什么

这是一支以 **Mili《world.execute(me);》** 为音轨的**非官方技术演示 MV**。画面不是剪辑、不是 AI 生成视频，而是把一台**真实跑起来的 fxtk 窗口**录下来，再把录屏、终端输出、逐句歌词和一段 TUI 舞台叙事放进同一个 HTML 页面里，**逐帧算出来**的。

- 主角是 **fxtk** —— 一个 377KB 单文件、零第三方动态库的 C GUI 框架（本仓库根目录即其源码，MIT）。
- 片中的 fxtk 画面来自**真实的 PC 模拟器**：mv/pv/rt_sim/（最初的 fxtk-rt@48c2ef9 + 自写 SDL2 模拟器）与 mv/pv/lyric_app/（用仓库内 v2.5 框架编译的 43 场景应用）。
- 整片 1920×1080 / 30fps / 217.33s，**每一个像素都是确定性函数在时间 t 上算出来的**。

## 怎么做的 / How it is made

1. **每个镜头是 t 的纯函数。** mv/pv/web/mv.html 里每个场景都有 update(t)，并对外暴露 window.renderFrame(t)；同一个 t 渲染多少次结果都一样，页面运行时不联网、不用 AI。
2. **Playwright 逐帧截图。** mv/pv/web/render.mjs 驱动无头 Chromium，按 [t0, t1) 区间逐帧截图：1080p 走 JPEG q96，720p 走 PNG。
3. **fxtk 场景 app 提供素材。** mv/pv/lyric_app/lyric.c 用 FX_SCENE=0..42 选场景（**43 个场景**）；每个场景在 Xephyr 里以 60fps 录制 **900 帧**（15.0s）到 footage/lyric_frames/<name>/，作为片中的真实录屏。
4. **分块编码再拼接。** scripts/22_render_1080p.sh 每 600 帧一块（20s）→ libx264 -crf 14 编码 → ffmpeg concat 无损拼接 → 挂上对齐好的音轨 audio/source_aligned.m4a。
5. **时间轴只有一条。** 字幕 lyricAt(t)、章节 secAt(t)、场景 sceneOf(t) 全部走同一个 renderMain(t) 入参，三者在同一时刻。

> 诚实提示：本片**没有**用补帧伪造流畅，**没有**演多窗口管理 / Wayland 后端 / 多点触控 / SVG 圆弧，**没有**宣称 1000fps；所有终端输出都由 scripts/10_forensics.sh、11_forensics2.sh 真实跑出来后原样放进画面，原文见 mv/pv/forensics/。逐条自检见 mv/pv/README.md。

## 目录结构 / Layout

    mv/pv/
      web/            mv.html（画面合成，renderFrame(t)）+ render.mjs（Playwright 逐帧截图）
                      assets.json（素材/节拍表）+ 调试脚本
      scripts/        01_align → 03_assets → 04_subs → 05_beats → 06_refs
                      07_restore_rt → 08_capture_rt → 10/11_forensics
                      20_render（720p）→ 21_rerender → 22_render_1080p → 23_capture_lyric
      lyric_app/      lyric.c（43 场景 fxtk 应用）+ build.sh（编译出 lyric_sim）
      rt_sim/         sim_driver.c + stubs/ + orig/（fxtk-rt@48c2ef9 原样）+ build.sh
      lyrics/         source.lrc（公开 LRC 文本）+ timeline.json + beats.json
      tools/          fxtk_studio.py（Xephyr 录制封装）等
      assets/char/    maid_whale.png（DeepSeek娘立绘，CC BY-NC-SA 4.0）
      forensics/      scripts/1[01]_forensics*.sh 的真实命令输出
      out/            成片与字幕（*.mp4 不入库；REPO_README.md 即本文件）
      ALIGN.md / TOKENIZER.md / CREDITS.md / SCRIPT.md / RECORD.md / IMPLEMENTATION.md

    mv/README.md、STORYBOARD.md、CONTEXT.md、HISTORY.md、COLLAB.md、PROMPT.md   制作文档

**未入库的大体积内容**（见根 .gitignore）：mv/pv/footage/（约 1.9GB 录屏帧）、mv/pv/build*/（中间帧/分块）、mv/pv/out/*.mp4、原曲音频 mv/pv/audio/*.m4a、mv/pv/assets/bg/（>5MB 壁纸）、依赖与二进制产物。

## 如何复现 / Reproduce

前置：ffmpeg、python3（PIL/numpy）、node + playwright-core + Chromium、gcc、Xephyr/xdotool（录制素材用）。

    # 0) 依赖（仅页面截图需要）
    cd mv/pv/web && npm install        # 得到 playwright-core

    # 1) 构建两个模拟器
    bash mv/pv/rt_sim/build.sh         # 最初的 fxtk-rt@48c2ef9 + SDL2 模拟器
    bash mv/pv/lyric_app/build.sh      # v2.5 框架编译出 lyric_sim（可 FX_SCENE=0..42 单场景预览）

    # 2) 自备音轨：把原曲放到 mv/pv/audio/source.m4a（版权原因不入库），
    #    再按 ALIGN.md 生成 mv/pv/audio/source_aligned.m4a（前导 4.89s 静音）。

    # 3) 录制素材（需要 Xephyr/xdotool/ffmpeg；产出 footage/lyric_frames/<name>/，43 场景 × 900 帧）
    bash mv/pv/scripts/23_capture_lyric.sh
    python3 mv/pv/scripts/03_assets.py     # 重新生成 web/assets.json

    # 4) 起本地静态服务（22 脚本也会自动拉起）
    cd mv/pv && python3 -m http.server 8901 --bind 127.0.0.1 &

    # 5) 一条命令出 1080p 成片
    bash mv/pv/scripts/22_render_1080p.sh  # -> mv/pv/out/world_execute_me_1080p.mp4

> 素材帧序列（footage/）未入库，因此**仅凭本仓库不能一键出片**；补上 source.m4a 与录屏素材后，渲染链本身是可复现的。
>
> The footage/ frame sequences are not shipped (too large); with the song and the recorded frames supplied, the render chain is reproducible.

## 歌词与音频对齐 / Lyric–audio alignment

- 成片音轨 mv/pv/audio/source_aligned.m4a = **4.89s 前导数字静音 + 原曲 + 尾部补到 217.34s**。
- 这样片源**第一个音符正好落在成片 t = 5.00s**（冷开场结束、切主界面那一刻）。旧文件 source_pad.m4a 把 5s 静音补在了**文件末尾**，会让音乐早 4.89s 起、字幕整体晚约 4.9s——已被弃用，不要再用它合片。
- 数值口径：真实人声在片源时间轴上 = t_timeline + 0.10s；片源自带 0.11s 静音，故 LYOFF = 5.0 + 0.10 − 0.11 = 4.99s，COLDEND = 5.0s。
- 生成命令与逐 10ms 能量 / onset / 回归实测（前 20 句、53 句、逐段残差 rms 0.44s）全部记录在 **mv/pv/ALIGN.md**。
- 歌词文本来自公开 LRC，**一个字没改**，只改起始时间；逐句残差 ±0.4s（最大 0.88s）仍存在，是已知取舍。

## token 条说明 / Token strip

片中歌词带每个词下方显示的是**真实 tokenizer 输出**的 token ID（0x + 大写 hex，最多 3 片，超出折成 +N；行尾给「本行 N tok · tokenizer: <页面名>」）。口径如下，页面每个词格的悬停 title 也写明了同一件事：

| 页面显示名（剧情里的虚构模型名） | 真实来源词表 | vocab | 来源 |
|---|---|---|---|
| deepseek-v4-flash-0731 | **DeepSeek-V4 系列共享词表**（V4-Pro / V4-Flash 共享） | **129280** | AndersonBY/deepseek-tokenizer @ 525044c24386953b542ef5262d0c69d309a5d67f |
| deepseek-v4.1-flash | **deepseek-ai/DeepSeek-V3（替身）** | 128815 | huggingface.co/deepseek-ai/DeepSeek-V3 |

**必须澄清的三点：**

1. deepseek-v4-flash-0731 / deepseek-v4.1-flash 是页面剧情里的**虚构模型名**，**不是词表本名**；
2. deepseek-v4.1-flash 因 **v4.1 词表未公开，暂用 deepseek-ai/DeepSeek-V3 替身**——**不要把它写成「官方 v4.1 词表」**；
3. V4 与 V3 **共享 128000 BPE 基表**（V4 多出的只是 added special tokens），故**在本片语料上两者逐片 ID 完全相同**（87 行 / 442 片全等）。

编码口径：按空白切词，每个词编码输入「空格 + word」（词前带空格），add_special_tokens=False；结果预计算成静态表 RB_IDS / RB_TOKS 嵌入 mv.html，运行时零网络、零哈希。完整实测、抽样表与验收数字见 **mv/pv/TOKENIZER.md**。

## 版权与致谢 / Credits & licensing

- **音乐**：world.execute(me); — **Mili**（专辑《Miracle Milk》）。**词曲版权归 Mili / 原作者所有**；本仓库仅作技术演示，**音频与视频不作为开源许可（MIT）的一部分**，请勿二次传播或商用。
- **角色形象**：《**DeepSeek娘**》(女仆装鲸鱼娘)，作者 **@ZipZipPipe**（来源：萌娘百科条目原图，去水印 / 裁边）。按 **CC BY-NC-SA 4.0**（署名—非商业性使用—相同方式共享）使用。
- **fxtk**：源码来自原作者项目 **leexamp/fxtk-pc**（本仓库根目录即该项目，MIT）；mv/pv/rt_sim/orig/ 取自 **leexamp/fxtk-rt @ 48c2ef9**，**一个字节未改**（见 mv/pv/rt_sim/PROVENANCE.md）。本仓库对 fxtk 源码**只做构建 / 使用，未修改其源码**。
- **歌词**：文本来自公开 LRC，版权属于 **Mili / momocashew**。
- **第三方工具**：Xephyr / X.Org、ffmpeg、Playwright + Chromium、Python PIL / numpy、文泉驿与 DejaVu 字体等，各自遵循其原始许可。
- **素材来源与风险**的完整记录见 **mv/pv/CREDITS.md**。

## 免责声明 / Disclaimer

**非官方同人作品**，与 Mili、DeepSeek 官方均无关联。片中含「AI 合成 / 程序化生成」内容；投稿到平台时请按要求勾选相应标注。本仓库按 **MIT** 发布**代码与文档**，但**不覆盖**其中的音乐、角色立绘与由其衍生的音视频。

---

## 成片获取 / Where is the film

成片 world_execute_me_1080p.mp4 约 **243MB**，超出普通 Git 仓库的合理体积，**未纳入版本管理**。建议通过 **GitHub Releases** 单独上传发布（或按「如何复现」自行渲染）；**不要**用 git add 直接提交视频。
