#include "__correctness_helpers.h"

static sf_header *epilogue() {
    sf_header *epilogue = (sf_header *)((char *)sf_mem_end() - sizeof(sf_epilogue));
    return epilogue;
}

static bool free_list_is_empty(sf_free_list_node *fnp) {
    return (fnp->head.links.next == &fnp->head &&
	    fnp->head.links.prev == &fnp->head);
}

static int free_list_count(sf_header *ahp) {
    int count = 0;
    sf_header *hp = ahp->links.next;
    while(hp != ahp) {
	count++;
	hp = hp->links.next;
    }
    return count;
}

static sf_free_list_node *find_free_list_for_size(size_t size) {
    sf_free_list_node *fnp = sf_free_list_head.next;
    int limit = LOOP_LIMIT;
    while(fnp != &sf_free_list_head && fnp->size < size && limit--)
        fnp = fnp->next;
    // Search for first freelist with something in it.
    limit = LOOP_LIMIT;
    while(fnp != &sf_free_list_head && limit--) {
        if(!free_list_is_empty(fnp))
            break;
        fnp = fnp->next;
    }
    if(fnp == &sf_free_list_head)
	return NULL;
    return fnp;
}

static bool block_is_in_free_list(sf_header *ahp) {
    sf_free_list_node *fnp = find_free_list_for_size(SIZE_FROM_HEADER(ahp));
    if(fnp == NULL)
	return false;
    sf_header *hp = fnp->head.links.next;
    int limit = LOOP_LIMIT;
    while(hp != &fnp->head && limit--) {
	if(hp == ahp)
	    return true;
    }
    return false;
}

void assert_free_list_is_empty(sf_free_list_node *fnp) {
    cr_assert(free_list_is_empty(fnp),
	      "Free list %p (size %u) is not empty", fnp, fnp->size);
}

void assert_free_list_is_valid(sf_free_list_node *fnp, size_t size) {
    int limit = LOOP_LIMIT;
    sf_header *hp = fnp->head.links.next;
    while(hp != &fnp->head && limit--) {
	cr_assert_eq(SIZE_FROM_HEADER(hp), size,
		     "Block at %p is in wrong free list (size=%lu, exp=%lu)",
		     SIZE_FROM_HEADER(hp), size);
    }
    cr_assert(limit != 0, "Bad free list for size %lu", size);
}

void assert_free_lists_are_valid() {
    sf_free_list_node *fnp = sf_free_list_head.next;
    int limit = LOOP_LIMIT;
    while(fnp != &sf_free_list_head && limit--) {
	assert_free_list_is_valid(fnp, fnp->size);
        fnp = fnp->next;
    }
    cr_assert(limit != 0, "Bad list of free lists");
}

void assert_block_is_valid(sf_header *hp) {
    cr_assert(ALIGNED_PAYLOAD(&hp->payload),
	      "Payload for block %p is not properly aligned", hp);

    cr_assert(VALID_BLOCK_SIZE(hp),
	      "Block size is not correctly set for %p. Got: %d",
	      hp, SIZE_FROM_HEADER(hp));

    if(!(NEXT_BLOCK_HEADER(hp) == epilogue() || VALID_NEXT_BLOCK_PREV_ALLOC(hp)))
	sf_show_heap();
    cr_assert(NEXT_BLOCK_HEADER(hp) == epilogue() || VALID_NEXT_BLOCK_PREV_ALLOC(hp),
	      "Prev allocated bit is not correctly set for %p. Should be: %d",
	      NEXT_BLOCK_HEADER(hp), hp->info.allocated);

    if(!(NEXT_BLOCK_HEADER(hp) == epilogue() ||
	 hp->info.allocated || NEXT_BLOCK_HEADER(hp)->info.allocated)) {
	printf("NEXT: %p, epilogue: %p\n", NEXT_BLOCK_HEADER(hp), epilogue());
	sf_show_heap();
    }
    cr_assert(NEXT_BLOCK_HEADER(hp) == epilogue() ||
	      hp->info.allocated || NEXT_BLOCK_HEADER(hp)->info.allocated,
	      "Uncoalesced adjacent free blocks %p and %p",
	      hp, NEXT_BLOCK_HEADER(hp));

    if (hp->info.allocated) {
/*
	cr_assert(VALID_REQ_SIZE(hp, hp->info.requested_size),
		  "Requested size is not correctly set for %p. Got: %d with block size %d",
		  hp, hp->info.requested_size, SIZE_FROM_HEADER(hp));
*/
	cr_assert(!block_is_in_free_list(hp),
		  "Allocated block at %p is also in a free list", hp);
    } else {
	cr_assert(VALID_FOOTER(hp), "block's footer does not match header for %p", hp);
	cr_assert(block_is_in_free_list(hp),
		  "Free block at %p is not contained in a free list", hp);
    }
}

