
#include "cjs_sdl.h"
#include "cjs.h"



//全局变量
char cheat_menu_yes = 0; //定义chj文件是否读取过
int all_ec_nums = -1; //定义chj文件有效的数量，从0开始计数
extern unsigned long g_key;
extern SDL_Surface *g_bg;
char rom_name[1024];

EC_CHEATS sdl_ecs[MAX_cheats];




/***
文件更新函数
***/
void chj_update(EC_CHEATS *ff_ecs, unsigned long key, int ec_index)
{
	int val=ff_ecs[ec_index].ec_default;
	int max_val=ff_ecs[ec_index].ec_max;
	if (key == SDL_INPUT_RIGHT){
		if (val < max_val){
			val ++;
			}
		}
	else if (key == SDL_INPUT_LEFT){
		if (val > 0){
			val --;
			}
		}
	ff_ecs[ec_index].ec_default=val;

}
/***
匹配文件映射到内存
***/
void cheats_mem(EC_CHEATS *ff_ecs)
{
	//清空
	S9xDeleteCheats();
	//增加需要的内容
	int cheats_max,cheat_max,cheat_default=0;
	int i,ii=0;
	int add,value=0;
	cheats_max = all_ec_nums + 1;
	for (i=0;i<cheats_max;i++)
	{
		cheat_default = ff_ecs[i].ec_default;
		if ( cheat_default>0)
		{
			cheat_max = ff_ecs[i].ec_cheats[cheat_default].key_nums;
			for (ii=0;ii<cheat_max;ii++)
			{
				add =ff_ecs[i].ec_cheats[cheat_default].key_add[ii];
				value=ff_ecs[i].ec_cheats[cheat_default].key_value[ii];
				S9xAddCheat(1,0,add,value);
			}
		}
	}
}

/***
获取ec文件名
***/
char *get_ecfile(char *ecname)
{
	//获取金手指文件文件名
	char *home;
	home=getenv("HOME");
	char *outer_ptr=NULL;
	char *inner_ptr;
	inner_ptr = rom_name;
	char *finall_name_temp;
	char finall_name[1024];
	 while((finall_name_temp=strtok_r(inner_ptr, "/", &outer_ptr))!=NULL)  
	{
		inner_ptr=NULL;
		strcpy(finall_name,finall_name_temp);
	}
	inner_ptr = finall_name;
	finall_name_temp=strtok_r(inner_ptr,".",&outer_ptr);
	strcpy(finall_name,finall_name_temp);
	char *ec_finall_name;
	ec_finall_name=join3(home,"/.pocketsnes/cheats/");
	ec_finall_name=join3(ec_finall_name,finall_name);
	ec_finall_name=join3(ec_finall_name,".cht");
	strcpy(ecname,ec_finall_name);
	return ec_finall_name;
}
char *get_ecfile2(char *ecname)
{
	//获取金手指文件文件名
	char *home;
	char *ec_finall_name;
	char *rom_base;
	char *ec_base;
	rom_base = gnu_basename(rom_name);
	ec_finall_name = str_modifi(rom_name,"cht");
	FILE *fp=fopen(ec_finall_name,"r");
	if(fp==NULL)
	{
		home=getenv("HOME");
		ec_base = str_modifi(rom_base,"cht");
		ec_finall_name=str_join(3,home,"/.pocketsnes/cheats/",ec_base);
	}
	else{
		fclose(fp);
	}
	strcpy(ecname,ec_finall_name);
	return ec_finall_name;
}



