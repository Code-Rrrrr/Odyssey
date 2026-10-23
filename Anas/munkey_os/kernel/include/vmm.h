#ifndef VMM_H                                   // Check if VMM_H macro is not defined to prevent multiple inclusions
#define VMM_H                                   // Define VMM_H macro to guard this header file

#include "types.h"                              // Include fixed-width integer types (uint64_t, bool, etc.)

#define PAGE_SIZE 4096                          // Standard x86_64 page size is 4096 bytes (4 KB)
#define ENTRIES_PER_TABLE 512                   // Each x86_64 paging table holds 512 64-bit entries (512 * 8 = 4096 bytes)

// x86_64 Page Table Entry (PTE) hardware bit flags
#define PTE_PRESENT       (1ULL << 0)           // Bit 0: Page is present in physical memory
#define PTE_WRITABLE      (1ULL << 1)           // Bit 1: Page is writable (if 0, page is read-only)
#define PTE_USER          (1ULL << 2)           // Bit 2: User-mode (Ring 3) access allowed (if 0, supervisor/Ring 0 only)
#define PTE_WRITE_THROUGH (1ULL << 3)           // Bit 3: Page-level write-through caching enabled
#define PTE_CACHE_DISABLE (1ULL << 4)           // Bit 4: Page-level cache disable (useful for MMIO registers)
#define PTE_ACCESSED      (1ULL << 5)           // Bit 5: Set by CPU when page is read or written
#define PTE_DIRTY         (1ULL << 6)           // Bit 6: Set by CPU when page is written to
#define PTE_HUGE          (1ULL << 7)           // Bit 7: 2 MB page (in PD) or 1 GB page (in PDPT)
#define PTE_GLOBAL        (1ULL << 8)           // Bit 8: Global page (not flushed from TLB on CR3 reload)
#define PTE_NX            (1ULL << 63)          // Bit 63: No-Execute bit (prevents instruction fetches from this page)

// Mask to extract the 4 KB-aligned 40-bit physical frame address (bits 12..51) from a 64-bit Page Table Entry
#define PTE_ADDR_MASK     0x000FFFFFFFFFF000ULL // Clears flags in bits 0..11 and upper bits 52..63

// Structure representing any of the 4 levels of x86_64 page tables (PML4, PDPT, PD, or PT)
typedef struct __attribute__((aligned(4096))) { // Force 4096-byte alignment as required by x86_64 hardware
    uint64_t entries[ENTRIES_PER_TABLE];        // Array of 512 64-bit page table entries
} page_table_t;                                 // Alias the structure as page_table_t

// Initialize the Virtual Memory Manager (reads active PML4 from CR3 or sets up kernel paging)
void vmm_init(void);

// Map a single 4 KB virtual address to a 4 KB physical address with the given PTE flags
bool vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);

// Unmap a single 4 KB virtual address (clears its PTE and flushes the TLB entry)
void vmm_unmap_page(uint64_t virt_addr);

// Translate a virtual address to its mapped physical address (returns 0 if not mapped)
uint64_t vmm_get_phys_addr(uint64_t virt_addr);

// Switch the active top-level PML4 page table by loading its physical address into CR3
void vmm_switch_pml4(page_table_t* new_pml4);

// Return a pointer to the currently active PML4 page table
page_table_t* vmm_get_current_pml4(void);

// Invalidate a single virtual address in the CPU's Translation Lookaside Buffer (TLB)
void vmm_flush_tlb(uint64_t virt_addr);

#endif                                          // End of VMM_H header guard
