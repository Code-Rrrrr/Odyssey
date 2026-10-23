#include "../include/heap.h"                    // Include kernel heap declarations and heap_block_t definition
#include "../include/pmm.h"                     // Include PMM to allocate backing physical 4 KB frames for the heap
#include "../include/vmm.h"                     // Include VMM to map backing physical frames into the heap's virtual range

static heap_block_t* heap_head = NULL;          // Pointer to the very first block header at the start of the kernel heap
static uint64_t heap_start_addr = 0;            // Starting virtual address of the kernel heap region
static uint64_t heap_end_addr = 0;              // Current ending virtual address of the mapped kernel heap region

// Helper to align any requested byte size UP to a 16-byte boundary (required by x86_64 ABI)
static inline size_t align16(size_t size) {     // Takes an arbitrary byte count
    return (size + 15) & ~(size_t)15;           // Round up to the nearest multiple of 16
}                                               // End of align16

// Expand the mapped virtual memory range of the kernel heap by a given number of bytes
static bool heap_expand(size_t extra_bytes) {   // Returns true if expansion succeeded, false if out of memory
    // Round the requested expansion size up to a multiple of PAGE_SIZE (4096 bytes)
    size_t aligned_bytes = (extra_bytes + PAGE_SIZE - 1) & ~(size_t)(PAGE_SIZE - 1); // Align up to 4 KB boundary
    size_t num_pages = aligned_bytes / PAGE_SIZE; // Compute how many new 4 KB pages need to be allocated and mapped

    uint64_t old_end = heap_end_addr;           // Save the previous heap end address before growing

    for (size_t i = 0; i < num_pages; i++) {    // Loop once for each new 4 KB page needed
        void* phys_frame = pmm_alloc_block();   // Allocate a fresh 4 KB physical frame from the PMM
        if (phys_frame == NULL) {               // Check if physical memory is exhausted
            return false;                       // Abort expansion if no physical frame is available
        }                                       // End of NULL check

        // Map the new physical frame at the current end of the virtual heap as Present + Writable
        if (!vmm_map_page(heap_end_addr, (uint64_t)(uintptr_t)phys_frame, PTE_PRESENT | PTE_WRITABLE)) { // Map page
            pmm_free_block(phys_frame);         // Free the physical frame if VMM mapping failed
            return false;                       // Abort expansion on VMM failure
        }                                       // End of vmm_map_page check

        heap_end_addr += PAGE_SIZE;             // Advance the heap's virtual end address by 4096 bytes
    }                                           // End of page allocation loop

    // Create a new free block header at the start of the newly mapped memory region
    heap_block_t* new_block = (heap_block_t*)(uintptr_t)old_end; // Place block header at the old heap end address
    new_block->magic = HEAP_MAGIC;              // Stamp with magic value ('MUNK') for integrity verification
    new_block->is_free = true;                  // Mark this newly mapped region as free
    new_block->size = aligned_bytes - sizeof(heap_block_t); // Usable payload size is total bytes minus header size
    new_block->next = NULL;                     // This is now the last block in the heap list
    new_block->prev = NULL;                     // Will be linked to the previous tail block below

    if (heap_head == NULL) {                    // If this is the very first expansion during heap_init()
        heap_head = new_block;                  // Set the new block as the head of the heap linked list
    } else {                                    // Otherwise, append the new block to the existing linked list tail
        heap_block_t* curr = heap_head;         // Start scanning from the head of the heap
        while (curr->next != NULL) {            // Traverse until we reach the last block in the list
            curr = curr->next;                  // Move to next block
        }                                       // End of traversal loop

        // If the previous tail block is already free, merge (coalesce) the new space directly into it
        if (curr->is_free) {                    // Check if the old tail block is currently free
            curr->size += aligned_bytes;        // Add the entire newly mapped byte size into the tail block
        } else {                                // If the old tail block is currently allocated
            curr->next = new_block;             // Link old tail's next pointer to the new block
            new_block->prev = curr;             // Link new block's prev pointer back to the old tail
        }                                       // End of tail merge check
    }                                           // End of heap_head check

    return true;                                // Heap expansion succeeded
}                                               // End of heap_expand

// Initialize the kernel heap at 'start_virt_addr' with 'initial_size' bytes
bool heap_init(uint64_t start_virt_addr, size_t initial_size) { // Returns true on success
    heap_head = NULL;                           // Reset the heap linked list head pointer
    heap_start_addr = start_virt_addr;          // Record the starting virtual address of the heap
    heap_end_addr = start_virt_addr;            // Initially, mapped end equals start before heap_expand() runs

    return heap_expand(initial_size);           // Allocate and map the initial heap pages and create first free block
}                                               // End of heap_init

