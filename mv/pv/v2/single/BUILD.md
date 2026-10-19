# world.execute(me); — 桌面多窗口 MV (fxtk)

一份 `movie.c`，Linux / Windows 各编译一次，行为一致。所有画面由 fxtk 绘制，窗口是**系统真实窗口**。
设计稿 1280×720；启动时按屏幕**等比例缩放**（1920×1080 → 1.5×，2560×1440 → 2×，非 16:9 居中留边）。

- 歌曲：Mili — world.execute(me);（212.33s，音频不入库）
- 成片：`mv/pv/out/world_execute_me_final_1080p.mp4`（1920×1080 / 60fps / H.264 + AAC / 3:40，含片尾声明）
- 维护文档：[MAINTENANCE.md](MAINTENANCE.md)

## 文件

| 文件 | 说明 |
|---|---|
| `movie.c` | MV 程序：时间轴 / 台词 / 6 窗口编排 / 81 句歌词构图 / 生成艺术 |
| `script.inc` | 左栏台词（fxtk 版本史：远古/rt → v1.0 → v1.2 → v2.1 → v2.3 → v2.4 → v2.5） |
| `BUILD.md` | 本文件：文件、编译、运行、画面结构、录屏 |
| `MAINTENANCE.md` | 维护/二次开发：数据表、动画、后端、坑位、QA、成片合成、发布 |
| `movie` / `world_execute_me.exe` | 构建产物（不入库，本地保留） |

## 编译

依赖 fxtk 仓库本体（`components/fxtk`、`drivers`、`third_party/sokol`）。

### Linux（在 fxtk-pc 仓库根目录）

    gcc -DFXTK_MW -O2 mv/pv/v2/single/movie.c \
        drivers/fxtk_sokol_mw.c drivers/fxtk_sokol_driver.c \
        drivers/fxtk_font_stb.c drivers/fxtk_win.c components/fxtk/*.c \
        -Icomponents/fxtk -Idrivers -Ithird_party/sokol \
        -o mv/pv/v2/single/movie \
        -lX11 -lXi -lXcursor -lGL -ldl -lpthread -lm

### Windows（MinGW-w64；Linux 上交叉编译）

    x86_64-w64-mingw32-gcc -DFXTK_MW -DSOKOL_GLCORE -O2 \
        mv/pv/v2/single/movie.c \
        drivers/fxtk_sokol_mw.c drivers/fxtk_sokol_driver.c \
        drivers/fxtk_font_stb.c drivers/fxtk_win.c components/fxtk/*.c \
        -Icomponents/fxtk -Idrivers -Ithird_party/sokol \
        -o mv/pv/v2/single/world_execute_me.exe \
        -lopengl32 -lgdi32 -luser32 -lole32 -loleaut32 -luuid -lshell32 -lwinmm -mwindows

Windows 本机用 MinGW 编译时命令相同（去掉 `x86_64-w64-mingw32-` 前缀、路径换本机）。

## 运行

    ./movie                 # 静音, 默认 60fps, 从 0 秒播放
    WEE_T0=130 ./movie      # 从 130 秒预览
    WEE_FPS=30 ./movie      # 限帧
    WEE_NODRIFT=1 ./movie   # 窗口不随每句歌词漂移
    WEE_STATIC=1 ./movie    # 关闭所有窗口移动/轻推动画 (录屏最稳)
    WEE_AUDIO=/path/x.mp3 ./movie   # 需要声音时显式指定 (默认完全静音)

| 变量 | 默认 | 说明 |
|---|---|---|
| `WEE_DUR` | 212 | 总时长（秒） |
| `WEE_T0` | 0 | 起始秒 |
| `WEE_FPS` | 60 | 帧率上限 |
| `WEE_NODRIFT` | 0 | 1=关闭每句歌词的窗口漂移 |
| `WEE_STATIC` | 0 | 1=关闭所有窗口位移动画（录屏最稳） |
| `WEE_AUDIO` | 空 | 音频文件；不设置则完全静音 |

Windows：双击 `world_execute_me.exe`。**关掉任意窗口 = 退出**；两个平台都支持 **Esc 退出**。

## 画面结构

- **6 个真实窗口**：台词 `notes.txt` / 画面+频谱 `spectrum` / 终端 `cmd.exe` / 章节 `chapters` / 歌词 `player` / 通知 `notice`
- 尺寸（逻辑坐标）：台词 720×340（随行数长高）、画面 520×300、歌词条 1200×78、终端 720×140、章节 460×180、通知 360×88
- 台词 / 歌词 / 画面三扇窗**全片常开**；终端 / 章节 / 通知按章节开关
- **81 句歌词 = 81 行构图**：`LART[]` 每行 `{构图, 参数1, 参数2, 配色, 背景纹样}`，与 `LYR[]` 编译期对齐；`draw_motif()` 共 70+ 种构图（光标/电源正弦/保护罩/拼装块/圆/滑杆/星野/粒子/立方体/圆周/切线/限制条/无限/交直流/隔离/螺旋/隧道/时间轴/双圆/分辨率/叠正弦/心形/execution 条/故障/片头卡/过门/微笑/门/数字/尾卡/神光/茄子/番茄/猫/开发板/显示器/窗口/按钮/键盘/USB/发电机/手机/完成环/时钟…）；画面标题即当前歌词，左下角完整歌词、右下角 `LINE n/81`
- 左栏台词讲 **fxtk 版本史**（远古/rt → v1.0 → v1.2 → v2.1 → v2.3 → v2.4 → v2.5）；v2.5 写得很短，开发故事插在几声 `execution` 之间；章节窗口列各版本里程碑
- **Ein Dos Trois Ne Fem Liu 段（158.77–161.79s）**：六扇窗移动到 3×2 卡片阵列、各显示一个词，进出各闪白一次
- `11 TANGENTS` / `23 EXECUTION` 段用 fxtk 的 raymarch 钩子跑**真正的片元着色器光线步进**
- 70s、110s 两次整体重排；110s 闪白 + 深色切浅色
- 结束：196s 后只剩台词+歌词；208s 台词变暗；212s 自动退出

## 录屏（1920×1080）

1. 1080p 屏上舞台正好铺满，OBS 录全屏 / 显示器采集即可；输出 60fps。
2. 无 OBS 时 ffmpeg（Linux X11）：

       DISPLAY=:0 ffmpeg -f x11grab -video_size 1920x1080 -framerate 60 \
           -i :0.0 -t 213 -c:v libx264 -preset veryfast -crf 18 -pix_fmt yuv420p out.mp4

   Windows：OBS，或 `ffmpeg -f gdigrab -i desktop -framerate 60` 后裁剪。
3. 默认静音；要边录边听可 `WEE_AUDIO=song.mp3 ./movie`（录屏建议关掉）。
4. 建议 `WEE_NODRIFT=1`，最稳可再加 `WEE_STATIC=1`；212s 自动退出，中途 Esc / 关窗也会退出。

## 窗口消息

- Linux：Xlib `XPending/XNextEvent`；`WM_DELETE_WINDOW` / Esc / DestroyNotify → 退出
- Windows：Win32 `PeekMessageW/DispatchMessageW`；`WM_CLOSE/WM_DESTROY/ESC` → 退出
- 两个平台统一走 `fxtk_win` 后端 vtable，应用层不碰 X11/Win32 API
