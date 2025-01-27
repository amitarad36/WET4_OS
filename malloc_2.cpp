#include <unistd.h>  // For sbrk
#include <cstring>   // For memset, memmove
#include "malloc_metadata.h"

// Global pointer to the head of the allocation list
MallocMetadata* head = nullptr;

// ======================== HELPER FUNCTIONS ========================

// Inserts a new block at the beginning of the list
void insert_block(MallocMetadata* block) {
    block->next = head;
    block->prev = nullptr;
    if (head) head->prev = block;
    head = block;
}

// Searches for a free block of at least `size` bytes
MallocMetadata* find_free_block(size_t size) {
    MallocMetadata* current = head;
    while (current) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// Merges adjacent free blocks
void merge_free_blocks() {
    MallocMetadata* current = head;
    while (current) {
        if (current->is_free && current->next && current->next->is_free) {
            current->size += sizeof(MallocMetadata) + current->next->size;
            current->next = current->next->next;
            if (current->next) current->next->prev = current;
        }
        current = current->next;
    }
}

// ======================== MEMORY ALLOCATION FUNCTIONS ========================

void* smalloc(size_t size) {
    if (size == 0 || size > 100000000) return nullptr;

    MallocMetadata* block = find_free_block(size);
    if (block) {
        block->is_free = false;
        return (void*)(block + 1);
    }

    // No suitable free block found, allocate a new one
    MallocMetadata* new_block = (MallocMetadata*)sbrk(size + sizeof(MallocMetadata));
    if (new_block == (void*)-1) return nullptr;

    new_block->size = size;
    new_block->is_free = false;
    insert_block(new_block);

    return (void*)(new_block + 1);
}

void* scalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    if (num == 0 || size == 0 || total_size > 100000000) return nullptr;

    void* ptr = smalloc(total_size);
    if (ptr) memset(ptr, 0, total_size);
    return ptr;
}

void sfree(void* p) {
    if (!p) return;

    MallocMetadata* block = ((MallocMetadata*)p) - 1;
    if (block->is_free) return;  // Already freed

    block->is_free = true;
    merge_free_blocks();
}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > 100000000) return nullptr;
    if (!oldp) return smalloc(size);

    MallocMetadata* block = ((MallocMetadata*)oldp) - 1;
    if (size <= block->size) return oldp;

    void* newp = smalloc(size);
    if (!newp) return nullptr;

    memmove(newp, oldp, block->size);
    sfree(oldp);
    return newp;
}

// ======================== STATISTICS FUNCTIONS ========================

size_t _num_free_blocks() {
    size_t count = 0;
    for (MallocMetadata* current = head; current; current = current->next) {
        if (current->is_free) count++;
    }
    return count;
}

size_t _num_free_bytes() {
    size_t total = 0;
    for (MallocMetadata* current = head; current; current = current->next) {
        if (current->is_free) total += current->size;
    }
    return total;
}

size_t _num_allocated_blocks() {
    size_t count = 0;
    for (MallocMetadata* current = head; current; current = current->next) {
        count++;
    }
    return count;
}

size_t _num_allocated_bytes() {
    size_t total = 0;
    for (MallocMetadata* current = head; current; current = current->next) {
        total += current->size;
    }
    return total;
}

size_t _num_meta_data_bytes() {
    return _num_allocated_blocks() * sizeof(MallocMetadata);
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
