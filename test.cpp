#include <iostream>
extern void* smalloc(size_t size);
extern void sfree(void* p);

int main() {
    void* p1 = smalloc(50);
    sfree(p1);
    void* p2 = smalloc(50); // Should reuse p1’s memory

    if (p1 == p2) std::cout << "sfree: Reused memory\n";
    else std::cout << "sfree: Failed\n";

    return 0;
}
