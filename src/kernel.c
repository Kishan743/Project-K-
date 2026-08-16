#include <stdint.h>

void kernel_main(void)
{
    volatile uint16_t* video_memory = (uint16_t*)0xB8000;

    const char* message = "PROJECT K - KERNEL K v0.1 - BOOT SUCCESS";

    for (uint32_t i = 0; message[i] != '\0'; i++)
    {
        video_memory[i] = (uint16_t)message[i] | (uint16_t)0x0700;
    }

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
