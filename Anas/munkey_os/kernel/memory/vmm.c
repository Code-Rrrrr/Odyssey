#include "../include/vmm.h"                     // Include VMM declarations, PTE flags, and page_table_t definition
#include "../include/pmm.h"                     // Include PMM functions so VMM can allocate physical frames for new page tables

static page_table_t* current_pml4 = NULL;       // Pointer to the active top-level Page Map Level 4 (PML4) table

// Helper to zero out all 512 entries (4096 bytes) of a newly allocated page table
static void zero_page_table(page_table_t* table) { // Takes a pointer to a 4 KB page table
    for (uint32_t i = 0; i < ENTRIES_PER_TABLE; i++) { // Loop through all 512 entries
        table->entries[i] = 0;                  // Clear entry to 0 (marks PTE_PRESENT = 0, i.e., not present)
    }                                           // End of zeroing loop
}                                               // End of zero_page_table

// Extract the 9-bit PML4 index (bits 39..47) from a 64-bit canonical virtual address
static inline uint64_t pml4_index(uint64_t virt_addr) { // Takes virtual address
    return (virt_addr >> 39) & 0x1FF;           // Shift right by 39 bits and mask the lowest 9 bits (0..511)
}                                               // End of pml4_index

// Extract the 9-bit Page Directory Pointer Table (PDPT) index (bits 30..38) from a virtual address
static inline uint64_t pdpt_index(uint64_t virt_addr) { // Takes virtual address
    return (virt_addr >> 30) & 0x1FF;           // Shift right by 30 bits and mask the lowest 9 bits (0..511)
}                                               // End of pdpt_index

// Extract the 9-bit Page Directory (PD) index (bits 21..29) from a virtual address
static inline uint64_t pd_index(uint64_t virt_addr) { // Takes virtual address
    return (virt_addr >> 21) & 0x1FF;           // Shift right by 21 bits and mask the lowest 9 bits (0..511)
}                                               // End of pd_index

// Extract the 9-bit Page Table (PT) index (bits 12..20) from a virtual address
static inline uint64_t pt_index(uint64_t virt_addr) { // Takes virtual address
    return (virt_addr >> 12) & 0x1FF;           // Shift right by 12 bits and mask the lowest 9 bits (0..511)
}                                               // End of pt_index

// Flush a single virtual page from the CPU's Translation Lookaside Buffer (TLB)
void vmm_flush_tlb(uint64_t virt_addr) {        // Takes the virtual address whose translation changed
    __asm__ volatile("invlpg (%0)" : : "r"(virt_addr) : "memory"); // Execute x86_64 'invlpg' instruction to invalidate TLB entry
}                                               // End of vmm_flush_tlb

// Load a new PML4 physical address into the CPU's CR3 control register
void vmm_switch_pml4(page_table_t* new_pml4) {  // Takes pointer to the new PML4 table
    current_pml4 = new_pml4;                    // Update our global pointer tracking the active PML4
    uint64_t cr3_val = (uint64_t)(uintptr_t)new_pml4; // Convert PML4 pointer to a 64-bit physical address
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3_val) : "memory"); // Write physical address into CR3 (flushes non-global TLB)
}                                               // End of vmm_switch_pml4

// Retrieve the currently active PML4 table pointer
page_table_t* vmm_get_current_pml4(void) {      // Takes no arguments
    return current_pml4;                        // Return the active PML4 pointer
}                                               // End of vmm_get_current_pml4

// Initialize the Virtual Memory Manager by capturing the PML4 set up by Stage 2 bootloader
void vmm_init(void) {                           // Takes no arguments
    uint64_t cr3_val = 0;                       // Variable to hold the current value of the CR3 register
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3_val)); // Read the active CR3 register (set by stage2.asm) into cr3_val
    current_pml4 = (page_table_t*)(uintptr_t)(cr3_val & PTE_ADDR_MASK); // Mask out lower flag bits to get the PML4 base address
}                                               // End of vmm_init

