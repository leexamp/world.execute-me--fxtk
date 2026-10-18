/* 文本背景颜色测试: 12px 文本, bg 深色, 看方块颜色 */
#include "fxtk.h"
#include "fxtk_win.h"
#include "fxtk_mw.h"
#include <stdio.h>
#include <unistd.h>
int main(void){
    fxtk_mw_register();
    if (fxtk_win_init()!=0) return 1;
    int w=600,h=200;
    int win=fxtk_win_create(w,h,100,100,"txtbg");
    for(int f=0; f<200; f++){
        fxtk_win_begin(win);
        fx_set_color(FX_RGB(22,27,34)); fx_fill_rect(0,0,w-1,h-1);
        fxtk_draw_text_size(12, 20, 20, "1:02 / 3:32", FX_RGB(139,148,158), FX_RGB(22,27,34));
        fxtk_draw_text_size(12, 20, 60, "world.execute(me);", FX_RGB(139,148,158), FX_RGB(22,27,34));
        fxtk_draw_text_size(30, 20, 100, "If I can give you", FX_RGB(230,237,243), FX_RGB(22,27,34));
        fxtk_win_end(win);
        fxtk_win_poll();
        usleep(16000);
    }
    return 0;
}
