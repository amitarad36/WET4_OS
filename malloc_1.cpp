#include <unistd.h>
#include <stddef.h>

void* smalloc(size_t size) {
    // Check for invalid size requests
    if (size == 0 || size > 100000000) {
        return nullptr;
    }

    // Use sbrk to allocate memory
    void* ptr = sbrk(size);
    if (ptr == (void*)-1) { // sbrk failed
        return nullptr;
    }

    return ptr;
}
