#include <unistd.h>
#include <string.h>
#define MAX_MEMORY_ALLOCATED_SIZE 100000000 // 10^8

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

class SortedBlocks {
    MallocMetadata* list;

public:
    SortedBlocks() : list(NULL) {}

    MallocMetadata* get_start_of_block(void* block) {
        return (MallocMetadata*)((char*)block - sizeof(MallocMetadata));
    }

    void release_used_block(void* block_place) {
        MallocMetadata* current_block = get_start_of_block(block_place);
        current_block->is_free = true;
    }

    void add_block_to_list(MallocMetadata* block) {
        MallocMetadata* prev = NULL;
        MallocMetadata* current = list;
        while (current != NULL) {
            prev = current;
            current = current->next;
        }
        if (prev != NULL) {
            prev->next = block;
            block->prev = prev;
        }
        else {
            list = block;
        }
    }

    void* create_memory_for_block(size_t size) {
        MallocMetadata* current = list;
        while (current != NULL) {
            if (current->size >= size && current->is_free) {
                current->is_free = false;
                return current;
            }
            current = current->next;
        }
        size_t total_allocation_cost = size + sizeof(MallocMetadata);
        void* p_break = sbrk(total_allocation_cost);
        if (p_break == (void*)-1) {
            return NULL;
        }
        MallocMetadata* new_block_allocated = (MallocMetadata*)p_break;
        new_block_allocated->size = size;
        new_block_allocated->is_free = false;
        new_block_allocated->next = NULL;
        new_block_allocated->prev = NULL;
        add_block_to_list(new_block_allocated);
        return new_block_allocated;
    }

    size_t get_sum_of_all_bytes() {
        size_t sum = 0;
        MallocMetadata* current = list;
        while (current != NULL) {
            sum += current->size;
            current = current->next;
        }
        return sum;
    }

    size_t get_number_of_all_blocks() {
        size_t counter = 0;
        MallocMetadata* current = list;
        while (current != NULL) {
            counter++;
            current = current->next;
        }
        return counter;
    }

    size_t get_sum_of_all_free_bytes() {
        size_t sum = 0;
        MallocMetadata* current = list;
        while (current != NULL) {
            if (current->is_free) {
                sum += current->size;
            }
            current = current->next;
        }
        return sum;
    }

    size_t get_number_of_all_free_blocks() {
        size_t counter = 0;
        MallocMetadata* current = list;
        while (current != NULL) {
            if (current->is_free) {
                counter++;
            }
            current = current->next;
        }
        return counter;
    }
};

SortedBlocks list;

void* smalloc(size_t size) {
    if (size == 0 || size > MAX_MEMORY_ALLOCATED_SIZE) {
        return NULL;
    }
    void* p_break = list.create_memory_for_block(size);
    if (p_break == NULL) {
        return NULL;
    }
    return (char*)p_break + sizeof(MallocMetadata);
}

void* scalloc(size_t num, size_t size) {
    void* place = smalloc(size * num);
    if (place == NULL) {
        return NULL;
    }
    memset(place, 0, size * num);
    return place;
}

void sfree(void* p) {
    if (p != NULL) {
        list.release_used_block(p);
    }
}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > MAX_MEMORY_ALLOCATED_SIZE) {
        return NULL;
    }
    if (oldp == NULL) {
        return smalloc(size);
    }
    MallocMetadata* block_data = list.get_start_of_block(oldp);
    size_t old_block_size = block_data->size;
    if (old_block_size >= size) {
        return oldp;
    }
    void* new_block = smalloc(size);
    if (new_block == NULL) {
        return NULL;
    }
    memmove(new_block, oldp, old_block_size);
    sfree(oldp);
    return new_block;
}

size_t _num_free_blocks() {
    return list.get_number_of_all_free_blocks();
}

size_t _num_free_bytes() {
    return list.get_sum_of_all_free_bytes();
}

size_t _num_allocated_blocks() {
    return list.get_number_of_all_blocks();
}

size_t _num_allocated_bytes() {
    return list.get_sum_of_all_bytes();
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}

size_t _num_meta_data_bytes() {
    return (_size_meta_data() * _num_allocated_blocks());
}

