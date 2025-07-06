#ifndef EDITOR_H
#define EDITOR_H

#define MAX_FILES 10
#define FILE_WIDTH 80
#define FILE_HEIGHT 20



void start_editor(const char* filename);
char* get_keypress();

#endif