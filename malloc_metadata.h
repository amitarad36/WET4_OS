#ifndef MALLOC_METADATA_H
#define MALLOC_METADATA_H

#include <cstddef>  // For size_t

struct MallocMetadata {
    size_t size;            // Size of the allocated block (excluding metadata)
    bool is_free;           // Whether the block is free or not
    MallocMetadata* next;   // Pointer to the next block
    MallocMetadata* prev;   // Pointer to the previous block
};

#endif // MALLOC_METADATA_H
