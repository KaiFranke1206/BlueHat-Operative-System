#include "basic.h"
#include "util.h"

#define MAX_LINES 64

int stack[128] = {0};
int sp = 0;

int returnToCMD = 0;

typedef struct {
    int number;
    char content[64];
} BasicLine;

BasicLine program[MAX_LINES];
int line_count = 0;

void itoa(int value, char* buffer, int base) {
    if (base < 2 || base > 16) {
        buffer[0] = '\0';
        return;
    }

    char digits[] = "0123456789ABCDEF";
    char temp[32];
    int i = 0;
    int is_negative = 0;

    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value;
    }

    while (value != 0) {
        temp[i++] = digits[value % base];
        value /= base;
    }

    if (is_negative) {
        temp[i++] = '-';
    }

    // Reverse temp into buffer
    int j = 0;
    while (i > 0) {
        buffer[j++] = temp[--i];
    }
    buffer[j] = '\0';
}


void clear_program() {
    for (int i = 0; i < MAX_LINES; i++) {
        program[i].number = 0;
        program[i].content[0] = 0;
    }
    line_count = 0;
}

int find_line_index(int number) {
    for (int i = 0; i < line_count; i++) {
        if (program[i].number == number)
            return i;
    }
    return -1;
}

void list_program() {
    for (int i = 0; i < line_count; i++) {
        print_string("\n");
        char num[6];
        int n = program[i].number;
        int pos = 0;
        if (n == 0) num[pos++] = '0';
        else {
            char tmp[6];
            int len = 0;
            while (n > 0) {
                tmp[len++] = '0' + (n % 10);
                n /= 10;
            }
            while (len--) num[pos++] = tmp[len];
        }
        num[pos] = 0;
        print_string(num);
        print_string(" ");
        print_string(program[i].content);
    }
}

void add_line(int number, const char* content) {
    for (int i = 0; i < line_count; i++) {
        if (program[i].number == number) {
            print_string("Replacing line ");
            char buf[16];
            itoa(number, buf, 10);
            print_string(buf);
            print_string("\n");

            // Properly clear and overwrite
            for (int j = 0; j < 64; j++) {
                program[i].content[j] = 0;
            }
            for (int j = 0; content[j] && j < 63; j++) {
                program[i].content[j] = content[j];
            }
            program[i].content[63] = 0; // Ensure null-termination
            return;
        }
    }

    // Insert new line if not found
    if (line_count < MAX_LINES) {
        int i = 0;
        while (i < line_count && program[i].number < number) i++;
        for (int j = line_count; j > i; j--) {
            program[j] = program[j - 1];
        }

        program[i].number = number;
        for (int j = 0; j < 64; j++) {
            program[i].content[j] = 0;
        }
        for (int j = 0; content[j] && j < 63; j++) {
            program[i].content[j] = content[j];
        }
        program[i].content[63] = 0;
        line_count++;
    }
}


void push_stack(int value) {
    if (sp < 128) {
        stack[++sp] = value;
    } else {
        print_string("Stack Overflow");
    }
}

int pop_stack() {
    if (sp > 0) {
        return stack[sp--];
    } else {
        print_string("Stack Underflow");
        return 0;
    }
}

int stack_top() {
    if (sp > 0) {
        return stack[sp - 1];
    } else {
        print_string("Stack is empty\n");
        return 0;
    }
}

int read_number() {
    char buffer[16];
    int index = 0;

    print_string("Enter number: ");

    while (index < 15) {
        while (!(inb(0x64) & 1)); // wait for keypress
        unsigned char scancode = inb(0x60);

        // Basic US QWERTY scancode to ASCII mapping for 0–9 and Enter
        char c = 0;
        if (scancode >= 0x02 && scancode <= 0x0B) {
            c = "1234567890"[scancode - 0x02];
        } else if (scancode == 0x1C) { // Enter key
            break;
        }

        if (c) {
            buffer[index++] = c;
            char s[2] = {c, '\0'};
            print_string(s); // echo character
        }
    }

    buffer[index] = '\0';

    // Convert to integer
    int result = 0;
    for (int i = 0; i < index; i++) {
        result = result * 10 + (buffer[i] - '0');
    }

    print_string("\n");
    return result;
}

