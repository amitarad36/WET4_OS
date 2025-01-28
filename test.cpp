#include <iostream>
extern void* smalloc(size_t size);
extern size_t _num_meta_data_bytes();
extern size_t _size_meta_data();

int main() {
    size_t before = _num_meta_data_bytes();
    void* p1 = smalloc(100);  // Allocate memory
    size_t after = _num_meta_data_bytes();

    // Use p1 to avoid unused variable warning
    std::cout << "Allocated at: " << p1 << std::endl;

    std::cout << "Metadata Before: " << before << ", After: " << after << std::endl;

    if (after == before + _size_meta_data()) {
        std::cout << "Meta Data Tracking: Passed\n";
    }
    else {
        std::cout << "Meta Data Tracking: Failed\n";
    }

    return 0;
}
