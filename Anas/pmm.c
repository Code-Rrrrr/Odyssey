#include <stdint.h> // Provides standard integer types like uint32_t
#include <stddef.h> // Provides standard definitions like NULL

#define BLOCK_SIZE 4096 // Each frame is exactly 4KB

// We will point this to a safe place in memory later
uint32_t* memory_bitmap = 0; 
uint32_t total_blocks = 0;

// Mark a chunk as USED (Set the bit to 1)
void pmm_set_bit(uint32_t bit) {
    uint32_t array_index = bit / 32;
    uint32_t bit_offset = bit % 32;
    memory_bitmap[array_index] |= (1 << bit_offset);
}

// Mark a chunk as FREE (Clear the bit to 0)
void pmm_clear_bit(uint32_t bit) {
    uint32_t array_index = bit / 32;
    uint32_t bit_offset = bit % 32;
    memory_bitmap[array_index] &= ~(1 << bit_offset);
}

// Check if a chunk is used or free (Returns 1 if used, 0 if free)
int pmm_test_bit(uint32_t bit) {
    uint32_t array_index = bit / 32;
    uint32_t bit_offset = bit % 32;
    return (memory_bitmap[array_index] & (1 << bit_offset));
}


// Find a free 4KB chunk and return its physical memory address
void* pmm_alloc_block() {
    // 1. Scan the whole array
    for (uint32_t i = 0; i < (total_blocks / 32); i++) {
        
        // Optimization: If the integer is 0xFFFFFFFF, all 32 bits are 1 (Full)
        if (memory_bitmap[i] != 0xFFFFFFFF) {
            
            // 2. We found an integer with at least one free bit. Find exactly which one.
            for (int j = 0; j < 32; j++) {
                uint32_t current_bit = (i * 32) + j;
                
                // 3. If the bit is 0, it's free!
                if (!pmm_test_bit(current_bit)) {
                    pmm_set_bit(current_bit); // Mark it as used
                    
                    // Multiply the bit number by 4096 to get the physical memory address
                    uint64_t physical_address = current_bit * BLOCK_SIZE;
                    return (void*) physical_address;
                }
            }
        }
    }
    return NULL; // Out of memory!
}

// Give a 4KB chunk back to the system to be reused
void pmm_free_block(void* physical_address) {
    // Divide the address by 4096 to find out which bit it belongs to
    uint32_t bit = (uint32_t)((uint64_t)physical_address / BLOCK_SIZE);
    
    // Mark it as free
    pmm_clear_bit(bit);
}