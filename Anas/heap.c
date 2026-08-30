#include <stddef.h>

typedef struct HeapBlock {
    size_t size;               // How many bytes is this slice?
    int is_free;               // 1 if free, 0 if used
    struct HeapBlock* next;    // Pointer to the next slice in memory
} HeapBlock;

// The very first header in our heap memory
HeapBlock* heap_head = NULL;

void* my_malloc(size_t request_size) {
    HeapBlock* current = heap_head;

    // 1. Search the linked list for a free block that is big enough
    while (current != NULL) {
        if (current->is_free && current->size >= request_size) {
            
            // We found one! Mark it as used.
            current->is_free = 0;
            
            // Return the address AFTER the header.
            // (current + 1) jumps forward in memory by exactly the size of the header.
            return (void*)(current + 1); 
        }
        current = current->next;
    }

    // 2. If we reach here, we are out of space! 
    // We would need to ask Layer 3 (VMM) to map more 4KB pages here.
    return NULL; 
}

void my_free(void* ptr) {
    if (ptr == NULL) return;

    // 1. Step backwards in memory to find the hidden header
    HeapBlock* header = (HeapBlock*)ptr - 1;

    // 2. Mark the block as free so malloc() can use it again
    header->is_free = 1;
    
    // (In a real OS, you would also combine this free block 
    // with adjacent free blocks to prevent fragmentation).
}