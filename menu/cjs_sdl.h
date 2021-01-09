#ifndef CJS_SDL_H_INCLUDED
#define CJS_SDL_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "port.h"
#include "cheats.h"
#include "cjs_font.h"

//文件更新函数
void chj_update(EC_CHEATS *ff_ecs, unsigned long key, int ec_index);
//文件更新到内存
void cheats_mem(EC_CHEATS *ff_ecs);
//菜单函数
int ec_Settings();



//按键比对
void readkey();
int parsekey(unsigned long code);

//按键定义
#define SDL_INPUT_UP SDLK_UP
#define SDL_INPUT_DOWN SDLK_DOWN
#define SDL_INPUT_LEFT SDLK_LEFT
#define SDL_INPUT_RIGHT SDLK_RIGHT
#define SDL_INPUT_A SDLK_LCTRL
#define SDL_INPUT_B SDLK_LALT
#define SDL_INPUT_X SDLK_SPACE
#define SDL_INPUT_Y SDLK_LSHIFT
#define SDL_INPUT_L SDLK_TAB
#define SDL_INPUT_R SDLK_BACKSPACE
#define SDL_INPUT_START SDLK_RETURN
#define SDL_INPUT_SELECT SDLK_ESCAPE
#define SDL_INPUT_MENU SDLK_HOME
#endif // CJS_SDL_H_INCLUDED
