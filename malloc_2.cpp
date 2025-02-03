#include <unistd.h>
#include <cstring>
#include <cstddef>

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata* head = nullptr;

// Insert block in sorted order
void insert_sorted(MallocMetadata* block) {
    block->is_free = true;
    if (!head || block < head) {
        block->next = head;
        if (head) head->prev = block;
        head = block;
        return;
    }
    MallocMetadata* curr = head;
    while (curr->next && curr->next < block) {
        curr = curr->next;
    }
    block->next = curr->next;
    block->prev = curr;
    if (curr->next) curr->next->prev = block;
    curr->next = block;
}

// Find the first free block that fits
MallocMetadata* find_free_block(size_t size) {
    MallocMetadata* curr = head;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            curr->is_free = false;
            return curr;
        }
        curr = curr->next;
    }
    return nullptr;
}

// Merging adjacent free blocks
void merge_free_blocks() {
    MallocMetadata* curr = head;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += sizeof(MallocMetadata) + curr->next->size;
            curr->next = curr->next->next;
            if (curr->next) curr->next->prev = curr;
        }
        else {
            curr = curr->next;
        }
    }
}

void* smalloc(size_t size) {
    if (size == 0 || size > 100000000) return nullptr;

    MallocMetadata* block = find_free_block(size);
    if (block) return (void*)(block + 1);

    block = (MallocMetadata*)sbrk(size + sizeof(MallocMetadata));
    if (block == (void*)-1) return nullptr;

    block->size = size;
    block->is_free = false;
    insert_sorted(block);

    return (void*)(block + 1);
}

void sfree(void* p) {
    if (!p) return;
    MallocMetadata* block = ((MallocMetadata*)p) - 1;
    block->is_free = true;
    merge_free_blocks();
}

void* srealloc(void* oldp, size_t size) {
    if (size == 0) {
        sfree(oldp);
        return nullptr;
    }
    if (!oldp) return smalloc(size);

    MallocMetadata* block = ((MallocMetadata*)oldp) - 1;
    if (size <= block->size) return oldp;

    void* newp = smalloc(size);
    if (!newp) return nullptr;

    memcpy(newp, oldp, block->size);
    sfree(oldp);
    return newp;
}

size_t _num_allocated_bytes() {
    size_t total = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* curr = free_blocks[i];
        while (curr) {
            total += curr->size;
            curr = curr->next;
        }
    }
    return total;
}

size_t _num_free_blocks() {
    size_t count = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* curr = free_blocks[i];
        while (curr) {
            count++;
            curr = curr->next;
        }
    }
    return count;
}

size_t _num_free_bytes() {
    size_t total = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* curr = free_blocks[i];
        while (curr) {
            total += curr->size;
            curr = curr->next;
        }
    }
    return total;
}

size_t _num_allocated_blocks() {
    size_t count = INITIAL_BLOCK_COUNT; // Count initial 32 blocks
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* curr = free_blocks[i];
        while (curr) {
            count++;
            curr = curr->next;
        }
    }
    return count;
}

size_t _num_meta_data_bytes() {
    return _num_allocated_blocks() * sizeof(MallocMetadata);
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
