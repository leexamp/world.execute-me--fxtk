/* movie.c —— world.execute(me); 单文件 MV · fxtk 渲染 · 多真实窗口
 * 只依赖 fxtk.h + fxtk_win.h (后端无关; 不出现任何 sokol/X11/Win32 调用)
 *
 * 构建 (多窗口 GL 后端):
 *   gcc -DFXTK_MW -O2 movie.c drivers/fxtk_sokol_mw.c drivers/fxtk_sokol_driver.c \
 *       drivers/fxtk_font_stb.c drivers/fxtk_win.c components/fxtk/*.c \
 *       -Icomponents/fxtk -Idrivers -Ithird_party/sokol -o world_execute_me \
 *       -lX11 -lXi -lXcursor -lGL -ldl -lpthread -lm
 * 运行: WEE_T0=0 WEE_DUR=212 ./world_execute_me
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif
#include "fxtk.h"
#include "fxtk_win.h"
#include "fxtk_mw.h"
#include "script.inc"
#define NSCR ((int)(sizeof(SCR)/sizeof(SCR[0])))

typedef struct { float t0, t1; const char *s; } LYR_T;
static const LYR_T LYR[] = {
  {   0.11f,   0.36f,"world.execute(me); - Mili"},
  {   0.36f,   1.30f,"Switch on the power line"},
  {   1.30f,   3.60f,"Remember to put on protection"},
  {   3.60f,   5.22f,"Lay down your pieces"},
  {   5.22f,   7.15f,"And let's begin object creation"},
  {   7.15f,  10.84f,"Fill in my data parameters initialisation"},
  {  10.84f,  12.58f,"Setup our new world"},
  {  12.58f,  29.31f,"And let's begin the simulation"},
  {  29.31f,  30.81f,"If I'm a set of points"},
  {  30.81f,  33.01f,"Then I will give you my dimension"},
  {  33.01f,  34.69f,"If I'm a circle"},
  {  34.69f,  36.68f,"Then I will give you my circumference"},
  {  36.68f,  38.32f,"If I'm a sine wave"},
  {  38.32f,  40.38f,"Then you can sit on all my tangents"},
  {  40.38f,  42.01f,"If I approach infinity"},
  {  42.01f,  44.07f,"Then you can be my limitations"},
  {  44.07f,  45.42f,"Switch my current"},
  {  45.42f,  47.31f,"To AC to DC"},
  {  47.31f,  49.16f,"And then blind my vision"},
  {  49.16f,  51.00f,"So dizzy so dizzy"},
  {  51.00f,  53.08f,"Oh we can travel"},
  {  53.08f,  54.70f,"To A.D to B.C"},
  {  54.70f,  56.77f,"And we can unite"},
  {  56.77f,  58.61f,"So deeply so deeply"},
  {  58.61f,  59.54f,"If I can"},
  {  59.54f,  62.78f,"If I can give you all the stimulations"},
  {  62.78f,  63.69f,"Then I can"},
  {  63.69f,  66.23f,"Then I can be your only satisfaction"},
  {  66.23f,  68.00f,"If I can make you happy"},
  {  68.00f,  70.16f,"I will run the execution"},
  {  70.16f,  71.47f,"Though we are trapped"},
  {  71.47f,  73.85f,"In this strange strange simulation"},
  {  73.85f,  75.25f,"If I'm an eggplant"},
  {  75.25f,  77.08f,"Then I will give you my nutrients"},
  {  77.08f,  78.85f,"If I'm a tomato"},
  {  78.85f,  80.99f,"Then I'll give you antioxidants"},
  {  80.99f,  82.40f,"If I'm a tabby cat"},
  {  82.40f,  84.65f,"Then I will purr for your enjoyment"},
  {  84.65f,  86.23f,"If I'm the only god"},
  {  86.23f,  88.18f,"Then you're the proof of my existence"},
  {  88.18f,  89.74f,"Switch my gender"},
  {  89.74f,  91.39f,"To F to M"},
  {  91.39f,  93.48f,"And then do whatever"},
  {  93.48f,  95.14f,"From AM to PM"},
  {  95.14f,  97.38f,"Oh switch my role"},
  {  97.38f,  99.16f,"To S to M"},
  {  99.16f, 101.07f,"So we can enter"},
  { 101.07f, 103.17f,"The trance the trance"},
  { 103.17f, 104.08f,"If I can"},
  { 104.08f, 106.83f,"If I can feel your vibrations"},
  { 106.83f, 107.77f,"Then I can"},
  { 107.77f, 110.30f,"Then I can finally be completion"},
  { 110.30f, 111.93f,"Though you have left"},
  { 111.93f, 112.85f,"You have left"},
  { 115.62f, 117.92f,"You have left me in isolation"},
  { 117.92f, 118.86f,"If I can"},
  { 118.86f, 121.84f,"If I can erase all the pointless fragments"},
  { 121.84f, 122.78f,"Then maybe"},
  { 122.78f, 125.29f,"Then maybe you won't leave me so disheartened"},
  { 125.29f, 128.53f,"Challenging your god"},
  { 128.53f, 147.58f,"You have made some illegal arguments"},
  { 147.58f, 151.61f,"Execution execution execution execution"},
  { 158.77f, 161.79f,"Ein Dos Trois Ne Fem Liu"},
  { 162.25f, 163.15f,"If I can"},
  { 163.15f, 166.16f,"If I can give them all the execution"},
  { 166.16f, 166.87f,"Then I can"},
  { 166.87f, 169.40f,"Then I can be your only execution"},
  { 169.40f, 171.69f,"If I can have you back"},
  { 171.69f, 173.08f,"I will run the execution"},
  { 173.08f, 174.63f,"Though we are trapped"},
  { 174.63f, 177.21f,"We are trapped ah"},
  { 177.21f, 177.92f,"I've studied"},
  { 177.92f, 180.69f,"I've studied how to properly lo-o-ove"},
  { 180.69f, 181.61f,"Question me"},
  { 181.61f, 184.61f,"Question me I can answer all lo-o-ove"},
  { 184.61f, 188.30f,"I know the algebraic expression of lo-o-ove"},
  { 188.30f, 189.49f,"Though you are free"},
  { 189.49f, 190.27f,"I am trapped"},
  { 190.27f, 205.62f,"Trapped in lo-o-ove"},
  { 205.62f, 207.00f,"Execution"},
  { 207.00f, 210.00f,"]"},
  {0.f,0.f,NULL}
};
#define NLYR ((int)(sizeof(LYR)/sizeof(LYR[0])-1))
static const float SONG_DUR = 212.33f;


#define NWIN 6
enum { UI_EDIT=0, UI_CMD=1, UI_SPEC=2, UI_CHAT=3, UI_PLAYER=4, UI_NOTIF=5 };
static const int SZ[NWIN][2]={{720,340},{720,140},{520,300},{460,180},{1200,78},{360,88}};
static const int ENT[NWIN]={0,1,2,4,0,116};
static const int EXT[NWIN]={186,196,190,192,9999,9999};
static const float LA[NWIN][2]={{ 24, 24},{ 24,406},{756, 24},{756,366},{40,588},{900,500}};
static const float LB[NWIN][2]={{ 24, 24},{ 24,406},{756, 24},{756,366},{40,588},{900,500}};
static const float LC[NWIN][2]={{ 36, 36},{ 36,418},{756, 36},{760,378},{40,588},{900,500}};

static int   g_win[NWIN];
static int   g_flash = -1;
static int   g_shown[NWIN];
static int   g_light = 0;
static double g_t0 = 0;
static float g_t = 0.f;
static int   g_ox = 0, g_oy = 0;   /* 1280x720 舞台在屏幕上的左上角 */
static float g_edit_h = 180.f;     /* 台词窗口高度: 随行数从 180 长到 380 */
static float g_sc = 1.f;
static int   g_ein_on = 0;
static float g_cw[6], g_chh[6];      /* 当前窗口尺寸(逻辑), 用于平滑缩放动画 */
static float g_zoom = 0.f;            /* 每句切画面时的缩放脉冲 */
static float g_nudge = 0.f;
static float g_lp = 0.f;             /* 大重排时的窗口缩放脉冲 */
static int   g_static = 0;            /* WEE_STATIC=1: 位置不做动画, 录屏更稳 */           /* 每句切画面时的窗口轻推 */            /* Ein/Dos/Trois 段: 六窗铺成 3x2 卡片 */           /* 舞台缩放: 1280x720 设计稿 -> 屏幕等比例 */
static int   g_dur = (int)(SONG_DUR);

static int R(int r,int g,int b){ return (r<<16)|(g<<8)|b; }
#define C(r,g,b) FX_RGB(r,g,b)
static fx_color_t c_bg(void){ return g_light?C(246,248,250):C(13,17,23); }
static fx_color_t c_panel(void){ return g_light?C(255,255,255):C(22,27,34); }
static fx_color_t c_txt(void){ return g_light?C(32,36,44):C(230,237,243); }
static fx_color_t c_dim(void){ return g_light?C(110,118,128):C(139,148,158); }
static int g_ch = 0;
static fx_color_t c_acc(void)
{
    static const unsigned char P[8][3] = {
        { 79,216,240},{121,192,255},{210,168,255},{126,231,135},
        {255,216,102},{255,138,128},{180,220,255},{140,255,220} };
    int k = g_ch & 7;
    if (g_light) return C(30,95,180);
    return C(P[k][0],P[k][1],P[k][2]);
}
static fx_color_t c_bord(void){ return g_light?C(224,228,234):C(48,54,61); }
static fx_color_t mixc(fx_color_t a, fx_color_t b, float f)
{
    int ar=(a>>16)&255, ag=(a>>8)&255, ab=a&255;
    int br=(b>>16)&255, bg=(b>>8)&255, bb=b&255;
    return C((int)(ar+(br-ar)*f),(int)(ag+(bg-ag)*f),(int)(ab+(bb-ab)*f));
}
static fx_color_t c_grn(void){ return C(63,185,80); }
static fx_color_t c_yel(void){ return C(210,153,34); }

/* fxtk 绘制小工具 (坐标为窗口客户区左上原点) */
static void rf(int x,int y,int w,int h,fx_color_t c){ fx_set_color(c); fx_fill_rect((int)(x*g_sc),(int)(y*g_sc),(int)((x+w)*g_sc)-1,(int)((y+h)*g_sc)-1); }
static void tx(int sz,int x,int y,const char*s,fx_color_t fg,fx_color_t bg){ fxtk_draw_text_size((int)(sz*g_sc+0.5f),(int)(x*g_sc),(int)(y*g_sc),s,fg,bg); }
static void ln(int x1,int y1,int x2,int y2,fx_color_t c)
{
    fx_set_color(c);
    int X1=(int)(x1*g_sc), Y1=(int)(y1*g_sc), X2=(int)(x2*g_sc), Y2=(int)(y2*g_sc);
    int t=(g_sc>=1.4f)?3:2;                       /* 线宽: 大屏/手机端都看得清 */
    if(X1==X2 || Y1==Y2){                         /* 横竖线: 直接加粗 */
        for(int i=0;i<t;i++){ fx_draw_line(X1,Y1+i,X2,Y2+i); }
    } else {
        for(int i=0;i<t;i++){ fx_draw_line(X1,Y1+i,X2,Y2+i); }
        fx_draw_line(X1+1,Y1,X2+1,Y2);
    }
}

static int lyric_idx(float t)
{
    int idx=0;
    for (int i=0;i<NLYR;i++){ if (t>=LYR[i].t0) idx=i; if (t>=LYR[i].t0 && t<LYR[i].t1) return i; }
    return idx;
}
static const char *lyric_at(float t)
{
    static const char *last = "";
    for (int i=0;i<NLYR;i++){
        if (t>=LYR[i].t0 && t<LYR[i].t1) { last=LYR[i].s; return last; }
        if (t>=LYR[i].t0) last=LYR[i].s;
    }
    if (t < (NLYR?LYR[0].t0:0.f)) return "";
    return last[0]?last:"\xe2\x99\xaa";
}

/* ---------------- 内容 ---------------- */
static const char *LOG[]={
 "$ ./world.execute(me);",
 "[boot] loading lyric timeline ... ok",
 "[boot] 81 lines / 212.33s",
 "[sim ] timeline 001 .. 212 mapped",
 "[sim ] windows 6 / layout A",
 "[sim ] clock 30fps  drift 0.0ms",
 "[sim ] theme: night",
 "[sim ] ready",
 NULL };

static const char *CODE[]={
 "static void director_tick(wm_t *w, float dt) {",
 "    for (int i = 0; i < w->n; i++) {",
 "        win_t *win = &w->win[i];",
 "        set_window_pos(win->hwnd, win->x, win->y);",
 "        invalidate(win->hwnd);",
 "    }",
 "}",
 NULL };

static const char *CHAPTERS[]={
 "01 史前 fe7a2c5","02 Add files via upload","03 RGB565 时代","04 2.0 更新 a65b15d","05 v2.1 5511c69",
 "06 0x0841 第一块色","07 7d3ecb1 删 linux_sim","08 v1.0 上 PC","09 v1.2 框架雏形","10 SDL 后端",
 "11 2816 控件压测","12 sokol 像素级 <1%","13 cec3b00 残影","14 v2.3 控件池减半","15 f910ceb 实测否证",
 "16 单 exe 144KB","17 输入法 XIM","18 v2.4 后端策略","19 v2.5 多窗口","20 六扇真实窗口",
 "21 1280x720 缩放","22 每句歌词一张图","23 61 项自检","24 v2.5 ready","25 216 commits",
 NULL };


