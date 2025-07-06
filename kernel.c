#include "util.h"
#include "basic.h"
#include "editor.h"
#include "boot.h"
#include "multiboot.h"

void executeCommand();

#define VIDEO_MEMORY (char*)0xB8000
#define WIDTH 80
#define HEIGHT 25
#define KEYBOARD_PORT 0x60
#define MAX_LINE_LENGTH 80

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71
#define CMOS_SEC  0x00
#define CMOS_MIN  0x02
#define CMOS_HOUR 0x04
#define CMOS_DAY  0x07
#define CMOS_MONTH 0x08
#define CMOS_YEAR 0x09

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

extern char _kernel_start;
extern char _kernel_end;

char input_buffer[128] = {0};
unsigned char buffer_index = 0;

char command_buffer[64] = {0};
char argument_buffer[64] = {0};

unsigned int mem_lower_kb = 0;
unsigned int mem_upper_kb = 0;
multiboot_info_t* boot_info = 0;

char scancode_to_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','ß','´','\b',
    '\t','q','w','e','r','t','z','u','i','o','p','ü','+', '\n',
    0,  'a','s','d','f','g','h','j','k','l','ö','ä','#', 0,
    '^','y','x','c','v','b','n','m',',','.','-', 0,
    '*', 0, ' ', 0,
};

typedef struct {
    char name[16];
    char content[FILE_HEIGHT][FILE_WIDTH + 1];
} TextFile;

TextFile files[MAX_FILES] = {0};

typedef struct {
    int line_number;
    char line[MAX_LINE_LENGTH];
} BasicLine;

char* get_keypress() {
    static char result[2];
    static int shift_pressed = 0;

    while (1) {
        unsigned char scancode = inb(KEYBOARD_PORT);

        // Shift press
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
            continue;
        }

        // Shift release
        if (scancode == 0xAA || scancode == 0xB6) {
            shift_pressed = 0;
            continue;
        }

        if (scancode == 0xE0) {
            // Extended key prefix
            unsigned char extcode = inb(KEYBOARD_PORT);

            while (1) {
                extcode = inb(KEYBOARD_PORT);
                if (extcode < 0x80) break; // Ignore key releases
            }
            result[0] = 0;       // Signal extended
            result[1] = extcode; // Actual arrow code
            return result;
        }


        if (scancode < 128) {
            char ascii = scancode_to_ascii[scancode];

            if (ascii) {
                if (shift_pressed) {
                    // Convert lowercase letters to uppercase (QWERTZ aware)
                    if (ascii >= 'a' && ascii <= 'z') {
                        ascii = ascii - ('a' - 'A');
                    }

                    // German shift-aware symbol replacements
                    switch (ascii) {
                        case '1': ascii = '!'; break;
                        case '2': ascii = '"'; break;
                        case '3': ascii = '§'; break;
                        case '4': ascii = '$'; break;
                        case '5': ascii = '%'; break;
                        case '6': ascii = '&'; break;
                        case '7': ascii = '/'; break;
                        case '8': ascii = '('; break;
                        case '9': ascii = ')'; break;
                        case '0': ascii = '='; break;
                        case 'ß': ascii = '?'; break;
                        case '´': ascii = '`'; break;
                        case '+': ascii = '*'; break;
                        case '#': ascii = '\''; break;
                        case '^': ascii = '°'; break;
                        case ',': ascii = ';'; break;
                        case '.': ascii = ':'; break;
                        case '-': ascii = '_'; break;
                        case 'ü': ascii = 'Ü'; break;
                        case 'ö': ascii = 'Ö'; break;
                        case 'ä': ascii = 'Ä'; break;
                    }
                }

                result[0] = ascii;
                result[1] = scancode;

                // Wait for key release (optional)
                while (inb(KEYBOARD_PORT) != (scancode | 0x80));
                return result;
            }
        }
    }
}

unsigned char read_rtc_register(unsigned char reg) {
    outb(0x70, reg);
    return inb(0x71);
}

unsigned char bcd_to_binary(unsigned char bcd) {
    return ((bcd / 16) * 10) + (bcd & 0x0F);
}

void wait_release(unsigned char pressed_scancode) {
    unsigned char release_code = pressed_scancode | 0x80;
    while (1) {
        if (inb(KEYBOARD_PORT) == release_code) break;
    }
}

