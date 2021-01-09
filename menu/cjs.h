#ifndef CJS_H_INCLUDED
#define CJS_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#define cheat_MAX_keys  20
#define MAX_cheats  20


struct EC_CHEAT {
	char *key_name;
	unsigned char key_nums;
	int key_add[cheat_MAX_keys];
	int key_value[cheat_MAX_keys];

};
typedef struct {
	char *ec_name;
	int ec_default;
    int ec_max;
    EC_CHEAT   ec_cheats[cheat_MAX_keys];
} EC_CHEATS;

//string.h原型函数，windows下没有
char *strtok_rr(char *s, const char *delim, char **save_ptr);
//只分割逗号和分号，哪个先到先分哪个
char *strtok_ec(char *s,  char **save_ptr);
//char指针指向的值+1，返回char
void value_add(char *s);
//add和value长度检查
bool check_add_value(char *add, char *value);
//去除字符串中空白字符
char *strip(char *fu_in_strs, char *fu_out_strs);
//分割字符串
int split(char *fu_in, char *fu_out);
//读取特殊格式的文件
int ec_file(char  *f_name, EC_CHEATS *ff_ecs);
//判断字符是不是都是16进制字符
int check_hex(char *test_chars);
// 连接字符串
char* join3(char *s1, char *s2);
char* str_join(int str_nums, ...);

//更改字符串后缀名.zip-->.cht
char* str_modifi(char *s1, char *s2);
//获取basename
char* gnu_basename(char *pathname);

#endif // CJS_H_INCLUDED
