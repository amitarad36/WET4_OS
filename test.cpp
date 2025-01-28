#include <iostream>
extern void* smalloc(size_t size);
extern void sfree(void* p);

int main() {
    void* p1 = smalloc(0);
    void* p2 = smalloc(100000001);

    if (p1 == nullptr) std::cout << "smalloc(0): Passed\n";
    else std::cout << "smalloc(0): Failed\n";

    if (p2 == nullptr) std::cout << "smalloc(>100MB): Passed\n";
    else std::cout << "smalloc(>100MB): Failed\n";

    sfree(nullptr); // Should not crash
    std::cout << "sfree(nullptr): Passed\n";

    return 0;
}
