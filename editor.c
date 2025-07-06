#include "editor.h"
#include "util.h"

#define MAX_FILES 10

typedef struct {
    char name[16];
    char content[FILE_HEIGHT][FILE_WIDTH + 1]; // +1 for null terminator
} TextFile;

extern char* video_memory;
extern unsigned short cursor_pos;

extern TextFile files[MAX_FILES];

#define ARROW_UP    0x48
#define ARROW_DOWN  0x50
#define ARROW_LEFT  0x4B
#define ARROW_RIGHT 0x4D

void update_cursor_position(int x, int y) {
    cursor_pos = y * WIDTH + x;
    update_cursor();
}

void clear_screen_blue() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int idx = (y * WIDTH + x) * 2;
            video_memory[idx] = ' ';
            video_memory[idx + 1] = 0x1F;
        }
    }
}

void draw_editor_screen(int file_index) {
    for (int y = 0; y < FILE_HEIGHT; y++) {
        for (int x = 0; x < FILE_WIDTH; x++) {
            int idx = (y * WIDTH + x) * 2;
            video_memory[idx] = files[file_index].content[y][x];
            video_memory[idx + 1] = 0x1F; // Blue background, white text
        }
    }
}

void start_editor(const char* filename) {
    int file_index = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (compare_strings(files[i].name, filename)) {
            file_index = i;
            break;
        }
    }
    if (file_index == -1) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (!files[i].name[0]) {
                copy_string(files[i].name, filename);
                for (int y = 0; y < FILE_HEIGHT; y++) {
                    for (int x = 0; x < FILE_WIDTH; x++) {
                        files[i].content[y][x] = ' ';
                    }
                    files[i].content[y][FILE_WIDTH] = 0;
                }
                file_index = i;
                break;
            }
        }
    }
    if (file_index == -1) {
        print_string("\nNo space for new file.");
        return;
    }

    int cx = 0, cy = 0;
    clear_screen_blue();
    draw_editor_screen(file_index);

    while (1) {
        update_cursor_position(cx, cy);
        char* key = get_keypress();

        if (key[0] == 27) break; // ESC

        if (key[0] == 0) {
            switch (key[1]) {
                case ARROW_UP: if (cy > 0) cy--; break;
                case ARROW_DOWN: if (cy < FILE_HEIGHT - 1) cy++; break;
                case ARROW_LEFT: if (cx > 0) cx--; break;
                case ARROW_RIGHT: if (cx < FILE_WIDTH - 1) cx++; break;
            }
        } else if (key[0] == '\b') {
            if (cx > 0) {
                cx--;
                files[file_index].content[cy][cx] = ' ';
            }
        } else if (key[0] >= 32 && key[0] <= 126) {
            files[file_index].content[cy][cx] = key[0];
            cx = (cx + 1) % FILE_WIDTH;
        } else if (key[0] == '\n') {
            if (cy < FILE_HEIGHT - 1) {
                cy++;
                cx = 0;
            }
        }


        // Redraw line with blue background
        for (int x = 0; x < FILE_WIDTH; x++) {
            int idx = (cy * WIDTH + x) * 2;
            video_memory[idx] = files[file_index].content[cy][x];
            video_memory[idx + 1] = 0x1F; // Blue background
        }
    }
}