// Split a large free block into two blocks if it has enough leftover space after satisfying 'req_size'
static void split_block(heap_block_t* block, size_t req_size) { // Takes block to split and aligned requested size
    // Check if leftover space can fit a new header plus at least 16 bytes of usable payload
    if (block->size >= req_size + sizeof(heap_block_t) + 16) { // Ensure splitting doesn't create tiny unusable fragments
        // Calculate the memory address where the new remainder block header will live
        uint8_t* remainder_addr = (uint8_t*)block + sizeof(heap_block_t) + req_size; // Advance past header + payload
        heap_block_t* remainder = (heap_block_t*)(uintptr_t)remainder_addr; // Cast address to a block header pointer

        remainder->magic = HEAP_MAGIC;          // Stamp remainder block with heap magic number
        remainder->is_free = true;              // Mark the remainder block as free for future allocations
        remainder->size = block->size - req_size - sizeof(heap_block_t); // Compute remaining usable bytes
        remainder->next = block->next;          // Link remainder's next to whatever followed the original block
        remainder->prev = block;                // Link remainder's prev back to the original block

        if (block->next != NULL) {              // If there was a block after the original block
            block->next->prev = remainder;      // Update that following block's prev pointer to point to remainder
        }                                       // End of next block check

        block->next = remainder;                // Link original block's next pointer to the new remainder block
        block->size = req_size;                 // Shrink original block's size to the exact requested payload size
    }                                           // End of split size check
}                                               // End of split_block

// Allocate 'size' bytes of memory from the kernel heap using a first-fit search
void* kmalloc(size_t size) {                    // Takes byte count to allocate; returns pointer to usable memory or NULL
    if (size == 0 || heap_head == NULL) {       // Reject zero-byte requests or calls made before heap_init()
        return NULL;                            // Return NULL
    }                                           // End of sanity check

    size_t aligned_size = align16(size);        // Align the requested size up to a 16-byte multiple

    heap_block_t* curr = heap_head;             // Start first-fit search at the head of the block list
    while (curr != NULL) {                      // Loop through all blocks in the heap
        if (curr->is_free && curr->size >= aligned_size) { // Check if block is free and large enough
            split_block(curr, aligned_size);    // Split the block if it is significantly larger than needed
            curr->is_free = false;              // Mark this block as allocated (no longer free)
            return (void*)((uint8_t*)curr + sizeof(heap_block_t)); // Return pointer right after the block header
        }                                       // End of free & size check
        curr = curr->next;                      // Move to the next block in the doubly-linked list
    }                                           // End of search loop

    // No existing block was big enough; expand the heap by enough pages to fit header + aligned_size
    if (!heap_expand(aligned_size + sizeof(heap_block_t))) { // Request more physical/virtual pages for the heap
        return NULL;                            // Return NULL if the OS is completely out of memory
    }                                           // End of heap_expand check

    return kmalloc(size);                       // Retry allocation now that the heap has been expanded
}                                               // End of kmalloc

// Allocate 'size' bytes from the kernel heap and zero-fill the allocated memory
void* kzalloc(size_t size) {                    // Takes byte count to allocate and zero
    void* ptr = kmalloc(size);                  // Allocate raw memory block using kmalloc
    if (ptr != NULL) {                          // If the allocation succeeded
        uint8_t* byte_ptr = (uint8_t*)ptr;      // Cast returned void pointer to a byte pointer
        for (size_t i = 0; i < size; i++) {     // Loop over every requested byte
            byte_ptr[i] = 0;                    // Set each byte to 0
        }                                       // End of zero-fill loop
    }                                           // End of NULL check
    return ptr;                                 // Return the zero-initialized memory pointer
}                                               // End of kzalloc

// Free a block of memory previously allocated by kmalloc/kzalloc and merge adjacent free blocks
void kfree(void* ptr) {                         // Takes payload pointer returned by kmalloc/kzalloc
    if (ptr == NULL) {                          // Ignore NULL pointers safely (standard C free behavior)
        return;                                 // Nothing to free
    }                                           // End of NULL check

    // Step backwards by sizeof(heap_block_t) bytes from the payload pointer to locate its block header
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t)); // Get header address

    if (block->magic != HEAP_MAGIC) {           // Verify magic number to catch invalid pointers or buffer overflows
        return;                                 // Corrupt or invalid pointer; abort free to prevent list corruption
    }                                           // End of magic verification

    block->is_free = true;                      // Mark the block as free

    // Forward coalescing: if the immediately following block is also free, merge it into this block
    if (block->next != NULL && block->next->is_free) { // Check if next block exists and is free
        block->size += sizeof(heap_block_t) + block->next->size; // Absorb next block's header and payload size
        block->next = block->next->next;        // Bypass the absorbed block in the forward list links
        if (block->next != NULL) {              // If a block exists after the absorbed block
            block->next->prev = block;          // Update its prev pointer back to the current merged block
        }                                       // End of next->prev update
    }                                           // End of forward coalescing

    // Backward coalescing: if the immediately preceding block is also free, merge this block into prev
    if (block->prev != NULL && block->prev->is_free) { // Check if previous block exists and is free
        block->prev->size += sizeof(heap_block_t) + block->size; // Absorb current block's header and payload into prev
        block->prev->next = block->next;        // Bypass current block in prev's next pointer
        if (block->next != NULL) {              // If a block exists after current block
            block->next->prev = block->prev;    // Point that block's prev pointer to block->prev
        }                                       // End of next->prev update
    }                                           // End of backward coalescing
}                                               // End of kfree
