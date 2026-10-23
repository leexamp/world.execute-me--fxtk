# world.execute(me); 桌面多窗口 MV — 一键构建 (v2.5.2)
#
#   make            Linux 可执行文件 ./movie
#   make win        Windows 可执行文件 world_execute_me.exe (需要 mingw-w64)
#   make run        构建并运行
#   make clean
#
# 手动命令/细节见 BUILD.md。

CC      ?= gcc
ROOT    := ../../../..
CORE    := $(ROOT)/components/fxtk/fxtk.c $(ROOT)/components/fxtk/fxtk_draw.c \
           $(ROOT)/components/fxtk/fxtk_widgets.c $(ROOT)/components/fxtk/fxtk_extra.c \
           $(ROOT)/components/fxtk/fxtk_effects.c $(ROOT)/components/fxtk/fxtk_backends.c \
           $(ROOT)/components/fxtk/fxtk_gesture.c $(ROOT)/components/fxtk/fxtk_par.c \
           $(ROOT)/components/fxtk/fxtk_flex.c $(ROOT)/components/fxtk/fxtk_svg.c
INC     := -I$(ROOT)/components/fxtk -I$(ROOT)/drivers -I$(ROOT)/third_party/sokol
# Linux 发布档: -Os + 段裁剪 + strip (体积约 318KB)
FLAGS   := -DFXTK_MW -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -s
# Windows: MinGW 下与 -ffunction-sections 组合会产出 17MB 未剥离产物, 故不加这两项
WINFLAGS:= -DFXTK_MW -Os -Wl,--gc-sections -s
SRC     := movie.c
DEPS    := $(SRC) script.inc $(CORE) $(ROOT)/drivers/fxtk_sokol_mw.c \
           $(ROOT)/drivers/fxtk_sokol_driver.c $(ROOT)/drivers/fxtk_font_stb.c \
           $(ROOT)/drivers/fxtk_win.c
LDLIBS  := -lX11 -lXi -lXcursor -lGL -ldl -lpthread -lm

.PHONY: all run clean win
all: movie
movie: $(DEPS)
	$(CC) $(FLAGS) $(INC) $(SRC) $(ROOT)/drivers/fxtk_sokol_mw.c $(ROOT)/drivers/fxtk_sokol_driver.c \
	    $(ROOT)/drivers/fxtk_font_stb.c $(ROOT)/drivers/fxtk_win.c $(CORE) -o $@ $(LDLIBS)
win: world_execute_me.exe
world_execute_me.exe: $(DEPS)
	x86_64-w64-mingw32-gcc $(WINFLAGS) $(INC) $(SRC) $(ROOT)/drivers/fxtk_sokol_mw.c \
	    $(ROOT)/drivers/fxtk_sokol_driver.c $(ROOT)/drivers/fxtk_font_stb.c $(ROOT)/drivers/fxtk_win.c $(CORE) \
	    -o $@ -lopengl32 -lgdi32 -luser32 -lole32 -loleaut32 -luuid -lshell32 -lwinmm -mwindows
run: movie
	./movie
clean:
	rm -f movie world_execute_me.exe