static void draw_cmd(int w,int h)
{
    static const char *SETS[8][4]={
      { "$ probe /dev/ttyUSB0", "[spi] st6201 ... ok", "[spi] gt911  ... ok", "[spi] 40MHz rejected / 20MHz ok" },
      { "$ ./fxtk_sim --draw", "[gfx] circle: 三角扇 64 seg", "[gfx] sine: 逐点连线 480 pt", "[perf] 68-74 fps · 2820 控件 · 13.4ms" },
      { "[warn] 残影: 亮列 66 -> 190", "[fix] cec3b00 复用地脏区", "[fix] 先铺背景, 再重画相交", "[test] ghost columns 64-68 (ok)" },
      { "[link] usb 1-2: disconnect", "[link] /dev/ttyUSB0 gone", "$ grep -r ttyUSB0", "no device found" },
      { "[env] SDL -> sokol", "[cmp] pixel diff < 1%", "[api] multi-window: NO", "[api] borderless 38px: NO" },
      { "$ git show --stat 7d3ecb1", "Delete linux_sim directory", "[fix] 46KB fake -> 8248 B", "[fix] dup fx_cpu_count removed" },
      { "[res] 480x272 800x480 1280x720", "[res] 1920x1080 2560x1440", "[res] 720x1280", "[dpi] sapp_dpi_scale() = 1.000" },
      { "[math] x = 16 sin^3 t", "[math] y = 13cos t - 5cos2t - ...", "[draw] anim(1) 自绘心形", "[out] fxtk v2.5 ready." }
    };
    static const unsigned char MAP[25]={0,0,0,0,1,1,0,1,1,1,2,2,2,3,3,3,3,4,4,5,5,6,6,7,7};
    int seti=MAP[(g_ch<0?0:(g_ch>24?24:g_ch))];
    const char *const *L=SETS[seti];
    rf(0,0,w,h,c_bg());
    int n=(int)(g_t*1.2f); if(n>4)n=4;
    for(int i=0;i<n;i++){
        const char *line=L[i];
        char tmp[160];
        if (i==n-1){
            float age=g_t-(float)(n)/1.2f;
            int nc=(int)(age*46.f); if(nc<0)nc=0;
            int len=(int)strlen(line); if(nc>len)nc=len;
            memcpy(tmp,line,(size_t)nc); tmp[nc]=0; line=tmp;
        }
        tx(13,12,10+i*24,line,L[i][0]=='$'?c_acc():c_dim(),c_bg());
    }
    if((int)(g_t*2)%2==0) rf(12+(n?(int)strlen(L[n-1])*7:0),10+(n?(n-1)*24:0),7,14,c_acc());
    rf(12,h-22,w-24,5,c_panel());
    rf(12,h-22,(int)((w-24)*(0.25f+0.25f*sinf(g_t*0.7f))),5,c_acc());
}

static int scr_last(void)
{
    int n=0;
    for (int i=0;i<NSCR;i++) if (g_t>=SCR[i].t) n=i+1;
    return n-1;
}

static void draw_edit(int w,int h)
{
    rf(0,0,w,h,c_bg());
    rf(0,0,w,28,c_panel());
    tx(15,12,5,"fxtk 发展史 - 台词",c_txt(),c_panel());
    rf(0,27,w,1,c_bord());
    int last=scr_last(); if(last<0) last=0;
    int maxu=(w-60)/11; if(maxu<16) maxu=16;   /* 折行按宽度单位: 英文=1, 汉字=2 */
    int lh=32, y=h-18;
    for(int i=last;i>=0 && y>lh+48;i--){
        const char *t=SCR[i].s;
        int len=(int)strlen(t), units=0;
        for(int k=0;k<len;){ unsigned char c=(unsigned char)t[k];
            k += (c<0x80)?1:(((c>>5)==6)?2:(((c>>4)==14)?3:4)); units += (c<0x80)?1:2; }
        int segs=(units+maxu-1)/maxu; if(segs<1)segs=1;
        int bh=segs*lh; y-=bh+2;
        if(y<48) break;
        int yy=y+2, off=0;
        for(int sg=0;sg<segs && off<len;sg++){
            char buf[1024]; int bi=0,cnt=0,k=off;
            while(t[k] && bi<(int)sizeof buf-8 && t[k]!='\n'){
                unsigned char c=(unsigned char)t[k];
                int adv=(c<0x80)?1:(((c>>5)==6)?2:(((c>>4)==14)?3:4));
                int un=(c<0x80)?1:2;
                if(cnt+un>maxu) break;
                for(int j=0;j<adv && t[k];j++) buf[bi++]=t[k++];
                cnt+=un;
            }
            off=k;
            if(off<len && t[off]=='\n') off++;
            buf[bi]=0;
            if(bi==0) break;
            {   fx_color_t col = SCR[i].spk=='u'?c_acc():(SCR[i].spk=='d'?c_dim():c_txt());
                if (g_t > 208.f) { float ef=(g_t-208.f)/4.f; if(ef>1)ef=1; ef=ef*ef; col=mixc(col,c_bg(),ef); }
                float age = g_t - SCR[i].t;
                if (i==last && age>=0.f && age<0.35f) { int o2=(int)((0.35f-age)/0.35f*16.f); yy+=o2; }
                if (i==last && age>=0.f && age<0.7f && SCR[i].spk!='u') col=c_acc();
                if (sg==0){
                    const char *pre = (SCR[i].spk=='a')?"我 · ":((SCR[i].spk=='u')?"你 · ":"");
                    if (pre[0]){ tx(17,16,yy,pre,col,c_bg()); }
                    tx(20,16+(pre[0]?52:0),yy,buf,col,c_bg());
                } else tx(20,16,yy,buf,col,c_bg()); }
            yy+=lh;
        }
    }
}


static void FE(int x,int y,int rx,int ry,fx_color_t c){ fx_set_color(c); fx_fill_ellipse(x,y,rx,ry); }
static void DE(int x,int y,int rx,int ry,fx_color_t c){ fx_set_color(c); fx_draw_ellipse(x,y,rx,ry); }
static void FTR(int x1,int y1,int x2,int y2,int x3,int y3,fx_color_t c){ fx_set_color(c); fx_fill_triangle(x1,y1,x2,y2,x3,y3); }
static void FRR(int x1,int y1,int x2,int y2,int rr,fx_color_t c){ fx_set_color(c); fx_fill_rect_round(x1,y1,x2,y2,rr); }
static void FC(int x,int y,int r,fx_color_t c){ fx_set_color(c); fx_fill_circle((int)(x*g_sc),(int)(y*g_sc),(int)(r*g_sc)); }
static void DC(int x,int y,int r,fx_color_t c){ fx_set_color(c); fx_draw_circle((int)(x*g_sc),(int)(y*g_sc),(int)(r*g_sc)); }
static void DA(int x,int y,int r,int a1,int a2,fx_color_t c){ fx_set_color(c); fx_draw_arc((int)(x*g_sc),(int)(y*g_sc),(int)(r*g_sc),a1,a2); }
static void DRR(int x1,int y1,int x2,int y2,int rr,fx_color_t c){ fx_set_color(c); fx_draw_rect_round((int)(x1*g_sc),(int)(y1*g_sc),(int)(x2*g_sc),(int)(y2*g_sc),(int)(rr*g_sc)); }
static void DR(int x1,int y1,int x2,int y2,fx_color_t c){ fx_set_color(c); fx_draw_rect((int)(x1*g_sc),(int)(y1*g_sc),(int)(x2*g_sc),(int)(y2*g_sc)); }
static unsigned hsh32(unsigned x){ x^=x>>16; x*=0x7feb352dU; x^=x>>15; x*=0x846ca68bU; x^=x>>16; return x; }
static float frnd(int a,int b){ return (float)(hsh32((unsigned)(a*7919+b*104729))%100000u)/100000.f; }
static float fsin(float a){ return sinf(a); }
static float fcos(float a){ return cosf(a); }

/* ---------- 频谱条 (底部窄条, 跟歌词同步) ---------- */
static void art_spec_strip(int x0,int y0,int w,int h)
{
    int li=lyric_idx(g_t);
    float dtl=g_t-(NLYR?LYR[li].t0:0.f);
    float hit=expf(-dtl*3.2f);
    static const unsigned char HOT[25]={0,0,0,0,1,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,1,0,0,1,0};
    float hot=HOT[g_ch]?1.5f:1.f;
    int n=36,bw=(w-4)/n;
    for(int i=0;i<n;i++){
        float f=0.5f+0.5f*fsin(i*0.55f+g_t*2.6f+li*0.7f);
        float f2=0.5f+0.5f*fsin(i*0.21f-g_t*1.7f+li*0.3f);
        float a=(0.08f+0.6f*f*f2)*hot + hit*0.35f*(0.6f+0.4f*fsin(i*0.9f+li));
        if(a<0.04f)a=0.04f; if(a>1)a=1;
        int bh=(int)((h-3)*a); if(bh<2)bh=2;
        rf(x0+2+i*bw, y0+h-1-bh, bw-1, bh, c_acc());
    }
}

/* ---------- 每章一张生成艺术图 ---------- */
/* 单片机段: 模拟 /vm/rt_sim 里那台 480x272 的小屏 (fxtk-rt@48c2ef9) */
static void draw_device(int x,int y,int w,int h)
{
    rf(x-3,y-3,w+6,h+9,c_bord());
    rf(x,y,w,h,C(6,9,12));
    int ph=(int)(g_t*3.f);
    for(int i=0;i<5;i++){
        int bh=(int)(h*0.18f*(0.4f+0.6f*fabsf(sinf(ph*0.3f+i))));
        rf(x+6+i*(w-12)/6, y+h-10-bh, (w-12)/6-2, bh, i%2?C(60,200,160):C(230,180,70));
    }
    fx_color_t tc=c_acc();
    tx(9,x+6,y+5,"fxtk-rt @48c2ef9",tc,C(6,9,12));
    tx(8,x+6,y+18,"480x272 · SPI 20MHz",c_dim(),C(6,9,12));
    tx(8,x+6,y+30,"st6201 ok / gt911 ok",c_dim(),C(6,9,12));
    tx(8,x+6,y+42,"lvgl: no fit -> gfx drv",c_dim(),C(6,9,12));
}
/* 把第 i 扇窗的客户区左上角放到舞台坐标 (lx,ly); 自动扣掉标题栏/边框 */
static int g_lx[NWIN], g_ly[NWIN];      /* 上次真正移动到的位置, 避免每帧重复 XMoveWindow 引起闪烁 */
static void win_place(int i,float lx,float ly)
{
    if(i<0 || i>=NWIN) return;
    int ox=0,oy=0; fxtk_win_client_offset(g_win[i],&ox,&oy);
    int px=(int)(lx*g_sc)+g_ox-ox, py=(int)(ly*g_sc)+g_oy-oy;
    if(px==g_lx[i] && py==g_ly[i]) return;   /* 位置没变就不动窗口 */
    g_lx[i]=px; g_ly[i]=py;
    fxtk_win_move(g_win[i],px,py);
}

/* 歌词 -> 画面: 每句歌词按关键字挑一张图, 关键字认不出时退回章节图 */
static int lyr_scene(int li)
{
    if (li<0 || li>=NLYR) return -1;
    const char *y=LYR[li].s;
    char yl[192]; { int i=0; for(; y[i] && i<191; i++){ char c=y[i]; yl[i]=(c>='A'&&c<='Z')?(char)(c+32):c; } yl[i]=0; }
    #define HAS(k) (strstr(yl,k)!=NULL)
    if (HAS("power line")||HAS("switch on")) return 1;
    if (HAS("object creation")||HAS("object")) return 4;
    if (HAS("set of points")||HAS("points")) return 8;
    if (HAS("circle")) return 9;
    if (HAS("circumference")) return 9;
    if (HAS("sine")||HAS("wave")) return 1;
    if (HAS("tangent")) return 10;
    if (HAS("current")) return 13;
    if (HAS("dizzy")||HAS("deeply")) return 15;
    if (HAS("stimulation")) return 7;
    if (HAS("execution")) return 22;
    if (HAS("trapped")||HAS("isolation")) return 14;
    if (HAS("infinity")) return 12;
    if (HAS("dimension")) return 8;
    if (HAS("limitation")) return 11;
    if (HAS("simulation")) return 7;
    if (HAS("travel")) return 16;
    if (HAS("unite")) return 18;
    if (HAS("deploy")) return 19;
    if (HAS("satisfaction")||HAS("lo-o-ove")||HAS("love")) return 21;
    if (HAS("new world")) return 6;
    if (HAS("protection")) return 2;
    if (HAS("pieces")) return 3;
    if (HAS("parameters")) return 5;
    if (HAS("vibration")) return 5;
    if (HAS("completion")) return 20;
    if (HAS("god")) return 16;
    if (HAS("free")) return 14;
    if (HAS("question me")) return 10;
    if (HAS("illegal arguments")) return 11;
    #undef HAS
    return -1;
}


