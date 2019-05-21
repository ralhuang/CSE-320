#include "__correctness_helpers.h"

/*
 * Do one malloc and check that the prologue and epilogue are correctly initialized.
 */
Test(sf_memsuite_grading, initialization, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 1;
    (void)sf_malloc(sz);
    assert_prologue_is_valid();
    assert_epilogue_is_valid();
    assert_heap_is_valid();
}

/*
 * Single malloc tests, up to the size that forces a non-minimum block size.
 */
Test(sf_memsuite_grading, single_malloc_1, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 1;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(4016, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_4, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 4;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(4016, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_8, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 8;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(4016, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_16, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 16;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(4016, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, single_malloc_32, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 32;
    void *x = sf_malloc(sz);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    assert_nonnull_payload_pointer(x);
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(4000, 1);
    assert_errno_eq(0);
}

/*
 * Single malloc test, of a size exactly equal to what is left after initialization.
 */
Test(sf_memsuite_grading, single_malloc_4040, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 4040;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    assert_heap_is_valid();
    assert_free_block_count(0);
    assert_errno_eq(0);
}

/*
 * Single malloc test, of a size just larger than what is left after initialization.
 */
Test(sf_memsuite_grading, single_malloc_4041, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 4041;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(4080, 1);
    assert_errno_eq(0);
}

/*
 * Single malloc test, of multiple pages.
 */
Test(sf_memsuite_grading, single_malloc_12000, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 12000;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(224, 1);
    assert_errno_eq(0);
}

/*
 * Single malloc test, unsatisfiable.
 * There should be left one single large block.
 */
Test(sf_memsuite_grading, single_malloc_16384, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 16384;
    void *x = sf_malloc(sz);
    assert_null_payload_pointer(x);
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(16336, 1);
    assert_errno_eq(ENOMEM);
}

/*
 * Malloc/free with/without coalescing.
 */

Test(sf_memsuite_grading, malloc_free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz1 = 100;
    size_t sz2 = 200;
    size_t sz3 = 300;
    void *x = sf_malloc(sz1);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    void *y = sf_malloc(sz2);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(sz2), sz2);
    void *z = sf_malloc(sz3);
    assert_nonnull_payload_pointer(z);
    assert_block_info(PAYLOAD_TO_HEADER(z), 1, ADJUSTED_BLOCK_SIZE(sz3), sz3);

    sf_free(y);

    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    assert_block_info(PAYLOAD_TO_HEADER(y), 0, ADJUSTED_BLOCK_SIZE(sz2), 0);
    assert_block_info(PAYLOAD_TO_HEADER(z), 1, ADJUSTED_BLOCK_SIZE(sz3), sz3);
    assert_heap_is_valid();
    assert_free_block_count(2);
    assert_free_list_count(208, 1);
    assert_free_list_count(3408, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_coalesce_lower, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz1 = 100;
    size_t sz2 = 200;
    size_t sz3 = 300;
    size_t sz4 = 400;
    void *x = sf_malloc(sz1);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    void *y = sf_malloc(sz2);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(sz2), sz2);
    void *z = sf_malloc(sz3);
    assert_nonnull_payload_pointer(z);
    assert_block_info(PAYLOAD_TO_HEADER(z), 1, ADJUSTED_BLOCK_SIZE(sz3), sz3);
    void *w = sf_malloc(sz4);
    assert_nonnull_payload_pointer(w);
    assert_block_info(PAYLOAD_TO_HEADER(w), 1, ADJUSTED_BLOCK_SIZE(sz4), sz4);

    sf_free(y);
    sf_free(z);
    size_t sz = ADJUSTED_BLOCK_SIZE(sz2) + ADJUSTED_BLOCK_SIZE(sz3);

    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    assert_block_info(PAYLOAD_TO_HEADER(y), 0, sz, 0);
    assert_block_info(PAYLOAD_TO_HEADER(w), 1, ADJUSTED_BLOCK_SIZE(sz4), sz4);
    assert_heap_is_valid();
    assert_free_block_count(2);
    assert_free_list_count(sz, 1);
    assert_free_list_count(2992, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_coalesce_upper, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz1 = 100;
    size_t sz2 = 200;
    size_t sz3 = 300;
    size_t sz4 = 400;
    void *x = sf_malloc(sz1);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    void *y = sf_malloc(sz2);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(sz2), sz2);
    void *z = sf_malloc(sz3);
    assert_nonnull_payload_pointer(z);
    assert_block_info(PAYLOAD_TO_HEADER(z), 1, ADJUSTED_BLOCK_SIZE(sz3), sz3);
    void *w = sf_malloc(sz4);
    assert_nonnull_payload_pointer(w);
    assert_block_info(PAYLOAD_TO_HEADER(w), 1, ADJUSTED_BLOCK_SIZE(sz4), sz4);

    sf_free(z);
    sf_free(y);
    size_t sz = ADJUSTED_BLOCK_SIZE(sz2) + ADJUSTED_BLOCK_SIZE(sz3);

    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    assert_block_info(PAYLOAD_TO_HEADER(y), 0, sz, 0);
    assert_block_info(PAYLOAD_TO_HEADER(w), 1, ADJUSTED_BLOCK_SIZE(sz4), sz4);
    assert_heap_is_valid();
    assert_free_block_count(2);
    assert_free_list_count(sz, 1);
    assert_free_list_count(2992, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_coalesce_both, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz1 = 100;
    size_t sz2 = 200;
    size_t sz3 = 300;
    size_t sz4 = 400;
    void *x = sf_malloc(sz1);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    void *y = sf_malloc(sz2);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(sz2), sz2);
    void *z = sf_malloc(sz3);
    assert_nonnull_payload_pointer(z);
    assert_block_info(PAYLOAD_TO_HEADER(z), 1, ADJUSTED_BLOCK_SIZE(sz3), sz3);
    void *w = sf_malloc(sz4);
    assert_nonnull_payload_pointer(w);
    assert_block_info(PAYLOAD_TO_HEADER(w), 1, ADJUSTED_BLOCK_SIZE(sz4), sz4);

    sf_free(x);
    sf_free(z);
    sf_free(y);
    size_t sz = ADJUSTED_BLOCK_SIZE(sz1) + ADJUSTED_BLOCK_SIZE(sz2) + ADJUSTED_BLOCK_SIZE(sz3);

    assert_block_info(PAYLOAD_TO_HEADER(x), 0, sz, 0);
    assert_heap_is_valid();
    assert_free_block_count(2);
    assert_free_list_count(sz, 1);
    assert_free_list_count(2992, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_first_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz1 = 100;
    size_t sz2 = 200;
    void *x = sf_malloc(sz1);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    void *y = sf_malloc(sz2);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(sz2), sz2);

    sf_free(x);

    assert_block_info(PAYLOAD_TO_HEADER(x), 0, ADJUSTED_BLOCK_SIZE(sz1), 0);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(sz2), sz2);
    assert_heap_is_valid();
    assert_free_block_count(2);
    assert_free_list_count(ADJUSTED_BLOCK_SIZE(sz1), 1);
    assert_free_list_count(3728, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, malloc_free_last_block, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz1 = 100;
    size_t sz2 = 3928;
    void *x = sf_malloc(sz1);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    void *y = sf_malloc(sz2);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(sz2), sz2);

    sf_free(y);

    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz1), sz1);
    assert_block_info(PAYLOAD_TO_HEADER(y), 0, ADJUSTED_BLOCK_SIZE(sz2), 0);
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(3936, 1);
    assert_errno_eq(0);
}

