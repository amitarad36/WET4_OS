#include <iostream>
extern void* smalloc(size_t size);
extern size_t _num_meta_data_bytes();
extern size_t _size_meta_data();

int main() {
    size_t before = _num_meta_data_bytes();  // Ensure it's used in output
    void* p1 = smalloc(100);
    size_t after = _num_meta_data_bytes();

    std::cout << "Metadata Before: " << before << ", After: " << after << std::endl;

    if (after == before + _size_meta_data()) {
        std::cout << "Meta Data Tracking: Passed\n";
    }
    else {
        std::cout << "Meta Data Tracking: Failed\n";
    }

    return 0;
}
