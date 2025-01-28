#include <iostream>
extern void* smalloc(size_t size);
extern void sfree(void* p);

int main() {
    void* p1 = smalloc(64);
    void* p2 = smalloc(64);

    sfree(p1); // Free first block

    void* p3 = smalloc(64); // Should reuse p1's memory

    if (p1 == p3) std::cout << "Memory Reuse: Passed\n";
    else std::cout << "Memory Reuse: Failed\n";

    return 0;
}
