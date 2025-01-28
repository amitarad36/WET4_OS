#include <iostream>
#include <cstring>
extern void* smalloc(size_t size);
extern void* srealloc(void* oldp, size_t size);
extern void sfree(void* p);

int main() {
    char* p1 = (char*)smalloc(10);
    strcpy(p1, "Hello");

    char* p2 = (char*)srealloc(p1, 20);

    if (p2 && strcmp(p2, "Hello") == 0) std::cout << "srealloc: Success\n";
    else std::cout << "srealloc: Failed\n";

    return 0;
}
