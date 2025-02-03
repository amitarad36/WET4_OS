#include <iostream>
#include <vector>
#include "malloc_3.cpp"

void test_allocation() {
    std::cout << "Testing smalloc() allocations...\n";
    void* p1 = smalloc(100);
    void* p2 = smalloc(200);
    void* p3 = smalloc(500);

    std::cout << "Allocated p1: " << p1 << "\n";
    std::cout << "Allocated p2: " << p2 << "\n";
    std::cout << "Allocated p3: " << p3 << "\n";
}

void test_calloc() {
    std::cout << "Testing scalloc() allocations...\n";
    void* p1 = scalloc(10, 10);
    void* p2 = scalloc(5, 20);
    std::cout << "Allocated p1: " << p1 << "\n";
    std::cout << "Allocated p2: " << p2 << "\n";
}

void test_free() {
    std::cout << "Testing sfree() deallocation...\n";
    void* p1 = smalloc(256);
    void* p2 = smalloc(512);

    std::cout << "Allocated p1: " << p1 << "\n";
    std::cout << "Allocated p2: " << p2 << "\n";

    sfree(p1);
    std::cout << "Freed p1\n";

    sfree(p2);
    std::cout << "Freed p2\n";
}

void test_reallocation() {
    std::cout << "Testing srealloc()...\n";
    void* p1 = smalloc(300);
    std::cout << "Allocated p1: " << p1 << "\n";

    p1 = srealloc(p1, 600);
    std::cout << "Reallocated p1 to larger size: " << p1 << "\n";

    p1 = srealloc(p1, 150);
    std::cout << "Reallocated p1 to smaller size: " << p1 << "\n";

    sfree(p1);
    std::cout << "Freed p1\n";
}

void test_mmap() {
    std::cout << "Testing mmap() large allocations...\n";
    void* p1 = smalloc(130 * 1024);
    std::cout << "Allocated large block using mmap: " << p1 << "\n";

    sfree(p1);
    std::cout << "Freed mmap block\n";
}

void test_statistics() {
    std::cout << "Testing statistics functions...\n";
    std::cout << "Free blocks: " << _num_free_blocks() << "\n";
    std::cout << "Free bytes: " << _num_free_bytes() << "\n";
    std::cout << "Meta-data bytes: " << _num_meta_data_bytes() << "\n";
    std::cout << "Meta-data size: " << _size_meta_data() << "\n";
}

int main() {
    test_allocation();
    test_calloc();
    test_free();
    test_reallocation();
    test_mmap();
    test_statistics();
    return 0;
}
