#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    // printf("memstart = %p\n", sf_mem_start());
    sf_mem_init();

    // void *x = sf_malloc(sizeof(double) * 8);
  	// void *y = sf_realloc(x, sizeof(int));

    // printf("%p\n", y);
    // sf_show_blocks();
    // sf_show_free_lists();

    //
    sf_mem_fini();

    return EXIT_SUCCESS;
}