void parse_buffer() {
    buffer_index = 0;
    int arg_index = 0;
    int cmd_index = 0;
    int command0argument1 = 0;

    char current_character = input_buffer[buffer_index];
    while (current_character != 0) {
        if (current_character == ' ' && command0argument1 == 0) {
            command0argument1 = 1;
        } else {
            if (command0argument1) {
                if (arg_index < sizeof(argument_buffer) - 1)
                    argument_buffer[arg_index++] = current_character;
            } else {
                if (cmd_index < sizeof(command_buffer) - 1)
                    command_buffer[cmd_index++] = current_character;
            }
        }
        buffer_index++;
        current_character = input_buffer[buffer_index];
    }

    command_buffer[cmd_index] = 0;
    argument_buffer[arg_index] = 0;
}

void wait_for_kb_controller() {
    while (inb(0x64) & 0x02);
}

void command_reboot() {
    print_string("\nRebooting...\n");
    __asm__ __volatile__("cli");
    wait_for_kb_controller();
    outb(0x64, 0xFE);
    while (1) __asm__ __volatile__("hlt");
}

void command_rtc_time() {
    unsigned char sec = bcd_to_binary(read_rtc_register(CMOS_SEC));
    unsigned char min = bcd_to_binary(read_rtc_register(CMOS_MIN));
    unsigned char hour = bcd_to_binary(read_rtc_register(CMOS_HOUR));
    unsigned char day = bcd_to_binary(read_rtc_register(CMOS_DAY));
    unsigned char month = bcd_to_binary(read_rtc_register(CMOS_MONTH));
    unsigned char year = bcd_to_binary(read_rtc_register(CMOS_YEAR));

    char buffer[64];
    print_string("\nCurrent RTC time: ");

    int_to_string(hour + 2, buffer); print_string(buffer); print_string(":");
    int_to_string(min, buffer); print_string(buffer); print_string(":");
    int_to_string(sec, buffer); print_string(buffer);

    print_string("  ");

    int_to_string(day, buffer); print_string(buffer); print_string(".");
    int_to_string(month, buffer); print_string(buffer); print_string(".");
    int_to_string(2000 + year, buffer); print_string(buffer);
}

void command_benchmark() {
    print_string("\nBenchmarking CPU for 5 seconds...\n");

    // Wait for next second to align start time
    unsigned char start_sec = bcd_to_binary(read_rtc_register(CMOS_SEC));
    unsigned char current_sec;
    do {
        current_sec = bcd_to_binary(read_rtc_register(CMOS_SEC));
    } while (current_sec == start_sec);

    // Start benchmarking
    unsigned int iterations = 0;
    start_sec = bcd_to_binary(read_rtc_register(CMOS_SEC));

    do {
        iterations++;
        current_sec = bcd_to_binary(read_rtc_register(CMOS_SEC));
    } while (((current_sec - start_sec) & 0x3F) < 5); // handles wrap-around

    char buffer[64];
    print_string("Done.\nIterations: ");
    int_to_string(iterations, buffer);
    print_string(buffer);
    print_string("\n");

    print_string("Benchmark Index: ");
    int_to_string(iterations / 100000, buffer); // scaled for readability
    print_string(buffer);
    print_string(" (x10^5 iters/5s)\n");
}


void command_poke() {
    char* addr_str = argument_buffer;
    char* value_str = argument_buffer;

    while (*value_str && *value_str != ' ') value_str++;
    if (*value_str == 0) {
        print_string("\nUsage: poke <addr> <value>");
        return;
    }

    *value_str = 0;
    value_str++;

    unsigned int addr = hex_to_uint(addr_str);
    unsigned char value = (unsigned char)hex_to_uint(value_str);
    unsigned char* ptr = (unsigned char*)addr;
    *ptr = value;

    print_string("\n[OK] Wrote ");
    char out[16];
    out[0] = "0123456789ABCDEF"[(value >> 4) & 0xF];
    out[1] = "0123456789ABCDEF"[value & 0xF];
    out[2] = 0;
    print_string(out);

}

void command_peek() {
    if (argument_buffer[0] == 0) {
        print_string("\nUsage: peek <addr>");
        return;
    }

    unsigned int addr = hex_to_uint(argument_buffer);
    unsigned char value = *((unsigned char*)addr);

    print_string("\nValue at address: 0x");
    char out[5];
    out[0] = "0123456789ABCDEF"[(value >> 4) & 0xF];
    out[1] = "0123456789ABCDEF"[value & 0xF];
    out[2] = 0;
    print_string(out);
}

void command_echo() {
    print_string("\n");
    print_string(argument_buffer);
}

void command_ls() {
    print_string("\nFiles:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].name[0]) {
            print_string("- ");
            print_string(files[i].name);
            print_string("\n");
        }
    }
}

