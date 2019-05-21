#ifndef SFMM_TEST_H
#define SFMM_TEST_H

#include <criterion/criterion.h>
#include "sfmm.h"
#include <signal.h>
#include <stdio.h>
#include <errno.h>

#define ALIGNMENT_SHIFT 4
#define ALIGNMENT (0x1 << ALIGNMENT_SHIFT)
#define ALIGNMENT_MASK (ALIGNMENT-1)
#define ALIGNED_SIZE(size) (((size) & ALIGNMENT_MASK) == 0)
#define ALIGNED_PAYLOAD(pp) ((((uint64_t)pp) & (ALIGNMENT_MASK)) == 0)
#define ALIGN_SIZE(size) ((((size) + ALIGNMENT-1) >> ALIGNMENT_SHIFT) << ALIGNMENT_SHIFT)

#define SIZE_FROM_INFO(ip) ((ip)->block_size<<4)
#define SIZE_FROM_HEADER(hp) (SIZE_FROM_INFO(&(hp)->info))
#define PAYLOAD_TO_HEADER(pp) ((sf_header *)((char *)(pp) - sizeof(sf_block_info)))
#define FOOTER_FROM_HEADER(hp) ((sf_footer *)((char *)(hp) + SIZE_FROM_HEADER(hp) - sizeof(sf_footer)))
#define NEXT_BLOCK_HEADER(hp) ((sf_header *)((char *)(hp) + SIZE_FROM_HEADER(hp)))
#define NEXT_BLOCK_FOOTER(hp) (FOOTER_FROM_HEADER(NEXT_BLOCK_HEADER(hp)))
#define PREV_BLOCK_FOOTER(hp) ((sf_footer *)((char *)(hp) - sizeof(sf_footer)))
#define PREV_BLOCK_HEADER(hp) ((sf_header *)((char *)(hp) - (BLOCK_SIZE(PREV_BLOCK_FOOTER(hp)))))

#define max(x, y) ((x) > (y) ? (x) : (y))

#define ADJUSTED_BLOCK_SIZE(req)                                                    \
   (ALIGN_SIZE(max(sizeof(sf_block_info)+(req), sizeof(sf_header)+sizeof(sf_footer))))

#define MIN_BLOCK_SIZE (ADJUSTED_BLOCK_SIZE(1))

#define VALID_BLOCK_SIZE(hp)                                                        \
   (SIZE_FROM_HEADER(hp) >= MIN_BLOCK_SIZE &&                                       \
    SIZE_FROM_HEADER(hp) <= 100 * PAGE_SZ &&                                        \
    ALIGNED_SIZE(SIZE_FROM_HEADER(hp)))

#define VALID_REQ_SIZE(hp, req)                                                     \
    (((req) + sizeof(sf_block_info)) <= SIZE_FROM_HEADER(hp))

#define VALID_FOOTER(hp)                                                            \
    (FOOTER_FROM_HEADER(hp)->info.allocated == (hp)->info.allocated &&              \
     FOOTER_FROM_HEADER(hp)->info.prev_allocated == (hp)->info.prev_allocated &&    \
     FOOTER_FROM_HEADER(hp)->info.block_size == (hp)->info.block_size)

#define VALID_NEXT_BLOCK_PREV_ALLOC(hp)                                             \
    (NEXT_BLOCK_HEADER(hp)->info.prev_allocated == (hp)->info.allocated)

#define LOOP_LIMIT 1000

void assert_free_list_is_empty(sf_free_list_node *fnp);
void assert_free_list_is_valid(sf_free_list_node *fnp, size_t size);
void assert_free_lists_are_valid();
void assert_prologue_is_valid(void);
void assert_epilogue_is_valid(void);
void assert_block_is_valid(sf_header *hp);
void assert_heap_is_valid(void);
void assert_block_info(sf_header *hp, int alloc, size_t b_size, size_t r_size);
void assert_nonnull_payload_pointer(void *pp);
void assert_null_payload_pointer(void *pp);
void assert_free_list_count(size_t size, int count);
void assert_free_block_count(int count);
void assert_errno_eq(int n);

#endif
