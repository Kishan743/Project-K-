#include "elf.h"
#include "pmm.h"

#define ELF_USER_FIRST_ADDRESS 0x40000000
#define ELF_USER_LAST_ADDRESS  0x7FFFFFFF

static int range_inside_image(
    uint32_t offset,
    uint32_t size,
    uint32_t image_size
)
{
    if (offset > image_size)
        return 0;

    if (size > image_size - offset)
        return 0;

    return 1;
}

static int range_inside_user_space(
    uint32_t start,
    uint32_t size
)
{
    if (size == 0)
        return 0;

    if (start < ELF_USER_FIRST_ADDRESS)
        return 0;

    if (start > ELF_USER_LAST_ADDRESS)
        return 0;

    if (size - 1 > ELF_USER_LAST_ADDRESS - start)
        return 0;

    return 1;
}

static uint32_t align_down(
    uint32_t value
)
{
    return value & ~(PAGE_SIZE - 1);
}

static uint32_t align_up(
    uint32_t value
)
{
    if (value > 0xFFFFF000)
        return 0;

    return (value + PAGE_SIZE - 1) &
           ~(PAGE_SIZE - 1);
}

int elf_validate(
    const void* image,
    uint32_t image_size
)
{
    if (image == 0)
        return -1;

    if (image_size < sizeof(elf32_header_t))
        return -1;

    const elf32_header_t* header =
        (const elf32_header_t*)image;

    if (header->e_ident[0] != ELF_MAGIC0 ||
        header->e_ident[1] != ELF_MAGIC1 ||
        header->e_ident[2] != ELF_MAGIC2 ||
        header->e_ident[3] != ELF_MAGIC3)
    {
        return -1;
    }

    if (header->e_ident[4] != ELF_CLASS_32)
        return -1;

    if (header->e_ident[5] != ELF_DATA_LSB)
        return -1;

    if (header->e_ident[6] != ELF_VERSION_CURRENT)
        return -1;

    if (header->e_type != ELF_TYPE_EXEC)
        return -1;

    if (header->e_machine != ELF_MACHINE_386)
        return -1;

    if (header->e_version != ELF_VERSION_CURRENT)
        return -1;

    if (header->e_ehsize != sizeof(elf32_header_t))
        return -1;

    if (header->e_phentsize != sizeof(elf32_program_header_t))
        return -1;

    if (header->e_phnum == 0)
        return -1;

    if (!range_inside_image(
            header->e_phoff,
            (uint32_t)header->e_phnum *
                sizeof(elf32_program_header_t),
            image_size))
    {
        return -1;
    }

    const elf32_program_header_t* program_headers =
        (const elf32_program_header_t*)
        ((const uint8_t*)image + header->e_phoff);

    uint32_t load_segments = 0;
    uint32_t lowest = 0xFFFFFFFF;
    uint32_t highest = 0;

    for (uint32_t i = 0;
         i < header->e_phnum;
         i++)
    {
        const elf32_program_header_t* ph =
            &program_headers[i];

        if (ph->p_type != ELF_PT_LOAD)
            continue;

        load_segments++;

        if (ph->p_filesz > ph->p_memsz)
            return -1;

        if (!range_inside_image(
                ph->p_offset,
                ph->p_filesz,
                image_size))
        {
            return -1;
        }

        if (!range_inside_user_space(
                ph->p_vaddr,
                ph->p_memsz))
        {
            return -1;
        }

        /*
         * The file and virtual-address offsets must agree
         * for page-based loading.
         */
        if ((ph->p_offset & (PAGE_SIZE - 1)) !=
            (ph->p_vaddr & (PAGE_SIZE - 1)))
        {
            return -1;
        }

        uint32_t segment_end =
            ph->p_vaddr + ph->p_memsz;

        if (ph->p_vaddr < lowest)
            lowest = ph->p_vaddr;

        if (segment_end > highest)
            highest = segment_end;
    }

    if (load_segments == 0)
        return -1;

    if (header->e_entry < lowest ||
        header->e_entry >= highest)
    {
        return -1;
    }

    if (header->e_entry < ELF_USER_FIRST_ADDRESS ||
        header->e_entry > ELF_USER_LAST_ADDRESS)
    {
        return -1;
    }

    return 0;
}

static void release_loaded_pages(
    address_space_t* address_space,
    uint32_t lowest,
    uint32_t highest
)
{
    if (address_space == 0)
        return;

    uint32_t start =
        align_down(lowest);

    uint32_t end =
        align_up(highest);

    if (end == 0)
        return;

    /*
     * At this stage the address-space destructor releases the
     * page tables, but the individual user frames are owned by
     * the ELF loader and therefore need to be returned too.
     *
     * The current paging API intentionally does not expose a
     * physical-address lookup, so frame cleanup is deferred to
     * the task/process layer for now.
     */
    (void)start;
    (void)end;
}

