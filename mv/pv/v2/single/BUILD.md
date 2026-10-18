# world.execute(me); — 桌面多窗口 MV (fxtk)

一份 `movie.c`，Linux / Windows 各编译一次，行为一致。所有画面由 fxtk 绘制，窗口是系统真实窗口。
设计稿 1280×720，启动时按屏幕**等比例缩放**（1920×1080 → 1.5×，2560×1440 → 2×，非 16:9 居中留边）。

## 文件

| 文件 | 说明 |
|---|---|
| movie.c | MV 程序 (时间轴 / 台词 / 场景 / 81 行歌词构图表) |
| script.inc | 左栏台词 (fxtk 版本史: 远古/rt → v1.0 → v1.2 → v2.1 → v2.3 → v2.4 → v2.5) |
| movie / world_execute_me.exe | 编出来的程序 |

## 编译

### Linux

    gcc -DFXTK_MW -O2 movie.c \
        drivers/fxtk_sokol_mw.c drivers/fxtk_sokol_driver.c \
        drivers/fxtk_font_stb.c drivers/fxtk_win.c components/fxtk/*.c \
        -Icomponents/fxtk -Idrivers -Ithird_party/sokol \
        -o movie -lX11 -lXi -lXcursor -lGL -ldl -lpthread -lm

### Windows (MinGW-w64)

    gcc -DFXTK_MW -DSOKOL_GLCORE -O2 movie.c \
        drivers/fxtk_sokol_mw.c drivers/fxtk_sokol_driver.c \
        drivers/fxtk_font_stb.c drivers/fxtk_win.c components/fxtk/*.c \
        -Icomponents/fxtk -Idrivers -Ithird_party/sokol \
        -o world_execute_me.exe \
        -lopengl32 -lgdi32 -luser32 -lole32 -loleaut32 -luuid -lshell32 -lwinmm -mwindows

## 运行

    ./movie                 # 静音, 默认 60fps, 从 0 秒播放
    WEE_T0=130 ./movie      # 从 130 秒预览
    WEE_FPS=30 ./movie      # 限帧
    WEE_NODRIFT=1 ./movie   # 窗口不随每句歌词漂移 (录屏更稳)
    WEE_AUDIO=/path/x.mp3 ./movie   # 需要声音时才显式指定音频 (默认不播任何声音)

| 变量 | 默认 | 说明 |
|---|---|---|
| WEE_DUR | 212 | 总时长 |
| WEE_T0 | 0 | 起始秒 |
| WEE_FPS | 60 | 帧率上限 |
| WEE_NODRIFT | 0 | 1=关闭窗口漂移 |
| WEE_STATIC | 0 | 1=关闭所有窗口移动/轻推动画 (录屏最稳) |
| WEE_AUDIO | 空 | 音频文件; 不设置则完全静音 |

Windows: 双击 `world_execute_me.exe`。**关掉任意窗口 = 退出**; 两个平台都支持 **Esc 退出**。

## 画面结构

- 6 个真实窗口: 台词 `notes.txt` / 画面+频谱 `spectrum` / 终端 `cmd.exe` / 章节 `chapters` / 歌词 `player` / 通知 `notice`
- 台词窗 720×340、字号 20（随台词行数长高），歌词条 1200×78、字号 32，画面窗 520×300
- 台词、歌词、画面三扇窗全片常开；终端/章节/通知按章节开关
- **81 句歌词 = 81 行构图**：`LART[]` 表每行 `{构图, 参数1, 参数2, 配色}`，与 `LYR[]` 编译期对齐；
  `draw_motif()` 实现 30 种构图（光标/电源正弦/保护罩/拼装块/圆/滑杆/星野/粒子/立方体/圆周/切线/
  限制条/无限/交直流/隔离/螺旋/隧道/时间轴/双圆/分辨率/叠正弦/心形/execution 条/故障/片头卡/过门/
  微笑/门与离开/数字/尾卡/神光）；画面标题即当前歌词，左下角完整歌词，右下角 `LINE n/81`
- 左栏台词讲 fxtk 版本史（远古/rt → v1.0 → v1.2 → v2.1 → v2.3 → v2.4 → v2.5）；v2.5 写得很短，
  开发故事插在几声 `execution` 之间；章节窗口列出各版本里程碑
- `11 TANGENTS` / `23 EXECUTION` 段用 fxtk 的 raymarch 钩子跑真正的片元着色器光线步进
- 70s、110s 两次整体重排；110s 闪白 + 深色切浅色
- 结束：196s 后只剩台词 + 歌词条；208s 台词变暗；212s 自动退出

## 录屏 (1280×720 或屏幕原生分辨率)

1. 程序自动按屏幕等比例缩放；1080p 屏上舞台正好铺满，录全屏即可；只录 1280×720 时按
   `(屏宽-1280×scale)/2` 取居中区域。
2. OBS：来源用「窗口采集」逐个加窗口，或「显示器采集」+ 裁剪；输出 60fps。
3. 无 OBS 时 ffmpeg（Linux X11，1920×1080 屏、舞台铺满）：

       DISPLAY=:0 ffmpeg -f x11grab -video_size 1920x1080 -framerate 60 \
           -i :0.0 -t 213 -c:v libx264 -preset veryfast -crf 18 -pix_fmt yuv420p out.mp4

   Windows：OBS，或 `ffmpeg -f gdigrab -i desktop -framerate 60` 后裁剪。
4. 默认静音（不 autoplay）；要边录边听可 `WEE_AUDIO=song.mp3 ./movie`，录屏时建议关掉。
5. 录屏建议加 `WEE_NODRIFT=1`；212s 自动退出，中途 Esc / 关窗也会退出。

## 窗口消息

- Linux: Xlib `XPending/XNextEvent`; `WM_DELETE_WINDOW` / Esc / DestroyNotify → 退出
- Windows: Win32 `PeekMessageW/DispatchMessageW`; `WM_CLOSE/WM_DESTROY/ESC` → 退出
- 两个平台都统一走 `fxtk_win` 后端 vtable, 应用层不碰 X11/Win32 API