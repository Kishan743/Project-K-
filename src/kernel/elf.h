#ifndef PROJECT_K_ELF_H
#define PROJECT_K_ELF_H

#include <stdint.h>
#include "paging.h"

#define ELF32_IDENT_SIZE 16

#define ELF_MAGIC0 0x7F
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'

#define ELF_CLASS_32 1
#define ELF_DATA_LSB 1
#define ELF_VERSION_CURRENT 1

#define ELF_TYPE_EXEC 2
#define ELF_MACHINE_386 3

#define ELF_PT_LOAD 1

#define ELF_PF_X 0x1
#define ELF_PF_W 0x2
#define ELF_PF_R 0x4

typedef struct
{
    uint8_t e_ident[ELF32_IDENT_SIZE];

    uint16_t e_type;
    uint16_t e_machine;

    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;

    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;

    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;

} elf32_header_t;

typedef struct
{
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;

} elf32_program_header_t;

typedef struct
{
    uint32_t entry;
    uint32_t lowest_address;
    uint32_t highest_address;

} elf_load_result_t;

int elf_validate(
    const void* image,
    uint32_t image_size
);

int elf_load(
    address_space_t* address_space,
    const void* image,
    uint32_t image_size,
    elf_load_result_t* result
);

#endif
