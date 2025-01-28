#include <iostream>
extern void* smalloc(size_t size);
extern void sfree(void* p);
extern size_t _num_allocated_blocks();
extern size_t _num_free_blocks();

int main() {
    void* p1 = smalloc(100);
    void* p2 = smalloc(200);
    std::cout << "Allocated Blocks: " << _num_allocated_blocks() << "\n";

    sfree(p1);
    std::cout << "Free Blocks: " << _num_free_blocks() << "\n";

    return 0;
}
