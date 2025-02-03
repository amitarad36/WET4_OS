#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <size>\n", argv[0]);
        return 1;
    }

    int size = atoi(argv[1]);
    if (size <= 0) {
        fprintf(stderr, "Error: Size must be a positive integer\n");
        return 1;
    }

    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }

    printf("Allocated %d bytes at %p\n", size, ptr);
    free(ptr);

    return 0;
}
