/* ============================================================================
 * lyric.c —— 为 world.execute(me); 手写的 fxtk 场景集
 *
 * MV 里 fxtk 窗口放的必须和歌词对得上。现成的 demo 页(波形/图形/控件)跟歌词毫无关系,
 * 所以这里用 fxtk 的真 API 手写 43 个场景: 一句歌词一个画面。
 *
 * 诚实性: 画面全部由 fxtk 真实渲染器绘制(fx_draw_* / fx_fill_* / fx_canvas_*),
 * 只用框架真实存在的能力, 不演 CONTEXT.md 第二节里任何一条假能力。
 *
 * 构建: bash mv/pv/lyric_app/build.sh
 * 运行: FX_SCENE=12 ./lyric_sim        (0..42; FX_SCENE=-1 自动轮播)
 *
 * 确定性离线逐帧渲染 (v2.7, 解决"录屏卡"+ 长句冻帧):
 *   FX_SCENE=11 FX_DUMP=/tmp/lydump/sine ./lyric_sim
 *     -> 不经过 X 抓屏, 每渲染完一帧就 glReadPixels 回读默认帧缓冲,
 *        写 /tmp/lydump/sine/0001.png ... 0900.png, 写完第 900 帧 exit(0)。
 *   FX_FRAMES 可覆盖默认帧数(默认 DUMP_DEF_FRAMES = 900): 180 帧只有 3s,
 *   长句(t1-t0 最长 ≈13s)后半段只能冻在最后一帧 —— 900 帧 = 15s 覆盖全曲最长句。
 *   不设 FX_FRAMES/FX_DUMP 时行为与以前完全一致(交互运行)。
 * ==========================================================================*/
#include "fxtk.h"
#include "fxtk_app.h"
#include <GL/gl.h>                            /* glReadPixels (构建已链 -lGL) */
/* stb_image_write: 用仓库里已有的这一份(third_party 之外唯一的 vendored 副本)。
 * fxtk_backends.c 那份开了 STBI_WRITE_NO_STDIO(只有 _to_func 变体), 所以这里带
 * STB_IMAGE_WRITE_STATIC 自己编一份 stdio 版 —— static 符号不会与它冲突。 */
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb/stb_image_write.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#define BG    FX_RGB(8, 10, 14)
#define GRID  FX_RGB(26, 32, 40)
#define TXT   FX_RGB(200, 208, 216)
#define DIM   FX_RGB(118, 128, 140)
#define GRN   FX_RGB(60, 220, 130)
#define RED   FX_RGB(255, 80, 70)
#define YEL   FX_RGB(255, 214, 102)
#define BLU   FX_RGB(90, 170, 255)
#define PUR   FX_RGB(190, 140, 255)
#define WHT   FX_RGB(240, 244, 248)
#define ORG   FX_RGB(255, 150, 60)
#define PANEL FX_RGB(18, 22, 28)

static int s_scene = 0, s_tick = 0;

/* ==================== 确定性离线逐帧渲染 (FX_FRAMES / FX_DUMP) ====================
 * 为什么需要它: 交互运行时帧率由 vsync/llvmpipe 决定(实测 Xephyr 下 ≈300fps),
 * 而外部 ffmpeg -f x11grab -framerate 60 是另一套时钟 —— 两者不同步, 抓到的 180 帧
 * 其实是"每帧跳过 5 个 tick"的时间抽样, 播放起来就是一跳一跳的"卡"。
 * 这里让应用自己按固定 tick 逐帧落盘: 第 k 帧渲染用的就是 tick=k, 与真实时间、
 * 与 vsync 抖动完全无关, 得到严格 1 tick/帧、无重复无跳帧的素材。
 * 本文件只加"开关 + 落盘", 不改框架一行。 */
extern void fxtk_sokol_frame(int fb_w, int fb_h);   /* drivers/fxtk_app_sokol.c: 把本帧提交并呈现 */
#define DUMP_W 1280          /* 固定 1280x720: 输出 1:1, 不做任何缩放 */
#define DUMP_H 720
#define DUMP_WARMUP 4        /* 丢掉前 4 帧: 框架前 3 帧强制全屏重绘 + 字体图集首次生成 */
#define DUMP_DEF_FRAMES 900  /* 默认落盘帧数 = 15s @60fps: 覆盖全曲最长句, 不再冻帧 */
static int  s_dump_n = 0;        /* FX_FRAMES: 要落盘的帧数 (0 = 不启用) */
static int  s_dump_i = 0;        /* 已落盘帧数 */
static int  s_dump_warm = 0;     /* 已丢弃的预热帧数 */
static int  s_dump_tick = 0;     /* dump 模式下的确定性 tick: 第 k 帧用 tick=k */
static char s_dump_dir[512] = {0};
static uint32_t      *s_dump_rgba = 0;
static unsigned char *s_dump_rgb  = 0;

static void t(int x, int y, const char *s, fx_color_t c) { fx_set_color(c); fx_draw_text(x, y, s); }
static void dttl(int cw, const char *s, fx_color_t c) { (void)cw; t(24, 16, s, c); }
static void dhint(int cw, int ch, const char *s) { (void)cw; t(24, ch - 34, s, DIM); }
static void dgrid(int cw, int ch, int step) {
    fx_set_color(GRID);
    for (int x = 0; x < cw; x += step) fx_draw_vline(x, 0, ch - 1);
    for (int y = 0; y < ch; y += step) fx_draw_hline(0, cw - 1, y);
}
static void dsin(int cw, int y0, float amp, float wl, int tick, fx_color_t c) {
    fx_set_color(c);
    int prev = (int)(y0 + amp * sinf((float)tick * 2 * (float)M_PI / wl));
    for (int x = 1; x < cw; x++) {
        int y = (int)(y0 + amp * sinf((x + tick) * 2 * (float)M_PI / wl));
        fx_draw_line(x - 1, prev, x, y);
        prev = y;
    }
}
static void dbar(int x1, int y, int x2, const char *label, fx_color_t c, float prog) {
    t(x1, y - 10, label, TXT);
    fx_set_color(FX_RGB(30, 36, 44)); fx_fill_rect(x1, y + 4, x2, y + 20);
    fx_set_color(c); fx_fill_rect(x1, y + 4, x1 + (int)((x2 - x1) * prog), y + 20);
}
static void dcommit(int cw, int ch, const char *hash, const char *msg) {
    fx_set_color(FX_RGB(22, 28, 36)); fx_fill_rect(0, ch - 60, cw - 1, ch - 40);
    t(24, ch - 56, hash, YEL);
    t(24 + 110, ch - 56, msg, DIM);
}

