#ifndef CJS_FONT_H
#define CJS_FONT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "sal.h" //SDL_Surface *mScreen = NULL;
#include "port.h"
#include "cheats.h"
#include "cjs.h"
#include "cjs_sdl.h"

extern SDL_Surface *mScreen;

typedef uint32_t u32;
typedef int32_t  s32;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint8_t  u8;
typedef char     s8;



int InitFont();
void KillFont();
int SFC_Get_SCREEN();
void draw_bg(SDL_Surface *bg);                       //设置背景
void DrawText2(const char *textmsg, int x, int y);   //SDL_ttf
void DrawText3(const char *textmsg, int x, int y);   //内存操作不刷新
void SFC_Flip();                                     //刷新显示
void SDL_TEST();
//void str_log(char *info,char *log);
void str_log(char *info);

//矩形填充，用于着重显示选择项
void drawrect(Uint8 R, Uint8 G, Uint8 B,int x,int y,int width,int heigth);
//日志

//void sal_VideoPrint2(8,50,message1,SAL_RGB(31,31,31));
//void sal_VideoPrint(s32 x, s32 y, const char *buffer, u32 color);
#endif