// Helper to get the next-level page table from an entry, optionally allocating a new table if not present
static page_table_t* vmm_get_next_level(page_table_t* table, uint64_t index, bool allocate, uint64_t flags) { // Walks one level
    if (table->entries[index] & PTE_PRESENT) {  // Check if the entry at 'index' already points to a valid next-level table
        if (flags & PTE_USER) {                 // If the caller is requesting user-mode access for a page under this entry
            table->entries[index] |= PTE_USER;  // Ensure intermediate directory entry also permits user-mode traversal
        }                                       // End of user flag propagation
        return (page_table_t*)(uintptr_t)(table->entries[index] & PTE_ADDR_MASK); // Extract and return next table's address
    }                                           // End of present check

    if (!allocate) {                            // If entry is not present and caller did not ask to allocate missing tables
        return NULL;                            // Return NULL to indicate the translation path does not exist
    }                                           // End of allocate check

    // Allocate a new 4 KB physical frame from PMM to hold the missing next-level page table
    page_table_t* new_table = (page_table_t*)pmm_alloc_block(); // Request 1 free 4 KB block from the Physical Memory Manager
    if (new_table == NULL) {                    // Check if the PMM ran out of physical memory
        return NULL;                            // Return NULL on allocation failure
    }                                           // End of NULL check

    zero_page_table(new_table);                 // Clear all 512 entries in the newly allocated page table to 0

    // Install the new table's physical address into the parent entry with Present + Writable (+ User if requested)
    uint64_t entry_flags = PTE_PRESENT | PTE_WRITABLE | (flags & PTE_USER); // Combine permissive parent directory flags
    table->entries[index] = ((uint64_t)(uintptr_t)new_table & PTE_ADDR_MASK) | entry_flags; // Write physical address + flags

    return new_table;                           // Return pointer to the newly created next-level page table
}                                               // End of vmm_get_next_level

// Map a 4 KB virtual page to a 4 KB physical frame with the specified flags
bool vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) { // Returns true on success, false on OOM
    if (current_pml4 == NULL) {                 // Ensure VMM has been initialized with a valid PML4
        return false;                           // Cannot map pages without a top-level PML4 table
    }                                           // End of NULL PML4 check

    uint64_t pml4_idx = pml4_index(virt_addr);  // Extract Level 4 index (PML4: bits 39..47)
    uint64_t pdpt_idx = pdpt_index(virt_addr);  // Extract Level 3 index (PDPT: bits 30..38)
    uint64_t pd_idx   = pd_index(virt_addr);    // Extract Level 2 index (PD:   bits 21..29)
    uint64_t pt_idx   = pt_index(virt_addr);    // Extract Level 1 index (PT:   bits 12..20)

    // Walk from PML4 (Level 4) down to PDPT (Level 3), allocating the PDPT if it doesn't exist yet
    page_table_t* pdpt = vmm_get_next_level(current_pml4, pml4_idx, true, flags); // Get or create PDPT
    if (pdpt == NULL) {                         // Check if PDPT allocation failed
        return false;                           // Return false (out of physical memory)
    }                                           // End of PDPT check

    // Walk from PDPT (Level 3) down to Page Directory (Level 2), allocating the PD if it doesn't exist yet
    page_table_t* pd = vmm_get_next_level(pdpt, pdpt_idx, true, flags); // Get or create PD
    if (pd == NULL) {                           // Check if PD allocation failed
        return false;                           // Return false (out of physical memory)
    }                                           // End of PD check

    // Walk from Page Directory (Level 2) down to Page Table (Level 1), allocating the PT if it doesn't exist yet
    page_table_t* pt = vmm_get_next_level(pd, pd_idx, true, flags); // Get or create PT
    if (pt == NULL) {                           // Check if PT allocation failed
        return false;                           // Return false (out of physical memory)
    }                                           // End of PT check

    // Set the final Level 1 Page Table Entry to point to the target 4 KB physical frame with requested flags
    pt->entries[pt_idx] = (phys_addr & PTE_ADDR_MASK) | (flags | PTE_PRESENT); // Store physical frame address + flags
    vmm_flush_tlb(virt_addr);                   // Flush the TLB entry for this virtual address so CPU sees the new mapping

    return true;                                // Page successfully mapped
}                                               // End of vmm_map_page

