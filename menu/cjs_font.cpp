
#include <time.h>
#include "cjs_font.h"





//extern SDL_Surface *mScreen;
//extern SCheatData Cheat;
//SDL

TTF_Font *sdl_ttf_font = NULL;
SDL_Surface *g_font = NULL;
SDL_Surface *screen = mScreen;
SDL_Surface *gui_screen = NULL;
//Screen dimension constants
const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;
const int FONT_SIZE = 14;

SDL_Surface *g_bg;


int InitFont()
{
	// Load picture 
	if ( g_bg == NULL ){
		g_bg = SDL_LoadBMP("backdrop.bmp");
		if ( g_bg == NULL ){
			return -1;
		}
	}
	//SDL_ttf
	//预处理
	if( sdl_ttf_font == NULL ){
		TTF_Init();
		sdl_ttf_font = TTF_OpenFont("/usr/share/gmenu2x/skins/Default/fonts/SourceHanSans-Regular-04.ttf", FONT_SIZE);
		if(sdl_ttf_font == NULL){
			return -1;
		}
	}
	return 0;
}

void KillFont()
{
	SDL_FreeSurface(g_bg);
	//SDL_ttf
	TTF_CloseFont( sdl_ttf_font );
	TTF_Quit();
	//SDL_Quit();
}



void draw_bg(SDL_Surface *bg)
{
	if(bg)
		SDL_BlitSurface(bg, NULL, gui_screen, NULL);
		//SDL_BlitSurface(gui_screen, NULL, bg, NULL);
	else
		SDL_FillRect(gui_screen, NULL, (1<<11) | (8<<5) | 10);
}
int SFC_Get_SCREEN()
{
	SDL_Rect dstrect;
	if (screen == NULL)
	{
		
		if(SDL_VideoModeOK(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWSURFACE | SDL_DOUBLEBUF) != 0)
		{
		//设置输出屏幕
			screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
		}
		else{
			return -1;
		}
	}
	
	//设置内存模拟屏幕
	if (gui_screen == NULL)
	{
		gui_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0xf800, 0x7e0, 0x1f, 0);
		if(gui_screen == NULL) return -1;
	}
	//设置背景
	draw_bg(g_bg);
	//更新屏幕
	dstrect.x = (screen->w - SCREEN_WIDTH) / 2;
	dstrect.y = (screen->h - SCREEN_HEIGHT) / 2;
	SDL_BlitSurface(gui_screen, 0, screen, &dstrect);
	if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);		 //SDL1函数
	if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
	return 0;



}

void SFC_Flip()
{
	if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Rect dstrect;
	dstrect.x = (screen->w - 320) / 2;
	dstrect.y = (screen->h - 240) / 2;
	SDL_BlitSurface(gui_screen, 0, screen, &dstrect);
	SDL_Flip(screen);
	if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
}


//画像素
void DrawPixel(Uint8 R, Uint8 G, Uint8 B,int x,int y)
{
    Uint32 color = SDL_MapRGB(gui_screen->format, R, G, B);
    Uint32 *bufp;

    bufp = (Uint32 *)gui_screen->pixels + y*screen->pitch/4 + x;
    *bufp = color;
   // SDL_Flip(gui_screen);
 SDL_UpdateRect(gui_screen, x, y, 1, 1);
}

//矩形填充，用于着重显示选择项
void drawrect(Uint8 R, Uint8 G, Uint8 B,int x,int y,int width,int heigth)
{


	
	Uint32 color = SDL_MapRGB(gui_screen->format, R, G, B);
	SDL_Rect rect = { x, y, width, heigth };
	SDL_FillRect(gui_screen, &rect, color);
	

}















void DrawText2(const char *textmsg, int x, int y)
{


	if( textmsg == NULL ) return;

	SDL_Color textColor={255, 255, 255};//设置颜色
	SDL_Surface *message=NULL;
	message=TTF_RenderUTF8_Blended(sdl_ttf_font,textmsg, textColor );//加在成中文
	//message=TTF_RenderUTF8_Solid(sdl_ttf_font,textmsg, textColor );//加在成中文
	SDL_Rect dect;
	dect.x=x;
	dect.y=y;
	dect.h=0;
	dect.w=0;
	SDL_BlitSurface(message, NULL, gui_screen, &dect);
	SDL_FreeSurface(message);
	SFC_Flip();
	
}

void DrawText3(const char *textmsg, int x, int y)
{
	if( textmsg == NULL ) return;
	SDL_Color textColor={255, 255, 255};//设置颜色
	SDL_Surface *message=NULL;
	message=TTF_RenderUTF8_Blended(sdl_ttf_font,textmsg, textColor );//加在成中文
	//message=TTF_RenderUTF8_Solid(sdl_ttf_font,textmsg, textColor );//加在成中文
	SDL_Rect dect;
	dect.x=x;
	dect.y=y;
	dect.h=0;
	dect.w=0;
	SDL_BlitSurface(message, NULL, gui_screen, &dect);
	SDL_FreeSurface(message);
}



void SDL_TEST()
{
	SFC_Get_SCREEN();

	char textmsg[] = "SDL_TEST：中文显示";
	SDL_Color textColor={255, 255, 255};//设置颜色
	SDL_Surface *message=NULL;
	message=TTF_RenderUTF8_Blended(sdl_ttf_font,textmsg, textColor );//加在成中文
	SDL_Rect dect;
	dect.x=0;
	dect.y=0;
	dect.h=0;
	dect.w=0;
	SDL_BlitSurface(message, NULL, gui_screen, &dect);
	SDL_FreeSurface(message);
	SFC_Flip();
	int done = 0;
	int line = 1;
	while (!done) {
		//DrawText2("按A退出",0,line*FONT_SIZE);
		readkey();

		
		if (parsekey(SDLK_LCTRL)) {
			done=1;
			S9xDeleteCheats();
		}
		
		line ++;
		
		if (line == 10)
		{
			done = 1;
			S9xAddCheat(1,0,0x1F8A,9);
		}
		SDL_Delay(16);
	}
	//ec_Settings();
}
//
//log
void str_log(char *info)
{
	//获取时间
	time_t rawtime;
	struct tm * timeinfo;
	time (&rawtime);//获取Unix时间戳。
	timeinfo = localtime (&rawtime);;//转为时间结构。
	char now_time[128];
	strftime (now_time,sizeof(now_time),"%Y-%m-%d,%H:%M:%S - :",timeinfo);

	//log记录
	//char *fn=NULL;
	//fn=strdup(FCEU_MakeFName(FCEUMKF_CHEAT,0,0).c_str());
	//fn = "./debug.chj";
	//char *file_name=stringcat(log,"_debug.log");
	//free(fn);

	char log[] = "/media/sdcard/apps/sfc.log";
	FILE *fp_log = fopen (log,"a+");
	//free(file_name);
	//if(!fp_log) return 1;
	fputs(now_time, fp_log);
	fputs(info, fp_log);
	fputs(":", fp_log);
	fputs(log, fp_log);
	fputs("\n", fp_log);
	fflush(fp_log);
	fclose(fp_log);
}