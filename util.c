#include "util.h"

char* video_memory = VIDEO_MEMORY;
unsigned short cursor_pos = 0;

const char* build_date = __DATE__;
const char* build_time = __TIME__;

const char* kernel_version = KERNEL_VERSION;

unsigned char color = 0x0F;

char cpu_brand[49] = "Unknown";

void get_cpu_brand() {
    unsigned int regs[4];
    for (int i = 0; i < 3; i++) {
        __asm__ volatile (
            "cpuid"
            : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
            : "a"(0x80000002 + i)
        );

        int offset = i * 16;
        *((unsigned int*)(cpu_brand + offset))     = regs[0];
        *((unsigned int*)(cpu_brand + offset + 4)) = regs[1];
        *((unsigned int*)(cpu_brand + offset + 8)) = regs[2];
        *((unsigned int*)(cpu_brand + offset + 12))= regs[3];
    }

    cpu_brand[48] = '\0'; // Ensure null-termination
}

void scroll_if_needed() {
    if (cursor_pos >= WIDTH * HEIGHT) {
        for (int row = 1; row < HEIGHT; row++) {
            for (int col = 0; col < WIDTH; col++) {
                int from = (row * WIDTH + col) * 2;
                int to = ((row - 1) * WIDTH + col) * 2;
                video_memory[to] = video_memory[from];
                video_memory[to + 1] = video_memory[from + 1];
            }
        }

        for (int col = 0; col < WIDTH; col++) {
            int idx = ((HEIGHT - 1) * WIDTH + col) * 2;
            video_memory[idx] = ' ';
            video_memory[idx + 1] = color;
        }

        cursor_pos -= WIDTH; // Only move up one line, keep column
    }
}

void newline() {
    cursor_pos -= cursor_pos % WIDTH; // Move to start of current line
    cursor_pos += WIDTH;              // Move to start of next line
    scroll_if_needed();
}

void change_color(const char* str) {
    color = hex_to_uint(str);

}

void print_string(const char* str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\n') {
            newline();
        } else if (str[i] == '\r') {
            // Move cursor to beginning of current line
            cursor_pos -= cursor_pos % WIDTH;
        } else {
            int index = cursor_pos * 2;
            video_memory[index] = str[i];
            video_memory[index + 1] = color;
            cursor_pos++;
            scroll_if_needed();
        }
    }
    update_cursor();
}


int compare_strings(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

unsigned int hex_to_uint(const char* str) {
    unsigned int result = 0;
    if (str[0] == '0' && str[1] == 'x') {
        str += 2;
    }
    while (*str) {
        char c = *str++;
        result <<= 4;
        if (c >= '0' && c <= '9') result |= c - '0';
        else if (c >= 'A' && c <= 'F') result |= c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') result |= c - 'a' + 10;
        else break;
    }
    return result;
}

// Optional: port I/O, if not moved to separate file
void outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void outw(unsigned short port, unsigned short val) {
    __asm__ __volatile__ ("outw %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void update_cursor() {
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(cursor_pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((cursor_pos >> 8) & 0xFF));
}

int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}


void copy_string(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = 0;
}

int string_to_int(const char* str) {
    int result = 0;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

void int_to_string(int value, char* buffer) {
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = 0;
        return;
    }

    int is_negative = 0;
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    char temp[12];
    int i = 0;
    while (value > 0 && i < 11) {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (is_negative) {
        temp[i++] = '-';
    }

    // Reverse
    int j = 0;
    while (i > 0) {
        buffer[j++] = temp[--i];
    }
    buffer[j] = 0;
}
