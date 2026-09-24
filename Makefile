CC = gcc
AS = nasm
LD = ld

CFLAGS = -Isrc -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -g -Wall -Wextra
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

KERNEL = build/kernel.bin
ISO = project-k.iso

.PHONY: all iso run debug clean

all: iso

build/boot.o: boot/boot.asm
	mkdir -p build
	$(AS) $(ASFLAGS) boot/boot.asm -o build/boot.o

build/kernel.o: src/kernel.c src/kernel/multiboot.h src/kernel/pmm.h src/kernel/paging.h src/kernel/heap.h src/kernel/task.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel.c -o build/kernel.o

build/terminal.o: src/drivers/terminal.c src/drivers/terminal.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/drivers/terminal.c -o build/terminal.o

build/framebuffer.o: src/drivers/framebuffer.c src/drivers/framebuffer.h src/kernel/multiboot.h src/kernel/paging.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/drivers/framebuffer.c -o build/framebuffer.o

build/gdt.o: src/kernel/gdt.c src/kernel/gdt.h src/kernel/tss.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/gdt.c -o build/gdt.o

build/tss.o: src/kernel/tss.c src/kernel/tss.h src/kernel/gdt.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/tss.c -o build/tss.o

build/gdt_flush.o: src/kernel/gdt_flush.asm
	mkdir -p build
	$(AS) $(ASFLAGS) src/kernel/gdt_flush.asm -o build/gdt_flush.o

build/pic.o: src/kernel/pic.c src/kernel/pic.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/pic.c -o build/pic.o

build/pit.o: src/kernel/pit.c src/kernel/pit.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/pit.c -o build/pit.o

build/timer.o: src/kernel/timer.c src/kernel/timer.h src/kernel/pit.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/timer.c -o build/timer.o

build/keyboard.o: src/kernel/keyboard.c src/kernel/keyboard.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/keyboard.c -o build/keyboard.o

build/console.o: src/kernel/console.c src/kernel/console.h src/kernel/keyboard.h src/kernel/timer.h src/kernel/task.h src/drivers/terminal.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/console.c -o build/console.o

build/idt.o: src/kernel/idt.c src/kernel/idt.h src/kernel/task.h src/kernel/pic.h src/kernel/timer.h src/kernel/keyboard.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/idt.c -o build/idt.o

build/idt_flush.o: src/kernel/idt_flush.asm
	mkdir -p build
	$(AS) $(ASFLAGS) src/kernel/idt_flush.asm -o build/idt_flush.o

build/isr.o: src/kernel/isr.asm
	mkdir -p build
	$(AS) $(ASFLAGS) src/kernel/isr.asm -o build/isr.o

build/heap.o: src/kernel/heap.c src/kernel/heap.h src/kernel/pmm.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/heap.c -o build/heap.o

build/pmm.o: src/kernel/pmm.c src/kernel/pmm.h src/kernel/multiboot.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/pmm.c -o build/pmm.o

build/paging.o: src/kernel/paging.c src/kernel/paging.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/paging.c -o build/paging.o

build/task.o: src/kernel/task.c src/kernel/task.h src/kernel/heap.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/kernel/task.c -o build/task.o

$(KERNEL): build/boot.o build/kernel.o build/terminal.o build/gdt.o build/tss.o build/gdt_flush.o build/pic.o build/pit.o build/timer.o build/keyboard.o build/console.o build/pmm.o build/heap.o build/idt.o build/idt_flush.o build/isr.o build/paging.o build/task.o build/framebuffer.o linker.ld
	mkdir -p build
	$(LD) $(LDFLAGS) -o $(KERNEL) build/boot.o build/kernel.o build/terminal.o build/gdt.o build/tss.o build/gdt_flush.o build/pic.o build/pit.o build/timer.o build/keyboard.o build/console.o build/pmm.o build/heap.o build/idt.o build/idt_flush.o build/isr.o build/paging.o build/task.o build/framebuffer.o

iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/kernel.bin
	printf 'set timeout=0\nset default=0\n\nmenuentry "Project K" {\n    multiboot2 /boot/kernel.bin\n    boot\n}\n' > iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso

run: iso
	qemu-system-x86_64 -boot d -cdrom $(ISO)

debug: iso
	qemu-system-i386 -boot d -cdrom $(ISO) -S -gdb tcp::1234

clean:
	rm -rf build iso $(ISO)
