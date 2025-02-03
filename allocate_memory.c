#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
    dup(0);
    void* memory = malloc(atoi(argv[1]));
    dup(0);
    free(memory);
}