void run_program() {
    int pc = 0;

    while (pc < line_count) {
        // Allow breaking with 'c'
        if (inb(0x64) & 1) {
            unsigned char code = inb(0x60);
            if (code == 0x2E) { // scancode for 'c'
                print_string("\nprogram interrupted by 'c'\n");
                break;
            }
        }

        char* line = program[pc].content;

        if (starts_with(line, "printt \"")) {
            char* start = line + 8;
            char* end = start;
            while (*end && *end != '"') end++;
            *end = '\0';
            print_string(start);
            print_string("\n");
            pc++;
        } else if (starts_with(line, "printv")) {
            int val = stack_top();
            char buffer[32];
            itoa(val, buffer, 10);
            print_string(buffer);
            print_string("\n");
            pc++;
        } else if (starts_with(line, "push ")) {
            int value = 0;
            char* ptr = line + 5;
            while (*ptr >= '0' && *ptr <= '9') {
                value = value * 10 + (*ptr - '0');
                ptr++;
            }
            push_stack(value);
            pc++;
        } else if (compare_strings(line, "pop")) {
            pop_stack();
            pc++;
        } else if (compare_strings(line, "dup")) {
            int val = stack_top();
            push_stack(val);
            pc++;
        } else if (compare_strings(line, "add")) {
            int b = pop_stack();
            int a = pop_stack();
            push_stack(a + b);
            pc++;
        } else if (compare_strings(line, "sub")) {
            int a = pop_stack();
            int b = pop_stack();
            push_stack(a - b);
            pc++;
        } else if (starts_with(line, "biz ")) {
            int target = 0;
            char* ptr = line + 4;
            while (*ptr >= '0' && *ptr <= '9') {
                target = target * 10 + (*ptr - '0');
                ptr++;
            }
            if (stack_top() == 0) {
                int idx = find_line_index(target);
                if (idx >= 0) {
                    pc = idx;
                } else {
                    print_string("\nError: line not found\n");
                    break;
                }
            } else {
                pc++;
            }
        } else if (starts_with(line, "binz ")) {
            int target = 0;
            char* ptr = line + 5;
            while (*ptr >= '0' && *ptr <= '9') {
                target = target * 10 + (*ptr - '0');
                ptr++;
            }
            if (stack_top() != 0) {
                int idx = find_line_index(target);
                if (idx >= 0) {
                    pc = idx;
                } else {
                    print_string("\nError: line not found\n");
                    break;
                }
            } else {
                pc++;
            }
        } else if (compare_strings(line, "in")) {
            int input = read_number();
            push_stack(input);
            pc++;
        } else if (starts_with(line, "jmp ")) {
            int target = 0;
            char* ptr = line + 4;
            while (*ptr >= '0' && *ptr <= '9') {
                target = target * 10 + (*ptr - '0');
                ptr++;
            }
            int target_index = find_line_index(target);
            if (target_index >= 0) {
                pc = target_index;
            } else {
                print_string("\nError: line not found\n");
                break;
            }
        } else if (compare_strings(line, "end")) {
            break;
        } else {
            print_string("\nUnknown: ");
            print_string(line);
            break;
        }
    }
}


void start_basic_repl() {
    print_string("\nWelcome to EG-Basic REPL!\n");
    print_string("Type 'help' for a list of commands.\n");

    clear_program();

    char key[2] = {0};
    char line[128];

    while (!returnToCMD) {
        print_string("\nbasic> ");
        key[0] = 0;
        int len = 0;
        line[0] = 0;

        while ((key[0] != '\n') && (len < 127)) {
            char* pressed = get_keypress();
            key[0] = pressed[0];
            key[1] = pressed[1];

            char character = key[0];

            if (character == '\b' && len > 0 && cursor_pos > 0) {
                len--;
                line[len] = 0;
                cursor_pos--;
                int index = cursor_pos * 2;
                video_memory[index] = ' ';
                video_memory[index + 1] = color;
                update_cursor();
            } else if (character != '\n') {
                line[len++] = character;
                char characterToPrint[2] = {character, 0};
                print_string(characterToPrint);
            }
        }

        line[len] = 0;

        if (compare_strings(line, "exit")) {
            returnToCMD = 1;
        } else if (compare_strings(line, "list")) {
            list_program();
        } else if (compare_strings(line, "run")) {
            run_program();
        } else if (compare_strings(line, "new")) {
            clear_program();
        } else if (line[0] >= '0' && line[0] <= '9') {
            int number = 0;
            int pos = 0;
            while (line[pos] >= '0' && line[pos] <= '9') {
                number = number * 10 + (line[pos] - '0');
                pos++;
            }
            while (line[pos] == ' ') pos++;
            add_line(number, line + pos);
        } else {
            print_string("\nUnknown Command");
        }
    }
}
