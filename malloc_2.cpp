#include <unistd.h>
#include <string.h>
#define MAX_MEMORY_ALLOCATED_SIZE 100000000 // 10^8

struct MallocMetadata {
    size_t block_size;
    bool is_available;
    MallocMetadata* next_block;
    MallocMetadata* prev_block;
};

class MemoryManager {
    MallocMetadata* head;

public:
    MemoryManager() : head(NULL) {}

    MallocMetadata* get_block_start(void* memory) {
        return (MallocMetadata*)((char*)memory - sizeof(MallocMetadata));
    }

    void mark_block_free(void* memory) {
        MallocMetadata* target_block = get_block_start(memory);
        target_block->is_available = true;
    }

    void insert_sorted(MallocMetadata* new_block) {
        MallocMetadata* prev = NULL;
        MallocMetadata* curr = head;
        while (curr != NULL) {
            prev = curr;
            curr = curr->next_block;
        }
        if (prev != NULL) {
            prev->next_block = new_block;
            new_block->prev_block = prev;
        }
        else {
            head = new_block;
        }
    }

    void* allocate_new_block(size_t request_size) {
        MallocMetadata* curr = head;
        while (curr != NULL) {
            if (curr->block_size >= request_size && curr->is_available) {
                curr->is_available = false;
                return curr;
            }
            curr = curr->next_block;
        }
        size_t total_size = request_size + sizeof(MallocMetadata);
        void* new_memory = sbrk(total_size);
        if (new_memory == (void*)-1) {
            return NULL;
        }
        MallocMetadata* new_block = (MallocMetadata*)new_memory;
        new_block->block_size = request_size;
        new_block->is_available = false;
        new_block->next_block = NULL;
        new_block->prev_block = NULL;
        insert_sorted(new_block);
        return new_block;
    }

    size_t total_allocated_memory() {
        size_t total = 0;
        MallocMetadata* curr = head;
        while (curr != NULL) {
            total += curr->block_size;
            curr = curr->next_block;
        }
        return total;
    }

    size_t total_blocks() {
        size_t count = 0;
        MallocMetadata* curr = head;
        while (curr != NULL) {
            count++;
            curr = curr->next_block;
        }
        return count;
    }

    size_t free_memory_total() {
        size_t free_memory = 0;
        MallocMetadata* curr = head;
        while (curr != NULL) {
            if (curr->is_available) {
                free_memory += curr->block_size;
            }
            curr = curr->next_block;
        }
        return free_memory;
    }

    size_t free_blocks_count() {
        size_t count = 0;
        MallocMetadata* curr = head;
        while (curr != NULL) {
            if (curr->is_available) {
                count++;
            }
            curr = curr->next_block;
        }
        return count;
    }
};

MemoryManager memory_manager;

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
    MallocMetadata* block_metadata = memory_manager.get_block_start(old_memory);
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
    return memory_manager.free_blocks_count();
}

size_t _num_free_bytes() {
    return memory_manager.free_memory_total();
}

size_t _num_allocated_blocks() {
    return memory_manager.total_blocks();
}

size_t _num_allocated_bytes() {
    return memory_manager.total_allocated_memory();
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes() {
    return (_size_meta_data() * _num_allocated_blocks());
}
