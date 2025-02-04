#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <cstdint>

#define MAX_MEMORY_ALLOCATED_SIZE 100000000
#define MAX_ORDER 10
#define MAX_SIZE_BLOCK 128*1024
#define BLOCK_UNIT 128
#define ALIGNMENT_FACTOR 32*128*1024


struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

class BuddyMemoryManager {
    MallocMetadata* allocated_by_mmap;
    MallocMetadata* free_lists[MAX_ORDER + 1];

public:
    BuddyMemoryManager() : allocated_by_mmap(NULL) {
        for (int i = 0; i <= MAX_ORDER; i++) {
            free_lists[i] = NULL;
        }
    }

    void first_assign() {
        void* current_p_break = sbrk(0);
        intptr_t aligned_p_break_adress = ((intptr_t)current_p_break + ALIGNMENT_FACTOR - 1) & ~(ALIGNMENT_FACTOR - 1);
        sbrk(aligned_p_break_adress - (intptr_t)current_p_break);

        for (int i = 0; i < 32; i++) {
            void* p_break = sbrk(MAX_SIZE_BLOCK);
            MallocMetadata* new_block_allocated = (MallocMetadata*)p_break;
            new_block_allocated->size = MAX_SIZE_BLOCK - sizeof(MallocMetadata);
            new_block_allocated->is_free = true;
            insert_block_to_array(new_block_allocated);
        }
    }

    void insert_block_to_array(MallocMetadata* block) {
        int order = get_order_of_block(block);
        MallocMetadata* list = free_lists[order];

        if (list == NULL) {
            free_lists[order] = block;
            block->next = NULL;
            block->prev = NULL;
            return;
        }

        while (list->next != NULL) list = list->next;
        list->next = block;
        block->prev = list;
        block->next = NULL;
    }

    void delete_block_from_array(MallocMetadata* block) {
        int order = get_order_of_block(block);

        if (block->prev == NULL) {
            free_lists[order] = block->next;
            if (block->next != NULL) block->next->prev = NULL;
        }
        else {
            block->prev->next = block->next;
            if (block->next != NULL) block->next->prev = block->prev;
        }

        block->next = NULL;
        block->prev = NULL;
    }

    int get_order_of_block(MallocMetadata* block) {
        size_t size_with_metadata = (block->size + sizeof(MallocMetadata)) / BLOCK_UNIT;
        int order = 0;
        while (size_with_metadata > 1) {
            size_with_metadata /= 2;
            order++;
        }
        return order;
    }

    void* allocate_new_block(size_t size) {
        if (size >= MAX_SIZE_BLOCK) {
            return allocate_block_with_mmap(size);
        }
        return allocate_block_without_mmap(size);
    }

    MallocMetadata* allocate_block_with_mmap(size_t size) {
        void* mmap_block = mmap(NULL, sizeof(MallocMetadata) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mmap_block == MAP_FAILED) return NULL;

        MallocMetadata* block_allocated = (MallocMetadata*)mmap_block;
        block_allocated->size = size;
        block_allocated->is_free = false;
        block_allocated->next = allocated_by_mmap;
        allocated_by_mmap = block_allocated;
        return block_allocated;
    }

    MallocMetadata* allocate_block_without_mmap(size_t size) {
        MallocMetadata* best_fit = find_best_block_for_allocation(size);
        if (best_fit == NULL) return NULL;

        delete_block_from_array(best_fit);
        best_fit->is_free = false;
        return best_fit;
    }

    MallocMetadata* find_best_block_for_allocation(size_t size) {
        int order = get_order_of_block((MallocMetadata*)(((char*)sbrk(0)) + size));
        for (int i = order; i <= MAX_ORDER; i++) {
            if (free_lists[i] != NULL) return free_lists[i];
        }
        return NULL;
    }

    void mark_block_free(void* memory) {
        MallocMetadata* block = (MallocMetadata*)((char*)memory - sizeof(MallocMetadata));
        block->is_free = true;
        insert_block_to_array(block);
    }
};

BuddyMemoryManager memory_manager;
bool initialized = false;

void* smalloc(size_t size) {
    if (!initialized) {
        memory_manager.first_assign();
        initialized = true;
    }
    if (size == 0 || size > MAX_MEMORY_ALLOCATED_SIZE) return NULL;

    void* allocated_memory = memory_manager.allocate_new_block(size);
    if (allocated_memory == NULL) return NULL;

    return (char*)allocated_memory + sizeof(MallocMetadata);
}

void* scalloc(size_t num, size_t size) {
    void* allocated_memory = smalloc(num * size);
    if (allocated_memory == NULL) return NULL;
    memset(allocated_memory, 0, num * size);
    return allocated_memory;
}

void sfree(void* memory) {
    if (memory == NULL) return;
    MallocMetadata* block = (MallocMetadata*)((char*)memory - sizeof(MallocMetadata));

    if (block->size >= MAX_SIZE_BLOCK) {
        munmap(block, block->size + sizeof(MallocMetadata));
    }
    else {
        memory_manager.mark_block_free(memory);
    }
}

void* srealloc(void* old_memory, size_t new_size) {
    if (new_size == 0 || new_size > MAX_MEMORY_ALLOCATED_SIZE) return NULL;
    if (old_memory == NULL) return smalloc(new_size);

    MallocMetadata* block = (MallocMetadata*)((char*)old_memory - sizeof(MallocMetadata));
    if (block->size >= new_size) return old_memory;

    void* new_memory = smalloc(new_size);
    if (new_memory == NULL) return NULL;

    memmove(new_memory, old_memory, block->size);
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
    return memory_manager.get_number_of_all_blocks();
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