void command_cat() {
    for (int i = 0; i < MAX_FILES; i++) {
        if (compare_strings(files[i].name, argument_buffer)) {
            print_string("\n");
            for (int y = 0; y < FILE_HEIGHT; y++) {
                print_string(files[i].content[y]);
            }
            return;
        }
    }
    print_string("\nFile not found.");
}

void command_clear() {
    cursor_pos = 0;
    for (int i = 0; i <= WIDTH * HEIGHT; i++) {
        video_memory[i * 2] = 0;
        video_memory[i * 2 + 1] = color;
    }
}

extern multiboot_info_t* boot_info;

void command_info() {
    char buffer[64];

    print_string("\nSystem Info:");

    // Kernel Version
    print_string("\n- Kernel Version: ");
    print_string(kernel_version);

    // Kernel Size
    unsigned int size = (unsigned int)(&_kernel_end) - (unsigned int)(&_kernel_start);
    print_string("\n- Kernel Size: ");
    int_to_string(size, buffer); print_string(buffer); print_string(" bytes");

    // CPU Brand
    print_string("\n- CPU Brand: ");
    print_string(cpu_brand);


    // Stack Pointer
    unsigned int esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    print_string("\n- Stack Pointer: ");
    int_to_string(esp, buffer); print_string(buffer);

    // Screen Size
    print_string("\n- Screen Size: ");
    int_to_string(WIDTH, buffer); print_string(buffer); print_string("x");
    int_to_string(HEIGHT, buffer); print_string(buffer);

    // Build Time
    print_string("\n- Build Time: ");
    print_string(build_date); print_string(" "); print_string(build_time);

    // Multiboot Info
    if (!boot_info) {
        print_string("\n(No multiboot info available)");
        return;
    }

    // RAM
    if (boot_info->flags & 0x1) {
        print_string("\n- RAM Lower: ");
        int_to_string(boot_info->mem_lower, buffer); print_string(buffer); print_string(" KB");

        print_string("\n- RAM Upper: ");
        int_to_string(boot_info->mem_upper, buffer); print_string(buffer); print_string(" KB");

        print_string("\n- Total RAM: ");
        int_to_string((boot_info->mem_lower + boot_info->mem_upper) / 1024, buffer);
        print_string(buffer); print_string(" MB");
    }

    // Boot device
    if (boot_info->flags & 0x2) {
        print_string("\n- Boot Device: 0x");
        int_to_string(boot_info->boot_device, buffer); print_string(buffer);
    }

    // Bootloader Command Line
    if (boot_info->flags & 0x4) {
        print_string("\n- Boot Command Line: ");
        print_string((char*)boot_info->cmdline);
    }

    // Modules
    if (boot_info->flags & 0x8) {
        print_string("\n- Module Count: ");
        int_to_string(boot_info->mods_count, buffer); print_string(buffer);

        print_string("\n- Module Addr: 0x");
        int_to_string(boot_info->mods_addr, buffer); print_string(buffer);
    }

    // Symbol table (skipped unless using ELF/a.out)

    // Memory Map
    if (boot_info->flags & 0x40) {
        print_string("\n- BIOS Memory Map:\n");
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)boot_info->mmap_addr;

        while ((unsigned int)mmap < boot_info->mmap_addr + boot_info->mmap_length) {
            print_string("  - Base: 0x");
            int_to_string(mmap->addr_high, buffer); print_string(buffer);  // high first for 64-bit
            print_string("");  // spacing
            int_to_string(mmap->addr_low, buffer); print_string(buffer);

            print_string(", Length: 0x");
            int_to_string(mmap->len_high, buffer); print_string(buffer);
            print_string("");  // spacing
            int_to_string(mmap->len_low, buffer); print_string(buffer);

            print_string(", Type: ");
            int_to_string(mmap->type, buffer); print_string(buffer);
            print_string("\n");

            mmap = (multiboot_memory_map_t*)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
        }
    }
}

void command_color() {
    if (argument_buffer[0] == 0) {
        print_string("\nUsage: color <hex>");
        return;
    }

    change_color(argument_buffer);
    print_string("\n[OK] Color changed to 0x");
    char buf[4];
    unsigned char val = color;
    buf[0] = "0123456789ABCDEF"[(val >> 4) & 0xF];
    buf[1] = "0123456789ABCDEF"[val & 0xF];
    buf[2] = 0;
    print_string(buf);
}

void command_shutdown() {
    outw(0x604, 0x2000);
}

