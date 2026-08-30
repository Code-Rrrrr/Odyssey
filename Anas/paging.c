#include <stdint.h>

// Bit flags for page table entries
#define PAGE_PRESENT    0b00000001 // The page is currently in memory
#define PAGE_RW         0b00000010 // The page is Read/Write (if 0, it's Read-Only)
#define PAGE_USER       0b00000100 // User-space can access this (if 0, Kernel only)

// A Page Table is just an array of 512 64-bit integers
typedef uint64_t PageTable[512]; 

// We ask Layer 2 (PMM) for a 4KB block to act as our top-level table
PageTable* pml4_table;

// Extract the 9-bit index for each level from a Virtual Address
uint64_t get_pml4_index(uint64_t v_addr) { return (v_addr >> 39) & 0x1FF; }
uint64_t get_pdp_index(uint64_t v_addr)  { return (v_addr >> 30) & 0x1FF; }
uint64_t get_pd_index(uint64_t v_addr)   { return (v_addr >> 21) & 0x1FF; }
uint64_t get_pt_index(uint64_t v_addr)   { return (v_addr >> 12) & 0x1FF; }



void vmm_map_page(PageTable* pml4, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) {
    uint64_t pml4_idx = get_pml4_index(virtual_addr);
    uint64_t pdp_idx  = get_pdp_index(virtual_addr);
    uint64_t pd_idx   = get_pd_index(virtual_addr);
    uint64_t pt_idx   = get_pt_index(virtual_addr);

    // 1. Check PML4. Does the PDP table exist?
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        uint64_t new_pdp = (uint64_t)pmm_alloc_block(); // Get a 4KB block from Layer 2
        // Clear the new block to 0 here (code omitted for brevity)
        pml4[pml4_idx] = new_pdp | PAGE_PRESENT | PAGE_RW | PAGE_USER; 
    }
    
    // Get the address of the PDP table (ignore the flag bits)
    PageTable* pdp = (PageTable*)(pml4[pml4_idx] & ~0xFFF);

    // 2. Check PDP. Does the PD table exist?
    if (!(pdp[pdp_idx] & PAGE_PRESENT)) {
        uint64_t new_pd = (uint64_t)pmm_alloc_block();
        pdp[pdp_idx] = new_pd | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }
    PageTable* pd = (PageTable*)(pdp[pdp_idx] & ~0xFFF);

    // 3. Check PD. Does the PT table exist?
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        uint64_t new_pt = (uint64_t)pmm_alloc_block();
        pd[pd_idx] = new_pt | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }
    PageTable* pt = (PageTable*)(pd[pd_idx] & ~0xFFF);

    // 4. Finally, link the exact Page Table entry to our Target Physical Address!
    pt[pt_idx] = physical_addr | flags;
}


void vmm_activate_paging(PageTable* pml4) {
    // Tell the CPU where the top-level table is
    __asm__ volatile("mov %0, %%cr3" : : "r" (pml4));
}