void assert_prologue_is_valid(void) {
    sf_prologue *prologue = (sf_prologue *)sf_mem_start();

    // We are being somewhat loose about checking the prologue.
    // Examples given in the assignment handout and statements on Piazza might have
    // misled students about whether the block size and allocated fields need to be set.

    //assert_block_is_valid(&prologue->header);
    //cr_assert_eq(SIZE_FROM_HEADER(&prologue->header), MIN_BLOCK_SIZE,
    //		 "Prologue has incorrect size");

    sf_header *hp = &prologue->header;
    cr_assert(hp->info.allocated, "Prologue is not marked allocated");
    cr_assert(VALID_FOOTER(hp), "Prologue's footer does not match header for %p", hp);

    // check that the first block has its prev_allocated bit set to 1
    cr_assert(VALID_NEXT_BLOCK_PREV_ALLOC(&prologue->header),
	      "Prev allocated bit for first block is not set correctly");
}

void assert_epilogue_is_valid(void) {
    sf_epilogue *ep = (sf_epilogue *)epilogue();

    cr_assert(ep->footer.info.allocated,
	      "Epilogue is not marked allocated");
}

void assert_heap_is_valid(void) {
    char *heap_p = sf_mem_start(), *end_heap = sf_mem_end();

    // check if heap is empty then free list must be empty as well
    if (heap_p == end_heap) {
	cr_assert(&sf_free_list_head == sf_free_list_head.next &&
                  &sf_free_list_head == sf_free_list_head.prev,
		  "The heap is empty, but the list of free lists is not");
    }

    // Correctness of the prologue is checked separately, because
    // it seems that many students didn't set the allocated bit or the block size
    // for the prologue, or even the prev_allocated field for the next block,
    // so asserting those here would cause most assignments to fail all tests.

    heap_p += sizeof(sf_prologue);  // Ignore the size field here -- we know the size.
    char *stop = end_heap - sizeof(sf_epilogue);
    sf_header *block;
    int limit = LOOP_LIMIT;
    while(heap_p < stop && limit--) {
        block = (sf_header *)heap_p;
	assert_block_is_valid(block);
        heap_p += SIZE_FROM_HEADER(block);
    }

    // check if epilogue is correctly set
    sf_epilogue *epilogue = (sf_epilogue *)stop;
    cr_assert((char *)heap_p == (char *)epilogue,
	      "Heap blocks end before epilogue is reached");
    //cr_assert(epilogue->footer.info.allocated,
    //	      "Epilogue is not marked allocated");

    // check that the free lists contain blocks of their declared size
    assert_free_lists_are_valid();
}

void assert_block_info(sf_header *hp, int alloc, size_t b_size, size_t r_size) {
    cr_assert(hp->info.allocated == alloc,
	      "Block %p has wrong allocation status (got %d, expected %d)",
	      hp, hp->info.allocated, alloc);
    cr_assert(SIZE_FROM_HEADER(hp) == b_size,
	      "Block %p has wrong block_size (got %u, expected %u)",
	      hp, SIZE_FROM_HEADER(hp), b_size);
/*
    if(hp->info.allocated) {
	cr_assert(hp->info.requested_size == r_size,
		  "Block %p has wrong requested_size (got %u, expected %u)",
		  hp, hp->info.requested_size, r_size);
    }
 */
}

void assert_nonnull_payload_pointer(void *pp) {
    cr_assert(pp != NULL, "Payload pointer should not be NULL");
}

void assert_null_payload_pointer(void *pp) {
    cr_assert(pp == NULL, "Payload pointer should be NULL");
}

void assert_free_list_count(size_t size, int count) {
    sf_free_list_node *fnp = sf_free_list_head.next;
    int limit = LOOP_LIMIT;
    while(fnp != &sf_free_list_head && limit--) {
	if(fnp->size == size)
	    break;
	fnp = fnp->next;
    }
    cr_assert(fnp != &sf_free_list_head && fnp->size == size,
	      "No free list of size %lu was found", size);
    int flc = free_list_count(&fnp->head);
    cr_assert_eq(flc, count,
		 "Wrong number of blocks in free list for size %lu (exp=%d, found=%d)",
		 size, flc);
}

void assert_free_block_count(int count) {
    int n = 0;
    sf_free_list_node *fnp = sf_free_list_head.next;
    int limit = LOOP_LIMIT;
    while(fnp != &sf_free_list_head && limit--) {
	n += free_list_count(&fnp->head);
	fnp = fnp->next;
    }
    cr_assert_eq(n, count, "Wrong number of free blocks (exp=%d, found=%d)", count, n);
}

void assert_errno_eq(int n) {
    cr_assert_eq(sf_errno, n, "sf_errno has incorrect value (value=%d, exp=%d)", sf_errno, n);
}
