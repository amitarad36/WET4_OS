#include <unistd.h>
#include <string.h>
#define MAX_MEMORY_ALLOCATED_SIZE 100000000 // 10^8
#define MIN_BLOCK_SIZE 128
#define MAX_ORDER 10
#define MMAP_THRESHOLD 131072

struct MallocMetadata {
    size_t block_size;
    bool is_available;
    MallocMetadata* next_block;
    MallocMetadata* prev_block;
};

class BuddyMemoryManager {
    MallocMetadata* free_lists[MAX_ORDER + 1];

public:
    BuddyMemoryManager() {
        for (int i = 0; i <= MAX_ORDER; i++) {
            free_lists[i] = NULL;
        }
    }

    size_t get_order(size_t size) {
        size_t order = 0;
        size_t block_size = MIN_BLOCK_SIZE;
        while (block_size < size && order < MAX_ORDER) {
            block_size *= 2;
            order++;
        }
        return order;
    }

    void* allocate_new_block(size_t request_size) {
        size_t order = get_order(request_size);
        if (order > MAX_ORDER) {
            return NULL;
        }
        if (free_lists[order] != NULL) {
            MallocMetadata* block = free_lists[order];
            free_lists[order] = block->next_block;
            block->is_available = false;
            return block;
        }
        size_t block_size = 1 << (order + 7);
        void* new_memory = sbrk(block_size + sizeof(MallocMetadata));
        if (new_memory == (void*)-1) {
            return NULL;
        }
        MallocMetadata* new_block = (MallocMetadata*)new_memory;
        new_block->block_size = block_size;
        new_block->is_available = false;
        return new_block;
    }

    void mark_block_free(void* memory) {
        MallocMetadata* block = (MallocMetadata*)((char*)memory - sizeof(MallocMetadata));
        block->is_available = true;
        size_t order = get_order(block->block_size);
        block->next_block = free_lists[order];
        free_lists[order] = block;
    }

    MallocMetadata* get_free_list(int order) {
        if (order < 0 || order > MAX_ORDER) return NULL;
        return free_lists[order];
    }
};

BuddyMemoryManager memory_manager;

void* smalloc(size_t size) {
    if (size == 0 || size > MAX_MEMORY_ALLOCATED_SIZE) {
        return NULL;
    }
    void* allocated_memory = memory_manager.allocate_new_block(size);
    if (allocated_memory == NULL) {
        return NULL;
    }
    return (char*)allocated_memory + sizeof(MallocMetadata);
}

void* scalloc(size_t num, size_t size) {
    void* allocated_memory = smalloc(num * size);
    if (allocated_memory == NULL) {
        return NULL;
    }
    memset(allocated_memory, 0, num * size);
    return allocated_memory;
}

void sfree(void* memory) {
    if (memory != NULL) {
        memory_manager.mark_block_free(memory);
    }
}

void* srealloc(void* old_memory, size_t new_size) {
    if (new_size == 0 || new_size > MAX_MEMORY_ALLOCATED_SIZE) {
        return NULL;
    }
    if (old_memory == NULL) {
        return smalloc(new_size);
    }
    MallocMetadata* block_metadata = (MallocMetadata*)((char*)old_memory - sizeof(MallocMetadata));
    size_t current_size = block_metadata->block_size;
    if (current_size >= new_size) {
        return old_memory;
    }
    void* new_memory = smalloc(new_size);
    if (new_memory == NULL) {
        return NULL;
    }
    memmove(new_memory, old_memory, current_size);
    sfree(old_memory);
    return new_memory;
}

size_t _num_free_blocks() {
    size_t count = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* curr = memory_manager.get_free_list(i);
        while (curr != NULL) {
            count++;
            curr = curr->next_block;
        }
    }
    return count;
}

size_t _num_free_bytes() {
    size_t free_memory = 0;
    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* curr = memory_manager.get_free_list(i);
        while (curr != NULL) {
            free_memory += curr->block_size;
            curr = curr->next_block;
        }
    }
    return free_memory;
}

size_t _num_allocated_blocks() {
    return _num_free_blocks();
}

size_t _num_allocated_bytes() {
    return _num_free_bytes();
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes() {
    return (_size_meta_data() * _num_allocated_blocks());
}
