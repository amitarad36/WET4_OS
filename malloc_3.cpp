#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <cstddef>
#include <cstdint>

#define MAX_ORDER 10
#define MIN_BLOCK_SIZE 128
#define INITIAL_BLOCK_COUNT 32
#define INITIAL_HEAP_SIZE (INITIAL_BLOCK_COUNT * (128 * 1024))

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

class MemoryManager {
public:
    MallocMetadata* free_blocks[MAX_ORDER + 1];

    MemoryManager() {
        for (int i = 0; i <= MAX_ORDER; i++) {
            free_blocks[i] = nullptr;
        }
    }

    void insert_block(int order, MallocMetadata* block) {
        block->is_free = true;
        block->next = free_blocks[order];
        if (free_blocks[order]) free_blocks[order]->prev = block;
        free_blocks[order] = block;
    }

    MallocMetadata* get_free_block(int order) {
        if (!free_blocks[order]) return nullptr;
        MallocMetadata* block = free_blocks[order];
        free_blocks[order] = free_blocks[order]->next;
        if (free_blocks[order]) free_blocks[order]->prev = nullptr;
        block->is_free = false;
        return block;
    }

    MallocMetadata* get_buddy(MallocMetadata* block) {
        return (MallocMetadata*)((uintptr_t)block ^ block->size);
    }

    void merge_block(int order, MallocMetadata* block) {
        if (order >= MAX_ORDER) return;

        MallocMetadata* buddy = get_buddy(block);
        if (!buddy || !buddy->is_free || buddy->size != block->size) return;

        if (buddy < block) block = buddy;
        block->size *= 2;
        insert_block(order + 1, block);
        merge_block(order + 1, block);
    }
};

MemoryManager manager;
bool initialized = false;

void initialize_buddy_allocator() {
    if (initialized) return;
    initialized = true;

    void* heap_start = sbrk(INITIAL_HEAP_SIZE);
    if (heap_start == (void*)-1) return;

    for (int i = 0; i < INITIAL_BLOCK_COUNT; i++) {
        MallocMetadata* block = (MallocMetadata*)((char*)heap_start + (i * (128 * 1024)));
        block->size = 128 * 1024;
        block->is_free = true;
        manager.insert_block(MAX_ORDER, block);
    }
}

void* smalloc(size_t size) {
    if (size == 0) return nullptr;
    initialize_buddy_allocator();

    int order = 0;
    size += sizeof(MallocMetadata);

    while (static_cast<size_t>(1 << order) < size && order < MAX_ORDER) {
        order++;
    }

    MallocMetadata* block = manager.get_free_block(order);
    if (!block) return nullptr;
    return (void*)(block + 1);
}


void* scalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    if (num == 0 || size == 0) return nullptr;

    void* ptr = smalloc(total_size);
    if (ptr) memset(ptr, 0, total_size);
    return ptr;
}

void sfree(void* p) {
    if (!p) return;

    MallocMetadata* block = ((MallocMetadata*)p) - 1;
    block->is_free = true;
    int order = 0;
    while ((1 << order) < block->size && order < MAX_ORDER) order++;

    manager.insert_block(order, block);
    manager.merge_block(order, block);
}

void* srealloc(void* oldp, size_t new_size) {
    if (new_size == 0) {
        sfree(oldp);
        return nullptr;
    }
    if (!oldp) return smalloc(new_size);

    MallocMetadata* block = ((MallocMetadata*)oldp) - 1;
    size_t old_size = block->size;

    if (old_size == new_size) return oldp;

    void* newp = smalloc(new_size);
    if (!newp) return nullptr;

    memcpy(newp, oldp, old_size < new_size ? old_size : new_size);
    sfree(oldp);
    return newp;
}

size_t _num_allocated_bytes() {
    size_t total = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* curr = manager.free_blocks[i];
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
        MallocMetadata* curr = manager.free_blocks[i];
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
        MallocMetadata* curr = manager.free_blocks[i];
        while (curr) {
            total += curr->size;
            curr = curr->next;
        }
    }
    return total;
}

size_t _num_allocated_blocks() {
    size_t count = INITIAL_BLOCK_COUNT;
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* curr = manager.free_blocks[i];
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
