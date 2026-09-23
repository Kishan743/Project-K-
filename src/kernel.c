#include <stdint.h>

#include "drivers/terminal.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/pic.h"
#include "kernel/timer.h"
#include "kernel/keyboard.h"
#include "kernel/console.h"
#include "kernel/multiboot.h"
#include "kernel/pmm.h"
#include "kernel/paging.h"
#include "kernel/heap.h"
#include "kernel/task.h"
#include "drivers/framebuffer.h"

static void test_heap(void);
static void test_pmm(void);

static void print_uint(uint32_t value)
{
    char buffer[11];
    int i = 0;

    if (value == 0)
    {
        terminal_putchar('0');
        return;
    }

    while (value > 0)
    {
        buffer[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (i > 0)
    {
        terminal_putchar(buffer[--i]);
    }
}

static multiboot2_tag_framebuffer_t*
find_framebuffer(uint32_t info_address)
{
    multiboot2_info_t* info =
        (multiboot2_info_t*)info_address;

    uint8_t* current =
        (uint8_t*)info + 8;

    uint8_t* end =
        (uint8_t*)info + info->total_size;

    while (current < end)
    {
        multiboot2_tag_t* tag =
            (multiboot2_tag_t*)current;

        if (tag->type == MULTIBOOT2_TAG_FRAMEBUFFER)
        {
            return (multiboot2_tag_framebuffer_t*)tag;
        }

        if (tag->type == MULTIBOOT2_TAG_END)
        {
            break;
        }

        if (tag->size < 8)
        {
            break;
        }

        current += (tag->size + 7) & ~7u;
    }

    return 0;
}

static void show_multiboot_info(uint32_t magic,
                                uint32_t info_address)
{
    terminal_setcolor(
        VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_BLACK
    );

    terminal_write("Multiboot 2 magic: ");

    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        terminal_write("OK\n");
    }
    else
    {
        terminal_write("INVALID\n");
        return;
    }

    multiboot2_info_t* info =
        (multiboot2_info_t*)info_address;

    terminal_setcolor(
        VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_BLACK
    );

    terminal_write("Multiboot 2 information size: ");
    print_uint(info->total_size);
    terminal_write(" bytes\n");
}




static void task_demo_a(void* argument)
{
    (void)argument;

    while (1)
    {
        task_t* task =
            task_get_current();

        task->work_counter++;
    }
}

static void task_demo_b(void* argument)
{
    (void)argument;

    while (1)
    {
        task_t* task =
            task_get_current();

        task->work_counter++;
    }
}

static void test_tasks(void)
{
    terminal_setcolor(
        VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "\nPreemptive Task Scheduler\n"
    );

    terminal_setcolor(
        VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_BLACK
    );

    task_initialize();

    int task_a =
        task_create(task_demo_a, 0);

    int task_b =
        task_create(task_demo_b, 0);

    if (task_a < 0 || task_b < 0)
    {
        terminal_write(
            "Task creation FAILED.\n"
        );

        return;
    }

    terminal_write("Task A created: ");
    print_uint((uint32_t)task_a);
    terminal_putchar('\n');

    terminal_write("Task B created: ");
    print_uint((uint32_t)task_b);
    terminal_putchar('\n');

    terminal_write(
        "Preemptive scheduler ready.\n"
    );
}

void kernel_main(uint32_t multiboot_magic,
                 uint32_t multiboot_info)
{
    /*
     * Early initialization:
     * PMM -> paging -> framebuffer -> terminal.
     */
    pmm_initialize(multiboot_info);

    paging_initialize();

    int framebuffer_result =
        framebuffer_initialize(multiboot_info);

    terminal_initialize();

    /*
     * Kernel banner.
     */
    terminal_setcolor(
        VGA_COLOR_LIGHT_GREEN,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "PROJECT K - KERNEL K v0.7\n"
    );

    terminal_setcolor(
        VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "================================\n"
    );

    terminal_setcolor(
        VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_BLACK
    );

    if (framebuffer_result == 0)
    {
        terminal_write(
            "Graphical framebuffer terminal: OK\n"
        );
    }
    else
    {
        terminal_write(
            "Graphical framebuffer: FAILED\n"
        );
    }

    /*
     * Multiboot 2 information.
     */
    show_multiboot_info(
        multiboot_magic,
        multiboot_info
    );

    /*
     * Physical memory manager.
     */
    terminal_setcolor(
        VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "\nPhysical Memory Manager\n"
    );

    terminal_setcolor(
        VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_BLACK
    );

    terminal_write("Total frames: ");
    print_uint(pmm_get_total_frames());
    terminal_putchar('\n');

    terminal_write("Free frames: ");
    print_uint(pmm_get_free_frames());
    terminal_putchar('\n');

    test_pmm();

    /*
     * Framebuffer information.
     */
    multiboot2_tag_framebuffer_t* framebuffer =
        find_framebuffer(multiboot_info);

    terminal_setcolor(
        VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "\nFramebuffer Information\n"
    );

    terminal_setcolor(
        VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_BLACK
    );

    if (framebuffer != 0)
    {
        terminal_write(
            "Status: PRESENT\n"
        );

        terminal_write("Resolution: ");

        print_uint(
            framebuffer->framebuffer_width
        );

        terminal_putchar('x');

        print_uint(
            framebuffer->framebuffer_height
        );

        terminal_putchar('\n');

        terminal_write("BPP: ");

        print_uint(
            framebuffer->framebuffer_bpp
        );

        terminal_putchar('\n');

        terminal_write("Pitch: ");

        print_uint(
            framebuffer->framebuffer_pitch
        );

        terminal_putchar('\n');

        terminal_write("Address: ");

        terminal_write_hex(
            (uint32_t)
            framebuffer->framebuffer_addr
        );

        terminal_putchar('\n');

        terminal_write("Type: ");

        print_uint(
            framebuffer->framebuffer_type
        );

        terminal_putchar('\n');
    }
    else
    {
        terminal_write(
            "Status: NOT AVAILABLE\n"
        );
    }

    /*
     * Kernel heap.
     */
    terminal_setcolor(
        VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "\nKernel Heap\n"
    );

    heap_initialize();

    terminal_setcolor(
        VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "Heap initialized successfully.\n"
    );

    test_heap();

    /*
     * CPU and interrupt subsystem.
     */
    terminal_setcolor(
        VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "\nCPU and Interrupt Subsystems\n"
    );

    gdt_initialize();

    pic_initialize();
    idt_initialize();

    timer_initialize(100);
    keyboard_initialize();

    pic_clear_mask(0);
    pic_clear_mask(1);

    terminal_setcolor(
        VGA_COLOR_LIGHT_GREEN,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "GDT initialized successfully.\n"
    );

    terminal_write(
        "IDT initialized successfully.\n"
    );

    terminal_write(
        "Timer initialized at 100 Hz.\n"
    );

    terminal_write(
        "Keyboard IRQ1 initialized.\n"
    );

    terminal_write(
        "Keyboard input buffer initialized.\n"
    );

    /*
     * Console.
     */
    console_initialize();

    terminal_write(
        "Console initialized.\n"
    );

    test_tasks();

    /*
     * Final system status.
     */
    terminal_setcolor(
        VGA_COLOR_LIGHT_CYAN,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "\n================================\n"
    );

    terminal_write(
        "PROJECT K SYSTEM READY\n"
    );

    terminal_write(
        "================================\n"
    );

    terminal_setcolor(
        VGA_COLOR_LIGHT_GREY,
        VGA_COLOR_BLACK
    );

    terminal_write(
        "Memory, paging, heap, interrupts,\n"
    );

    terminal_write(
        "keyboard and console are active.\n"
    );

    terminal_write(
        "Waiting for input...\n\n"
    );

    /*
     * Enable hardware interrupts only after the complete
     * scheduler/task environment is ready.
     */
    __asm__ volatile ("sti");

    /*
     * Main kernel loop.
     */
    while (1)
    {
        __asm__ volatile ("hlt");

        console_process_input();
    }
}

static void test_heap(void)
{
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("Testing kernel heap...\n");

    /*
     * Allocate two blocks.
     */
    void* a = kmalloc(256);

    if (a == 0)
    {
        terminal_write("Heap allocation FAILED.\n");
        return;
    }

    terminal_write("Heap allocation: OK\n");

    void* b = kmalloc(256);

    if (b == 0)
    {
        terminal_write("Heap second allocation FAILED.\n");
        return;
    }

    /*
     * Free the first block.
     */
    kfree(a);

    /*
     * Allocate a smaller block.
     * The allocator should reuse the freed block
     * and split it.
     */
    void* c = kmalloc(64);

    if (c != a)
    {
        terminal_write("Heap split/reuse FAILED.\n");
        return;
    }

    terminal_write("Heap split/reuse: OK\n");

    /*
     * Free both adjacent regions.
     */
    kfree(b);
    kfree(c);

    /*
     * The allocator should coalesce the adjacent
     * free blocks into a larger block.
     */
    void* d = kmalloc(400);

    if (d != a)
    {
        terminal_write("Heap coalescing FAILED.\n");
        return;
    }

    terminal_write("Heap coalescing: OK\n");

    kfree(d);

    terminal_write("Heap free: OK\n");
}

static void test_pmm(void)
{
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    terminal_write("Testing Physical Memory Manager...\n");

    uint32_t free_before =
        pmm_get_free_frames();

    void* frames[16];

    /*
     * Allocate 16 physical frames.
     */
    for (int i = 0; i < 16; i++)
    {
        frames[i] = pmm_alloc_frame();

        if (frames[i] == 0)
        {
            terminal_write("PMM multi-allocation FAILED.\n");
            return;
        }
    }

    /*
     * Verify that exactly 16 frames were consumed.
     */
    uint32_t free_after_alloc =
        pmm_get_free_frames();

    if (free_after_alloc != free_before - 16)
    {
        terminal_write("PMM allocation count FAILED.\n");
        return;
    }

    /*
     * Verify that all allocated frames are unique.
     */
    for (int i = 0; i < 16; i++)
    {
        for (int j = i + 1; j < 16; j++)
        {
            if (frames[i] == frames[j])
            {
                terminal_write("PMM duplicate frame FAILED.\n");
                return;
            }
        }
    }

    terminal_write("PMM multi-allocation: OK\n");
    terminal_write("PMM frame uniqueness: OK\n");

    /*
     * Free all 16 frames.
     */
    for (int i = 0; i < 16; i++)
    {
        pmm_free_frame(frames[i]);
    }

    /*
     * Verify that the original count is restored.
     */
    uint32_t free_after_free =
        pmm_get_free_frames();

    if (free_after_free != free_before)
    {
        terminal_write("PMM multi-free count FAILED.\n");
        return;
    }

    terminal_write("PMM multi-free: OK\n");
}
