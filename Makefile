CC = gcc
AS = nasm
LD = ld

CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -g -Wall -Wextra
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

KERNEL = build/kernel.bin
ISO = project-k.iso

.PHONY: all iso run debug clean

all: iso

build/boot.o: boot/boot.asm
	mkdir -p build
	$(AS) $(ASFLAGS) boot/boot.asm -o build/boot.o

build/kernel.o: src/kernel.c
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel.c -o build/kernel.o

build/terminal.o: src/drivers/terminal.c src/drivers/terminal.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/drivers/terminal.c -o build/terminal.o

$(KERNEL): build/boot.o build/kernel.o build/terminal.o linker.ld
	$(LD) $(LDFLAGS) -o $(KERNEL) build/boot.o build/kernel.o build/terminal.o

iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/kernel.bin
	printf 'set timeout=0\nset default=0\n\nmenuentry "Project K" {\n    multiboot /boot/kernel.bin\n    boot\n}\n' > iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso

run: iso
	qemu-system-x86_64 -boot d -cdrom $(ISO)

debug: iso
	qemu-system-i386 -boot d -cdrom $(ISO) -S -gdb tcp::1234

clean:
	rm -rf build iso $(ISO)
