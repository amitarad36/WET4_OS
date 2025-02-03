#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    unsigned int x = atoi(argv[1]);
    void* memory = malloc(x);
    free(memory);
    return 0;
}