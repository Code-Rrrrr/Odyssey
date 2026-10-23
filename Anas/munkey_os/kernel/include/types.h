#ifndef TYPES_H                                 // Check if TYPES_H macro is not yet defined to prevent double inclusion
#define TYPES_H                                 // Define TYPES_H macro so subsequent includes of this file are skipped

typedef unsigned char      uint8_t;             // 8-bit unsigned integer (0 to 255), occupying 1 byte
typedef signed char        int8_t;              // 8-bit signed integer (-128 to 127), occupying 1 byte
typedef unsigned short     uint16_t;            // 16-bit unsigned integer (0 to 65,535), occupying 2 bytes
typedef signed short       int16_t;             // 16-bit signed integer (-32,768 to 32,767), occupying 2 bytes
typedef unsigned int       uint32_t;            // 32-bit unsigned integer (0 to 4,294,967,295), occupying 4 bytes
typedef signed int         int32_t;             // 32-bit signed integer (-2,147,483,648 to 2,147,483,647), occupying 4 bytes
typedef unsigned long long uint64_t;            // 64-bit unsigned integer (0 to 2^64 - 1), occupying 8 bytes on x86_64
typedef signed long long   int64_t;             // 64-bit signed integer (-2^63 to 2^63 - 1), occupying 8 bytes on x86_64

typedef uint64_t           size_t;              // Unsigned 64-bit integer type used for object sizes and counts
typedef uint64_t           uintptr_t;           // Unsigned 64-bit integer guaranteed to hold a memory pointer/address
typedef int64_t            intptr_t;            // Signed 64-bit integer guaranteed to hold a signed pointer offset

typedef uint8_t            bool;                // Boolean type represented as a single 8-bit unsigned byte
#define true               1                    // Boolean truth value (1)
#define false              0                    // Boolean false value (0)

#define NULL               ((void*)0)           // Null pointer constant pointing to memory address 0

#endif                                          // End of TYPES_H header guard
