#ifndef PMM_H                                   // Check if PMM_H macro is not defined to prevent multiple inclusions
#define PMM_H                                   // Define PMM_H macro to guard this header file

#include "types.h"                              // Include fixed-width integer and boolean type definitions

#define PMM_BLOCK_SIZE 4096                     // Each physical page frame (block) is 4096 bytes (4 KB) in size
#define PMM_BLOCKS_PER_BYTE 8                   // Each byte in our bitmap tracks 8 physical blocks (1 bit per 4 KB frame)

#define E820_USABLE           1                 // BIOS E820 memory type 1: Usable (free) RAM available for OS allocation
#define E820_RESERVED         2                 // BIOS E820 memory type 2: Reserved by hardware/BIOS (do not touch)
#define E820_ACPI_RECLAIMABLE 3                 // BIOS E820 memory type 3: ACPI tables (can be reclaimed after parsing ACPI)
#define E820_ACPI_NVS         4                 // BIOS E820 memory type 4: ACPI Non-Volatile Storage (must never be touched)
#define E820_BAD_MEMORY       5                 // BIOS E820 memory type 5: Defective RAM detected by BIOS

// Structure representing a single 24-byte entry produced by BIOS INT 0x15, EAX=0xE820
typedef struct __attribute__((packed)) {        // Use 'packed' attribute so the compiler adds zero padding between fields
    uint64_t base_addr;                         // 64-bit physical starting address of the memory region
    uint64_t length;                            // 64-bit length of the memory region in bytes
    uint32_t type;                              // 32-bit region type (1 = usable RAM, 2 = reserved, etc.)
    uint32_t acpi_ext;                          // 32-bit ACPI 3.0 extended attributes bitfield
} e820_entry_t;                                 // Alias the structure as e820_entry_t

// Initialize the Physical Memory Manager using the BIOS E820 map and bitmap placement address
void pmm_init(e820_entry_t* mmap, uint32_t entry_count, uint64_t bitmap_phys_addr);

// Mark a contiguous physical address range as free (0 in the bitmap) so it can be allocated
void pmm_init_region(uint64_t base_addr, uint64_t length);

// Mark a contiguous physical address range as used/reserved (1 in the bitmap) to protect it
void pmm_deinit_region(uint64_t base_addr, uint64_t length);

// Find and allocate a single free 4 KB physical page frame, returning its physical address
void* pmm_alloc_block(void);

// Free a previously allocated 4 KB physical page frame by its physical address
void pmm_free_block(void* phys_addr);

// Return the total number of 4 KB physical blocks managed by the PMM
uint64_t pmm_get_total_blocks(void);

// Return the number of currently used/reserved 4 KB physical blocks
uint64_t pmm_get_used_blocks(void);

// Return the number of currently free (available) 4 KB physical blocks
uint64_t pmm_get_free_blocks(void);

#endif                                          // End of PMM_H header guard
