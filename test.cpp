#include <iostream>

// Declare External Functions (Defined in malloc_3.cpp)
extern void* smalloc(size_t size);
extern void sfree(void* p);
extern size_t _num_free_blocks();
extern size_t _num_allocated_blocks();

int main() {
    void* p1 = smalloc(100);
    void* p2 = smalloc(200);

    std::cout << "Allocated Blocks: " << _num_allocated_blocks() << std::endl;
    std::cout << "Pointer p1: " << p1 << ", Pointer p2: " << p2 << std::endl;

    sfree(p1);
    sfree(p2);

    std::cout << "Free Blocks after freeing: " << _num_free_blocks() << std::endl;
    return 0;
}