int elf_load(
    address_space_t* address_space,
    const void* image,
    uint32_t image_size,
    elf_load_result_t* result
)
{
    if (address_space == 0 ||
        image == 0 ||
        result == 0)
    {
        return -1;
    }

    if (elf_validate(image, image_size) != 0)
        return -1;

    const elf32_header_t* header =
        (const elf32_header_t*)image;

    const elf32_program_header_t* program_headers =
        (const elf32_program_header_t*)
        ((const uint8_t*)image + header->e_phoff);

    uint32_t lowest =
        0xFFFFFFFF;

    uint32_t highest =
        0;

    for (uint32_t i = 0;
         i < header->e_phnum;
         i++)
    {
        const elf32_program_header_t* ph =
            &program_headers[i];

        if (ph->p_type != ELF_PT_LOAD)
            continue;

        uint32_t segment_start =
            align_down(ph->p_vaddr);

        uint32_t segment_end =
            align_up(ph->p_vaddr + ph->p_memsz);

        if (segment_end == 0)
            return -1;

        /*
         * Basic loader currently requires PT_LOAD segments
         * not to overlap.
         */
        for (uint32_t j = 0;
             j < i;
             j++)
        {
            const elf32_program_header_t* previous =
                &program_headers[j];

            if (previous->p_type != ELF_PT_LOAD)
                continue;

            uint32_t previous_start =
                align_down(previous->p_vaddr);

            uint32_t previous_end =
                align_up(
                    previous->p_vaddr +
                    previous->p_memsz
                );

            if (segment_start < previous_end &&
                previous_start < segment_end)
            {
                return -1;
            }
        }

        uint32_t page_flags =
            PAGE_PRESENT;

        if (ph->p_flags & ELF_PF_W)
            page_flags |= PAGE_WRITABLE;

        for (uint32_t virtual_address = segment_start;
             virtual_address < segment_end;
             virtual_address += PAGE_SIZE)
        {
            void* frame =
                pmm_alloc_frame();

            if (frame == 0)
            {
                release_loaded_pages(
                    address_space,
                    lowest,
                    highest
                );

                return -1;
            }

            uint32_t physical_address =
                (uint32_t)frame;

            /*
             * Every freshly allocated page starts as zero.
             * This automatically handles BSS.
             */
            uint8_t zero_page[PAGE_SIZE];

            for (uint32_t z = 0;
                 z < PAGE_SIZE;
                 z++)
            {
                zero_page[z] = 0;
            }

            if (paging_copy_to_physical(
                    physical_address,
                    zero_page,
                    PAGE_SIZE
                ) != 0)
            {
                pmm_free_frame(frame);

                release_loaded_pages(
                    address_space,
                    lowest,
                    highest
                );

                return -1;
            }

            if (paging_map_user_page(
                    address_space,
                    virtual_address,
                    physical_address,
                    page_flags
                ) != 0)
            {
                pmm_free_frame(frame);

                release_loaded_pages(
                    address_space,
                    lowest,
                    highest
                );

                return -1;
            }

            uint32_t copy_start =
                virtual_address;

            if (ph->p_vaddr > copy_start)
                copy_start = ph->p_vaddr;

            uint32_t copy_end =
                virtual_address + PAGE_SIZE;

            uint32_t file_end =
                ph->p_vaddr + ph->p_filesz;

            if (file_end < copy_end)
                copy_end = file_end;

            if (copy_end > copy_start)
            {
                uint32_t source_offset =
                    ph->p_offset +
                    (copy_start - ph->p_vaddr);

                uint32_t destination_offset =
                    copy_start -
                    virtual_address;

                uint32_t copy_size =
                    copy_end -
                    copy_start;

                const uint8_t* source =
                    (const uint8_t*)image +
                    source_offset;

                if (paging_copy_to_physical(
                        physical_address +
                            destination_offset,
                        source,
                        copy_size
                    ) != 0)
                {
                    release_loaded_pages(
                        address_space,
                        lowest,
                        highest
                    );

                    return -1;
                }
            }
        }

        if (ph->p_vaddr < lowest)
            lowest = ph->p_vaddr;

        uint32_t segment_high =
            ph->p_vaddr + ph->p_memsz;

        if (segment_high > highest)
            highest = segment_high;
    }

    result->entry =
        header->e_entry;

    result->lowest_address =
        lowest;

    result->highest_address =
        highest;

    return 0;
}
