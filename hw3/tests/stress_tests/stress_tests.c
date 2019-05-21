#include "__stress_helpers.h"

// random block assigments. Tried to give equal opportunity for each possible order to appear.
// But if the heap gets populated too quickly, try to make some space by realloc(half) existing
// allocated blocks.
Test(sf_memsuite_grading, stress_test, .init = sf_mem_init, .fini = sf_mem_fini, .timeout = 5) {
    errno = 0;

    int order_range = 13;
    int nullcount = 0;

    void *tracked[100];

    for(int i = 0; i < 100; i++) {
        int order = (rand() % order_range);
	size_t extra = (rand() % (1 << order));
	size_t req_sz = (1 << order) + extra;

        tracked[i] = sf_malloc(req_sz);
        // if there is no free to malloc
        if(tracked[i] == NULL) {
            order--;
            while(order >= 0) {
		req_sz = (1 << order) + (extra % (1 << order));
                tracked[i] = sf_malloc(req_sz);
                if(tracked[i] != NULL)
                    break;
                else
                    order--;
            }
        }

        // tracked[i] can still be NULL
        if(tracked[i] == NULL) {
            nullcount++;
            // It seems like there is not enough space in the heap.
            // Try to halve the size of each existing allocated block in the heap,
            // so that next mallocs possibly get free blocks.
            for(int j = 0; j < i; j++) {
                if(tracked[j] == NULL)
                    continue;
                sf_header *hdr = PAYLOAD_TO_HEADER(tracked[j]);
                req_sz = hdr->info.block_size >> 1;
                tracked[j] = sf_realloc(tracked[j], req_sz);
            }
        }
        errno = 0;
    }

    for(int i = 0; i < 100; i++) {
        if(tracked[i] != NULL)
            sf_free(tracked[i]);
    }

    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(16336, 1);
}
