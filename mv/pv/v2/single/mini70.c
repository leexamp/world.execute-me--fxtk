/* 最小复现: 1200x70 窗口, 整窗填红 + 四角标记 */
#include "fxtk.h"
#include "fxtk_win.h"
#include "fxtk_mw.h"
#include <stdio.h>
#include <unistd.h>
int main(void){
    fxtk_mw_register();
    if (fxtk_win_init()!=0) return 1;
    int w=1200,h=70;
    int win=fxtk_win_create(w,h,40,600,"mini70");
    if(win<0){ fprintf(stderr,"create failed\n"); return 2; }
    for(int f=0; f<200; f++){
        fxtk_win_begin(win);
        fx_set_color(FX_RGB(255,0,0));
        fx_fill_rect(0,0,w-1,h-1);
        fx_set_color(FX_RGB(0,255,0)); fx_fill_rect(0,0,13,13);
        fx_set_color(FX_RGB(0,0,255)); fx_fill_rect(w-14,h-14,w-1,h-1);
        fxtk_win_end(win);
        fxtk_win_poll();
        usleep(16000);
    }
    return 0;
}
