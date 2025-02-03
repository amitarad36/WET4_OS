#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <cstddef>
#include <cstdint>

#define MAX_ORDER 10
#define MIN_BLOCK_SIZE 128
#define INITIAL_BLOCK_COUNT 32
#define INITIAL_HEAP_SIZE (INITIAL_BLOCK_COUNT * (128 * 1024))
#define ALIGNMENT (INITIAL_BLOCK_COUNT * (128 * 1024))

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata* free_blocks[MAX_ORDER + 1] = { nullptr };
bool initialized = false;

void initialize_buddy_allocator();
void insert_block_sorted(int order, MallocMetadata* block);
void remove_block(int order, MallocMetadata* block);
void merge_block(int order, MallocMetadata* block);
MallocMetadata* get_buddy(MallocMetadata* block);
MallocMetadata* split_block_until_fit(int order, size_t required_size);
int get_order(size_t size);
void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t new_size);
size_t _num_allocated_bytes();
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_meta_data_bytes();
size_t _size_meta_data();

void initialize_buddy_allocator() {
    if (initialized) return;
    initialized = true;

    void* heap_start = sbrk(INITIAL_HEAP_SIZE);
    if (heap_start == (void*)-1) return;

    for (int i = 0; i < INITIAL_BLOCK_COUNT; i++) {
        MallocMetadata* block = (MallocMetadata*)((char*)heap_start + (i * (128 * 1024)));
        block->size = 128 * 1024;
        block->is_free = true;
        insert_block_sorted(MAX_ORDER, block);
    }
}

void insert_block_sorted(int order, MallocMetadata* block) {
    block->is_free = true;
    block->next = nullptr;
    block->prev = nullptr;

    if (!free_blocks[order]) {
        free_blocks[order] = block;
        return;
    }

    MallocMetadata* current = free_blocks[order];
    MallocMetadata* prev = nullptr;

    while (current && current < block) {
        prev = current;
        current = current->next;
    }

    block->next = current;
    block->prev = prev;
    if (current) current->prev = block;
    if (prev) prev->next = block;
    else free_blocks[order] = block;
}

void remove_block(int order, MallocMetadata* block) {
    if (!block) return;

    if (block->prev) block->prev->next = block->next;
    if (block->next) block->next->prev = block->prev;
    if (free_blocks[order] == block) free_blocks[order] = block->next;

    block->next = nullptr;
    block->prev = nullptr;
}

MallocMetadata* get_buddy(MallocMetadata* block) {
    if (!block) return nullptr;
    return (MallocMetadata*)((uintptr_t)block ^ block->size);
}

void merge_block(int order, MallocMetadata* block) {
    if (order >= MAX_ORDER) return;

    MallocMetadata* buddy = get_buddy(block);

    if (!buddy || !buddy->is_free || buddy->size != block->size) return;

    remove_block(order, buddy);

    if (buddy < block) block = buddy;
    block->size *= 2;

    insert_block_sorted(order + 1, block);
    merge_block(order + 1, block);
}

int get_order(size_t size) {
    size += sizeof(MallocMetadata);
    int order = 0;
    size_t block_size = MIN_BLOCK_SIZE;

    while (block_size < size) {
        block_size *= 2;
        order++;
        if (order > MAX_ORDER) return -1;
    }
    return order;
}

MallocMetadata* split_block_until_fit(int order, size_t required_size) {
    if (!free_blocks[order]) {
        return nullptr;
    }

    MallocMetadata* block = free_blocks[order];
    remove_block(order, block);

    while (order > 0) {
        if (!block) {
            return nullptr;
        }

        size_t half_size = block->size / 2;
        if (required_size + sizeof(MallocMetadata) > half_size) break;

        MallocMetadata* buddy = (MallocMetadata*)((char*)block + half_size);
        buddy->size = half_size;
        buddy->is_free = true;

        block->size = half_size;
        insert_block_sorted(order - 1, buddy);
        order--;
    }

    return block;
}

void* smalloc(size_t size) {
    if (size == 0) return nullptr;
    initialize_buddy_allocator();

    if (size >= 128 * 1024) {
        void* addr = mmap(nullptr, size + sizeof(MallocMetadata),
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (addr == MAP_FAILED) return nullptr;

        MallocMetadata* block = (MallocMetadata*)addr;
        block->size = size;
        block->is_free = false;
        return (void*)(block + 1);
    }

    int order = get_order(size);
    while (order <= MAX_ORDER && !free_blocks[order]) {
        split_block_until_fit(order + 1, size);
    }

    if (!free_blocks[order]) return nullptr;

    MallocMetadata* block = free_blocks[order];
    remove_block(order, block);
    block->is_free = false;
    return (void*)(block + 1);
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

    if (block->size >= 128 * 1024) {
        munmap(block, block->size + sizeof(MallocMetadata));
        return;
    }

    block->is_free = true;
    int order = get_order(block->size);
    insert_block_sorted(order, block);
    merge_block(order, block);
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