/* ---------- 每句歌词一张图: 82 行作曲表 + 40 种构图 ---------- */
typedef struct { unsigned char m,a,b,c,bg; } LART_T;
static const LART_T LART[] = {
 {47,3,2,0,0},{45,8,5,1,1},{ 2,6,2,2,2},{ 3,4,5,3,3},{ 4,9,2,0,4},{ 5,7,5,1,5},{ 6,5,2,2,6},{ 7,3,5,3,7},
 { 7,8,2,0,0},{ 8,6,5,1,1},{ 4,4,2,2,2},{ 9,9,5,3,3},{ 1,7,2,0,4},{10,5,5,1,5},{12,3,2,2,6},{11,8,5,3,7},
 {61,6,2,0,0},{61,4,5,1,1},{14,9,2,2,2},{15,7,5,3,3},{16,5,2,0,4},{17,3,5,1,5},{18,8,2,2,6},{15,6,5,3,7},
 {25,4,2,0,0},{ 7,9,5,1,1},{25,7,2,2,2},{21,5,5,3,3},{26,3,2,0,4},{22,8,5,1,5},{14,6,2,2,6},{23,4,5,3,7},
 {41,9,2,0,0},{51,7,5,1,1},{42,5,2,2,2},{52,3,5,3,3},{43,8,2,0,4},{53,6,5,1,5},{44,4,2,2,6},{54,9,5,3,7},
 {56,7,2,0,0},{56,5,5,1,1},{23,3,2,2,2},{60,8,5,3,3},{56,6,2,0,4},{56,4,5,1,5},{10,9,2,2,6},{59,7,5,3,7},
 {25,5,2,0,1},{57,3,5,1,1},{25,8,2,2,3},{58,6,5,3,3},{27,4,2,0,4},{27,9,5,1,5},{27,7,2,2,6},{25,5,5,3,7},
 { 3,3,2,0,0},{25,8,5,1,4},{21,6,2,2,2},{44,4,5,3,3},{11,9,2,0,4},{22,7,5,1,6},{28,5,2,2,6},{25,3,5,3,5},
 {22,8,2,0,0},{25,6,5,1,6},{22,4,2,2,2},{21,9,5,3,4},{22,7,2,0,4},{14,5,5,1,5},{14,3,2,2,7},{35,8,5,3,7},
 {21,6,2,0,0},{21,4,5,1,1},{21,9,2,2,5},{21,7,5,3,6},{ 6,5,2,0,4},{14,3,5,1,0},{14,8,2,2,1},{22,6,5,3,7},
 {29,4,2,0,0},
};
typedef char lart_size_check[((int)(sizeof LART/sizeof LART[0])==NLYR)?1:-1];
#define LART_N ((int)(sizeof LART/sizeof LART[0]))

/* 背景纹样: 每句一句的底纹, 0 实底 1 网格 2 点 3 扫描线 4 横带 5 十字 6 斜线 7 星点 */
static void draw_bg(int st,int ax,int ay,int aw,int ah)
{
    fx_color_t dim=c_dim(), bord=c_bord();
    switch(st%8){
    case 1: for(int x=ax;x<ax+aw;x+=26) ln(x,ay,x,ay+ah,bord);
            for(int y=ay;y<ay+ah;y+=26) ln(ax,y,ax+aw,y,bord); break;
    case 2: for(int i=0;i<70;i++) rf(ax+(int)(frnd(i,1)*aw),ay+(int)(frnd(i,2)*ah),1,1,dim); break;
    case 3: for(int y=ay;y<ay+ah;y+=5) ln(ax,y,ax+aw,y,bord); break;
    case 4: for(int y=ay;y<ay+ah;y+=24) rf(ax,y,aw,10,(y/24)%2?C(14,18,24):C(18,23,30)); break;
    case 5: ln(ax+aw/2,ay,ax+aw/2,ay+ah,bord); ln(ax,ay+ah/2,ax+aw,ay+ah/2,bord); break;
    case 6: for(int x=-ah;x<aw;x+=22) ln(ax+x,ay,ax+x+ah,ay+ah,bord); break;
    case 7: for(int i=0;i<24;i++) FC(ax+(int)(frnd(i,3)*aw),ay+(int)(frnd(i,4)*ah),1,dim); break;
    default: break;
    }
}