/*
 * Check that malloc leaves no splinter.
 */

Test(sf_memsuite_grading, malloc_with_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 4039;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    assert_heap_is_valid();
    assert_free_block_count(0);
    assert_errno_eq(0);
}

/*
 *  Allocate small blocks until memory exhausted.
 */
Test(sf_memsuite_grading, malloc_to_exhaustion, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 100;
    int limit = 200;
    void *x;
    while((x = sf_malloc(sz)) != NULL && limit--) {
	sf_header *hp = PAYLOAD_TO_HEADER(x);
	size_t size = SIZE_FROM_HEADER(hp);
	// Not all blocks will be the same size due to splitting restrictions.
	cr_assert(size == ADJUSTED_BLOCK_SIZE(sz) ||
		  size == (ADJUSTED_BLOCK_SIZE(sz) + (1 << ALIGNMENT_SHIFT)),
		  "block has incorrect size (size=%lu, exp=%lu or %lu)",
		  size, ADJUSTED_BLOCK_SIZE(sz),
                  ADJUSTED_BLOCK_SIZE(sz) + (1 << ALIGNMENT_SHIFT));
    }
    cr_assert_eq(limit, 55, "Memory not exhausted when it should be");
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(64, 1);
    assert_errno_eq(ENOMEM);
}

/*
 * Check FIFO discipline on malloc/free.
 */

