#include "../include/pmm.h"                     // Include PMM declarations, constants, and E820 struct definitions

static uint8_t* pmm_bitmap = NULL;              // Pointer to the bitmap array in memory (1 bit per 4 KB physical frame)
static uint64_t pmm_max_blocks = 0;             // Total number of 4 KB frames spanning up to the highest physical address
static uint64_t pmm_used_blocks = 0;            // Number of 4 KB frames currently marked as used/reserved (bit = 1)
static uint64_t pmm_bitmap_size = 0;            // Size of the bitmap array in bytes (pmm_max_blocks / 8)

// Helper function to mark a specific frame index as used (sets its bit to 1)
static inline void bitmap_set(uint64_t bit) {   // Takes the 4 KB frame index (bit number)
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));    // Locate byte (bit / 8) and set bit offset (bit % 8) using bitwise OR
}                                               // End of bitmap_set

// Helper function to mark a specific frame index as free (clears its bit to 0)
static inline void bitmap_clear(uint64_t bit) { // Takes the 4 KB frame index (bit number)
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));   // Locate byte (bit / 8) and clear bit offset (bit % 8) using bitwise AND NOT
}                                               // End of bitmap_clear

// Helper function to check if a specific frame index is used (returns true if bit is 1)
static inline bool bitmap_test(uint64_t bit) {  // Takes the 4 KB frame index (bit number)
    return (pmm_bitmap[bit / 8] & (1 << (bit % 8))) != 0; // Test whether the bit at (bit % 8) in byte (bit / 8) is non-zero
}                                               // End of bitmap_test

// Internal helper to scan the bitmap and find the index of the first free (0) frame
static int64_t bitmap_find_first_free(void) {   // Returns frame index on success, or -1 if out of physical memory
    for (uint64_t i = 0; i < pmm_bitmap_size; i++) { // Iterate through each byte of the bitmap
        if (pmm_bitmap[i] != 0xFF) {            // Quick check: if byte is 0xFF (all 8 bits are 1), skip all 8 frames at once
            for (uint8_t j = 0; j < 8; j++) {   // Inspect each of the 8 bits inside this non-full byte
                uint64_t bit = i * 8 + j;       // Calculate the global frame index corresponding to byte i, bit j
                if (bit >= pmm_max_blocks) {    // Ensure we do not look past the total number of physical frames
                    return -1;                  // Reached end of physical memory without finding a free frame
                }                               // End of bounds check
                if (!(pmm_bitmap[i] & (1 << j))) { // Check if the j-th bit is 0 (meaning the frame is free)
                    return (int64_t)bit;        // Return the free frame index immediately
                }                               // End of bit zero check
            }                                   // End of inner bit loop
        }                                       // End of byte != 0xFF check
    }                                           // End of outer byte loop
    return -1;                                  // Every single frame in the bitmap is used; return -1 (out of memory)
}                                               // End of bitmap_find_first_free

