#include "util.h"
#include "boot.h"
#include "multiboot.h"


void boot_delay() {
    for (volatile int i = 0; i < 100000000; i++);  // crude delay
}

void boot_count_up(unsigned int limit, const char* label) {
    char buffer[64];

    for (unsigned int i = 0; i <= limit; i += 8) {
        print_string("\r");  // Return to start of line
        print_string(label);
        int_to_string(i, buffer);
        print_string(buffer);
        print_string(" KB   "); // Padding to clear previous digits

        update_cursor();
        for (volatile int j = 0; j < 100000; j++);
    }

    newline();
}

void simulate_boot(multiboot_info_t* mbi) {
    char buffer[64];

    print_string("EG-Kernel Boot Loader\n");
    print_string("Build: ");
    print_string(build_date);
    print_string(" ");
    print_string(build_time);
    newline();

    boot_delay();

    print_string("\n[+] Initializing BIOS...\n");
    boot_delay();

    print_string("[+] Detecting CPU... ");
    get_cpu_brand();
    print_string(cpu_brand);
    newline();

    print_string("[+] Counting Memory...\n");
    unsigned int total_kb = 0;
    boot_count_up(32768, "  ");  // Fallback: fake 32 MB

    print_string("[+] Setting up video memory at 0xB8000...\n");
    boot_delay();

    print_string("[+] Loading system modules... ");
    boot_delay();
    print_string("done.\n");

    print_string("[+] Mounting root file system... ");
    boot_delay();
    print_string("done.\n");

    print_string("\nWelcome to EG-Term!\n");
    print_string("Type 'help' for a list of commands.\n");
}
