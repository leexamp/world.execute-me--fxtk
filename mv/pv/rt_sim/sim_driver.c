/**
 * sim_driver.c — 给 **最初的 fxtk（fxtk-rt 最早版本，ESP32 专用、没有模拟器）** 补一个 PC 模拟器。
 *
 * 原始版本还原自 leexamp/fxtk-rt 提交 48c2ef9 (2026-08-06 "Add files via upload")：
 * 那一版只有 ESP-IDF 工程（ST6201 SPI 屏 + GT911 触摸 + FreeRTOS），**没有任何 PC 端代码**。
 * 这里只做一件事：**把硬件那一层换掉，图形库和 UI 一行不改**。
 *
 *   - 硬件屏驱动 fxtk_st6201.c  →  本文件里的 fx_st6201_driver（写进内存帧缓冲，再上屏）
 *   - GT911 触摸 fxtk_touch.c   →  本文件里的 fx_gt911_*（鼠标当手指）
 *   - esp_log / FreeRTOS        →  stubs/ 里两个头文件
 *
 * 也就是说：**画面上每一个像素都是 48c2ef9 那份原始代码自己算出来的**，
 * 包括那个年代的瑕疵（左上角触摸调试字、拖影、黑缺口、闪烁）。
 */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fxtk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define FB_W 480
#define FB_H 272

static uint16_t s_fb[FB_W * FB_H];
static int      s_wx0, s_wy0, s_wx1, s_wy1, s_cx, s_cy;
static SDL_Window   *s_win;
static SDL_Renderer *s_ren;
static SDL_Texture  *s_tex;
static int s_mx = -1, s_my = -1, s_down = 0;
static int s_scale = 1;
static long s_frames = 0;

/* ---- 可选: 逐帧导出 PPM（给需要精确抽帧的时候用；正常录屏走 Xephyr + x11grab） ---- */
static const char *s_dump_dir = NULL;
static int s_dump_every = 0;
static void dump_ppm(void)
{
    if (!s_dump_dir || !s_dump_every || (s_frames % s_dump_every)) return;
    char path[512];
    snprintf(path, sizeof path, "%s/f%06ld.ppm", s_dump_dir, s_frames);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", FB_W, FB_H);
    for (int i = 0; i < FB_W * FB_H; i++) {
        uint16_t p = s_fb[i];
        unsigned char rgb[3] = {
            (unsigned char)(((p >> 11) & 0x1F) * 255 / 31),
            (unsigned char)(((p >> 5) & 0x3F) * 255 / 63),
            (unsigned char)((p & 0x1F) * 255 / 31) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

/* ================= fx_driver_t: 内存帧缓冲 ================= */
static int sim_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return -1; }
    const char *sc = getenv("SIM_SCALE");
    if (sc) s_scale = atoi(sc) > 0 ? atoi(sc) : 1;
    s_dump_dir = getenv("SIM_DUMP");
    if (s_dump_dir) { const char *e = getenv("SIM_DUMP_EVERY"); s_dump_every = e ? atoi(e) : 1; }
    s_win = SDL_CreateWindow("fxtk (fxtk-rt 48c2ef9 复原) · 480x272 · PC sim",
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             FB_W * s_scale, FB_H * s_scale, SDL_WINDOW_SHOWN);
    if (!s_win) { fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); return -1; }
    s_ren = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_ACCELERATED);
    if (!s_ren) s_ren = SDL_CreateRenderer(s_win, -1, 0);
    s_tex = SDL_CreateTexture(s_ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, FB_W, FB_H);
    memset(s_fb, 0, sizeof s_fb);
    return 0;
}

static void sim_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    s_wx0 = x0; s_wy0 = y0; s_wx1 = x1; s_wy1 = y1;
    s_cx = x0; s_cy = y0;
}

static void sim_push(const uint16_t *px, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        if (s_cy >= 0 && s_cy < FB_H && s_cx >= 0 && s_cx < FB_W) s_fb[s_cy * FB_W + s_cx] = px[i];
        if (++s_cx > s_wx1) { s_cx = s_wx0; s_cy++; }
    }
}

static void sim_hold_begin(void) {}
static void sim_hold_end(void) {}

static void sim_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    if (x0 > x1) { uint16_t t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { uint16_t t = y0; y0 = y1; y1 = t; }
    if (x1 >= FB_W) x1 = FB_W - 1;
    if (y1 >= FB_H) y1 = FB_H - 1;
    for (int y = y0; y <= y1; y++) {
        uint16_t *row = &s_fb[y * FB_W];
        for (int x = x0; x <= x1; x++) row[x] = color;
    }
}

fx_driver_t fx_st6201_driver = {
    .width = FB_W, .height = FB_H,
    .init = sim_init,
    .set_window = sim_set_window,
    .push_pixels = sim_push,
    .hold_begin = sim_hold_begin,
    .hold_end = sim_hold_end,
    .fill_rect = sim_fill_rect,
    .touch_read = NULL,          /* 由 app 填 fx_gt911_read，和真机一样 */
};

/* ================= GT911 → 鼠标 ================= */
int fx_gt911_init(void) { return 0; }

int fx_gt911_read(int *x, int *y, int *pressed)
{
    if (x) *x = s_mx;
    if (y) *y = s_my;
    if (pressed) *pressed = s_down;
    return 1;   /* 非零 = 本轮有采样。fxtk.c 是 if (touch_read(...))，返回 0 会整段跳过 —— 踩过一次 */
}

/* ================= 呈现 + 事件（由 vTaskDelay 驱动） ================= */
void sim_present(void)
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { SDL_Quit(); exit(0); }
        if (e.type == SDL_MOUSEBUTTONDOWN) { s_down = 1; s_mx = e.button.x / s_scale; s_my = e.button.y / s_scale; }
        if (e.type == SDL_MOUSEBUTTONUP)   { s_down = 0; s_mx = e.button.x / s_scale; s_my = e.button.y / s_scale; }
        if (e.type == SDL_MOUSEMOTION)     { s_mx = e.motion.x / s_scale; s_my = e.motion.y / s_scale; }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) { SDL_Quit(); exit(0); }
    }
    SDL_UpdateTexture(s_tex, NULL, s_fb, FB_W * 2);
    SDL_RenderClear(s_ren);
    SDL_RenderCopy(s_ren, s_tex, NULL, NULL);
    SDL_RenderPresent(s_ren);
    dump_ppm();
    s_frames++;
}

/* ================= FreeRTOS stub：主循环的节拍就是这一刻 ================= */
void vTaskDelay(TickType_t ticks)
{
    sim_present();
    if (ticks) SDL_Delay((Uint32)ticks);
}

extern void app_main(void);

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    app_main();          /* 原始 app.c 的入口，一行没改 */
    return 0;
}
