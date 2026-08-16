AS = nasm
CC = gcc
LD = ld

ASFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld

KERNEL = build/kernel.bin

all: iso

build/boot.o: boot/boot.asm
	mkdir -p build
	$(AS) $(ASFLAGS) $< -o $@

build/kernel.o: src/kernel.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): build/boot.o build/kernel.o linker.ld
	$(LD) $(LDFLAGS) -o $@ build/boot.o build/kernel.o

iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/kernel.bin
	printf 'set timeout=0\nset default=0\n\nmenuentry "Project K" {\n    multiboot /boot/kernel.bin\n    boot\n}\n' > iso/boot/grub/grub.cfg
	grub-mkrescue -o project-k.iso iso

run: iso
	qemu-system-x86_64 -cdrom project-k.iso

clean:
	rm -rf build iso project-k.iso

.PHONY: all iso run clean