void command_run() {
    if (argument_buffer[0] == 0) {
        print_string("\nUsage: run <filename>");
        return;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (compare_strings(files[i].name, argument_buffer)) {
            print_string("\nRunning script: ");
            print_string(argument_buffer);
            print_string("\n");

            for (int y = 0; y < FILE_HEIGHT; y++) {
                const char* line = files[i].content[y];
                int skip = 1;
                for (int c = 0; line[c]; c++) {
                    if (line[c] != ' ' && line[c] != '\t' && line[c] != '\r' && line[c] != '\n') {
                        skip = 0;
                        break;
                    }
                }
                if (skip) continue;

                copy_string(input_buffer, line);
                parse_buffer();
                executeCommand();
            }
            return;
        }
    }

    print_string("\nFile not found.");
}

void executeCommand() {
    if (compare_strings(command_buffer, "echo")) {
        command_echo();
    } else if (compare_strings(command_buffer, "help")) {
        print_string("\nAvailable commands:\n");
        print_string("- echo <text>\n");
        print_string("- clear\n");
        print_string("- help\n");
        print_string("- poke <address> <value>\n");
        print_string("- peek <address>\n");
        print_string("- editor <file name>\n");
        print_string("- ls\n");
        print_string("- cat <file name>\n");
        print_string("- eg-basic\n");
        print_string("- rtc-time\n");
        print_string("- info\n");
        print_string("- color <hex>         (e.g. 0F = black on white)\n");
        print_string("- reboot");
    } else if (compare_strings(command_buffer, "poke")) {
        command_poke();
    } else if (compare_strings(command_buffer, "peek")) {
        command_peek();
    } else if (compare_strings(command_buffer, "reboot")) {
        command_reboot();
    } else if (compare_strings(command_buffer, "clear")){
        command_clear();
    } else if (compare_strings(command_buffer, "eg-basic")) {
        start_basic_repl();
    } else if (compare_strings(command_buffer,"cat")) {
        command_cat();
    } else if (compare_strings(command_buffer, "ls")) {
        command_ls();
    } else if (compare_strings(command_buffer, "editor")) {
       if (argument_buffer[0]) start_editor(argument_buffer);
       else print_string("Usage: editor <filename>");
    } else if (compare_strings(command_buffer, "rtc-time")) {
        command_rtc_time();
    } else if (compare_strings(command_buffer, "info")) {
        command_info();
    } else if (compare_strings(command_buffer, "benchmark")) {
        command_benchmark();
    } else if (compare_strings(command_buffer, "color")) {
        command_color();
    } else if (compare_strings(command_buffer, "run")) {
        command_run();
    } else if (compare_strings(command_buffer, "shutdown")) {
        command_shutdown();
    } else {
        print_string("\nUnknown command: ");
        print_string(command_buffer);
    }
}

void kernel_main(unsigned int magic, unsigned int addr) {

    multiboot_info_t* mbi = (multiboot_info_t*)addr;

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        print_string("Invalid GRUB magic\n");
        return;
    }

    boot_info = (multiboot_info_t*)addr;

    if (mbi->flags & 1) {
        mem_lower_kb = mbi->mem_lower;
        mem_upper_kb = mbi->mem_upper;
    }

    if (mbi->flags & 1) {  // Bit 0: mem_* fields are valid
        unsigned int total_kb = mbi->mem_lower + mbi->mem_upper;
        unsigned int total_mb = total_kb / 1024;

        char buffer[32];
        print_string("Total RAM: ");
        int_to_string(total_mb, buffer);
        print_string(buffer);
        print_string(" MB\n");
    }

    get_cpu_brand();
    
    simulate_boot(mbi);

    while (1) {
        print_string("\n> ");
        char key[2] = {0};
        buffer_index = 0;
        input_buffer[0] = 0;

        while ((key[0] != '\n') && (buffer_index < 127)) {
            char* pressed = get_keypress();
            key[0] = pressed[0];
            key[1] = pressed[1];

            char character = key[0];

            if (character == '\b' && buffer_index > 0 && cursor_pos > 0) {
                buffer_index--;
                input_buffer[buffer_index] = 0;
                cursor_pos--;
                int index = cursor_pos * 2;
                video_memory[index] = ' ';
                video_memory[index + 1] = color;
                update_cursor();
            } else if (character != '\n') {
                input_buffer[buffer_index++] = character;
                char characterToPrint[2] = {character, 0};
                print_string(characterToPrint);
            }
        }

        input_buffer[buffer_index] = 0;
        parse_buffer();
        executeCommand();
    }
}
