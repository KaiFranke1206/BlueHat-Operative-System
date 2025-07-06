all: clean
	nasm -f elf32 kernel_entry.asm -o kernel_entry.o
	i386-elf-gcc -m32 -ffreestanding -fno-stack-protector -nostdlib -c kernel.c -o kernel.o
	i386-elf-gcc -m32 -ffreestanding -fno-stack-protector -nostdlib -c util.c -o util.o
	i386-elf-gcc -m32 -ffreestanding -fno-stack-protector -nostdlib -c basic.c -o basic.o
	i386-elf-gcc -m32 -ffreestanding -fno-stack-protector -nostdlib -c editor.c -o editor.o
	i386-elf-gcc -m32 -ffreestanding -fno-stack-protector -nostdlib -c boot.c -o bootsim.o
	ld -m elf_i386 -T link.ld -o kernel.bin kernel_entry.o kernel.o util.o basic.o editor.o bootsim.o

	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o egterm.iso iso/

clean:
	rm -rf *.o *.iso *.bin iso

run: all
	qemu-system-i386 -cdrom egterm.iso
