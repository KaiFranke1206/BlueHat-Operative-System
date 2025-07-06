#ifndef UTIL_H
#define UTIL_H

#define VIDEO_MEMORY (char*)0xB8000
#define WIDTH 80
#define HEIGHT 25

#define KERNEL_VERSION "EG-Kernel 0.3.1-dev"
extern const char* kernel_version;

extern const char* build_date;
extern const char* build_time;

extern char* video_memory;
extern unsigned short cursor_pos;

extern char cpu_brand[49];
extern unsigned char color;

void get_cpu_brand();
void scroll_if_needed();
void newline();
void change_color(const char* str);
void print_string(const char* str);
int compare_strings(const char* a, const char* b);
unsigned int hex_to_uint(const char* str);
void outb(unsigned short port, unsigned char val);
void outw(unsigned short port, unsigned short val);
unsigned char inb(unsigned short port);
void update_cursor();
int starts_with(const char* str, const char* prefix);
void copy_string(char* dest, const char* src);
int string_to_int(const char* str);
void int_to_string(int value, char* buffer);

#endif
