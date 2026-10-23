#ifndef HEAP_H                                  // Check if HEAP_H macro is not defined to prevent multiple inclusions
#define HEAP_H                                  // Define HEAP_H macro to guard this header file

#include "types.h"                              // Include fixed-width integer types (size_t, uint64_t, bool, etc.)

#define HEAP_MAGIC 0x4D554E4B                   // Magic number ('MUNK' in ASCII hex) to detect heap corruption/invalid frees

// Doubly-linked list header placed immediately before every allocated or free heap chunk
typedef struct heap_block {                     // Forward-declare struct heap_block so pointers can reference it
    uint32_t magic;                             // Magic value (HEAP_MAGIC) to verify header integrity
    bool     is_free;                           // True (1) if this chunk is free; False (0) if currently allocated
    size_t   size;                              // Size of the usable data payload (in bytes) following this header
    struct heap_block* next;                    // Pointer to the next contiguous block header in the heap
    struct heap_block* prev;                    // Pointer to the previous contiguous block header in the heap
} heap_block_t;                                 // Alias the structure as heap_block_t

// Initialize the kernel heap at a given virtual start address with an initial byte size
bool heap_init(uint64_t start_virt_addr, size_t initial_size);

// Allocate 'size' bytes of 16-byte aligned memory from the kernel heap (returns NULL on failure)
void* kmalloc(size_t size);

// Allocate 'size' bytes of zero-initialized memory from the kernel heap
void* kzalloc(size_t size);

// Free a previously allocated kernel heap pointer and coalesce adjacent free blocks
void kfree(void* ptr);

#endif                                          // End of HEAP_H header guard