Test(sf_memsuite_grading, malloc_free_fifo, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 100;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *u = sf_malloc(sz);
    assert_nonnull_payload_pointer(u);
    assert_block_info(PAYLOAD_TO_HEADER(u), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *y = sf_malloc(sz);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *v = sf_malloc(sz);
    assert_nonnull_payload_pointer(v);
    assert_block_info(PAYLOAD_TO_HEADER(v), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *z = sf_malloc(sz);
    assert_nonnull_payload_pointer(z);
    assert_block_info(PAYLOAD_TO_HEADER(z), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *w = sf_malloc(sz);
    assert_nonnull_payload_pointer(w);
    assert_block_info(PAYLOAD_TO_HEADER(w), 1, ADJUSTED_BLOCK_SIZE(sz), sz);

    sf_free(x);
    sf_free(y);
    sf_free(z);

    void *z1 = sf_malloc(sz);
    assert_nonnull_payload_pointer(z1);
    assert_block_info(PAYLOAD_TO_HEADER(z1), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *y1 = sf_malloc(sz);
    assert_nonnull_payload_pointer(y1);
    assert_block_info(PAYLOAD_TO_HEADER(y1), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *x1 = sf_malloc(sz);
    assert_nonnull_payload_pointer(x1);
    assert_block_info(PAYLOAD_TO_HEADER(x1), 1, ADJUSTED_BLOCK_SIZE(sz), sz);

    cr_assert(x == x1 && y == y1 && z == z1,
	      "malloc/free does not follow FIFO discipline");

    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(3376, 1);
    assert_errno_eq(0);
}

/*
 * Realloc tests.
 */

Test(sf_memsuite_grading, realloc_larger, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 1;
    size_t nsz = 1024;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *y = sf_realloc(x, nsz);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(nsz), nsz);
    assert_heap_is_valid();
    assert_free_block_count(2);
    assert_free_list_count(32, 1);
    assert_free_list_count(2976, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_smaller, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 1024;
    size_t nsz = 1;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *y = sf_realloc(x, nsz);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(nsz), nsz);
    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(4016, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_same, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 1024;
    size_t nsz = 1024;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *y = sf_realloc(x, nsz);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(nsz), nsz);
    cr_assert_eq(x, y, "realloc to same size did not return same payload pointer");
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(3008, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 1024;
    size_t nsz = 1020;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *y = sf_realloc(x, nsz);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(nsz), nsz);
    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");
    assert_heap_is_valid();
    assert_free_block_count(1);
    assert_free_list_count(3008, 1);
    assert_errno_eq(0);
}

Test(sf_memsuite_grading, realloc_size_0, .init = sf_mem_init, .fini = sf_mem_fini) {
    size_t sz = 1024;
    void *x = sf_malloc(sz);
    assert_nonnull_payload_pointer(x);
    assert_block_info(PAYLOAD_TO_HEADER(x), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *y = sf_malloc(sz);
    assert_nonnull_payload_pointer(y);
    assert_block_info(PAYLOAD_TO_HEADER(y), 1, ADJUSTED_BLOCK_SIZE(sz), sz);
    void *z = sf_realloc(x, 0);
    assert_null_payload_pointer(z);
    assert_block_info(PAYLOAD_TO_HEADER(x), 0, ADJUSTED_BLOCK_SIZE(sz), 0);
    assert_heap_is_valid();
    assert_free_block_count(2);
    assert_free_list_count(1040, 1);
    assert_free_list_count(1968, 1);
    assert_errno_eq(0);
}

/*
 * Illegal pointer tests.
 */

Test(sf_memsuite_grading, free_null, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT, .timeout=5) {
    size_t sz = 1;
    (void)sf_malloc(sz);
    sf_free(NULL);
    cr_assert_fail("SIGABRT should have been received");
}

Test(sf_memsuite_grading, free_unallocated, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT, .timeout=5) {
    size_t sz = 1;
    void *x = sf_malloc(sz);
    sf_free(x);
    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

Test(sf_memsuite_grading, free_bad_requested_size, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT, .timeout=5) {
    size_t sz = 1;
    void *x = sf_malloc(sz);
    PAYLOAD_TO_HEADER(x)->info.requested_size = SIZE_FROM_HEADER(PAYLOAD_TO_HEADER(x));
    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

Test(sf_memsuite_grading, free_prev_alloc, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT, .timeout=5) {
    size_t sz = 1;
    void *x = sf_malloc(sz);
    PAYLOAD_TO_HEADER(x)->info.prev_allocated = 0;
    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}
