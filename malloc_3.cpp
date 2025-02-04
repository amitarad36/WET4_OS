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
    MallocMetadata* allocated_list;

public:
    BuddyMemoryManager() : allocated_list(NULL) {
        for (int i = 0; i <= MAX_ORDER; i++) {
            free_lists[i] = NULL;
        }
    }

    MallocMetadata* get_allocated_list() { return allocated_list; }

    void add_to_allocated_list(MallocMetadata* block) {
        block->next_block = allocated_list;
        if (allocated_list != NULL) {
            allocated_list->prev_block = block;
        }
        block->prev_block = NULL; 
        allocated_list = block;
    }

    size_t total_blocks() {
        size_t count = 0;
        for (int i = 0; i <= MAX_ORDER; i++) {
            MallocMetadata* curr = free_lists[i];
            while (curr != NULL) {
                count++;
                curr = curr->next_block;
            }
        }
        MallocMetadata* curr_allocated = allocated_list;
        while (curr_allocated != NULL) {
            count++;
            curr_allocated = curr_allocated->next_block;
        }
        return count;
    }

    void* allocate_new_block(size_t request_size) {
        size_t order = get_order(request_size);
        if (order > MAX_ORDER) {
            return NULL;
        }

        size_t block_size = 1 << (order + 7);
        void* new_memory = sbrk(block_size + sizeof(MallocMetadata));
        if (new_memory == (void*)-1) {
            return NULL;
        }

        MallocMetadata* new_block = (MallocMetadata*)new_memory;
        new_block->block_size = block_size;
        new_block->is_available = false;
        new_block->next_block = NULL;
        new_block->prev_block = NULL;

        if (free_lists[order] == NULL) {
            free_lists[order] = new_block;
        }
        else {
            MallocMetadata* current = free_lists[order];
            while (current->next_block != NULL) {
                current = current->next_block;
            }
            current->next_block = new_block;
            new_block->prev_block = current;
        }

        return new_block;
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

    void mark_block_free(void* memory) {
        MallocMetadata* block = (MallocMetadata*)((char*)memory - sizeof(MallocMetadata));
        block->is_available = true;

        if (block->prev_block) {
            block->prev_block->next_block = block->next_block;
        }
        else {
            allocated_list = block->next_block; 
        }
        if (block->next_block) {
            block->next_block->prev_block = block->prev_block;
        }

        size_t order = get_order(block->block_size);
        block->next_block = free_lists[order];
        if (free_lists[order] != NULL) {
            free_lists[order]->prev_block = block;
        }
        free_lists[order] = block;
        block->prev_block = NULL;
    }

    MallocMetadata* get_free_list(int order) {
        if (order < 0 || order > MAX_ORDER) return NULL;
        return free_lists[order];
    }

    void set_free_list(int order, MallocMetadata* block) {
        if (order < 0 || order > MAX_ORDER) return;
        free_lists[order] = block;
    }

    void remove_from_allocated_list(MallocMetadata* block) {
        if (!block || !allocated_list) return;

        if (block == allocated_list) { 
            allocated_list = block->next_block;
            if (allocated_list) {
                allocated_list->prev_block = NULL;
            }
            return;
        }

        if (block->prev_block) {
            block->prev_block->next_block = block->next_block;
        }

        if (block->next_block) {
            block->next_block->prev_block = block->prev_block;
        }
    }

};

BuddyMemoryManager memory_manager;

void* smalloc(size_t size) {
    if (size == 0 || size > MAX_MEMORY_ALLOCATED_SIZE) {
        return NULL;
    }

    MallocMetadata* block = (MallocMetadata*)memory_manager.allocate_new_block(size);
    if (block == NULL) {
        return NULL;
    }

    memory_manager.add_to_allocated_list(block);

    return (char*)block + sizeof(MallocMetadata);
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
    if (memory == NULL) return;

    MallocMetadata* block = (MallocMetadata*)((char*)memory - sizeof(MallocMetadata));
    block->is_available = true;

    memory_manager.remove_from_allocated_list(block);

    if (block->next_block && block->next_block->is_available) {
        block->block_size += sizeof(MallocMetadata) + block->next_block->block_size;
        block->next_block = block->next_block->next_block;
    }

    size_t order = memory_manager.get_order(block->block_size);
    block->next_block = memory_manager.get_free_list(order);
    memory_manager.set_free_list(order, block);
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
    return memory_manager.total_blocks();
}

size_t _num_free_bytes() {
    return memory_manager.total_blocks() * MIN_BLOCK_SIZE;
}

size_t _num_allocated_blocks() {
    size_t count = 0;

    for (int i = 0; i <= MAX_ORDER; i++) {
        MallocMetadata* curr = memory_manager.get_free_list(i);
        while (curr != NULL) {
            count++;
            curr = curr->next_block;
        }
    }

    MallocMetadata* curr_allocated = memory_manager.get_allocated_list();
    while (curr_allocated != NULL) {
        count++;
        curr_allocated = curr_allocated->next_block;
    }

    return count;
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
