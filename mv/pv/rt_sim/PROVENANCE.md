# 最初的 fxtk —— 从 git 历史复原 + 自写 PC 模拟器

## 这是什么

`mv/pv/rt_sim/orig/` 里的每一个文件都是从 **GitHub `leexamp/fxtk-rt`** 的
**提交 `48c2ef9`（2026-08-06，"+""Add files via upload""+"）** 原样取出的，
**一个字节都没有改**（`scripts/07_restore_rt.sh` 可复现这次取出）。

那一版是 **fxtk 最早的形态**：只有 ESP-IDF 工程，**没有任何 PC 端代码/模拟器**：

| 原始文件 | 作用 |
|---|---|
| `components/fxtk/fxtk.c / fxtk_draw.c / fxtk_font.c / fxtk_widgets.c` | GUI 库本体（控件池 / 布局 / 光栅化 / 字体） |
| `components/fxtk/fxtk_st6201.c` | 硬件屏驱动：480×272 ST6201，4 线 SPI 直写 |
| `components/fxtk/fxtk_touch.c` | 触摸驱动：GT911，硬件 I2C |
| `components/fxtk/cn_gray.c / ascii_gray16.c` | 字库（7445 汉字 + Arial 16px 灰度） |
| `main/app.c` | 演示应用（3 页标签：波形 / 图形 / 控件），入口 `app_main()` |

## 模拟器怎么来的（只换硬件层）

`sim_driver.c` 提供和真机**同名同接口**的两样东西，应用代码不用动：

- `fx_driver_t fx_st6201_driver` —— 原来往 SPI 推像素，现在写进 480×272 的 RGB565 内存帧缓冲，再用 SDL2 上屏；
- `fx_gt911_init()` / `fx_gt911_read()` —— 原来读 GT911，现在读鼠标；
- `stubs/` 里两个头文件把 `esp_log.h` 与 `freertos/` 补上，`vTaskDelay()` 顺便负责"画这一帧"。

所以：**画面里每一个像素都是 48c2ef9 那份原始代码自己算出来的**，
包括那个年代的毛病 —— 左上角的触摸调试字、拖影、黑缺口、闪烁都还在，没有修。

## 跑

```bash
bash mv/pv/rt_sim/build.sh
DISPLAY=:98 SIM_SCALE=1 ./mv/pv/rt_sim/fxtk_rt_sim      # 需要一台 X（我这边用 Xephyr）
SIM_DUMP=/tmp/frames SIM_DUMP_EVERY=1 ./fxtk_rt_sim      # 可选: 逐帧导出 PPM
```

> 原始工程用 ESP-IDF 构建（`idf.py build`）；这里只是把 `fxtk_st6201.c` / `fxtk_touch.c`
> 两个**硬件驱动**换成模拟器实现，**库和 UI 的源码保持原样**。

## 已经跑通了（2026-10-11）

```bash
bash mv/pv/rt_sim/build.sh              # -> fxtk_rt_sim
bash mv/pv/scripts/08_capture_rt.sh     # Xephyr + xdotool 真实事件, 录成 footage/rt_orig_*.mp4
```

- 三个页面全部真的能切：**波形 / 图形 / 控件**（3 页标签，与 ESP 版一致）；
- 滑条拖动、波形开关、3×3 数字键盘、"重建键盘/重置" 都是真事件驱动的；
- 录到的素材：`footage/rt_orig_pages.mp4`（12s / 360 帧）、`footage/rt_orig_ui.mp4`（14s / 420 帧）+ 两张 still，
  已进 `web/assets.json` 的 16 段素材里。

### 两个坑（都记下来，免得下回再踩）

1. **`fx_gt911_read()` 必须返回非零**。`fxtk.c` 里是 `if (s_drv->touch_read(&x,&y,&p))`——
   返回 0 会被整段跳过，鼠标怎么点都没反应。
2. **合成点击必须"按住"至少一帧**。这个 UI 每 16ms 轮询采样鼠标状态，
   `xdotool ... click 1`（按下即抬起，间隔 <1ms）会被整帧吞掉，点不动任何东西。
   要用 `mousedown; sleep 0.13; mouseup`。

### 画面里那些"毛病"是原版的，不是我加的

顶部那行糊掉的绿字是 `fx_set_touch_debug(1)` 的触摸调试输出 + 标题叠字；
画布拖影/黑缺口是那一代**没有 span 行缓冲**导致的（后来 fxtk-rt v1.3 的 CHANGELOG 才写"无拖影伪影 + 非黑背景无黑色缺口"）。
这些**保留**，因为正是"那个时代的灰"的证据。