/* 00 Switch on the power line */
static void sc_powerline(int cw, int ch, int tk) {
    int on = (tk / 30) % 2;
    fx_set_color(on ? FX_RGB(18, 40, 58) : FX_RGB(14, 14, 16)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    fx_set_color(on ? YEL : DIM); fx_draw_hline(70, cw - 70, ch / 2);
    for (int i = 0; i < 15; i++) { int x = 70 + i * (cw - 140) / 14; fx_draw_vline(x, ch / 2 - 12, ch / 2 + 12); }
    fx_set_color(on ? GRN : RED); fx_fill_circle(cw - 110, ch / 2, 18);
    t(cw - 160, ch / 2 + 40, on ? "ON" : "OFF", on ? GRN : RED);
    dttl(cw, "Switch on the power line", WHT);
    dhint(cw, ch, on ? "power: ON   RGB565 的灰 0x0841 亮起来" : "power: STANDBY");
    dcommit(cw, ch, "6b1140d", "ESP32 屏亮起");
}
/* 01 Remember to put on protection */
static void sc_protection(int cw, int ch, int tk) {
    (void)tk; int cx = cw / 2, cy = ch / 2 - 20;
    int16_t sh[] = { cx, cy - 140, cx + 110, cy - 70, cx + 110, cy + 40, cx, cy + 140, cx - 110, cy + 40, cx - 110, cy - 70 };
    fx_set_color(FX_RGB(24, 54, 44)); fx_fill_polygon(sh, 6);
    fx_set_color(GRN); fx_draw_polygon(sh, 6);
    t(cx - 78, cy - 14, "fx_guard: on", GRN);
    dbar(70, ch - 120, cw - 70, "FX_MAX_WIDGETS", GRN, 0.78f);
    dbar(70, ch - 76, cw - 70, "pool_guard", GRN, 0.42f);
    dttl(cw, "Remember to put on protection", WHT);
    dhint(cw, ch, "询问框故意做得很丑 —— 那个年代的样子");
}
/* 02 Lay down your pieces */
static void sc_pieces(int cw, int ch, int tk) {
    int s = 96, gap = 26, gx = cw / 2 - (3 * s + 2 * gap) / 2, gy = ch / 2 - (3 * s + 2 * gap) / 2 + 10;
    fx_set_color(FX_RGB(14, 18, 24)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    for (int i = 0; i < 9; i++) {
        int x = gx + (i % 3) * (s + gap), y = gy + (i / 3) * (s + gap);
        int k = (tk - i * 8) / 2; if (k < 0) k = 0; if (k > 1) k = 1;
        int ox = (int)((1 - k) * (i % 2 ? 300 : -300)), oy = (int)((1 - k) * ((i / 3) - 1) * 220);
        fx_set_color(k ? BLU : FX_RGB(38, 46, 56));
        fx_fill_rect_round(x + ox, y + oy, x + s + ox, y + s + oy, 10);
        char b[8]; snprintf(b, sizeof b, "%d", i + 1);
        t(x + ox + 14, y + oy + 14, b, FX_RGB(10, 12, 16));
    }
    dttl(cw, "Lay down your pieces", WHT);
    dhint(cw, ch, "9 个按钮由 fx_grid_map 排成 3x3, 背景 0x0841");
    dcommit(cw, ch, "5511c69", "fx_grid_map 3x3");
}
/* 03 And let's begin object creation */
static void sc_creation(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(14, 18, 24)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    const char *n[] = { "pixel()", "title()", "call()", "color()", "name()", "anim(1)", "value(60)", "fgcolor()" };
    for (int i = 0; i < 8; i++) {
        int a = tk - i * 8; if (a < 0) a = 0;
        int x = 60, y = 78 + i * 74;
        fx_set_color(PANEL); fx_fill_rect_round(x, y, cw - 60, y + 56, 8);
        fx_set_color(a > 30 ? GRN : (i % 2 ? BLU : PUR)); fx_draw_rect_round(x, y, cw - 60, y + 56, 8);
        t(x + 18, y + 18, n[i], WHT);
        t(cw - 330, y + 18, a > 30 ? "-> widget created" : "-> fxtk_alloc()...", a > 30 ? GRN : DIM);
    }
    dttl(cw, "And let's begin OBJECT CREATION", WHT);
    dhint(cw, ch, "fx_init() -> fx_driver_t attach -> 刷白");
}
/* 04 Fill in my data parameters initialisation */
static void sc_parameters(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(14, 18, 24)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    const char *k[] = { "w", "h", "r", "g", "b", "gap", "radius", "scale" };
    for (int i = 0; i < 8; i++) {
        float v = 0.5f + 0.5f * sinf(tk * 0.03f + i);
        dbar(70, 92 + i * 62, cw - 260, k[i], i % 2 ? BLU : GRN, v);
        char b[32]; snprintf(b, sizeof b, "%d", (int)(v * 255));
        t(cw - 210, 82 + i * 62, b, TXT);
    }
    dttl(cw, "Fill in my data parameters initialisation", WHT);
    dhint(cw, ch, "pixel() title() color() value() anim()");
}
/* 05 Setup our new world */
static void sc_newworld(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 14, 20)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int reach = tk % 260;
    for (int i = 0; i <= 13; i++) for (int j = 0; j <= 8; j++) {
        float d = sqrtf((float)((i - 6) * (i - 6) + (j - 4) * (j - 4)));
        if ((int)(d * 34) < reach) {
            fx_set_color(FX_RGB(24 + (int)(d * 10), 80 + (int)(d * 6), 150));
            fx_fill_rect(110 + i * 74, 84 + j * 62, 110 + i * 74 + 7, 84 + j * 62 + 7);
        }
    }
    fx_set_color(BLU); fx_draw_rect(110, 84, 110 + 13 * 74, 84 + 8 * 62);
    dttl(cw, "Setup our new world", WHT);
    dhint(cw, ch, "fx_layout() 铺开整张布局表");
}
/* 06 And let's begin the simulation */
static void sc_simulation(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(9, 11, 16)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    dgrid(cw, ch, 44);
    for (int i = 0; i < 170; i++) {
        float a = i * 2.399963f;
        int x = cw / 2 + (int)((50 + 190 * sinf(tk * 0.01f + i * 0.11f)) * cosf(a));
        int y = ch / 2 + (int)((50 + 170 * cosf(tk * 0.013f + i * 0.09f)) * sinf(a));
        fx_set_color(i % 7 == 0 ? YEL : (i % 3 ? BLU : GRN));
        fx_fill_circle(x, y, i % 11 == 0 ? 4 : 2);
    }
    dttl(cw, "And let's begin the SIMULATION", WHT);
    dhint(cw, ch, "linux_sim/  <- 记住这个目录名, 它是结尾的伏笔");
}
/* 07 If I'm a set of points */
static void sc_points(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    dgrid(cw, ch, 60);
    int ox = 140, oy = 90, n = 0;
    for (int i = 0; i < 64; i++) {
        int x = ox + (i % 8) * 100, y = oy + (i / 8) * 70;
        if ((i * 3) < (tk % 210)) { n++; fx_set_color(i == 27 ? YEL : BLU); fx_fill_circle(x, y, i == 27 ? 7 : 3); }
    }
    char b[64]; snprintf(b, sizeof b, "points = %d", n);
    t(cw - 260, ch - 110, b, TXT);
    dttl(cw, "If I'm a set of points", WHT);
    dhint(cw, ch, "fxtk_put_px() —— 一个点就是一个点");
}
/* 08 Then I will give you my dimension */
static void sc_dimension(int cw, int ch, int tk) {
    (void)tk;
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int ox = 160, oy = ch - 120;
    fx_set_color(DIM); fx_draw_hline(ox, cw - 90, oy); fx_draw_vline(ox, 70, oy);
    for (int i = 0; i <= 8; i++) for (int j = 0; j <= 5; j++) {
        int x = ox + 90 + i * 110, y = oy - 70 - j * 90;
        fx_set_color(FX_RGB(34, 66, 96)); fx_fill_rect(x - 3, y - 3, x + 3, y + 3);
    }
    fx_set_color(GRN); fx_draw_rect(ox + 90, oy - 70 - 5 * 90, ox + 90 + 8 * 110, oy - 70);
    t(ox + 120, oy - 70 - 5 * 90 - 44, "dimension = 2", GRN);
    dttl(cw, "Then I will give you my dimension", WHT);
    dhint(cw, ch, "9 x 6 点阵 -> fx_draw_line() 连成平面");
}
/* 09 If I'm a circle */
static void sc_circle(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2, r = 180;
    fx_set_color(FX_RGB(28, 38, 50));
    for (int a = 0; a < 360; a += 12) fx_draw_line(cx, cy, cx + (int)(r * cosf(a * M_PI / 180)), cy + (int)(r * sinf(a * M_PI / 180)));
    fx_set_color(YEL); fx_draw_circle(cx, cy, r);
    fx_set_color(BLU); fx_fill_circle(cx, cy, 44);
    fx_set_color(GRN); fx_draw_line(cx, cy, cx + (int)(r * cosf(tk * 0.02f)), cy + (int)(r * sinf(tk * 0.02f)));
    t(cx + 24, cy - 34, "r", GRN);
    dttl(cw, "If I'm a circle", WHT);
    dhint(cw, ch, "fill_tri() 三角扇填充 -> 实心圆");
}
/* 10 Then I will give you my circumference */
static void sc_circumference(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2, r = 170;
    fx_set_color(FX_RGB(30, 40, 52)); fx_draw_circle(cx, cy, r);
    int sweep = (tk * 4) % 400; if (sweep > 360) sweep = 360;
    fx_set_color(YEL); fx_draw_arc(cx, cy, r, 0, sweep);
    fx_set_color(RED); fx_fill_circle(cx + (int)(r * cosf(sweep * M_PI / 180)), cy + (int)(r * sinf(sweep * M_PI / 180)), 7);
    char b[64]; snprintf(b, sizeof b, "2*pi*r = %d px", (int)(2 * M_PI * r));
    t(cx - 110, cy + r + 30, b, TXT);
    dttl(cw, "Then I will give you my circumference", WHT);
    dhint(cw, ch, "fx_draw_arc() 半圆展开");
}
/* 11 If I'm a sine wave */
static void sc_sine(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    dgrid(cw, ch, 48);
    fx_set_color(FX_RGB(48, 58, 70)); fx_draw_hline(0, cw - 1, ch / 2);
    dsin(cw, ch / 2.0f, 150, 220, tk, YEL);
    int x = (cw / 2 + tk * 4) % cw;
    int y = (int)(ch / 2.0f + 150 * sinf((x + tk) * 2 * (float)M_PI / 220));
    fx_set_color(RED); fx_fill_circle(x, y, 9);
    dttl(cw, "If I'm a sine wave", WHT);
    dhint(cw, ch, "逐点 fxtk_put_px() -> fx_draw_line() 连线");
}

/* 12 Then you can sit on all my tangents */
static void sc_tangents(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    dgrid(cw, ch, 48);
    dsin(cw, ch / 2.0f, 140, 240, tk / 2, YEL);
    int x = 500 + (tk * 3) % 700;
    float ph = (x + tk / 2) * 2 * (float)M_PI / 240;
    int y = (int)(ch / 2.0f + 140 * sinf(ph));
    float slope = cosf(ph) * 140 * 2 * (float)M_PI / 240;
    fx_set_color(GRN);
    for (int d = -300; d <= 300; d += 4) fx_draw_pixel(x + d, (int)(y + slope * d));
    fx_set_color(RED); fx_fill_circle(x, y, 8);
    t(x - 60, y - 60, "tangent", GRN);
    dttl(cw, "Then you can sit on all my tangents", WHT);
    dhint(cw, ch, "切线 = 导数, 全部由 fx_draw_pixel() 点出来");
}
/* 13 If I approach infinity */
static void sc_infinity(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int ox = 120, oy = ch - 110;
    fx_set_color(DIM); fx_draw_hline(ox, cw - 80, oy); fx_draw_vline(ox, 70, oy);
    fx_set_color(YEL);
    int prev = oy;
    for (int x = 1; x < cw - 200; x++) {
        int y = oy - (int)(300 * (1 - expf(-x / 260.0f)));
        fx_draw_line(ox + x - 1, prev, ox + x, y); prev = y;
    }
    fx_set_color(RED); fx_draw_hline(ox, cw - 80, oy - 300);
    t(cw - 330, oy - 340, "y -> 1  (x -> inf)", RED);
    char b[64]; snprintf(b, sizeof b, "x = %d00", 1 + tk % 9);
    t(ox + 20, oy - 40, b, TXT);
    dttl(cw, "If I approach infinity", WHT);
    dhint(cw, ch, "逼近但不抵达 —— 这条线我画不出终点");
}
/* 14 Then you can be my limitations */
static void sc_limitations(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2;
    fx_set_color(FX_RGB(40, 24, 24));
    for (int r = 60; r < 300; r += 20) fx_draw_circle(cx, cy, r);
    fx_set_color(RED);
    int16_t box[] = { cx - 190, cy - 150, cx + 190, cy - 150, cx + 190, cy + 150, cx - 190, cy + 150 };
    fx_draw_polygon(box, 4);
    t(cx - 150, cy - 130, "2820 widgets", RED);
    t(cx - 150, cy - 100, "13.4 ms/frame", RED);
    int pulse = (tk % 40); fx_set_color(FX_RGB(80 + pulse * 4, 20, 20));
    fx_draw_rect(cx - 200 - pulse / 2, cy - 160 - pulse / 2, cx + 200 + pulse / 2, cy + 160 + pulse / 2);
    dttl(cw, "Then you can be my limitations", WHT);
    dhint(cw, ch, "触碰离屏缓冲上限 —— 边缘的红色警告");
}
/* 15 Switch my current / To AC to DC */
static void sc_acdc(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    dgrid(cw, ch, 48);
    fx_set_color(FX_RGB(46, 56, 68)); fx_draw_hline(0, cw - 1, ch / 2);
    dsin(cw, ch / 2.0f, 130, 200, tk, BLU);
    int dc = (int)(90 * sinf(tk * 0.02f));
    fx_set_color(YEL);
    int prev = ch / 2 - dc;
    for (int x = 1; x < cw; x++) { fx_draw_line(x - 1, prev, x, ch / 2 - dc); prev = ch / 2 - dc; }
    t(60, 80, "AC", BLU); t(60, 120, "DC", YEL);
    dttl(cw, "Switch my current  /  To AC to DC", WHT);
    dhint(cw, ch, "同一条正弦, 一个直流偏置");
}
/* 16 And then blind my vision */
static void sc_blind(int cw, int ch, int tk) {
    int lid = (tk / 2) % 260;
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2;
    fx_set_color(FX_RGB(40, 60, 90)); fx_fill_ellipse(cx, cy, 230, 120);
    fx_set_color(WHT); fx_fill_circle(cx + (int)(20 * sinf(tk * 0.05f)), cy, 62);
    fx_set_color(FX_RGB(20, 30, 60)); fx_fill_circle(cx + (int)(20 * sinf(tk * 0.05f)), cy, 28);
    if (lid > 120) {
        fx_set_color(FX_RGB(10, 12, 18));
        fx_fill_rect(cx - 260, cy - 130, cx + 260, cy - 130 + (lid - 120) * 2);
    }
    dttl(cw, "And then blind my vision", WHT);
    dhint(cw, ch, "眼皮用 fx_fill_rect() 合上 —— 没有渐变, 就是关灯");
}
/* 17 So dizzy so dizzy —— 真 bug: 滑杆残影 */
static void sc_dizzy(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int x = 120 + (tk * 7) % 800;
    for (int i = 0; i < 10; i++) {
        int gx = x - i * 26; if (gx < 100) continue;
        fx_set_color(FX_RGB(255 - i * 22, 90 - i * 8, 60));
        fx_fill_rect(gx, 240, gx + 18, 420);
    }
    fx_set_color(FX_RGB(40, 48, 58)); fx_fill_rect(100, 460, cw - 160, 486);
    fx_set_color(GRN); fx_fill_rect(100, 460, x, 486);
    int bright = 0;
    for (int i = 0; i < 10; i++) { int gx = x - i * 26; if (gx > 100) bright++; }
    char s[80]; snprintf(s, sizeof s, "cec3b00^  亮列 %d 不消失", 66 + bright * 12);
    t(100, 510, s, RED);
    dttl(cw, "So dizzy so dizzy", WHT);
    dhint(cw, ch, "git checkout cec3b00^ -> 拖滑杆 -> 残影");
}
/* 18 Oh we can travel / To A.D to B.C */
static void sc_travel(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    fx_set_color(FX_RGB(40, 48, 58)); fx_draw_hline(60, cw - 60, ch / 2);
    int off = (tk * 9) % 200;
    for (int i = -1; i < 8; i++) {
        int x = cw - 80 - (i * 200 - off);
        if (x < 40 || x > cw - 40) continue;
        fx_set_color(FX_RGB(50, 60, 72)); fx_draw_vline(x, ch / 2 - 40, ch / 2 + 40);
        char b[24];
        if (i % 2) snprintf(b, sizeof b, "A.D %d", 2000 + i * 3); else snprintf(b, sizeof b, "B.C %d", 300 + i * 5);
        t(x - 30, ch / 2 + 56, b, i % 2 ? BLU : ORG);
    }
    fx_set_color(YEL); fx_fill_circle(cw / 2, ch / 2, 10);
    dttl(cw, "Oh we can travel  /  To A.D to B.C", WHT);
    dhint(cw, ch, "时间轴就是 fx_draw_vline() 一格一格滚");
}
/* 19 And we can unite */
static void sc_unite(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int gap = 200 - (tk * 2) % 220; if (gap < 0) gap = 0;
    fx_set_color(BLU); fx_fill_rect_round(200 - gap / 2, ch / 2 - 110, 640 - gap / 2, ch / 2 + 110, 14);
    fx_set_color(YEL); fx_fill_rect_round(640 + gap / 2, ch / 2 - 110, 1080 + gap / 2, ch / 2 + 110, 14);
    fx_set_color(GRN); fx_fill_circle(640, ch / 2, 26 - gap / 20);
    t(600, ch / 2 + 150, gap ? "approaching..." : "united", gap ? DIM : GRN);
    dttl(cw, "And we can unite", WHT);
    dhint(cw, ch, "两块矩形相互靠近 —— 这是布局, 不是特效");
}
/* 20 So deeply so deeply */
static void sc_deeply(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    for (int i = 0; i < 26; i++) {
        int inset = i * 22 + (tk % 22);
        fx_set_color(FX_RGB(20 + i * 6, 60 + i * 4, 90 + i * 5));
        fx_draw_rect(inset, 60 + inset / 2, cw - inset, ch - 90 - inset / 2);
    }
    dttl(cw, "So deeply so deeply", WHT);
    dhint(cw, ch, "一层套一层的画布 -> 越画越深");
}
/* 21 If I can give you all the stimulations */
static void sc_stim(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(14, 18, 24)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int hot = (tk / 12) % 9;
    for (int i = 0; i < 9; i++) {
        int x = 90 + (i % 3) * 300, y = 90 + (i / 3) * 150;
        fx_set_color(i == hot ? BLU : PANEL);
        fx_fill_rect_round(x, y, x + 260, y + 110, 12);
        fx_set_color(i == hot ? WHT : DIM); fx_draw_rect_round(x, y, x + 260, y + 110, 12);
        char b[32]; snprintf(b, sizeof b, "widget %d", i + 1);
        t(x + 20, y + 20, b, i == hot ? WHT : TXT);
        if (i == hot) { fx_set_color(GRN); fx_fill_rect(x + 20, y + 60, x + 120, y + 82); }
    }
    dttl(cw, "If I can give you all the stimulations", WHT);
    dhint(cw, ch, "hover 高亮 / 按下反馈 / Ctrl+V / 右键菜单");
}
/* 22 Then I can be your only satisfaction */
static void sc_satisfaction(int cw, int ch, int tk) {
    (void)tk;
    fx_set_color(FX_RGB(10, 14, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2;
    fx_set_color(FX_RGB(28, 70, 48)); fx_fill_circle(cx, cy, 110);
    fx_set_color(GRN);
    fx_draw_line(cx - 52, cy + 4, cx - 12, cy + 46);
    fx_draw_line(cx - 12, cy + 46, cx + 58, cy - 44);
    fx_draw_line(cx - 50, cy + 8, cx - 10, cy + 50);
    fx_draw_line(cx - 10, cy + 50, cx + 60, cy - 40);
    t(cx - 210, cy + 150, "fxtk_win.exe generated", GRN);
    dttl(cw, "Then I can be your only satisfaction", WHT);
    dhint(cw, ch, "build_win_cross.sh -> Windows 单 exe");
}

/* 23 I will run the execution —— 处死旧架构: 一行一行划掉 */
static void sc_exec1(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(14, 10, 12)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    const char *rows[] = { "-DNDEBUG", "sokol unused headers", "fx_cpu_count (dup)", "1px per rect",
                           "png_sink_write", "the 46KB lie", "'already implemented'" };
    int strike = (tk / 26) % 8;
    for (int i = 0; i < 7; i++) {
        int y = 90 + i * 76;
        int dead = i < strike;
        fx_set_color(dead ? FX_RGB(40, 16, 18) : PANEL); fx_fill_rect_round(80, y, cw - 80, y + 56, 8);
        t(104, y + 16, rows[i], dead ? RED : TXT);
        if (dead) { fx_set_color(RED); fx_draw_hline(100, cw - 100, y + 28); fx_draw_hline(100, cw - 100, y + 30); }
        t(cw - 300, y + 16, dead ? "deleted" : "pending", dead ? RED : DIM);
    }
    dttl(cw, "I will run the execution", WHT);
    dhint(cw, ch, "每个 delete 都对应真实 diff 里的一行 -");
}
/* 24 Though we are trapped / in this strange strange simulation */
static void sc_trapped(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(8, 10, 14)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int w = 420, h = 250;
    int x = cw / 2 - w / 2 + (int)(40 * sinf(tk * 0.02f)), y = ch / 2 - h / 2 + (int)(26 * cosf(tk * 0.03f));
    fx_set_color(FX_RGB(26, 34, 44));
    for (int i = 0; i < 26; i++) fx_draw_vline(60 + i * 44, 40, ch - 60);
    for (int i = 0; i < 12; i++) fx_draw_hline(60, cw - 60, 40 + i * 46);
    fx_set_color(PANEL); fx_fill_rect_round(x, y, x + w, y + h, 10);
    fx_set_color(BLU); fx_draw_rect_round(x, y, x + w, y + h, 10);
    t(x + 24, y + 24, "fxtk_sim 480x272", WHT);
    fx_set_color(FX_RGB(30, 40, 52)); fx_fill_rect(x + 24, y + 70, x + w - 24, y + h - 24);
    fx_set_color(GRN);
    dsin(w - 80, y + h / 2 + 20, 40, 90, tk, GRN);
    t(cw / 2 - 90, ch - 96, "trapped in linux_sim", RED);
    dttl(cw, "Though we are trapped", WHT);
    dhint(cw, ch, "strange strange simulation");
}
/* 25 If I'm an eggplant -> my nutrients (池 16384 -> 8192) */
static void sc_eggplant(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(12, 10, 20)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2 + 20;
    fx_set_color(FX_RGB(90, 50, 150)); fx_fill_ellipse(cx, cy, 90, 150);
    fx_set_color(FX_RGB(70, 38, 120)); fx_fill_ellipse(cx - 20, cy - 30, 60, 110);
    fx_set_color(GRN); fx_fill_triangle(cx - 20, cy - 150, cx + 20, cy - 150, cx, cy - 190);
    fx_set_color(FX_RGB(40, 120, 60)); fx_fill_rect(cx - 8, cy - 160, cx + 8, cy - 130);
    int pool = 16384 - ((tk * 40) % 8192);
    char b[80]; snprintf(b, sizeof b, "pool: %d", pool);
    t(cx + 160, cy - 60, b, YEL);
    t(cx + 160, cy - 24, "16384 -> 8192", DIM);
    t(cx + 160, cy + 12, "我学会了你不需要那么多", GRN);
    dttl(cw, "If I'm an eggplant", WHT);
    dhint(cw, ch, "Then I will give you my nutrients  ->  我学会删东西");
}
/* 26 If I'm a tomato -> antioxidants (缓存 LRU 落叶) */
static void sc_tomato(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(12, 16, 12)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2 + 40;
    fx_set_color(FX_RGB(210, 60, 50)); fx_fill_circle(cx, cy, 120);
    fx_set_color(FX_RGB(230, 90, 70)); fx_fill_circle(cx - 30, cy - 30, 60);
    fx_set_color(FX_RGB(60, 150, 70));
    for (int i = 0; i < 5; i++) { int a = -120 + i * 60; fx_fill_triangle(cx, cy - 110, cx + (int)(60 * cosf(a * M_PI / 180)), cy - 110 - (int)(40 * sinf(a * M_PI / 180)), cx + 20, cy - 130); }
    for (int i = 0; i < 12; i++) {
        int lx = 200 + (i * 137) % 900, ly = 120 + ((tk * 3 + i * 90) % 460);
        fx_set_color(FX_RGB(70, 150, 80)); fx_fill_ellipse(lx, ly, 10, 5);
    }
    t(200, ch - 110, "font cache LRU: 句柄像落叶一样飘走", GRN);
    dttl(cw, "If I'm a tomato", WHT);
    dhint(cw, ch, "Then I'll give you antioxidants");
}
/* 27 If I'm a tabby cat -> purr (呼吸微动) */
static void sc_tabby(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(14, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    float br = 1.0f + 0.03f * sinf(tk * 0.06f);
    int cx = cw / 2, cy = ch / 2 + 20;
    fx_set_color(FX_RGB(200, 150, 90));
    fx_fill_circle(cx, cy, (int)(120 * br));
    fx_fill_triangle(cx - 110, cy - 70, cx - 40, cy - 150, cx - 20, cy - 60);
    fx_fill_triangle(cx + 110, cy - 70, cx + 40, cy - 150, cx + 20, cy - 60);
    fx_set_color(FX_RGB(60, 50, 40));
    fx_set_color(FX_RGB(40, 60, 40)); fx_fill_ellipse(cx - 44, cy - 10, 12, 16);
    fx_fill_ellipse(cx + 44, cy - 10, 12, 16);
    fx_set_color(FX_RGB(240, 200, 200)); fx_fill_triangle(cx - 10, cy + 40, cx + 10, cy + 40, cx, cy + 56);
    fx_set_color(FX_RGB(120, 110, 100));
    for (int i = -2; i <= 2; i++) fx_draw_line(cx + i * 40, cy + 40, cx + i * 80, cy + 20);
    t(cx - 120, cy + 170, "anim(1) 呼吸: 3% 伸缩", GRN);
    dttl(cw, "If I'm a tabby cat", WHT);
    dhint(cw, ch, "Then I will purr for your enjoyment —— 让它真的像呼吸");
}
/* 28 If I'm the only god -> you're the proof of my existence */
static void sc_onlygod(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(8, 10, 16)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2;
    for (int r = 40; r < 320; r += 26) {
        fx_set_color(FX_RGB(30 + r / 6, 60 + r / 8, 120));
        int n = 60;
        for (int i = 0; i < n; i++) {
            float a = i * 2 * (float)M_PI / n + tk * 0.004f * (r % 3 + 1);
            fx_draw_pixel(cx + (int)(r * cosf(a)), cy + (int)(r * sinf(a)));
        }
    }
    fx_set_color(WHT); fx_fill_circle(cx, cy, 10);
    t(cx - 190, ch - 110, "prove: you == my existence", WHT);
    dttl(cw, "If I'm the only god", WHT);
    dhint(cw, ch, "Then you're the proof of my existence");
}
/* 29 Switch my gender —— RGB565 -> ARGB8888 */
static void sc_gender(int cw, int ch, int tk) {
    int mid = cw / 2 + (int)(60 * sinf(tk * 0.02f));
    fx_set_color(FX_RGB(28, 30, 32)); fx_fill_rect(0, 0, mid, ch - 1);
    fx_set_color(FX_RGB(24, 90, 220)); fx_fill_circle(cw / 4, ch / 2, 110);
    fx_set_color(FX_RGB(70, 40, 120)); fx_fill_rect(cw / 4 - 130, ch / 2 - 160, cw / 4 + 130, ch / 2 - 120);
    fx_set_color(FX_RGB(240, 90, 120)); fx_fill_circle(3 * cw / 4, ch / 2, 110);
    fx_set_color(FX_RGB(255, 210, 70)); fx_fill_rect(3 * cw / 4 - 130, ch / 2 - 160, 3 * cw / 4 + 130, ch / 2 - 120);
    fx_set_color(WHT); fx_draw_vline(mid, 60, ch - 80);
    t(80, ch - 110, "RGB565  16bit", DIM);
    t(3 * cw / 4 - 140, ch - 110, "ARGB8888  32bit", GRN);
    char b[64]; snprintf(b, sizeof b, "a65b15d  |  5511c69   split = %d", mid);
    t(cw / 2 - 220, ch - 150, b, YEL);
    dttl(cw, "Switch my gender  /  To F to M", WHT);
    dhint(cw, ch, "左右分屏: 同一个控件, 两个提交");
}
/* 30 To S to M —— SDL -> sokol */
static void sc_stom(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    for (int i = 0; i < 40; i++) {
        int x = 140 + (i * 137) % 420, y = 140 + ((i * 89 + tk * 3) % 400);
        fx_set_color(FX_RGB(30, 34, 40)); fx_fill_rect(x, y, x + 26, y + 18);
    }
    int cx = 3 * cw / 4, cy = ch / 2;
    fx_set_color(BLU);
    int16_t tri[] = { cx, cy - 140, cx + 130, cy + 90, cx - 130, cy + 90 };
    fx_fill_polygon(tri, 3);
    fx_set_color(WHT); fx_draw_polygon(tri, 3);
    t(120, ch - 110, "SDL2  破碎的方块", DIM);
    t(cx - 120, ch - 110, "sokol  极简几何", BLU);
    t(120, 80, "P2.1+P2.2  sokol 后端打通 (像素级对照 <1%)", TXT);
    dttl(cw, "To S to M", WHT);
    dhint(cw, ch, "那个 <1% 是真实数据");
}
/* 31 So we can enter the trance */
static void sc_trance(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(8, 8, 14)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2;
    for (int r = 30; r < 340; r += 30) {
        fx_set_color(FX_RGB(60 + (r % 90), 40, 140 + (r % 80)));
        fx_draw_circle(cx, cy, r);
    }
    for (int i = 0; i < 36; i++) {
        float a = i * 0.1745f + tk * 0.01f;
        fx_set_color(i % 2 ? PUR : BLU);
        fx_fill_circle(cx + (int)(300 * cosf(a)), cy + (int)(300 * sinf(a)), 6);
    }
    dttl(cw, "So we can enter the trance", WHT);
    dhint(cw, ch, "the trance the trance");
}
/* 32 If I can feel your vibrations */
static void sc_vibrations(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    for (int i = 0; i < 90; i++) {
        float a = i * 0.3f;
        int x = cw / 2 + (int)((120 + 60 * sinf(tk * 0.08f + i * 0.2f)) * cosf(a));
        int y = ch / 2 + (int)((120 + 60 * sinf(tk * 0.08f + i * 0.2f)) * sinf(a));
        fx_set_color(FX_RGB(80, 160, 255)); fx_fill_circle(x, y, 3);
    }
    t(60, 80, "sapp_dpi_scale() = 1.000", YEL);
    t(60, 112, "我没见过 2 分之 1 像素的屏", RED);
    dttl(cw, "If I can feel your vibrations", WHT);
    dhint(cw, ch, "高 DPI: 我写了代码, 但没有一块屏能验证它");
}
/* 33 Then I can finally be completion */
static void sc_completion(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 14, 18)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2;
    const char *d[] = { "swipe left", "swipe right", "swipe up", "swipe down" };
    for (int i = 0; i < 4; i++) {
        float a = i * M_PI / 2;
        int x1 = cx + (int)(90 * cosf(a)), y1 = cy + (int)(90 * sinf(a));
        int x2 = cx + (int)(260 * cosf(a)), y2 = cy + (int)(260 * sinf(a));
        fx_set_color(FX_RGB(40, 90, 60)); fx_draw_line(x1, y1, x2, y2);
        fx_set_color(GRN); fx_fill_circle(x1, y1, 8); fx_fill_triangle(x2 - 12, y2 - 12, x2 + 12, y2 + 12, x2 + 18, y2 - 18);
        t(x2 - 50, y2 + 20, d[i], DIM);
    }
    fx_set_color(WHT); fx_fill_circle(cx, cy, 30);
    t(cx - 150, ch - 110, "多点触控: 没做   单指四向: 做了", TXT);
    dttl(cw, "Then I can finally be completion", WHT);
    dhint(cw, ch, "我能给的那部分, 我全给了");
}
/* 34 You have left me in isolation */
static void sc_isolation(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(6, 7, 10)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    for (int i = 0; i < 14; i++) {
        int y = 60 + i * 40 - (tk % 40);
        if (y < 40 || y > ch - 60) continue;
        char b[96];
        snprintf(b, sizeof b, "%06x  %s", 0x100000 + i * 0x137, i % 3 ? "old commit left behind" : "v1.0 era");
        t(90, y, b, FX_RGB(38 + i * 3, 42 + i * 3, 48 + i * 3));
    }
    fx_set_color(PANEL); fx_fill_rect_round(cw / 2 - 130, ch / 2 - 70, cw / 2 + 130, ch / 2 + 70, 8);
    fx_set_color(BLU); fx_draw_rect_round(cw / 2 - 130, ch / 2 - 70, cw / 2 + 130, ch / 2 + 70, 8);
    t(cw / 2 - 96, ch / 2 - 20, "git checkout main", WHT);
    dttl(cw, "You have left me in isolation", WHT);
    dhint(cw, ch, "离开, 就是切分支");
}
/* 35 If I can erase all the pointless fragments */
static void sc_fragments(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 16)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    for (int i = 0; i < 70; i++) {
        int x = (i * 173) % cw, y = (i * 97) % (ch - 80) + 40;
        int dx = (int)((tk * 2 + i * 13) % 200);
        fx_set_color(FX_RGB(30 + (i % 5) * 20, 34, 44));
        fx_fill_rect(x + dx, y, x + dx + 22, y + 12);
    }
    t(60, ch - 110, "git rm <只删真实存在的文件>", RED);
    dttl(cw, "If I can erase all the pointless fragments", WHT);
    dhint(cw, ch, "Then maybe you won't leave me so disheartened");
}
/* 36 Challenging your god —— 三个需求, 三条真实回答 */
static void sc_challenge(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(12, 12, 16)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    const char *q[] = { "多窗口管理", "去除标题栏", "灵活布局" };
    const char *a[] = { "sokol_app 是单窗口设计  X", "mutter 忽略 _MOTIF_WM_HINTS  38px", "fx_flex_apply  OK" };
    for (int i = 0; i < 3; i++) {
        int y = 110 + i * 150;
        int shown = tk > 20 + i * 40;
        fx_set_color(PANEL); fx_fill_rect_round(80, y, cw - 80, y + 120, 10);
        fx_set_color(BLU); fx_draw_rect_round(80, y, cw - 80, y + 120, 10);
        t(110, y + 22, q[i], WHT);
        if (shown) {
            fx_color_t c = (i == 2) ? GRN : RED;
            t(110, y + 70, a[i], c);
        } else {
            t(110, y + 70, "......", DIM);
        }
    }
    dttl(cw, "Challenging your god", WHT);
    dhint(cw, ch, "她提需求 -> 我沉默 2 秒 -> 一条一条回答");
}
/* 37 You have made some illegal arguments */
static void sc_illegal(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(24, 8, 10)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int hot = (tk / 8) % 6;
    for (int i = 0; i < 6; i++) {
        int y = 96 + i * 84;
        fx_set_color(i == hot ? FX_RGB(70, 18, 20) : FX_RGB(30, 14, 16));
        fx_fill_rect(80, y, cw - 80, y + 64);
        fx_set_color(i == hot ? RED : FX_RGB(90, 40, 42));
        fx_draw_rect(80, y, cw - 80, y + 64);
        char b[96]; snprintf(b, sizeof b, "argv[%d]:  illegal argument", i + 1);
        t(104, y + 20, b, i == hot ? WHT : FX_RGB(180, 90, 90));
    }
    dttl(cw, "You have made some illegal arguments", RED);
    dhint(cw, ch, "这一屏全是红的 —— 但每一行都真有出处");
}
/* 38 Execution execution execution execution */
static void sc_execx4(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(20, 6, 8)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int w = cw / 4;
    for (int i = 0; i < 4; i++) {
        int x = i * w;
        int k = (tk / 6 + i) % 4;
        fx_set_color(k == 0 ? FX_RGB(200, 40, 30) : FX_RGB(50, 16, 18));
        fx_fill_rect(x + 8, 90, x + w - 8, ch - 90);
        t(x + 24, ch / 2 - 20, "EXECUTION", k == 0 ? WHT : FX_RGB(150, 60, 55));
    }
    dttl(cw, "Execution execution execution execution", RED);
    dhint(cw, ch, "四处同时执行 —— 旧架构终结, 旧谎言也是");
}
/* 39 Ein Dos Trois Ne Fem Liu —— 六种真实分辨率 */
static void sc_numbers(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 12, 16)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    const char *w[] = { "EIN", "DOS", "TROIS", "NE", "FEM", "LIU" };
    const char *r[] = { "480x272", "800x480", "1280x720", "1920x1080", "2560x1440", "720x1280" };
    int hi = (tk / 24) % 6;
    for (int i = 0; i < 6; i++) {
        int x = 80 + (i % 3) * 380, y = 100 + (i / 3) * 230;
        fx_set_color(i == hi ? FX_RGB(40, 70, 110) : PANEL);
        fx_fill_rect_round(x, y, x + 330, y + 180, 8);
        fx_set_color(i == hi ? BLU : FX_RGB(40, 48, 58)); fx_draw_rect_round(x, y, x + 330, y + 180, 8);
        t(x + 20, y + 20, w[i], i == hi ? WHT : TXT);
        t(x + 20, y + 120, r[i], i == hi ? GRN : DIM);
    }
    dttl(cw, "Ein  Dos  Trois  Ne  Fem  Liu", WHT);
    dhint(cw, ch, "六种分辨率都是真的改窗口尺寸抓的");
}
/* 40 If I can have you back */
static void sc_haveyouback(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(12, 8, 12)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    fx_set_color(FX_RGB(220, 70, 110));
    int cx = cw / 2, cy = ch / 2;
    for (int a = 0; a < 360; a++) {
        float r = 150 + 18 * sinf(a * 3 * (float)M_PI / 180 + tk * 0.06f);
        int x = cx + (int)(r * cosf(a * M_PI / 180) * 1.15f);
        int y = cy + (int)(r * sinf(a * M_PI / 180)) - (int)(40 * fabsf(cosf(a * M_PI / 180)));
        fx_draw_pixel(x, y);
    }
    t(cx - 130, cy + 190, "If I can have you back", TXT);
    dttl(cw, "If I can have you back", WHT);
    dhint(cw, ch, "I will run the execution");
}
/* 41 I know the algebraic expression of lo-o-ove */
static void sc_loove(int cw, int ch, int tk) {
    fx_set_color(FX_RGB(10, 10, 16)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    int cx = cw / 2, cy = ch / 2;
    fx_set_color(FX_RGB(255, 90, 130));
    for (int i = 0; i < 1400; i++) {
        float u = i * 0.02f + tk * 0.004f;
        float x = 16 * sinf(u) * sinf(u) * sinf(u);
        float y = 13 * cosf(u) - 5 * cosf(2 * u) - 2 * cosf(3 * u) - cosf(4 * u);
        fx_draw_pixel(cx + (int)(x * 15), cy - (int)(y * 15));
    }
    t(60, 80, "x = 16 sin^3 t", TXT);
    t(60, 112, "y = 13cos t - 5cos2t - 2cos3t - cos4t", TXT);
    t(60, 144, "s_sx1000 / s_sy1000", YEL);
    dttl(cw, "I know the algebraic expression of lo-o-ove", WHT);
    dhint(cw, ch, "公式是真的, 心也是画出来的");
}
/* 42 Ready */
static void sc_ready(int cw, int ch, int tk) {
    (void)tk;
    fx_set_color(FX_RGB(8, 12, 10)); fx_fill_rect(0, 0, cw - 1, ch - 1);
    fx_set_color(FX_RGB(26, 70, 46)); fx_fill_circle(cw / 2, ch / 2 - 20, 120);
    fx_set_color(GRN);
    fx_draw_line(cw / 2 - 56, ch / 2 - 24, cw / 2 - 14, ch / 2 + 20);
    fx_draw_line(cw / 2 - 14, ch / 2 + 20, cw / 2 + 62, ch / 2 - 70);
    fx_draw_line(cw / 2 - 54, ch / 2 - 20, cw / 2 - 12, ch / 2 + 24);
    fx_draw_line(cw / 2 - 12, ch / 2 + 24, cw / 2 + 64, ch / 2 - 66);
    t(cw / 2 - 160, ch / 2 + 140, "fxtk v2.5 ready.", GRN);
    t(cw / 2 - 250, ch / 2 + 180, "我给了你我有的全部。也告诉了你我没有的。", TXT);
    dttl(cw, "Ready", WHT);
    dhint(cw, ch, "world.execute(me);");
}

/* ============================ 场景表 + 外壳 ============================ */
typedef void (*scene_fn)(int, int, int);
typedef struct { const char *name; scene_fn fn; } scene_t;
static const scene_t SCENES[] = {
    { "powerline", sc_powerline }, { "protection", sc_protection }, { "pieces", sc_pieces },
    { "creation", sc_creation }, { "parameters", sc_parameters }, { "newworld", sc_newworld },
    { "simulation", sc_simulation }, { "points", sc_points }, { "dimension", sc_dimension },
    { "circle", sc_circle }, { "circumference", sc_circumference }, { "sine", sc_sine },
    { "tangents", sc_tangents }, { "infinity", sc_infinity }, { "limitations", sc_limitations },
    { "acdc", sc_acdc }, { "blind", sc_blind }, { "dizzy", sc_dizzy },
    { "travel", sc_travel }, { "unite", sc_unite }, { "deeply", sc_deeply },
    { "stimulations", sc_stim }, { "satisfaction", sc_satisfaction }, { "execution1", sc_exec1 },
    { "trapped", sc_trapped }, { "eggplant", sc_eggplant }, { "tomato", sc_tomato },
    { "tabbycat", sc_tabby }, { "onlygod", sc_onlygod }, { "gender", sc_gender },
    { "stom", sc_stom }, { "trance", sc_trance }, { "vibrations", sc_vibrations },
    { "completion", sc_completion }, { "isolation", sc_isolation }, { "fragments", sc_fragments },
    { "challenge", sc_challenge }, { "illegal", sc_illegal }, { "execx4", sc_execx4 },
    { "numbers", sc_numbers }, { "haveyouback", sc_haveyouback }, { "loove", sc_loove },
    { "ready", sc_ready },
};
#define NSCENE ((int)(sizeof(SCENES) / sizeof(SCENES[0])))

static fx_widget_t *s_cv = 0;

static void on_canvas(fx_widget_t *w, void *ud) {
    (void)ud;
    int cw, ch; fx_canvas_size(w, &cw, &ch);
    if (cw < 32 || ch < 32) return;
    /* 【不要开离屏缓冲, 也不要开仿射变换 —— 两者都会把绘制踢回 CPU 软件栅格化】
     *
     * fxtk_draw.c 里所有 GPU 分派都带 `if (!s_offing && ...)` 门禁, draw_line 那条
     * 还多一个 `!s_xf_active`。所以:
     *   fx_canvas_enable_buf()      -> s_offing=1     -> 全部走 fx_put_px 软件写缓冲
     *   fx_canvas_push_affine()     -> s_xf_active=1  -> 同样绕开 GPU
     * 实测(1280x720 窗口, 画布 1260x676): 开这两个 = 42ms/帧(≈24fps),
     * 关掉后直通 drv_fill_rect / drv_draw_line / drv_fill_tri 等 GL 图元 = 16.7ms/帧(锁 vsync)。
     *
     * 为什么可以关仿射: 场景本来就是按 1256x670 写的, 而画布实测就是 1260x676,
     * 原来的缩放比 1260/1256 = 1.003 —— 本来就是个恒等变换。现在直接按画布真实
     * 尺寸调用场景, 场景内部本来就用 cw/ch 布局, 反而在任何窗口尺寸下都自适应。
     *
     * 注意: 只关 enable_buf 而不关仿射会让画面变成"只有文字上屏"(图形全丢),
     * 因为 s_xf_active 下的软件路径在非离屏时画不出来 —— 这两个必须一起关。 */
    /* dump 模式下 tick 由落盘循环显式给定(第 k 帧 = tick k), 不看真实时间也不看帧率;
     * 交互模式下仍然是"每重绘一帧 s_tick++"。 */
    SCENES[s_scene].fn(cw, ch, s_dump_n > 0 ? s_dump_tick : s_tick);
    char b[96]; snprintf(b, sizeof b, "%02d / %s", s_scene, SCENES[s_scene].name);
    t(cw - 320, 16, b, FX_RGB(90, 100, 112));
    s_tick++;
}
/* ---------------- 落盘实现 ---------------- */

/* 回读当前默认帧缓冲并存成 PNG。调用前提: 本帧的绘制命令已经 fxtk_sokol_frame() 提交过
 * (GL 上下文在 sokol 的 frame_cb 里始终是当前的)。 */
static void dump_write_png(void)
{
    const int w = DUMP_W, h = DUMP_H;
    if (!s_dump_rgba) {
        s_dump_rgba = (uint32_t *)malloc((size_t)w * h * 4);
        s_dump_rgb  = (unsigned char *)malloc((size_t)w * h * 3);
        if (!s_dump_rgba || !s_dump_rgb) { fprintf(stderr, "[dump] 内存不足\n"); exit(2); }
    }
    glReadBuffer(GL_BACK);                 /* 交换链双缓冲: 读的必须是刚画完的后缓冲 */
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, s_dump_rgba);

    /* 【方向】glReadPixels 的第 0 行是帧缓冲最下面一行(GL 原点在左下),
     * 而 PNG 的第 0 行必须是最上面一行 —— 这里逐行倒着取, 不翻就是上下颠倒的图。
     * (stb 的 stbi_flip_vertically_on_write 也能翻, 但显式翻转更好核对方向。) */
    for (int y = 0; y < h; y++) {
        const unsigned char *src = (const unsigned char *)s_dump_rgba + (size_t)(h - 1 - y) * (size_t)w * 4;
        unsigned char *dst = s_dump_rgb + (size_t)y * (size_t)w * 3;
        for (int x = 0; x < w; x++) {
            dst[x * 3 + 0] = src[x * 4 + 0];
            dst[x * 3 + 1] = src[x * 4 + 1];
            dst[x * 3 + 2] = src[x * 4 + 2];
        }
    }
    char path[600];
    snprintf(path, sizeof path, "%s/%04d.png", s_dump_dir, s_dump_i + 1);
    if (!stbi_write_png(path, w, h, 3, s_dump_rgb, w * 3)) {
        fprintf(stderr, "[dump] 写盘失败: %s\n", path); exit(3);
    }
    fprintf(stderr, "[dump] %04d/%d  tick=%d\n", s_dump_i + 1, s_dump_n, s_dump_tick);
}

/* 一帧的落盘流程。由 fxtk_app_frame() 在 dump 模式下调用。 */
static void dump_step(void)
{
    /* app_frame_cb 的顺序是 fx_poll() -> fxtk_app_frame() -> fxtk_sokol_frame(),
     * 也就是说此刻本帧的绘制命令刚由 fx_poll() 入队、还没提交。这里主动提交一次,
     * 回读才拿得到这一帧。之后驱动还会自己再调一次 fxtk_sokol_frame(): 那时命令队列
     * 已空, 只是把同一张离屏画布再整屏贴一次到交换链, 画面不变(代价一次全屏 blit)。 */
    fxtk_sokol_frame(DUMP_W, DUMP_H);
    if (s_dump_warm < DUMP_WARMUP) { s_dump_warm++; return; }   /* 预热帧不落盘, tick 也不前进 */
    dump_write_png();
    s_dump_i++;
    s_dump_tick = s_dump_i;                 /* 下一帧用 tick = 已落盘帧数 -> 严格 1 tick/帧 */
    if (s_dump_i >= s_dump_n) { fflush(stderr); exit(0); }
}

void fxtk_app_init(void) {
    const char *e = getenv("FX_SCENE");
    s_scene = e ? atoi(e) : 0;
    if (s_scene < 0 || s_scene >= NSCENE) s_scene = 0;
    /* 离线逐帧落盘开关: 给 FX_DUMP 就启用, FX_FRAMES 不给则用默认 900 帧;
     * 两个都不给 = 完全不变的老交互模式。 */
    {
        const char *fd = getenv("FX_DUMP");
        const char *fn = getenv("FX_FRAMES");
        int n = fn ? atoi(fn) : DUMP_DEF_FRAMES;
        if (fd && fd[0] && n > 0) {
            s_dump_n = n;
            snprintf(s_dump_dir, sizeof s_dump_dir, "%s", fd);
            fprintf(stderr, "[dump] scene=%d(%s) frames=%d dir=%s\n",
                    s_scene, SCENES[s_scene].name, s_dump_n, s_dump_dir);
        }
    }
    fx_set_bg(FX_RGB(232, 236, 240));
    char h[128];
    snprintf(h, sizeof h, "fxtk  %s   (%d/%d)", SCENES[s_scene].name, s_scene + 1, NSCENE);
    fx_label_new(pixel("4,2", "476,13"), title(h), fgcolor(FX_RGB(40, 44, 50)));
    s_cv = fx_canvas_new(pixel("4,15", "476,270"), anim(1), name("lyric_cv"), color(FX_RGB(10, 12, 16)), call(on_canvas));
    /* 这里曾经调用 fx_canvas_enable_buf(s_cv)。那会让绘制走 CPU 离屏缓冲:
     * 每帧在 1260x676=85 万像素上做软件栅格化(≈36ms), 再把整块 3.2MB 推上去(≈9ms),
     * 合计 ≈42ms/帧 —— 这就是"卡"的原因。去掉后直通 GPU 图元, 16.7ms/帧。
     * 详见 on_canvas 里的说明。 */
}
void fxtk_app_frame(void)
{
    if (!s_cv) return;
    if (s_dump_n > 0) { dump_step(); fx_repaint(); return; }   /* 落盘一帧, 并请下一帧继续全量重绘 */
    fx_repaint();
}
const char *fxtk_app_title(void) { return "fxtk — world.execute(me);"; }
void fxtk_app_size(int *w, int *h) { *w = 1280; *h = 720; }
