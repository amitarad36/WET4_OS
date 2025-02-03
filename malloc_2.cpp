#include <unistd.h>
#include <cstring>
#include <cstddef>

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

class FreeBlockList {
public:
    MallocMetadata* head;

    FreeBlockList() : head(nullptr) {}

    void insert(MallocMetadata* block) {
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

    void merge() {
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
};

FreeBlockList freeBlocks;

void* smalloc(size_t size) {
    if (size == 0 || size > 100000000) return nullptr;

    MallocMetadata* block = freeBlocks.find_free_block(size);
    if (block) return (void*)(block + 1);

    block = (MallocMetadata*)sbrk(size + sizeof(MallocMetadata));
    if (block == (void*)-1) return nullptr;

    block->size = size;
    block->is_free = false;
    freeBlocks.insert(block);

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
    freeBlocks.merge();
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

size_t _num_free_blocks() {
    size_t count = 0;
    for (MallocMetadata* curr = head; curr; curr = curr->next) {
        if (curr->is_free) count++;
    }
    return count;
}

size_t _num_free_bytes() {
    size_t total = 0;
    for (MallocMetadata* curr = head; curr; curr = curr->next) {
        if (curr->is_free) total += curr->size;
    }
    return total;
}

size_t _num_allocated_blocks() {
    size_t count = 0;
    for (MallocMetadata* curr = head; curr; curr = curr->next) {
        count++;
    }
    return count;
}

size_t _num_allocated_bytes() {
    size_t total = 0;
    for (MallocMetadata* curr = head; curr; curr = curr->next) {
        total += curr->size;
    }
    return total;
}

size_t _num_meta_data_bytes() {
    return _num_allocated_blocks() * sizeof(MallocMetadata);
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