// Unmap a 4 KB virtual page from the active address space
void vmm_unmap_page(uint64_t virt_addr) {       // Takes the virtual address to unmap
    if (current_pml4 == NULL) {                 // Ensure VMM has been initialized
        return;                                 // Nothing to do if no PML4 is active
    }                                           // End of NULL PML4 check

    uint64_t pml4_idx = pml4_index(virt_addr);  // Extract Level 4 index (PML4)
    uint64_t pdpt_idx = pdpt_index(virt_addr);  // Extract Level 3 index (PDPT)
    uint64_t pd_idx   = pd_index(virt_addr);    // Extract Level 2 index (PD)
    uint64_t pt_idx   = pt_index(virt_addr);    // Extract Level 1 index (PT)

    // Walk down the hierarchy without allocating missing tables (allocate = false)
    page_table_t* pdpt = vmm_get_next_level(current_pml4, pml4_idx, false, 0); // Look up PDPT
    if (pdpt == NULL) return;                   // Abort if PDPT does not exist

    page_table_t* pd = vmm_get_next_level(pdpt, pdpt_idx, false, 0); // Look up PD
    if (pd == NULL) return;                     // Abort if PD does not exist

    page_table_t* pt = vmm_get_next_level(pd, pd_idx, false, 0); // Look up PT
    if (pt == NULL) return;                     // Abort if PT does not exist

    pt->entries[pt_idx] = 0;                    // Clear the Page Table Entry to 0 (marks page as not present)
    vmm_flush_tlb(virt_addr);                   // Invalidate the TLB entry so CPU stops using the old cached translation
}                                               // End of vmm_unmap_page

// Resolve a virtual address to its physical address by walking the 4-level page tables
uint64_t vmm_get_phys_addr(uint64_t virt_addr) { // Returns physical address, or 0 if unmapped
    if (current_pml4 == NULL) {                 // Ensure VMM is initialized
        return 0;                               // Return 0 if no PML4 is active
    }                                           // End of NULL PML4 check

    uint64_t pml4_idx = pml4_index(virt_addr);  // Extract Level 4 index (PML4)
    uint64_t pdpt_idx = pdpt_index(virt_addr);  // Extract Level 3 index (PDPT)
    uint64_t pd_idx   = pd_index(virt_addr);    // Extract Level 2 index (PD)
    uint64_t pt_idx   = pt_index(virt_addr);    // Extract Level 1 index (PT)
    uint64_t offset   = virt_addr & 0xFFF;      // Extract the 12-bit byte offset within the 4 KB page (bits 0..11)

    page_table_t* pdpt = vmm_get_next_level(current_pml4, pml4_idx, false, 0); // Look up PDPT without allocating
    if (pdpt == NULL) return 0;                 // Not mapped at PML4 level

    page_table_t* pd = vmm_get_next_level(pdpt, pdpt_idx, false, 0); // Look up PD without allocating
    if (pd == NULL) return 0;                   // Not mapped at PDPT level

    // Check if the Page Directory entry is a 2 MB Huge Page (bit 7 = PTE_HUGE)
    if (pd->entries[pd_idx] & PTE_HUGE) {       // If stage2.asm mapped kernel memory using 2 MB huge pages
        uint64_t huge_base = pd->entries[pd_idx] & 0x000FFFFFFFE00000ULL; // Extract 2 MB-aligned physical base
        uint64_t huge_offset = virt_addr & 0x1FFFFF; // Extract 21-bit offset within the 2 MB huge page
        return huge_base + huge_offset;         // Return exact physical address inside the 2 MB page
    }                                           // End of huge page check

    page_table_t* pt = vmm_get_next_level(pd, pd_idx, false, 0); // Look up Level 1 PT without allocating
    if (pt == NULL) return 0;                   // Not mapped at PD level

    if (!(pt->entries[pt_idx] & PTE_PRESENT)) { // Check if the final 4 KB Page Table Entry is marked present
        return 0;                               // Page is not present in physical memory
    }                                           // End of present check

    return (pt->entries[pt_idx] & PTE_ADDR_MASK) + offset; // Combine 4 KB physical frame base with 12-bit page offset
}                                               // End of vmm_get_phys_addr
