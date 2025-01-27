#include <iostream>
#include <cstring>
extern void* scalloc(size_t num, size_t size);

int main() {
    int* arr = (int*)scalloc(5, sizeof(int));

    bool is_zero = true;
    for (int i = 0; i < 5; i++) {
        if (arr[i] != 0) is_zero = false;
    }

    if (arr && is_zero) std::cout << "scalloc: Success\n";
    else std::cout << "scalloc: Failed\n";

    return 0;
}
