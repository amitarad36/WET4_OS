#include <iostream>
extern void* smalloc(size_t size);

int main() {
    void* p1 = smalloc(50);
    void* p2 = smalloc(100);

    if (p1 && p2) std::cout << "smalloc: Success\n";
    else std::cout << "smalloc: Failed\n";

    return 0;
}
