#include <iostream>
extern void* smalloc(size_t size);
extern size_t _num_allocated_blocks();

int main() {
    void* p1 = smalloc(100);
    void* p2 = smalloc(200);

    std::cout << "Allocated Blocks: " << _num_allocated_blocks() << "\n";
    std::cout << "Pointer p1: " << p1 << ", Pointer p2: " << p2 << std::endl;

    return 0;
}
