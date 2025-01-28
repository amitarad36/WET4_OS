#include <iostream>
extern void* smalloc(size_t size);
extern void sfree(void* p);

int main() {
    void* pointers[1000];

    for (int i = 0; i < 1000; i++) {
        pointers[i] = smalloc(32);
        if (!pointers[i]) {
            std::cout << "smalloc failed at iteration " << i << "\n";
            return 1;
        }
    }

    for (int i = 0; i < 1000; i++) {
        sfree(pointers[i]);
    }

    std::cout << "Stress Test: Passed\n";
    return 0;
}
