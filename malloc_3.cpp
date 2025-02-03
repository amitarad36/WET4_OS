#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <cstddef>

#define MAX_ORDER 10
#define MIN_BLOCK_SIZE 128
#define INITIAL_BLOCK_COUNT 32
#define INITIAL_HEAP_SIZE (INITIAL_BLOCK_COUNT * (128 * 1024)) // 32 * 128KB
#define ALIGNMENT (INITIAL_BLOCK_COUNT * (128 * 1024)) // Ensures buddy alignment

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

void insert_block_sorted(int order, MallocMetadata* block);
void remove_block(int order, MallocMetadata* block);

MallocMetadata* free_blocks[MAX_ORDER + 1] = { nullptr };
bool initialized = false;

void initialize_buddy_allocator() {
    if (initialized) return;
    initialized = true;

    void* aligned_start = sbrk(0);
    size_t alignment_offset = (uintptr_t)aligned_start % ALIGNMENT;
    if (alignment_offset) sbrk(ALIGNMENT - alignment_offset);

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

MallocMetadata* get_buddy(MallocMetadata* block) {
    return (MallocMetadata*)((uintptr_t)block ^ block->size);
}

MallocMetadata* split_block_until_fit(int order, size_t required_size) {
    MallocMetadata* block = free_blocks[order];
    remove_block(order, block);

    while (order > 0) {
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

size_t _num_free_blocks() {
    size_t count = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* current = free_blocks[i];
        while (current) {
            count++;
            current = current->next;
        }
    }
    return count;
}

size_t _num_free_bytes() {
    size_t total = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* current = free_blocks[i];
        while (current) {
            total += current->size;
            current = current->next;
        }
    }
    return total;
}

size_t _num_meta_data_bytes() {
    return _num_free_blocks() * sizeof(MallocMetadata);
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