/***
显示信息
***/
int ec_Settings()
{
	SFC_Get_SCREEN();
	char ecname[1024];
	if(cheat_menu_yes == 0)
	{
		
		get_ecfile2(ecname);
		all_ec_nums=ec_file(ecname,sdl_ecs);
		cheat_menu_yes=1;
	}

	int start_y = 40; //开始高度
	int start_w = 15; //间距
	static int index = 0;
	static int start_now = index*start_w+start_y;
	int done = 0, y, i;
	int max_entries = 9;
	int menu_size = all_ec_nums+1;
	static int offset_start = 0;
	static int offset_end = menu_size > max_entries ? max_entries : menu_size;

	char tmp[32];
	int itmp;
	int cheat_index = 0;




	//先显示菜单，再更新菜单
	draw_bg(g_bg);
	DrawText3("金手指菜单", 0, 13);
	drawrect(255, 0, 0,  0,start_now,320,start_w);
	if (all_ec_nums > -1)
	{
		// Draw menu
		for (i = offset_start, y = 40; i < offset_end; i++, y += 15) {
			DrawText3(sdl_ecs[i].ec_name, 30, y);
			cheat_index=sdl_ecs[i].ec_default;
			sprintf(tmp, "%s", sdl_ecs[i].ec_cheats[cheat_index].key_name);
			DrawText3( tmp, 210, y);
			}
		// Draw offset marks
		
		// Update real screen
		SDL_Delay(16);
		SFC_Flip();




		while (!done) {
			// Parse input
			readkey();
			if (parsekey(SDL_INPUT_B))
				done = 1;
			if (parsekey(SDL_INPUT_UP)) {
				if (index > 0) 
				{
					index--;
					if (offset_start == 0)
					{
						start_now=start_now-start_w;
					}
					if ((offset_start > 0) && (index < offset_start)  )
					{
						offset_start--;
						offset_end--;
						start_now=0*start_w+start_y;
					}
					else if ((offset_start > 0) && (index >= offset_start)  )
					{
						start_now=start_now-start_w;
					}
				} else 
				{
					index = menu_size-1;
					offset_end = menu_size;
					offset_start = menu_size <= max_entries ? 0 : offset_end - max_entries;
					start_now=menu_size <= max_entries ? (menu_size-1)*start_w+start_y:(max_entries-1)*start_w+start_y;
					
				}
			}
			if (parsekey(SDL_INPUT_DOWN)) {
				if (index < (menu_size - 1)) 
				{
					index++;
					if (index < offset_end)
					{
						start_now=start_now+start_w;
					}
					if ((index >= offset_end) && (offset_end < menu_size)  ) 
					{
						offset_end++;
						offset_start++;
						start_now=(max_entries-1)*start_w+start_y;
					}
				} 
				else 
				{
					index = 0;
					offset_start = 0;
					offset_end = menu_size <= max_entries ? menu_size : max_entries;
					start_now=index*start_w+start_y;
					
				}
			}

			if (parsekey(SDL_INPUT_LEFT) || parsekey(SDL_INPUT_RIGHT) || parsekey(
					SDL_INPUT_A)){
				chj_update(sdl_ecs, g_key, index);
			}

			/***
			汉字处理
			***/
			draw_bg(g_bg);
			DrawText3("金手指菜单", 0, 13);
			drawrect(255, 0, 0,  0,start_now,320,start_w);
			// Draw menu
			for (i = offset_start, y = start_y; i < offset_end; i++, y += start_w) {
				//str_log(sdl_ecs[i].ec_name);
				DrawText3(sdl_ecs[i].ec_name, 30, y);
				cheat_index=sdl_ecs[i].ec_default;
				sprintf(tmp, "%s", sdl_ecs[i].ec_cheats[cheat_index].key_name);
				DrawText3( tmp, 210, y);
				}
			// Draw offset marks

			// Update real screen
			SDL_Delay(16);
			SFC_Flip();
		}
		cheats_mem(sdl_ecs);
	}
	else if(all_ec_nums == -2)
	{
		DrawText3("无金手指文件,按B退出", 0, start_now);
		DrawText3(ecname, 0, start_now+start_w);
		SDL_Delay(16);
		SFC_Flip();
		while (!done) {
			readkey();
			if (parsekey(SDL_INPUT_B))
				done = 1;
		}
	}
	else if(all_ec_nums == -1)
	{
		DrawText3("金手指文件未发现有效数据,按B退出", 0, start_now);
		SDL_Delay(16);
		SFC_Flip();
		while (!done) {
			readkey();
			if (parsekey(SDL_INPUT_B))
				done = 1;
		}
	}

	// Clear screen
	return 0;
}

//按键相关
unsigned long g_key = 0;
void readkey() 
{
	SDL_Event event;
	// loop, handling all pending events
	while(SDL_WaitEvent(&event))
		if(event.type==SDL_KEYDOWN)
		{
			SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY + 100, SDL_DEFAULT_REPEAT_INTERVAL);
			g_key = event.key.keysym.sym;
			return;
		}
}
int parsekey(unsigned long code) 
{
	if (g_key == code) 
	{
		return 1;
	}
	return 0;

}