// Initialize the Physical Memory Manager from the BIOS E820 memory map
void pmm_init(e820_entry_t* mmap, uint32_t entry_count, uint64_t bitmap_phys_addr) { // Takes E820 table, count, and where to place bitmap
    uint64_t max_phys_addr = 0;                 // Track the highest usable physical address reported by BIOS

    for (uint32_t i = 0; i < entry_count; i++) { // Loop through all BIOS E820 memory map entries
        if (mmap[i].type == E820_USABLE) {      // Only consider regions marked as usable RAM (type 1)
            uint64_t region_end = mmap[i].base_addr + mmap[i].length; // Compute the ending physical address of this region
            if (region_end > max_phys_addr) {   // Check if this region extends higher than our current maximum
                max_phys_addr = region_end;     // Update highest physical address seen so far
            }                                   // End of max check
        }                                       // End of usable type check
    }                                           // End of E820 scan loop

    pmm_max_blocks = max_phys_addr / PMM_BLOCK_SIZE; // Convert maximum physical address into total number of 4 KB blocks
    pmm_bitmap_size = (pmm_max_blocks + PMM_BLOCKS_PER_BYTE - 1) / PMM_BLOCKS_PER_BYTE; // Calculate bytes needed for bitmap (rounded up)
    pmm_bitmap = (uint8_t*)(uintptr_t)bitmap_phys_addr; // Point the bitmap array to the caller-supplied physical address

    // By default, mark ALL physical memory as used/reserved (0xFF = 11111111 in binary) for safety
    for (uint64_t i = 0; i < pmm_bitmap_size; i++) { // Loop over every byte in the bitmap
        pmm_bitmap[i] = 0xFF;                   // Set all 8 bits in the byte to 1 (reserved/used)
    }                                           // End of bitmap fill loop
    pmm_used_blocks = pmm_max_blocks;           // Initially all blocks are counted as used

    // Now selectively free only the regions that BIOS E820 explicitly says are usable RAM
    for (uint32_t i = 0; i < entry_count; i++) { // Loop through all BIOS E820 entries again
        if (mmap[i].type == E820_USABLE) {      // If this entry is usable RAM
            pmm_init_region(mmap[i].base_addr, mmap[i].length); // Mark the 4 KB frames inside this region as free (0)
        }                                       // End of usable check
    }                                           // End of usable region unlock loop

    // Protect the first 1 MB of physical memory (0x0 to 0x100000: IVT, BIOS data, bootloader, VGA buffer)
    pmm_deinit_region(0x0, 0x100000);           // Mark the lower 1 MB (256 frames) as used so it is never allocated

    // Protect the memory frames occupied by the PMM bitmap itself so we never overwrite our own bitmap
    pmm_deinit_region(bitmap_phys_addr, pmm_bitmap_size); // Mark the bitmap's physical address range as used
}                                               // End of pmm_init

// Unlock (free) a physical memory region so its 4 KB blocks can be allocated
void pmm_init_region(uint64_t base_addr, uint64_t length) { // Takes region start address and byte length
    // Align the start address UP to the nearest 4 KB boundary so we don't free a partially reserved page
    uint64_t aligned_base = (base_addr + PMM_BLOCK_SIZE - 1) & ~(uint64_t)(PMM_BLOCK_SIZE - 1); // Round up to multiple of 4096
    if (aligned_base >= base_addr + length) {   // Check if alignment pushed the base past the end of the region
        return;                                 // Region is smaller than a single aligned page; nothing to free
    }                                           // End of size check
    uint64_t adj_length = (base_addr + length) - aligned_base; // Calculate remaining length after upward base alignment
    uint64_t start_block = aligned_base / PMM_BLOCK_SIZE; // Convert aligned starting address to a 4 KB frame index
    uint64_t num_blocks = adj_length / PMM_BLOCK_SIZE; // Calculate how many full 4 KB frames fit in this region

    for (uint64_t i = 0; i < num_blocks; i++) { // Iterate over every full 4 KB block in the region
        uint64_t block_idx = start_block + i;   // Compute the current block index
        if (block_idx < pmm_max_blocks && bitmap_test(block_idx)) { // If within bounds and currently marked used
            bitmap_clear(block_idx);            // Clear the bit to 0 (mark frame as free)
            pmm_used_blocks--;                  // Decrement the count of used physical frames
        }                                       // End of test check
    }                                           // End of block loop
}                                               // End of pmm_init_region