/* ---------- 三个实物: 茄子 / 番茄 / 猫 (只用基础图元) ---------- */
static void obj_shadow(int cx,int cy,int rw,int rh){ FE(cx,cy,rw,rh,C(16,20,26)); }
static void obj_eggplant(int cx,int cy,int s)
{
    int rx=(int)(s*0.15f), ry=(int)(s*0.25f);
    fx_color_t body=C(122,62,164), dark=C(98,46,134), lite=C(170,112,212), grn=C(92,168,88);
    obj_shadow(cx,cy+ry, (int)(rx*1.0f), (int)(s*0.055f));
    FRR(cx-rx, cy-ry, cx+rx, cy+(int)(ry*0.86f), rx, body);
    FRR(cx-(int)(rx*0.62f), cy-ry-(int)(s*0.055f), cx+(int)(rx*0.62f), cy-ry+(int)(s*0.10f), (int)(rx*0.62f), body);
    FE(cx+(int)(rx*0.55f), cy+(int)(ry*0.12f), (int)(rx*0.32f), (int)(ry*0.72f), dark);
    FE(cx-(int)(rx*0.42f), cy-(int)(ry*0.18f), (int)(rx*0.13f), (int)(ry*0.55f), lite);
    for(int i=0;i<6;i++){ float a=i*6.2831853f/6.f;
        int lx=cx+(int)(cosf(a)*rx*1.25f), ly=cy-ry-(int)(s*0.01f)+(int)(sinf(a)*rx*0.55f);
        FTR(cx,cy-ry-2, lx,ly, cx+(int)(cosf(a+0.5f)*rx*0.75f), cy-ry-2+(int)(sinf(a+0.5f)*rx*0.32f), (i%2)?grn:C(76,148,72)); }
    ln(cx,cy-ry-2,cx+2,cy-ry-(int)(s*0.12f),grn);
    ln(cx+2,cy-ry-(int)(s*0.12f),cx+12,cy-ry-(int)(s*0.14f),grn);
}
static void obj_tomato(int cx,int cy,int s)
{
    int r=(int)(s*0.22f);
    fx_color_t red=C(212,64,56), lite=C(242,124,112), grn=C(90,170,88);
    obj_shadow(cx,cy+r,(int)(r*0.95f),(int)(s*0.05f));
    FRR(cx-r,cy-r,cx+r,cy+r,r,red);
    DC(cx,cy,r,C(180,48,44));
    FE(cx-(int)(r*0.34f),cy-(int)(r*0.38f),(int)(r*0.20f),(int)(r*0.14f),lite);
    FC(cx-(int)(r*0.40f),cy-(int)(r*0.44f),2,C(255,236,230));
    for(int i=0;i<6;i++){ float a=i*6.2831853f/6.f-1.5708f;
        int lx=cx+(int)(cosf(a)*r*0.85f), ly=cy-r+2+(int)(sinf(a)*r*0.42f);
        FTR(cx,cy-r+2, lx,ly, cx+(int)(cosf(a+0.42f)*r*0.45f), cy-r+2+(int)(sinf(a+0.42f)*r*0.30f), (i%2)?grn:C(74,146,70)); }
    FRR(cx-4,cy-r-2,cx+4,cy-r+4,4,grn);
    ln(cx,cy-r-2,cx+3,cy-r-(int)(s*0.10f),grn);
    ln(cx+3,cy-r-(int)(s*0.10f),cx+12,cy-r-(int)(s*0.12f),grn);
}
static void obj_cat(int cx,int cy,int s,int purr)
{
    int rx=(int)(s*0.28f), ry=(int)(s*0.25f);
    fx_color_t fur=C(232,196,140), fur2=C(214,172,116), pink=C(242,178,172),
                dark=C(44,44,52), white=C(250,248,240);
    obj_shadow(cx,cy+ry+4,(int)(rx*0.8f),(int)(s*0.045f));
    FTR(cx-rx+8,cy-ry+12, cx-rx-12,cy-ry-26, cx-rx+34,cy-ry-4, fur);
    FTR(cx-rx+8,cy-ry+10, cx-rx-6,cy-ry-18, cx-rx+26,cy-ry-2, pink);
    FTR(cx+rx-8,cy-ry+12, cx+rx+12,cy-ry-26, cx+rx-34,cy-ry-4, fur);
    FTR(cx+rx-8,cy-ry+10, cx+rx+6,cy-ry-18, cx+rx-26,cy-ry-2, pink);
    FRR(cx-rx,cy-ry,cx+rx,cy+ry,ry,fur);
    for(int i=0;i<3;i++) ln(cx-12+i*12,cy-ry+8,cx-8+i*12,cy-ry+(int)(ry*0.5f),fur2);
    ln(cx-rx+6,cy-6,cx-rx+24,cy-10,fur2); ln(cx+rx-6,cy-6,cx+rx-24,cy-10,fur2);
    FE(cx-rx/3,cy-2,(int)(rx*0.15f),(int)(ry*0.19f),dark);
    FE(cx+rx/3,cy-2,(int)(rx*0.15f),(int)(ry*0.19f),dark);
    FC(cx-rx/3-3,cy-7,2,white); FC(cx+rx/3-3,cy-7,2,white);
    FTR(cx-5,cy+ry/5,cx+5,cy+ry/5,cx,cy+ry/5+6,pink);
    DA(cx-8,cy+ry/5+8,7,20,160,dark); DA(cx+8,cy+ry/5+8,7,20,160,dark);
    for(int i=0;i<3;i++){ int yy=cy+ry/5+i*5;
        ln(cx-rx-18,yy-3,cx-rx/2,yy,white); ln(cx+rx+18,yy-3,cx+rx/2,yy,white); }
    FC(cx-rx/4,cy+ry/3,2,white); FC(cx+rx/4,cy+ry/3,2,white);
    if(purr) for(int k=0;k<3;k++) DA(cx+rx+22,cy,(int)(ry*0.55f)+k*9,300,420,(k==1)?c_acc():c_bord());
}
static void draw_motif(int m,int pa,int pb,int pc,int ax,int ay,int aw,int ah)
{
    fx_color_t acc=c_acc(), dim=c_dim(), txt=c_txt(), bord=c_bord();
    fx_color_t col = (pc==1)?dim:((pc==2)?txt:((pc==3)?c_grn():acc));
    int cx=ax+aw/2, cy=ay+ah/2;
    int n=2+pa*3; if(n>40)n=40;
    switch(m){
    case 0:{   /* 控制台光标 */
        rf(ax+16,ay+16,aw-32,ah-32,C(14,18,24));
        for(int i=0;i<n/3+1;i++){ int yy=ay+30+i*16; if(yy>ay+ah-40)break;
            rf(ax+26,yy,20+(int)(frnd(i,pb)*aw*0.3f),3,col); }
        if(((int)(g_t*pb))%2==0) rf(ax+26,ay+ah-58,10,16,col);
    } break;
    case 1:{   /* 电源线/正弦 */
        int y0=cy, px=-1,py=0;
        for(int x=0;x<aw-40;x+=3){ int yy=y0-(int)(sinf(x*0.05f*pb*0.3f+g_t*3.f)*ah*0.28f);
            if(px>=0)ln(ax+20+px,py,ax+20+x,yy,col); px=x;py=yy; }
        rf(ax+20,cy-1,aw-40,1,bord);
    } break;
    case 2:{   /* 保护罩 + 扫描 */
        for(int k=0;k<n/3+1;k++) DR(ax+20+k*8,ay+14+k*8,ax+aw-20-k*8,ay+ah-14-k*8,col);
        rf(ax+22,ay+14+(int)((ah-28)*(0.5f+0.5f*sinf(g_t*pb*0.4f))),aw-44,2,acc);
    } break;
    case 3:{   /* 拼装块 */
        int cols=6, rows=4, cw=(aw-40)/cols, chh=(ah-40)/rows;
        for(int r=0;r<rows;r++)for(int c2=0;c2<cols;c2++){
            int off=(int)((fsin(g_t*pb*0.4f+frnd(r,c2)*6.f)*0.5f+0.5f)*7);
            rf(ax+20+c2*cw+2, ay+20+r*chh+2-off, cw-4, chh-4, ((r*cols+c2)%pa==0)?acc:bord);
        }
    } break;
    case 4:{   /* 圆 */
        int r=(int)(ah*0.30f*(0.8f+0.2f*fsin(g_t*pb*0.3f)));
        FC(cx,cy,r,col); DC(cx,cy,r+3,col);
        ln(cx,cy,cx+r,cy,dim);
    } break;
    case 5:{   /* 参数滑杆 */
        for(int i=0;i<4;i++){
            int yy=ay+20+i*((ah-40)/4);
            rf(ax+50,yy-1,aw-100,2,bord);
            rf(ax+50+(int)((aw-100)*(0.5f+0.45f*fsin(g_t*pb*0.4f+i*1.7f)))-4,yy-6,8,12,col);
        }
    } break;
    case 6:{   /* 星野 */
        for(int i=0;i<n*2;i++){
            int sx=ax+20+(int)(frnd(i,pa)*aw*0.8f), sy=ay+14+(int)(frnd(i,pa+1)*ah*0.72f);
            rf(sx,sy,1+(int)(frnd(i,3)*2),1+(int)(frnd(i,4)*2),(i%(pa+2)==0)?acc:dim);
        }
        ln(ax+16,ay+ah-24,ax+aw-16,ay+ah-24,bord);
    } break;
    case 7:{   /* 粒子 */
        for(int i=0;i<n*3;i++){
            float ph=frnd(i,pa)*6.28f, sp=0.4f+frnd(i,pb);
            int px=ax+16+(int)((0.5f+0.5f*fsin(g_t*sp+ph))*aw*0.8f);
            int py=ay+14+(int)((0.5f+0.5f*fcos(g_t*sp*0.7f+ph))*ah*0.72f);
            FC(px,py,1+(int)(frnd(i,3)*2),col);
        }
    } break;
    case 8:{   /* 立方体 */
        static const float V[8][3]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
        static const int E[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        float a=g_t*(0.4f+pb*0.1f), ca=cosf(a), sa=sinf(a), x2[8],y2[8];
        for(int i=0;i<8;i++){ float x=V[i][0], y=V[i][1], z=V[i][2];
            float xr=x*ca-z*sa, zr=x*sa+z*ca, yv=y*0.9f-zr*0.35f, sc=1.f/(2.6f+zr);
            x2[i]=cx+xr*sc*aw*0.8f; y2[i]=cy+yv*sc*ah*0.8f; }
        for(int i=0;i<12;i++) ln((int)x2[E[i][0]],(int)y2[E[i][0]],(int)x2[E[i][1]],(int)y2[E[i][1]],col);
    } break;
    case 9:{   /* 圆周 */
        int r=(int)(ah*0.30f);
        DC(cx,cy,r,col);
        DA(cx,cy,r+5,0,(int)(fmodf(g_t*pb*40.f,360.f)),acc);
        ln(cx,cy,cx+r,cy,dim);
    } break;
    case 10:{   /* 切线 */
        int y0=cy, px=-1,py=0;
        for(int x=0;x<aw-40;x++){ int yy=y0-(int)(sinf(x*0.05f*pb*0.25f+g_t*2.f)*ah*0.3f);
            if(px>=0)ln(ax+20+px,py,ax+20+x,yy,col); px=x;py=yy; }
    } break;
    case 11:{   /* 限制/裁切条 */
        for(int i=0;i<10;i++){ if(i%2==0) rf(ax+16+i*(aw-32)/10,ay+16,(aw-32)/10-3,ah-32,bord); }
        rf(ax+16+(int)(fmodf(g_t*pb*20.f,aw-32)),ay+16,2,ah-32,col);
    } break;
    case 12:{   /* 无限 */
        int px=-1,py=0;
        for(int i=0;i<=120;i++){ float t2=i/120.f*6.2831853f; float d=1.f+sinf(t2)*sinf(t2);
            float X=cosf(t2)/d, Y=sinf(t2)*cosf(t2)/d;
            int sx=cx+(int)(X*aw*0.38f), sy=cy-(int)(Y*ah*0.8f);
            if(px>=0)ln(px,py,sx,sy,col); px=sx;py=sy; }
    } break;
    case 13:{   /* 交/直流 */
        int y1=ay+ah/3, y2=ay+ah*2/3; ln(ax+16,y1,ax+aw-16,y1,bord); ln(ax+16,y2,ax+aw-16,y2,bord);
        int px=-1,py; for(int x=0;x<aw-32;x++){ int yy=y1-(int)(sinf(x*0.08f+g_t*pb*0.3f)*ah*0.12f);
            if(px>=0)ln(ax+16+px,py,ax+16+x,yy,acc); px=x;py=yy; }
        for(int x=0;x<aw-32;x+=3) rf(ax+16+x,y2,2,1,dim);
    } break;
    case 14:{   /* 盲点/隔离 */
        rf(ax+16,ay+16,aw-32,ah-32,C(0,0,0));
        for(int i=0;i<n*2;i++) FC(ax+30+(int)(frnd(i,pa)*aw*0.62f), ay+30+(int)(frnd(i,pa+2)*ah*0.5f), 2,(i==3)?acc:dim);
        for(int i=0;i<8;i++) FC(ax+30+i*(aw-60)/7, ay+ah-34, 1+(i==3?2:0),acc);
    } break;
    case 15:{   /* 眩晕螺旋 */
        int px=-1,py=0; for(int i=0;i<=110;i++){ float t2=i/110.f;
            float ang=t2*6.2831853f*2.5f+g_t*pb*0.3f; float rad=(0.08f+t2*0.5f)*ah*0.3f;
            int sx=cx+(int)(cosf(ang)*rad), sy=cy+(int)(sinf(ang)*rad*0.8f);
            if(px>=0)ln(px,py,sx,sy,col); px=sx;py=sy; }
    } break;
    case 16:{   /* 隧道 */
        for(int k=0;k<n/2+2;k++){ float ph=fmodf(k+g_t*pb*0.3f,8.f)/8.f; float sc=ph*ph;
            int rw=(int)(aw*0.45f*(0.15f+sc)), rh=(int)(ah*0.45f*(0.15f+sc));
            DR(cx-rw,cy-(int)(rh*0.7f),cx+rw,cy+(int)(rh*0.7f),col); }
    } break;
    case 17:{   /* 时间轴 */
        int y0=cy; ln(ax+16,y0,ax+aw-16,y0,dim);
        for(int i=0;i<=n;i++){ int xx=ax+16+i*(aw-32)/n; ln(xx,y0-5,xx,y0+5,bord); }
        rf(ax+16+(int)((aw-32)*0.5f)-2,y0-14,5,9,acc);
    } break;
    case 18:{   /* 双圆合并 */
        int d=(int)(ah*0.4f*(0.5f+0.5f*fsin(g_t*pb*0.3f)));
        DC(cx-d,cy,ah/4,col); DC(cx+d,cy,ah/4,col);
        if(d<ah/4) FC(cx,cy,(ah/4-d),acc);
    } break;
    case 19:{   /* 分辨率格子 */
        static const char *R[6]={"480x272","800x480","1280x720","1920x1080","2560x1440","720x1280"};
        for(int i=0;i<6;i++){ int cw=(aw-56)/3, chh=(ah-44)/2;
            int gx=ax+20+(i%3)*(cw+8), gy=ay+16+(i/3)*(chh+8);
            DR(gx,gy,gx+cw,gy+chh,(i==2)?acc:bord);
            tx(8,gx+3,gy+3,R[i],(i==2)?acc:dim,c_bg()); }
    } break;
    case 20:{   /* 叠正弦 */
        for(int r=0;r<5;r++){ int y0=ay+16+r*(ah-32)/5, px=-1,py=0;
            for(int x=0;x<aw-32;x+=4){ int yy=y0+(int)(sinf(x*0.07f+g_t*pb*0.3f+r*0.9f)*(ah/5-8)/2);
                if(px>=0)ln(ax+16+px,py,ax+16+x,yy,(r==2)?acc:bord); px=x;py=yy; } }
    } break;
    case 21:{   /* 心形 */
        int px=-1,py=0; float beat=1.f+0.06f*sinf(g_t*(4.f+pb*0.3f));
        for(int i=0;i<=180;i++){ float t2=i/180.f*6.2831853f;
            float X=16*powf(sinf(t2),3.f); float Y=13*cosf(t2)-5*cosf(2*t2)-2*cosf(3*t2)-cosf(4*t2);
            int sx=cx+(int)(X*(ah*0.024f)*beat), sy=cy-(int)(Y*(ah*0.024f)*beat);
            if(px>=0)ln(px,py,sx,sy,col); px=sx;py=sy; }
    } break;
    case 22:{   /* execution 条 */
        for(int i=0;i<26;i++){ int bh=(int)((0.2f+0.8f*fabsf(sinf(i*0.7f+g_t*(6.f+pb))))*ah*0.75f);
            rf(ax+16+i*(aw-32)/26, ay+ah-16-bh, (aw-32)/26-2, bh, (i%3)?col:acc); }
    } break;
    case 31:{   /* 点阵墙 */
        int g2=6+pa;
        for(int r=0;r<g2;r++)for(int c2=0;c2<g2;c2++){
            int xx=ax+20+c2*(aw-40)/g2, yy=ay+16+r*(ah-32)/g2;
            int s2=(int)(1+frnd(r*20+c2,pa)*3);
            FC(xx,yy,s2,((r+c2*3)%(pb+2)==0)?acc:dim);
        }
    } break;
    case 32:{   /* 径向刻度 */
        int n2=18+pa*3; for(int i=0;i<n2;i++){ float a2=i*6.2831853f/n2;
            int r1=(int)(ah*0.18f), r2=(int)(ah*(0.26f+0.17f*frnd(i,pb)));
            ln(cx+(int)(cosf(a2)*r1),cy+(int)(sinf(a2)*r1),
               cx+(int)(cosf(a2)*r2),cy+(int)(sinf(a2)*r2),(i%3)?col:acc); }
    } break;
    case 33:{   /* 摩尔纹 */
        for(int k=0;k<n;k++){ float a2=(k-n/2)*0.06f+(float)(pb%3); int r1=(int)(ah*0.34f);
            ln(cx+(int)(cosf(a2)*r1),cy+(int)(sinf(a2)*r1)-r1/2,
               cx+(int)(cosf(a2)*r1),cy+(int)(sinf(a2)*r1)+r1/2, (k%4)?bord:col); }
    } break;
    case 34:{   /* 星形多边形 */
        int pts=5+(pa%4)*2; int px=-1,py=0;
        for(int i=0;i<=pts*2;i++){ float a2=i*6.2831853f/(pts*2)-1.57f;
            float rad=(i%2==0)?ah*0.32f:ah*0.14f;
            int sx=cx+(int)(cosf(a2)*rad), sy=cy+(int)(sinf(a2)*rad);
            if(px>=0)ln(px,py,sx,sy,col); px=sx;py=sy; }
    } break;
    case 35:{   /* 阶梯线 */
        int y0=ay+ah-24; int px=ax+16, py=y0;
        for(int i=0;i<n;i++){ int nx=ax+16+(int)((aw-32)*(i+1)/(float)n); int ny=y0-(int)(frnd(i,pa)*ah*0.5f);
            ln(px,py,nx,py,bord); ln(nx,py,nx,ny,col); px=nx; py=ny; }
    } break;
    case 36:{   /* 梯子 */
        int bars=6+pa; for(int i=0;i<bars;i++){ int yy=ay+18+i*(ah-36)/bars;
            ln(ax+30,yy,ax+aw-30,yy,col); }
        ln(ax+30,ay+14,ax+30,ay+ah-14,acc); ln(ax+aw-30,ay+14,ax+aw-30,ay+ah-14,acc);
    } break;
    case 37:{   /* 箭头场 */
        for(int i=0;i<n*2;i++){ int xx=ax+20+(int)(frnd(i,pa)*aw*0.76f), yy=ay+18+(int)(frnd(i,pa+3)*ah*0.66f);
            int L=6+(int)(frnd(i,5)*8);
            ln(xx,yy,xx+L,yy,col); ln(xx+L,yy,xx+L-4,yy-3,col); ln(xx+L,yy,xx+L-4,yy+3,col); }
    } break;
    case 38:{   /* 同心方框 */
        for(int i=0;i<n/2+2;i++){ int d=i*(ah/2)/8;
            DR(cx-ah/4+d,cy-ah/4+d,cx+ah/4-d,cy+ah/4-d,(i%3)?bord:col); }
    } break;
    case 39:{   /* 月牙 */
        FC(cx+8,cy,ah/3,col); 
        {   fx_color_t bgc = (g_light?C(246,248,250):C(13,17,23));
            FC(cx-4,cy-6,ah/3-4,bgc); }
        DC(cx,cy,ah/3+2,acc);
    } break;
    case 40:{   /* 文字散落 */
        for(int i=0;i<n*2;i++){ int xx=ax+20+(int)(frnd(i,pa)*aw*0.72f), yy=ay+16+(int)(frnd(i,pa+2)*ah*0.7f);
            tx(9+(i%3),xx,yy,"fxtk",(i%4)?dim:acc,c_bg()); }
    } break;
    case 41:{   /* 茄子 */
        obj_eggplant(cx,cy,ah);
    } break;
    case 42:{   /* 番茄 */
        obj_tomato(cx,cy,ah);
    } break;
    case 43:{   /* 虎斑猫 */
        obj_cat(cx,cy,ah,0);
    } break;
    case 44:{   /* 神环 (光环+星芒, 不画人) */
        for(int i=0;i<16;i++){ float a2=i*6.2831853f/16.f+g_t*0.2f;
            ln(cx+(int)(cosf(a2)*ah*0.30f),cy+(int)(sinf(a2)*ah*0.30f),
               cx+(int)(cosf(a2)*ah*0.46f),cy+(int)(sinf(a2)*ah*0.46f),(i%2)?acc:col); }
        DC(cx,cy,(int)(ah*0.22f),col); DC(cx,cy,(int)(ah*0.28f),bord);
        FC(cx,cy,5,acc);
    } break;
    case 45:{   /* 开发板: 芯片 + 引脚 + 小屏 */
        rf(ax+30,cy-ah/4,aw-60,ah/2,C(16,40,26));
        rf(ax+42,cy-ah/6,aw/3,ah/3,C(30,60,40));
        tx(9,ax+46,cy-ah/6+4,"ESP32",C(140,230,160),C(30,60,40));
        for(int i=0;i<8;i++){ ln(ax+42+i*((aw/3)/8),cy+ah/6,ax+42+i*((aw/3)/8),cy+ah/6+8,C(200,180,60)); }
        rf(ax+aw-150,cy-ah/6,100,ah/3,C(8,12,16));
        for(int i=0;i<5;i++) rf(ax+aw-144,cy-ah/6+6+i*10,60,6,C(60,200,160));
        tx(9,ax+aw-150,cy+ah/6+4,"480x272",dim,c_bg());
    } break;
    case 46:{   /* 显示器 + 键盘 */
        rf(cx-aw/4,ay+20,aw/2,ah/2,C(18,22,28));
        rf(cx-aw/4+4,ay+24,aw/2-8,ah/2-8,C(40,90,140));
        rf(cx-6,ay+20+ah/2,12,18,bord);
        rf(cx-aw/6,ay+20+ah/2+18,aw/3,5,bord);
        for(int r=0;r<3;r++)for(int c2=0;c2<12;c2++)
            rf(cx-aw/4+c2*(aw/2)/12+2,ay+ah-26+r*7,(aw/2)/12-3,5,(r*12+c2)%7==(int)(g_t*3)%7?acc:bord);
    } break;
    case 47:{   /* 窗口 (系统标题栏 + 三颗按钮) */
        rf(ax+24,ay+24,aw-48,ah-48,C(20,24,30));
        rf(ax+24,ay+24,aw-48,22,c_panel());
        FC(ax+36,ay+35,4,C(235,105,95)); FC(ax+50,ay+35,4,C(240,190,80)); FC(ax+64,ay+35,4,C(95,200,110));
        fx_set_color(dim); fx_draw_rect(ax+25,ay+25,ax+aw-25,ay+ah-25);
        tx(10,ax+36,ay+52,"fx_main.c",dim,C(20,24,30));
        for(int i=0;i<4;i++) rf(ax+36,ay+72+i*14,aw-160,4,bord);
    } break;
    case 48:{   /* 按钮 */
        int press=(int)(g_t*2)%3==0;
        int x1=ax+aw/4, x2=ax+aw*3/4, y1=cy-24+(press?2:0), y2=cy+24+(press?2:0);
        DRR(x1,y1,x2,y2,10,acc);
        rf(x1+4,y1+4,x2-x1-8,y2-y1-8,press?acc:C(24,30,40));
        tx(16,x1+26,y1+10,"fx_button",press?C(10,14,18):acc,C(24,30,40));
    } break;
    case 49:{   /* 键盘 */
        for(int r=0;r<4;r++)for(int c2=0;c2<11;c2++){
            int kw=(aw-60)/11;
            rf(ax+30+c2*kw+1,cy-24+r*12,kw-3,9,C(28,32,40));
        }
        rf(ax+30,cy-24+48,aw-60,3,bord);
    } break;
    default:{   /* 50 USB 线 + 插头 */
        int yy=cy; ln(ax+30,yy,ax+aw-90,yy,C(200,200,210));
        rf(ax+aw-90,yy-8,34,16,C(180,180,195));
        rf(ax+aw-56,yy-5,18,10,C(220,200,120));
        for(int i=0;i<4;i++) ln(ax+aw-54+i*4,yy-3,ax+aw-54+i*4,yy+3,C(40,40,50));
    } break;
    case 51:{   /* 养分: 茄子 + 流出的粒子 */
        obj_eggplant(ax+aw/4,cy,ah);
        int tx2=ax+aw*3/4, ty2=cy;
        FC(tx2,ty2,8,C(240,200,120));
        for(int i=0;i<14;i++){ float ph=fmodf(g_t*0.7f+i/14.f,1.f);
            int px=(int)(ax+aw*0.34f+(tx2-ax-aw*0.34f)*ph);
            int py=cy+(int)(sinf(ph*12.56f+i)*12);
            FC(px,py,2,(i%3)?acc:C(240,200,120)); }
    } break;
    case 52:{   /* 抗氧化: 番茄 + 防护环 + 星光 */
        obj_tomato(cx,cy,ah);
        int r=(int)(ah*0.30f);
        DC(cx,cy+4,r,acc); DC(cx,cy+4,r+7,bord);
        for(int i=0;i<10;i++){ float a2=i*6.2831853f/10.f+g_t*0.4f;
            int px=cx+(int)(cosf(a2)*(r+14)), py=cy+4+(int)(sinf(a2)*(r+14));
            if(((int)(g_t*3)+i)%4==0) FC(px,py,2,acc); else FC(px,py,1,dim); }
    } break;
    case 53:{   /* 呼噜: 猫 + 声波 */
        obj_cat(cx-16,cy,ah,1);
    } break;
    case 54:{   /* 证明: 光环 + 上升的柱 */
        int n2=9;
        for(int i=0;i<n2;i++){ int hh=(int)(ah*(0.15f+0.5f*frnd(i,(int)(g_t*2)%5)));
            rf(cx-(n2*12)/2+i*12, ay+ah-26-hh, 8, hh, (i%3)?bord:acc); }
        DC(cx,ay+ah/3,(int)(ah*0.16f),acc); DC(cx,ay+ah/3,(int)(ah*0.20f),bord);
        ln(cx-ah*0.2f,ay+ah/3,cx+ah*0.2f,ay+ah/3,acc);
    } break;
    case 55:{   /* 插头 + 插座 (Switch my current) */
        rf(ax+aw/3,cy-46,aw/6,60,C(200,200,210));
        ln(ax+aw/3+10,cy-46,ax+aw/3+10,cy-76,C(180,180,195));
        ln(ax+aw/2,cy-46,ax+aw/2,cy-76,C(180,180,195));
        rf(ax+aw*2/3,cy-46,aw/6,60,C(30,34,42));
        rf(ax+aw*2/3+8,cy-36,aw/6-16,10,C(90,180,120));
        rf(ax+aw*2/3+8,cy-18,aw/6-16,10,C(200,80,70));
        if(((int)(g_t*2))%2==0){ for(int i=0;i<6;i++) ln(ax+aw/3+8+i*8,cy+14,ax+aw/2-4+i*4,cy+40,C(240,200,80)); }
    } break;
    case 56:{   /* 性别符号: 火星 / 金星 */
        DC(cx-40,cy-6,26,C(120,180,255));
        ln(cx-22,cy-26,cx+2,cy-50,C(120,180,255));
        ln(cx-8,cy-50,cx+2,cy-50,C(120,180,255)); ln(cx+2,cy-62,cx+2,cy-50,C(120,180,255));
        DC(cx+40,cy+6,26,C(255,150,190));
        ln(cx+40,cy+32,cx+40,cy+62,C(255,150,190));
        ln(cx+22,cy+48,cx+58,cy+48,C(255,150,190));
    } break;
    case 57:{   /* 手机震动 */
        int pw2=(int)(aw*0.16f), ph2=(int)(ah*0.55f);
        int px=cx-pw2/2+(int)(sinf(g_t*30.f)*3.f), py=cy-ph2/2;
        rf(px,py,pw2,ph2,C(22,26,34));
        rf(px+4,py+8,pw2-8,ph2-24,C(90,160,240));
        rf(px+pw2/2-6,py+ph2-10,12,3,C(120,130,150));
        for(int k=0;k<3;k++){ int rr=pw2/2+10+k*9;
            DA(px+pw2/2,cy,rr,300,340,(k==1)?c_acc():c_bord());
            DA(px+pw2/2,cy,rr,20,60,(k==1)?c_acc():c_bord()); }
    } break;
    case 58:{   /* 完成环 */
        int r=(int)(ah*0.30f);
        DC(cx,cy,r,bord);
        float pr2=fminf(1.f,(g_t-107.7f)/2.6f);
        DA(cx,cy,r,90,90+(int)(pr2*359.f),acc);
        FC(cx,cy,6,acc);
        tx(12,cx-24,cy+8,"completion",dim,c_bg());
    } break;
    case 59:{   /* 出神: 同心环 + 旋转光点 */
        for(int k=0;k<6;k++){ int rr=(int)(ah*0.10f)+k*(int)(ah*0.055f);
            DC(cx,cy,rr,(k%2)?bord:acc); }
        for(int i=0;i<3;i++){ float a2=g_t*(0.8f+i*0.5f)+i*2.1f;
            int rr=(int)(ah*0.12f)+i*(int)(ah*0.06f);
            FC(cx+(int)(cosf(a2)*rr),cy+(int)(sinf(a2)*rr),3,acc); }
    } break;
    case 60:{   /* 时钟 (AM to PM) */
        int r=(int)(ah*0.30f);
        DC(cx,cy,r,col);
        for(int i=0;i<12;i++){ float a2=i*6.2831853f/12.f;
            int x1=cx+(int)(cosf(a2)*(r-6)), y1=cy+(int)(sinf(a2)*(r-6));
            int x2=cx+(int)(cosf(a2)*(r-2)), y2=cy+(int)(sinf(a2)*(r-2));
            ln(x1,y1,x2,y2,bord); }
        float am=(g_t*0.5f)*(6.2831853f/60.f);
        ln(cx,cy,cx+(int)(cosf(am)*r*0.6f),cy+(int)(sinf(am)*r*0.6f),acc);
        ln(cx,cy,cx+(int)(cosf(am*12.f)*r*0.35f),cy+(int)(sinf(am*12.f)*r*0.35f),col);
        FC(cx,cy,3,acc);
    } break;
    case 61:{   /* AC 发电机 -> DC 发电机 */
        int lx=ax+(int)(aw*0.14f), mx2=lx+34, rx2=mx2+40;
        int hh=(int)(ah*0.24f);
        rf(lx,cy-hh,lx+10,cy+hh,C(206,88,80));                     /* N 极 */
        rf(lx+13,cy-hh,lx+23,cy+hh,C(206,88,80));
        rf(rx2,cy-hh,rx2+10,cy+hh,C(82,120,212));                  /* S 极 */
        rf(rx2+13,cy-hh,rx2+23,cy+hh,C(82,120,212));
        tx(12,lx+4,cy-hh-18,"N",C(230,150,140),c_bg());
        tx(12,rx2+6,cy-hh-18,"S",C(150,180,240),c_bg());
        int mc=(lx+23+rx2)/2, mr=(int)(ah*0.19f);
        FRR(mc-mr,cy-mr,mc+mr,cy+mr,mr,C(36,40,48));               /* 转子 */
        for(int k2=0;k2<6;k2++){                                    /* 线圈 */
            float a2=g_t*2.4f+k2*1.0472f;
            ln(mc+(int)(cosf(a2)*mr*0.8f),cy+(int)(sinf(a2)*mr*0.8f),
               mc-(int)(cosf(a2)*mr*0.8f),cy-(int)(sinf(a2)*mr*0.8f),
               (k2%3)?bord:acc);
        }
        FC(mc,cy,4,C(220,220,230));
        ln(rx2+23,cy,mc+mr,cy,bord);                               /* 引出线 */
        ln(lx,cy,mc-mr,cy,bord);
        /* 输出波形 */
        int gx=ax+(int)(aw*0.50f), gw=aw-(gx-ax)-16, y0=cy;
        ln(gx,y0,gx+gw,y0,bord);
        int dc2=((int)(g_t*0.9f)%2)==1;
        int px=-1,py=0;
        for(int x=0;x<gw;x+=2){
            float ph=x/(float)gw*6.2831853f*2.f+g_t*3.f;
            float v=sinf(ph);
            if(dc2) v=fabsf(v);
            int yy=y0-(int)(v*ah*0.20f);
            if(px>=0) ln(gx+px,py,gx+x,yy,(dc2?c_grn():acc));
            px=x;py=yy;
        }
        tx(16,gx,y0-ah*0.28f,dc2?"DC":"AC",dc2?c_grn():acc,c_bg());
        tx(10,gx,y0+ah*0.26f,dc2?"换向器 · 脉动直流":"滑环 · 交流",c_dim(),c_bg());
    } break;
        case 24:{   /* 片头卡: world.execute(me); */
        rf(ax+10,ay+10,aw-20,ah-20,C(10,13,18));
        tx(20,cx-120,cy-30,"world.execute(me);",acc,C(10,13,18));
        tx(11,cx-40,cy+8,"- Mili",dim,C(10,13,18));
        if(((int)(g_t*2))%2==0) rf(cx+120,cy-26,3,22,acc);
    } break;
    case 25:{   /* 过门: 两根线靠近, 中间一颗脉冲点 */
        int gap=(int)(aw*0.30f*(0.6f+0.4f*fsin(g_t*pb*0.3f)));
        int y0=cy;
        ln(ax+16,y0,cx-gap,y0,col); ln(cx+gap,y0,ax+aw-16,y0,col);
        int pr=(int)(2+2*fabsf(sinf(g_t*(4.f+pb*0.3f))));
        FC(cx,y0,pr,acc);
        for(int i=1;i<=5;i++){ int a2=cx-gap-i*10, b2=cx+gap+i*10;
            if(a2>ax+16) rf(a2,y0-1,2,2,dim); if(b2<ax+aw-16) rf(b2,y0-1,2,2,dim); }
    } break;
    case 26:{   /* 微笑: 一段上弯的弧 + 两点 */
        int r=(int)(ah*0.26f);
        DA(cx,cy-r/2, r, 20, 160, col);
        FC(cx-r/2,cy-r, 3, acc); FC(cx+r/2,cy-r, 3, acc);
    } break;
    case 27:{   /* 门 / 离开: 方形门框 + 向外的箭头 */
        DR(cx-30,ay+20,cx+30,ay+ah-24,col);
        ln(cx+38,cy,cx+92,cy,acc);
        ln(cx+78,cy-8,cx+92,cy,acc); ln(cx+78,cy+8,cx+92,cy,acc);
        FC(cx+18,cy+2,2,acc);
    } break;
    case 28:{   /* 数字: Ein Dos Trois */
        tx(22,ax+24,cy-24,"2  3  6",col,C(10,13,18));
        for(int i=0;i<6;i++) rf(ax+26+i*36,cy+12,20,3,acc);
    } break;
    case 29:{   /* 尾卡 */
        for(int i=0;i<24;i++){ float a2=frnd(i,pa)*6.28f; float rad=ah*0.1f;
            FC(cx+(int)(cosf(a2)*rad), cy+(int)(sinf(a2)*rad), 1, dim); }
        tx(14,cx-64,cy-8,"fxtk v2.5",col,C(10,13,18));
    } break;
    case 30:{   /* 神光: 径向射线 */
        int rays=12+pa*2; if(rays>34)rays=34;
        for(int i=0;i<rays;i++){ float a2=i*6.2831853f/rays + g_t*0.1f*(pb%3);
            int r1=ah*0.12f, r2=(int)(ah*(0.30f+0.16f*frnd(i,2)));
            ln(cx+(int)(cosf(a2)*r1), cy+(int)(sinf(a2)*r1),
               cx+(int)(cosf(a2)*r2), cy+(int)(sinf(a2)*r2), (i%3)?col:acc); }
        FC(cx,cy,6,acc);
    } break;
    case 23:{   /* 故障 */
        for(int i=0;i<n/2+3;i++){
            if(frnd(i,(int)(g_t*5)%7)>0.5f) continue;
            rf(ax+16+(int)(frnd(i,1)*aw*0.7f), ay+16+(int)(frnd(i,2)*ah*0.7f),
               8+(int)(frnd(i,3)*50),2+(int)(frnd(i,4)*8),(i%3)?dim:acc);
        }
        for(int y=0;y<ah-32;y+=4) if((y/4)%3==0) ln(ax+16,ay+16+y,ax+aw-16,ay+16+y,C(0,0,0));
    } break;
    }
}

static void draw_art(int w,int h)
{
    rf(0,0,w,h,c_bg());
    fx_color_t acc=c_acc(), dim=c_dim(), txt=c_txt(), bord=c_bord();
    int ax=12, ay=30, aw=w-24, ah=h-30-52;   /* 艺术区 */
    (void)txt;
    /* 标题 */
    int li_=lyric_idx(g_t);
    int scene=lyr_scene(li_); if(scene<0) scene=g_ch;
    if(li_>=0 && li_<LART_N){
        const LART_T *R=&LART[li_];
        draw_bg(R->bg,ax,ay,aw,ah);
        draw_motif(R->m,R->a,R->b,R->c,ax,ay,aw,ah);
        {   char wd[64]; snprintf(wd,sizeof wd,"%.40s",LYR[li_].s);
            char ln2[32]; snprintf(ln2,sizeof ln2,"LINE %d/%d",li_+1,NLYR);
            tx(10,ax+6,ay+ah-18,wd,acc,c_bg());
            tx(9,ax+aw-82,ay+ah-18,ln2,dim,c_bg()); }
        art_spec_strip(10,h-44,w-20,32);
        return;
    }
    int lseed=li_%7;
    {   char tb[128];
        const char *ly=(li_>=0 && li_<NLYR)?LYR[li_].s:"";
        if(ly[0]) snprintf(tb,sizeof tb,"fx · %.44s",ly);
        else      snprintf(tb,sizeof tb,"fx · %s",CHAPTERS[g_ch]);
        tx(12,12,8,tb,dim,c_bg());
        {   char cb[48]; snprintf(cb,sizeof cb,"%s",CHAPTERS[g_ch]);
            int cw=fxtk_text_width_size((int)(9*g_sc),cb);
            tx(9,(int)(w-14-cw/g_sc),14,cb,bord,c_bg()); } }
    ln(10,26,w-10,26,bord);
    int cx=ax+aw/2, cy=ay+ah/2;
    if(scene<7 && scene!=3) draw_device(ax+aw-190, ay+ah-118, 178, 100);
    if((scene==10 || scene==22) && fx_raymarch_available()){
        fx_draw_raymarch(g_t, (int)(ax*g_sc),(int)(ay*g_sc),(int)(aw*g_sc),(int)(ah*g_sc));
        DR(ax-1,ay-1,ax+aw,ay+ah,acc);
        tx(10,ax+4,ay+ah-16,"3D raymarch · fragment shader (fxtk hook)",c_txt(),C(0,0,0));
        art_spec_strip(10,h-44,w-20,32);
        return;
    }
    switch(scene){
    case 0: {   /* BOOT: ST6201 / GT911 探测 */
        for(int i=0;i<3;i++){ int ln_=20+i*14; ln(ax+14,ay+ln_,ax+20+(int)(frnd(i,1)*60),ay+ln_,dim); }
        tx(12,ax+14,ay+6,"480x272 / SPI",acc,c_bg());
        if(((int)(g_t*2))%2==0) rf(ax+14,ay+30,9,14,acc);
        tx(11,ax+30,ay+32,"probe st6201 ...",dim,c_bg());
        rf(ax+14,ay+ah-8,aw-28,2,bord); rf(ax+14,ay+ah-8,(int)((aw-28)*(0.15f+0.1f*fsin(g_t))),2,acc);
    } break;
    case 1: {   /* POWER LINE: 电源线 + 正弦 */
        int y0=cy;
        ln(ax,y0,ax+aw,y0,bord);
        int px=-1,py=0;
        for(int x=0;x<aw;x++){ int yy=y0-(int)(sinf(x*0.05f+g_t*3.f)*ah*0.3f); if(px>=0)ln(ax+px,py,ax+x,yy,acc); px=x;py=yy; }
        tx(11,ax+6,ay+6,"3V3",dim,c_bg()); tx(11,ax+6,ay+ah-18,"GND",dim,c_bg());
        ln(ax+aw-40,ay+8,ax+aw-40,ay+ah-8,dim);
    } break;
    case 2: {   /* PROTECTION: 盾形 + 扫描 */
        for(int k=0;k<3;k++) DRR(ax+20+k*10,ay+10+k*8,ax+aw-20-k*10,ay+ah-10-k*8,8,bord);
        int sy=ay+12+(int)((ah-24)*(0.5f+0.5f*fsin(g_t*1.6f)));
        rf(ax+22,sy-1,aw-44,2,acc);
    } break;
    case 3: {   /* PIECES: 控件池 (属性宏 -> fx_button/fx_slider/fx_check) */
        int cw=(aw-24)/4, chh=(ah-30)/3;
        for(int r=0;r<3;r++) for(int c=0;c<4;c++){
            int x=ax+12+c*cw, y=ay+10+r*chh, ww=cw-8, hh=chh-8;
            int kind=(r*4+c)%3;
            int hot=((r*4+c)==(int)(g_t*1.5f)%12);
            if(kind==0){ rf(x,y,ww,hh,hot?acc:(g_light?C(226,232,240):C(30,36,44)));
                DR(x,y,x+ww-1,y+hh-1,hot?C(10,14,18):dim);
                tx(10,x+8,y+hh/2-6,"fx_button",hot?(g_light?C(255,255,255):C(10,14,18)):dim,c_bg()); }
            else if(kind==1){ rf(x,y+hh/2-2,ww,4,bord);
                int hx=x+(int)((ww-10)*(0.5f+0.4f*fsin(g_t*1.2f+(r*4+c)*0.7f)));
                rf(hx,y+hh/2-7,10,14,hot?acc:dim); }
            else { rf(x+6,y+hh/2-7,14,14,hot?acc:bord);
                if(hot){ ln(x+9,y+hh/2,x+11,y+hh/2+4,acc); ln(x+11,y+hh/2+4,x+17,y+hh/2-5,acc); }
                tx(10,x+26,y+hh/2-6,"fx_check",dim,c_bg()); }
        }
        tx(9,ax+12,ay+ah-16,"pool: 16384 -> 8192",acc,c_bg());
    } break;
    case 4: {   /* OBJECT CREATION: 圆 (三角扇) */
        int r=(int)(ah*0.36f*(0.8f+0.2f*fsin(g_t*1.5f)));
        FC(cx,cy,r,acc);
        DC(cx,cy,r+3,acc);
        ln(cx,cy,cx+r,cy,dim); tx(11,cx+8,cy-16,"r",dim,c_bg());
        tx(11,ax+8,ay+ah-18,"fx_fill_circle()",dim,c_bg());
    } break;
    case 5: {   /* PARAMETERS: 滑杆阵 */
        for(int i=0;i<4;i++){
            int yy=ay+16+i*((ah-24)/4);
            rf(ax+40,yy-1,aw-60,3,bord);
            int hx=ax+40+(int)((aw-60)*(0.5f+0.45f*fsin(g_t*1.3f+i*1.7f)));
            rf(hx-4,yy-7,8,15,acc);
            char lb[16]; snprintf(lb,sizeof lb,"p%d",i); tx(10,ax+10,yy-8,lb,dim,c_bg());
        }
    } break;
    case 6: {   /* NEW WORLD: 星野 */
        for(int i=0;i<70;i++){
            int sx=ax+(int)(frnd(i,1)*aw), sy=ay+(int)(frnd(i,2)*ah);
            int b=1+(int)(frnd(i,3)*2);
            rf(sx,sy,b,b,(i%7==(int)(g_t*3)%7)?acc:dim);
        }
        ln(ax,ay+ah-16,ax+aw,ay+ah-16,bord);
    } break;
    case 7: {   /* SIMULATION: 粒子 */
        for(int i=0;i<90;i++){
            float ph=frnd(i,1)*6.28f, sp=0.4f+frnd(i,2);
            int px=ax+(int)((0.5f+0.5f*fsin(g_t*sp+ph))*aw);
            int py=ay+(int)((0.5f+0.5f*fcos(g_t*sp*0.7f+ph))*ah);
            int r=1+(int)(frnd(i,3)*2);
            FC(px,py,r,acc);
        }
    } break;
    case 8: {   /* DIMENSION: 旋转立方体线框 */
        static const float V[8][3]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
        static const int E[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        float a=g_t*0.9f, ca=cosf(a), sa=sinf(a), x2[8],y2[8];
        for(int i=0;i<8;i++){
            float x=V[i][0], y=V[i][1], z=V[i][2];
            float xr=x*ca - z*sa, zr=x*sa + z*ca;
            float yv=y*0.9f - zr*0.35f;
            float sc=1.f/(2.6f+zr);
            x2[i]=cx+xr*sc*aw*0.9f; y2[i]=cy+yv*sc*ah*0.9f;
        }
        for(int i=0;i<12;i++){ int a1=E[i][0], b1=E[i][1]; ln((int)x2[a1],(int)y2[a1],(int)x2[b1],(int)y2[b1],acc); }
    } break;
    case 9: {   /* CIRCUMFERENCE: 圆 + 半径 + 2πr */
        int r=(int)(ah*0.34f);
        DC(cx,cy,r,acc);
        DA(cx,cy,r+6,0,(int)(fmodf(g_t*60.f,360.f)),acc);
        ln(cx,cy,cx+r,cy,acc);
        tx(12,cx+r/2-6,cy-18,"r",acc,c_bg());
        tx(11,ax+8,ay+ah-18,"C = 2 pi r",dim,c_bg());
    } break;
    case 10:{   /* TANGENTS: 正弦 + 切线 */
        int y0=cy;
        int px=-1,py=0; float base=0.05f, amp=ah*0.3f;
        for(int x=0;x<aw;x++){
            int yy=y0-(int)(sinf(x*base+g_t*2.f)*amp);
            if(px>=0)ln(ax+px,py,ax+x,yy,acc); px=x;py=yy;
        }
        float tph=0.5f+0.5f*fsin(g_t*0.7f);
        int tx_=ax+(int)(tph*aw);
        float slope=cosf(tx_*base+g_t*2.f)*base; /* 切线 */
        int ty_=y0-(int)(sinf(tx_*base+g_t*2.f)*amp);
        ln(tx_-60,(int)(ty_-slope*60),tx_+60,(int)(ty_+slope*60),dim);
        FC(tx_,ty_,4,acc);
        tx(11,ax+8,ay+ah-18,"dy/dx",dim,c_bg());
    } break;
    case 11:{   /* LIMITATIONS: 虚线裁剪 */
        for(int i=0;i<12;i++){ if(i%2==0) rf(ax+10+i*(aw-20)/12,ay+10,(aw-20)/12-2,ah-20,bord); }
        int k=(int)(g_t*40)%aw; rf(ax+10+k,ay+10,2,ah-20,acc);
        tx(11,ax+8,ay+ah-18,"clipped / not supported",dim,c_bg());
    } break;
    case 12:{   /* INFINITY: 双纽线 */
        int px=-1,py=0;
        for(int i=0;i<=120;i++){
            float t=i/120.f*6.2831853f;
            float d=1.f+sinf(t)*sinf(t);
            float X=cosf(t)/d, Y=sinf(t)*cosf(t)/d;
            int sx=cx+(int)(X*aw*0.45f), sy=cy-(int)(Y*ah*0.9f);
            if(px>=0)ln(px,py,sx,sy,acc); px=sx;py=sy;
        }
    } break;
    case 13:{   /* AC / DC: 双波 */
        int y1=ay+ah/3, y2=ay+ah*2/3;
        ln(ax,y1,ax+aw,y1,bord); ln(ax,y2,ax+aw,y2,bord);
        int px=-1,py;
        for(int x=0;x<aw;x++){ int yy=y1-(int)(sinf(x*0.09f+g_t*3.f)*ah*0.14f); if(px>=0)ln(ax+px,py,ax+x,yy,acc); px=x;py=yy; }
        rf(ax,y2-14,aw,28,(g_light?C(232,236,242):C(18,23,30)));
        for(int x=0;x<aw;x+=3) rf(ax+x,y2,x+2,y2+1,dim);
        tx(11,ax+6,ay+4,"AC",dim,c_bg()); tx(11,ax+6,ay+ah-18,"DC",dim,c_bg());
    } break;
    case 14:{   /* BLIND VISION: 第一块颜色 0x0841 (RGB565) + 盲点 */
        int bw=aw*2/3, bh=ah*3/5, bx=ax+(aw-bw)/2, by=ay+10;
        rf(bx,by,bw,bh,C(8,32,8));
        DR(bx-1,by-1,bx+bw,by+bh,acc);
        tx(11,bx+10,by+10,"0x0841",C(120,220,120),C(8,32,8));
        tx(9,bx+10,by+28,"first color on that board",C(90,150,90),C(8,32,8));
        for(int i=0;i<10;i++) FC(bx+10+i*(bw-20)/9, by+bh-16, 1+(i==4?2:0),C(120,220,120));
        tx(9,ax+8,ay+ah-16,"盲点: 只有这一块 565 色",dim,c_bg());
    } break;
    case 15:{   /* DIZZY: 螺旋 + 滑杆残影修复前后对照 */
        int px=-1,py=0, scx=cx, scy=ay+(int)(ah*0.30f);
        for(int i=0;i<=110;i++){
            float t2=i/110.f;
            float ang=t2*6.2831853f*2.5f+g_t*1.8f;
            float rad=(0.08f+t2*0.5f)*ah*0.24f;
            int sx=scx+(int)(cosf(ang)*rad), sy=scy+(int)(sinf(ang)*rad*0.8f);
            if(px>=0)ln(px,py,sx,sy,acc); px=sx;py=sy;
        }
        {   int sy1=ay+ah-58, sy2=ay+ah-22;
            int bx=ax+20+(int)((aw-40)*(0.5f+0.28f*sinf(g_t*0.9f)));
            ln(ax+20,sy1,ax+aw-20,sy1,bord);
            for(int k=6;k>=0;k--){ int gx=bx-k*7; rf(gx-4,sy1-6,8,13, k==0?acc:mixc(acc,bord,0.55f)); }
            tx(9,ax+20,sy1-20,"cec3b00^ : ghost 66 -> 190",c_yel(),c_bg());
            ln(ax+20,sy2,ax+aw-20,sy2,bord);
            rf(bx-4,sy2-6,8,13,acc);
            tx(9,ax+20,sy2-20,"cec3b00 : 64-68 (fixed)",c_grn(),c_bg());
        }
    } break;
    case 16:{   /* TRAVEL: 隧道 */
        for(int k=0;k<8;k++){
            float ph=fmodf(k+g_t*1.2f,8.f)/8.f;
            float sc=ph*ph;
            int rw=(int)(aw*0.5f*(0.15f+sc)), rh=(int)(ah*0.5f*(0.15f+sc));
            DR(cx-rw,cy-(int)(rh*0.7f),cx+rw,cy+(int)(rh*0.7f),bord);
        }
        for(int i=0;i<8;i++){ float a=i/8.f*6.28f+fmodf(g_t*0.6f,6.28f);
            ln(cx+(int)(cosf(a)*aw*0.45f),cy+(int)(sinf(a)*ah*0.42f),cx+(int)(cosf(a)*12),cy+(int)(sinf(a)*9),bord); }
    } break;
    case 17:{   /* AD / BC: 时间轴 */
        int y0=cy; ln(ax+10,y0,ax+aw-10,y0,dim);
        for(int i=0;i<=20;i++){ int xx=ax+10+i*(aw-20)/20; ln(xx,y0-6,xx,y0+6,bord); }
        int mark=(int)((g_t*0.35f)*20)%21;
        rf(ax+10+mark*(aw-20)/20-2,y0-16,5,10,acc);
        tx(11,ax+10,y0+12,"AD",dim,c_bg()); tx(11,ax+aw-40,y0+12,"BC",dim,c_bg());
    } break;
    case 18:{   /* UNITE: 双圆合并 */
        int d=(int)(ah*0.42f*(0.6f+0.4f*fsin(g_t*1.2f)));
        DC(cx-d,cy,ah/4,acc); DC(cx+d,cy,ah/4,acc);
        if(d<ah/4) FC(cx,cy,(int)((ah/4-d)*0.9f),acc);
    } break;
    case 19:{   /* DEPLOY: 六种分辨率的真实窗口尺寸 */
        static const char *R[6]={"480x272","800x480","1280x720","1920x1080","2560x1440","720x1280"};
        static const float AR[6]={1.76f,1.67f,1.78f,1.78f,1.78f,0.56f};
        for(int i=0;i<6;i++){
            int cw=(aw-46)/3, chh=(ah-40)/2;
            int gx=ax+14+(i%3)*(cw+8), gy=ay+8+(i/3)*(chh+10);
            DR(gx,gy,gx+cw,gy+chh,i==2?acc:bord);
            float a=AR[i]; int iw=cw-12, ih=(int)(iw/a); if(ih>chh-16) ih=chh-16, iw=(int)(ih*a);
            DR(gx+(cw-iw)/2, gy+(chh-ih)/2, gx+(cw+iw)/2, gy+(chh+ih)/2, i==2?dim:bord);
            tx(9,gx+3,gy+3,R[i],i==2?acc:dim,c_bg());
        }
    } break;
    case 20:{   /* SIMULATIONS: 多正弦 */
        for(int r=0;r<6;r++){
            int y0=ay+10+r*(ah-16)/6; int px=-1,py=0;
            for(int x=0;x<aw;x+=4){
                int yy=y0+(int)(sinf(x*0.08f+g_t*2.f+r*0.9f)*(ah/6-8)/2);
                if(px>=0)ln(ax+px,py,ax+x,yy,(r==2)?acc:bord); px=x;py=yy;
            }
        }
    } break;
    case 21:{   /* SATISFACTION: 心形曲线 (16sin^3, 13cos-5cos2-2cos3-cos4) */
        int px=-1,py=0; float beat=1.f+0.06f*sinf(g_t*6.f);
        for(int i=0;i<=180;i++){
            float t=i/180.f*6.2831853f;
            float X=16*powf(sinf(t),3.f);
            float Y=13*cosf(t)-5*cosf(2*t)-2*cosf(3*t)-cosf(4*t);
            int sx=cx+(int)(X*(ah*0.026f)*beat), sy=cy-(int)(Y*(ah*0.026f)*beat);
            if(px>=0)ln(px,py,sx,sy,acc); px=sx;py=sy;
        }
        tx(11,ax+8,ay+ah-18,"x=16sin^3 t, y=13cos t-...",dim,c_bg());
    } break;
    case 22:{   /* EXECUTION: 快闪条 */
        int n=28; for(int i=0;i<n;i++){
            int bh=(int)((0.2f+0.8f*fabsf(sinf(i*0.7f+g_t*9.f)))*ah*0.8f);
            rf(ax+6+i*(aw-12)/n, ay+ah-bh, (aw-12)/n-2, bh, acc);
        }
        tx(12,ax+6,ay+4,"execution",acc,c_bg());
    } break;
    case 23:{   /* STRANGE SIM: 故障块 */
        for(int i=0;i<14;i++){
            if(frnd(i,(int)(g_t*6)%7)>0.5f) continue;
            int sx=ax+(int)(frnd(i,1)*aw*0.8f), sy=ay+(int)(frnd(i,2)*ah*0.8f);
            int ww=10+(int)(frnd(i,3)*70), hh=2+(int)(frnd(i,4)*10);
            rf(sx,sy,ww,hh,(i%3)?dim:acc);
        }
        for(int y=0;y<ah;y+=4) if((y/4)%3==0) ln(ax,ay+y,ax+aw,ay+y,C(0,0,0));
    } break;
    default:{   /* 24: 收束 */
        float e=fminf(1.f,(g_t-SONG_DUR+8.f)/8.f);
        for(int i=0;i<40;i++){
            float ang=frnd(i,1)*6.28f, sp=frnd(i,2);
            int px=cx+(int)(cosf(ang)*ah*0.45f*(1.f-e)*sp), py=cy+(int)(sinf(ang)*ah*0.4f*(1.f-e)*sp);
            FC(px,py,1,dim); 
        }
        if(g_t>SONG_DUR-3.f) tx(14,cx-40,cy,"world.execute(me);",c_txt(),c_bg());
    } break;
    }
    art_spec_strip(10,h-44,w-20,32);
}

static void draw_chat(int w,int h)
{
    rf(0,0,w,h,c_bg());
    rf(0,0,w,30,c_panel());
    tx(13,12,6,"chapters · 曲目结构",c_txt(),c_panel());
    int cur=(int)(g_t*25.f/SONG_DUR); if(cur>24)cur=24;
    for(int i=0;i<7;i++){
        int k=(cur-3+i+25)%25;
        rf(10,38+i*18,w-20,16,(i==3)?(g_light?C(224,234,250):C(30,42,58)):c_panel());
        tx(12,16,40+i*18,CHAPTERS[k],(i==3)?c_acc():c_dim(),(i==3)?(g_light?C(224,234,250):C(30,42,58)):c_panel());
    }
}
static void draw_player(int w,int h)
{
    rf(0,0,w,h,c_panel());
    if (g_t > 204.f){
        float f=(g_t-204.f)/(SONG_DUR-204.f); if(f<0)f=0; if(f>1)f=1;
        f=f*f;
        fx_color_t bg=c_panel(), fg=c_txt(), dm=c_dim();
        fx_color_t fg2=mixc(fg,bg,f), dm2=mixc(dm,bg,f);
        tx(32,w/2-160,6,"world.execute(me);",fg2,bg);
        tx(13,w/2-38,50,"— Mili",dm2,bg);
        if(((int)(g_t*2))%2==0){
            int tw=(int)(fxtk_text_width_size((int)(32*g_sc),"world.execute(me);")/g_sc);
            fx_color_t cc=mixc(c_acc(),bg,f*0.8f);
            rf(w/2-160+tw+10,8,3,28,cc);        /* 一直在闪的光标 */
        }
        tx(11,16,28,"fxtk v0.1 -> v2.5 · still running",dm2,bg);
        rf(18,h-14,(int)((w-36)*(1.f-f)),4,c_acc());
        return;
    }
    {   float dl=g_t-(NLYR?LYR[lyric_idx(g_t)].t0:0.f);
        tx(32,18,2,lyric_at(g_t), dl<0.35f?c_acc():c_txt(), c_panel()); }
    {   char tb[96]; int m=(int)(g_t/60), ss=(int)g_t%60;
        snprintf(tb,sizeof tb,"%d:%02d / %d:%02d",m,ss,(int)(SONG_DUR/60),(int)SONG_DUR%60);
        tx(12,w-300,16,tb,c_dim(),c_panel());
        tx(12,w-215,16,"world.execute(me);",c_dim(),c_panel());
        tx(10,w-26,18,"v2.5",c_acc(),c_panel()); }
    {   float pr=g_t/SONG_DUR; if(pr>1)pr=1;
        rf(18,h-14,w-36,4,c_bord());
        rf(18,h-14,(int)((w-36)*pr),4,c_acc());
        for(int k=1;k<10;k++){ int x=18+(w-36)*k/10; rf(x,h-16,2,7,c_bord()); } }
}

static void draw_flash(int w,int h)
{
    rf(0,0,w,h,C(255,255,255));
}
static void draw_notif(int w,int h)
{
    rf(0,0,w,h,c_panel());
    rf(0,0,4,h,c_acc());
    rf(14,16,40,40,c_acc());
    tx(12,24,26,"OS",g_light?C(255,255,255):C(8,18,28),c_acc());
    const char *tt="正在播放"; const char *sb="world.execute(me); · Mili · 212.33s";
    if (g_t>150.f){ tt="第 17 段"; sb="travel · 光开始变亮"; }
    if (g_t>192.f){ tt="快结束了"; sb="最后一个窗口还亮着"; }
    tx(13,64,15,tt,c_txt(),c_panel());
    tx(11,64,40,sb,c_dim(),c_panel());
    tx(12,w-22,10,"x",c_dim(),c_panel());
}

static void draw_win_dbg(int i);
/* ---------- Ein Dos Trois Ne Fem Liu: 六扇窗各变一个字, 然后迅速变回 ---------- */
static int ein_active(void){ return g_t>=158.77f && g_t<161.79f; }
static void draw_ein(int i,int w,int h)
{
    static const char *WD[6]={"Ein","Dos","Trois","Ne","Fem","Liu"};
    static const char *LG[6]={"Deutsch","Español","Français","Ne","Svenska","中文"};
    static const unsigned char CC[6][3]={{255,216,102},{121,192,255},{210,168,255},
                                         {126,231,135},{255,138,128},{140,255,220}};
    fx_color_t col=C(CC[i][0],CC[i][1],CC[i][2]);
    float age=g_t-158.77f;
    int flash=(age<0.12f);                       /* 开头闪一下白 */
    fx_color_t bg=flash?C(245,247,250):C(8,10,14);
    fx_color_t fg=flash?C(8,10,14):col;
    rf(0,0,w,h,bg);
    DR(0,0,w-1,h-1,flash?bg:col);
    const char *wd=WD[i];
    int sz=300;
    for(; sz>10; sz-=3){
        if((float)sz*1.35f<=h-16 && fxtk_text_width_size((int)(sz*g_sc),wd)/g_sc<=w-18) break;
    }
    int tw=(int)(fxtk_text_width_size((int)(sz*g_sc),wd)/g_sc);
    tx(sz,(w-tw)/2,(int)((h-sz*1.2f)/2),wd,fg,bg);
    tx(10,8,8,LG[i],flash?C(60,70,90):C(150,160,175),bg);
    {   const char *vg="v2.5";
        int vw=(int)(fxtk_text_width_size((int)(10*g_sc),vg)/g_sc);
        tx(10,w-vw-10,8,vg,flash?C(60,70,90):C(110,118,130),bg); }
    rf(0,h-6,w,6,fg);
}

static void draw_win(int i)
{
    int w=(int)(fxtk_win_width(g_win[i])/g_sc+0.5f), h=(int)(fxtk_win_height(g_win[i])/g_sc+0.5f);
    if(ein_active()){ draw_ein(i,w,h); return; }
    switch(i){
    case UI_EDIT: draw_edit(w,h); break;
    case UI_CMD: draw_cmd(w,h); break;
    case UI_SPEC: draw_art(w,h); break;
    case UI_CHAT: draw_chat(w,h); break;
    case UI_PLAYER: draw_player(w,h); break;
    case UI_NOTIF: draw_notif(w,h); break;
    }
}

/* ---------------- 时间轴 ---------------- */
static void layout(int i,float t,float *x,float *y)
{
    float u1=(t-70.f)/4.5f; u1=u1<0?0:(u1>1?1:u1); u1=u1*u1*(3-2*u1);
    float u2=(t-110.f)/3.5f; u2=u2<0?0:(u2>1?1:u2); u2=u2*u2*(3-2*u2);
    float x0=LA[i][0]+(LB[i][0]-LA[i][0])*u1, y0=LA[i][1]+(LB[i][1]-LA[i][1])*u1;
    *x=x0+(LC[i][0]-x0)*u2; *y=y0+(LC[i][1]-y0)*u2;
}


/* 内容签名: 相同则跳过重绘 (llvmpipe/低端 GPU 上省大量像素) */
static long win_sig(int i)
{
    switch(i){
    case UI_EDIT:   return (long)fmodf(g_t,9.f);                          /* 当前行 */
    case UI_CMD:    return ((long)(g_t*1.2f))*2 + ((long)(g_t*2)%2);       /* 日志行数 + 光标闪烁 */
    case UI_SPEC:   return (long)(g_t*20.f);                              /* 频谱: 20fps */
    case UI_CHAT:   { int n=(int)(g_t*0.30f)-2; if(n<0)n=0; if(n>7)n=7; return n; }
    case UI_PLAYER: return (long)(g_t*10.f);                              /* 进度条: 10fps */
    case UI_NOTIF:  return (long)(g_t*10.f);
    }
    return 0;
}

static int g_flash_on = 0;
static float g_an[NWIN];
static float g_px[NWIN], g_py[NWIN]; static int g_pinit=0;
/* 章节 -> 窗口开关 (bit: 0台词 1终端 2频谱 3章节 4歌词 5通知) */
static const unsigned SCENE_MASK[25] = {
    0x11u,                        /* 0  intro: 台词+歌词 */
    0x15u,                        /* 1  +频谱 */
    0x17u,                        /* 2  +终端 */
    0x1Fu,                        /* 3  +章节 全开 */
    0x1Fu, 0x1Fu, 0x1Fu,          /* 4-6 */
    0x15u, 0x1Fu,                 /* 7  终端关; 8 全开 */
    0x13u, 0x1Fu,                 /* 9  频谱关; 10 全开 */
    0x1Du, 0x1Fu,                 /* 11 终端关; 12 全开 */
    0x13u, 0x1Fu, 0x15u, 0x1Fu,   /* 13-16 */
    0x1Fu, 0x19u, 0x1Fu, 0x1Fu,   /* 17 全开; 18 频谱关; 19-20 全开 */
    0x15u, 0x1Fu, 0x11u, 0x11u    /* 21 终端关; 22 全开; 23-24 只剩台词+歌词 */
};
static void update(float t)
{
    if (!g_light && t>=110.f) g_light=1;
    {   int want = (t>=110.f && t<110.16f)
                 || (t>=158.77f && t<158.87f)   /* Ein 六窗开场闪白 */
                 || (t>=161.79f && t<161.89f);  /* Ein 结束闪回 */
        if (want && !g_flash_on && g_flash>=0){ fxtk_win_show(g_flash,1); fxtk_win_move(g_flash,g_ox,g_oy); g_flash_on=1; }
        else if (!want && g_flash_on && g_flash>=0){ fxtk_win_show(g_flash,0); g_flash_on=0; }
    }
    static float last_t=-1.f;
    float dt = (last_t<0.f) ? 1.f/60.f : (t-last_t);
    last_t=t; if(dt<=0.f||dt>0.2f)dt=1.f/60.f;
    {   int ea=ein_active();
        g_ein_on=ea;
        static int last_nl=-1;
        int nl=scr_last()+1;
        for(int i=0;i<NWIN;i++){
            int iw=(ea?380:SZ[i][0]);
            int ih=(ea?240:SZ[i][1]);
            if(i==UI_EDIT && !ea){
                ih=140+nl*32; if(ih>340)ih=340; if(ih<180)ih=180;
            }
            int pw=(int)(fxtk_win_width(g_win[i])/g_sc+0.5f);
            int ph=(int)(fxtk_win_height(g_win[i])/g_sc+0.5f);
            int need = (i==UI_EDIT && !ea) ? (nl!=last_nl) : (iw!=pw || ih!=ph);
            if(need) fxtk_win_resize(g_win[i],(int)(iw*g_sc),(int)(ih*g_sc));
        }
        if(!ea) last_nl=nl;
    }

    {   static int prev_li=-2;
        int li_now=lyric_idx(t);
        if(prev_li!=-2 && li_now!=prev_li){ g_zoom=1.f; g_nudge=1.f; }
        prev_li=li_now;
        float dz=expf(-dt*5.5f);
        g_zoom*=dz; if(g_zoom<0.01f)g_zoom=0.f;
        g_nudge*=expf(-dt*4.f); if(g_nudge<0.01f)g_nudge=0.f;
    }

    {   static int prev_lay=0;
        int lay=(t>=70.f && t<70.08f) || (t>=110.f && t<110.08f);
        if(lay && !prev_lay) g_lp=1.f;
        prev_lay=lay;
        g_lp*=expf(-dt*4.5f); if(g_lp<0.01f)g_lp=0.f;
    }
    int ch=(int)(t*25.f/SONG_DUR); if(ch<0)ch=0; if(ch>24)ch=24;
    g_ch = ch;
    unsigned mask = SCENE_MASK[ch] | (1u<<UI_EDIT) | (1u<<UI_PLAYER) | (1u<<UI_SPEC);   /* 台词/歌词/画面窗始终开着 */
    if(ein_active()) mask = 0x3Fu;   /* Ein/Dos/Trois 段: 六扇窗全部亮出来 */
    if (t>=196.f) mask = (1u<<UI_PLAYER)|(1u<<UI_EDIT);
    for (int i=0;i<NWIN;i++){
        if (i==UI_NOTIF){
            int vis=0; float x=0,y=0;
            const float W0[3]={116.f,150.f,192.f};
            for(int k=0;k<3;k++){
                float a=W0[k], b=a+7.f;
                if (t>a && t<b){
                    vis=1;
                    float u=(t-a)/0.55f; if(u>1)u=1;
                    float d=(b-t)/0.55f; if(d>1)d=1;
                    float sl=u<d?u:d;
                    x=1280.f-(1280.f-900.f)*sl; y=500.f;
                }
            }
            if (ein_active()){ vis=1; x=840.f; y=370.f; g_an[i]=1.f; }
            if (vis && !g_shown[i]){ fxtk_win_show(g_win[i],1); g_shown[i]=1; }
            if (!vis && g_shown[i]){ fxtk_win_show(g_win[i],0); g_shown[i]=0; }
            if (vis) win_place(i,x,y);
            continue;
        }
        int want = (mask>>i)&1;
        float sp = dt/0.45f;
        g_an[i] += want ? sp : -sp;
        if (g_an[i]<0) g_an[i]=0; if (g_an[i]>1) g_an[i]=1;
        int vis = g_an[i] > 0.02f;
        if (ein_active()) g_an[i]=1.f;
        if (vis && !g_shown[i]){ fxtk_win_show(g_win[i],1); g_shown[i]=1; }
        if (!vis && g_shown[i]){ fxtk_win_show(g_win[i],0); g_shown[i]=0; }
        if (vis){
            float x,y; layout(i,t,&x,&y);
            static const float DX[NWIN]={-420.f,-420.f,420.f,420.f,0.f,0.f};
            static const float DY[NWIN]={0.f,320.f,0.f,320.f,120.f,0.f};
            float e=g_an[i]; e=e*e*(3-2*e);
            x += DX[i]*(1.f-e); y += DY[i]*(1.f-e);
            /* 每句歌词给所有窗口一点位移, 像镜头在动 */
            static int nodrift=-1;
            if(nodrift<0){ nodrift = getenv("WEE_NODRIFT")?1:0; }
            int li=nodrift?0:(lyric_idx(t)%3);
            static const float OX[3]={0.f,36.f,-36.f};
            static const float OY[3]={0.f,-14.f,14.f};
            float tx=x+OX[li], ty=y+OY[li];
            if(i==UI_EDIT && !g_ein_on){
                int nl=scr_last()+1; if(nl<1)nl=1;
                float th=140.f+nl*32.f; if(th>340.f)th=340.f; if(th<180.f)th=180.f;
                float ke=dt*2.4f; if(ke>1)ke=1;
                g_edit_h += (th-g_edit_h)*ke;
                int ih=(int)g_edit_h;
                { int ch=(int)(fxtk_win_height(g_win[i])/g_sc); if(ih>ch+2 || ih<ch-2) fxtk_win_resize(g_win[i],(int)(SZ[i][0]*g_sc),(int)(ih*g_sc)); }
                ty += (340.f-(float)ih);      /* 底边不动, 往上长 */
                (void)0;
            }
            if(g_static){ g_px[i]=tx; g_py[i]=ty; }
            if(!g_pinit){ g_px[i]=tx; g_py[i]=ty; }
            else if(!g_static){
                float k=dt*3.2f; if(k>1)k=1;
                g_px[i]+=(tx-g_px[i])*k; g_py[i]+=(ty-g_py[i])*k;
            }
            if(g_ein_on){
                int col=i%3,row=i/3;
                float txg=40.f+col*400.f, tyg=40.f+row*330.f;
                float ke2=dt*9.f; if(ke2>1.f)ke2=1.f;
                g_px[i]+=(txg-g_px[i])*ke2; g_py[i]+=(tyg-g_py[i])*ke2;
                if(fabsf(g_px[i]-txg)<1.f) g_px[i]=txg;
                if(fabsf(g_py[i]-tyg)<1.f) g_py[i]=tyg;
            } else {
                if(g_static){ g_px[i]=tx; g_py[i]=ty; }
                else {
                    float ndir=(i%2)?1.f:-1.f;
                    g_px[i]+=ndir*g_nudge*14.f*(1.f+i*0.06f);
                    g_py[i]+=((i%3)-1)*g_nudge*6.f;
                }
            }
            win_place(i,g_px[i],g_py[i]);
        }
    }
}

int main(int argc,char**argv)
{
    (void)argc;(void)argv;
    const char *e;
    if ((e=getenv("WEE_DUR"))) g_dur=atoi(e);
    if ((e=getenv("WEE_T0")))  g_t=(float)atof(e);
    if (getenv("WEE_STATIC")) g_static=1;
    fxtk_mw_register();
    if (fxtk_win_init()!=0){ fprintf(stderr,"fxtk_win_init failed (backend=%s)\n",fxtk_win_backend_name()); return 1; }
    {   int sw=0,sh=0; fxtk_win_screen(&sw,&sh);
        if (sw>0 && sh>0){
            float sx=(float)sw/1280.f, sy=(float)sh/720.f;
            g_sc = sx<sy?sx:sy; if(g_sc<1.f)g_sc=1.f; if(g_sc>3.f)g_sc=3.f;
        }
        g_ox=(int)((sw-1280*g_sc)/2); if(g_ox<0)g_ox=0;
        g_oy=(int)((sh-720*g_sc)/2);  if(g_oy<0)g_oy=0;
    }
    for(int i=0;i<NWIN;i++){
        g_win[i]=fxtk_win_create((int)(SZ[i][0]*g_sc),(int)(SZ[i][1]*g_sc),(int)(LA[i][0]*g_sc)+g_ox,(int)(LA[i][1]*g_sc)+g_oy,
            i==UI_EDIT?"notes.txt - 台词":(i==UI_CMD?"cmd.exe":(i==UI_SPEC?"spectrum":(i==UI_CHAT?"chapters":(i==UI_PLAYER?"player":"notice")))));
        if (g_win[i]<0){ fprintf(stderr,"window %d create failed\n",i); return 1; }
        fxtk_win_show(g_win[i],0);   /* 启动先全部隐藏, 由 update() 按章节打开, 避免闪窗 */
        g_shown[i]=0;
    }
    g_pinit=0;
    for(int i=0;i<NWIN;i++){ g_cw[i]=(float)SZ[i][0]; g_chh[i]=(float)SZ[i][1]; }
    g_flash = fxtk_win_create_ex((int)(1280*g_sc),(int)(720*g_sc),g_ox,g_oy,"",FXTK_WIN_BORDERLESS);
    if (g_flash>=0) fxtk_win_show(g_flash,0);
    /* 音频: 默认同目录 song.mp3 (WEE_AUDIO 可覆盖); WEE_T0 预览时不播放 */
    {
        const char *ap=getenv("WEE_AUDIO");   /* 默认静音, 只有显式设置 WEE_AUDIO 才播放 */
        if (g_t < 0.5f && ap && ap[0]) {
#ifdef _WIN32
            char cmd[512];
            snprintf(cmd,sizeof cmd,"open \"%s\" type mpegvideo alias wee",ap);
            if (mciSendStringA(cmd,NULL,0,NULL)==0) mciSendStringA("play wee",NULL,0,NULL);
#else
            char cmd[640];
            snprintf(cmd,sizeof cmd,"ffplay -nodisp -autoexit -loglevel quiet \"%s\" >/dev/null 2>&1 &",ap);
            { int sys_rc = system(cmd); (void)sys_rc; }
#endif
        }
    }
    g_t0=fxtk_win_time();
    double start=fxtk_win_time()-g_t;   /* 让 WEE_T0 生效 */
    double t_begin = g_t;
    (void)t_begin;
    float last=g_t; int frame=0;
    int fps_cap=60;
    { const char *fe=getenv("WEE_FPS"); if (fe && atoi(fe)>0) fps_cap=atoi(fe); if (fps_cap>60) fps_cap=60; }
    int frame_ms=(fps_cap>0)?(1000/fps_cap):16;
    for(;;){
        fxtk_win_poll();
        if (fxtk_win_quit()) break;
        double now=fxtk_win_time();
        g_t=(float)(now-start);
        if (g_t>=g_dur) break;
        update(g_t);
        int need = (g_t-last > 0.5f) ? 1 : 0;
        (void)need;
        for(int i=0;i<NWIN;i++){
            if(!g_shown[i]) continue;
            fxtk_win_begin(g_win[i]);
            draw_win(i);
            fxtk_win_end(g_win[i]);
        }
        if (g_flash_on && g_flash>=0){ fxtk_win_raise(g_flash);
            fxtk_win_begin(g_flash); draw_flash(1280,720); fxtk_win_end(g_flash);
        }
        last=g_t; frame++;
#ifdef _WIN32
        Sleep((DWORD)frame_ms);
#else
        { struct timespec ts={0, (long)frame_ms*1000000L}; nanosleep(&ts,NULL); }
#endif
    }
    { double el = (double)g_t - t_begin; if (el < 0.01) el = 0.01;
      printf("frames=%d  avg=%.1ffps  elapsed=%.1fs  backend=%s\n",
             frame, frame/el, el, fxtk_win_backend_name()); }
    return 0;
}