// Lock (reserve) a physical memory region so its 4 KB blocks cannot be allocated
void pmm_deinit_region(uint64_t base_addr, uint64_t length) { // Takes region start address and byte length
    // Align the start address DOWN to the 4 KB boundary so any partially overlapped frame is protected
    uint64_t aligned_base = base_addr & ~(uint64_t)(PMM_BLOCK_SIZE - 1); // Round down to multiple of 4096
    uint64_t end_addr = base_addr + length;     // Compute the exact ending address of the region
    // Align the end address UP to the 4 KB boundary so the trailing partial frame is also protected
    uint64_t aligned_end = (end_addr + PMM_BLOCK_SIZE - 1) & ~(uint64_t)(PMM_BLOCK_SIZE - 1); // Round up to multiple of 4096

    uint64_t start_block = aligned_base / PMM_BLOCK_SIZE; // Convert aligned start address to starting frame index
    uint64_t num_blocks = (aligned_end - aligned_base) / PMM_BLOCK_SIZE; // Compute total frames covered

    for (uint64_t i = 0; i < num_blocks; i++) { // Iterate over each frame in the range
        uint64_t block_idx = start_block + i;   // Calculate current frame index
        if (block_idx < pmm_max_blocks && !bitmap_test(block_idx)) { // If within bounds and currently marked free
            bitmap_set(block_idx);              // Set the bit to 1 (mark frame as used/reserved)
            pmm_used_blocks++;                  // Increment the count of used physical frames
        }                                       // End of test check
    }                                           // End of block loop
}                                               // End of pmm_deinit_region

// Allocate a single 4 KB physical frame and return its physical address
void* pmm_alloc_block(void) {                   // Takes no arguments; returns 4 KB-aligned physical address or NULL
    if (pmm_get_free_blocks() == 0) {           // Check if there are zero free blocks remaining
        return NULL;                            // Out of physical memory; return NULL
    }                                           // End of free blocks check

    int64_t frame = bitmap_find_first_free();   // Search the bitmap for the first available frame (bit == 0)
    if (frame < 0) {                            // If no free bit was found in the bitmap
        return NULL;                            // Out of physical memory; return NULL
    }                                           // End of frame < 0 check

    bitmap_set((uint64_t)frame);                // Mark the found frame as used (bit = 1) in the bitmap
    pmm_used_blocks++;                          // Increment the total used block counter

    uint64_t phys_addr = (uint64_t)frame * PMM_BLOCK_SIZE; // Calculate physical address: frame_index * 4096
    return (void*)(uintptr_t)phys_addr;         // Cast the physical address to a void pointer and return it
}                                               // End of pmm_alloc_block

// Free a previously allocated 4 KB physical frame
void pmm_free_block(void* phys_addr) {          // Takes the physical address of the 4 KB frame to release
    if (phys_addr == NULL) {                    // Guard against freeing NULL (address 0 is always reserved anyway)
        return;                                 // Do nothing if NULL pointer passed
    }                                           // End of NULL check

    uint64_t addr = (uint64_t)(uintptr_t)phys_addr; // Convert pointer to a 64-bit unsigned integer address
    uint64_t frame = addr / PMM_BLOCK_SIZE;     // Divide by 4096 to get the corresponding bitmap frame index

    if (frame < pmm_max_blocks && bitmap_test(frame)) { // Verify frame is valid and currently marked as used
        bitmap_clear(frame);                    // Clear the bit in the bitmap back to 0 (free)
        pmm_used_blocks--;                      // Decrement the used blocks counter
    }                                           // End of valid & used check
}                                               // End of pmm_free_block

// Get the total number of 4 KB frames tracked by the PMM
uint64_t pmm_get_total_blocks(void) {           // Takes no arguments
    return pmm_max_blocks;                      // Return total blocks
}                                               // End of pmm_get_total_blocks

// Get the number of 4 KB frames currently marked as in-use/reserved
uint64_t pmm_get_used_blocks(void) {            // Takes no arguments
    return pmm_used_blocks;                     // Return used blocks count
}                                               // End of pmm_get_used_blocks

// Get the number of 4 KB frames currently available for allocation
uint64_t pmm_get_free_blocks(void) {            // Takes no arguments
    return pmm_max_blocks - pmm_used_blocks;    // Subtract used blocks from total blocks to get free blocks
}                                               // End of pmm_get_free_blocks
