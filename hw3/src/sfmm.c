/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "helperfunctions.h"
#include <errno.h>

void *sf_malloc(size_t size) {
  // printf("IN MALLOC\n");
  if(size == 0)
    return NULL;
  if(size < 0) {
    sf_errno = ENOMEM;
    return NULL;
  }

  if(sf_mem_start() == NULL)
    sf_mem_init();

  //if THERE IS NO HEAP/FREELIST OF FREEBLOCKS
  if(sf_mem_start() == sf_mem_end()) {
    sf_mem_grow();
    set_logues();
    generateFBfromFL();
  }
  int rqSize = get_blockSize(size);

  //TRAVERSING THROUGH LIST OF FREELISTS TO FIND A FREELIST OF RQSIZE
  sf_free_list_node* node_ptr = &(sf_free_list_head);
  node_ptr = node_ptr->next;
  while(node_ptr->size != 0 && node_ptr->size != rqSize) {
    node_ptr = node_ptr->next;
  }

  //IF THERE IS NO FREELIST OF THAT SIZE
  if(node_ptr->size == 0) {
    if(rqSize < node_ptr->prev->size) { //There is no freelist of that size but need to split
      return split(node_ptr, size);
    } else {     //There is no freelist of that size, but the size is too small

      sf_epilogue* oldEpilogue = ((sf_epilogue*)(sf_mem_end() - sizeof(sf_footer)));

      sf_block_info epi_block_info;
      epi_block_info = oldEpilogue->footer.info;
      // printf("epi_block_info size = %d\n", (epi_block_info.block_size << 4));

      while((int)(node_ptr->prev->size - rqSize) < 0) {
        // printf("node_ptr-prev->size - rqSize = %d\n", ((int)(node_ptr->prev->size) - rqSize));
        if(sf_mem_grow() == NULL) {
          sf_errno = ENOMEM;
          return NULL;
        }

        node_ptr->prev->size = node_ptr->prev->size + PAGE_SZ;
      }
      // printf("node_ptr-prev->size - rqSize = %d\n", ((int)(node_ptr->prev->size) - rqSize));
      if((int)(node_ptr->prev->size - rqSize) <= 32) {

        sf_header* tempNext = node_ptr->prev->head.links.next->links.next;
        sf_header* tempPrev = &node_ptr->prev->head;

        node_ptr->prev->head.links.next->info.allocated = 1;
        node_ptr->prev->head.links.next->info.requested_size = rqSize;
        node_ptr->prev->head.links.next->info.block_size = (((int)(node_ptr->prev->size)) >> 4);

        sf_footer* updateFooter = (sf_footer*) ((void*)node_ptr->prev->head.links.next +  (int)node_ptr->prev->size - sizeof(sf_footer));
        updateFooter->info.block_size = (((int)(node_ptr->prev->size)) >> 4);
        void* answer = &(node_ptr->prev->head.links.next->payload);

        tempNext->links.prev = tempPrev;
        tempPrev->links.next = tempNext;

        if((void*)tempNext == &node_ptr->prev->head && (void*)tempPrev == &node_ptr->prev->head) {
          node_ptr->prev->prev->next = node_ptr->prev->next;
          node_ptr->prev->next->prev = node_ptr->prev->prev;
        }
        return answer;
      }

      sf_epilogue* newEpilogue = ((sf_epilogue*)(sf_mem_end() - sizeof(sf_footer)));
      newEpilogue->footer.info = epi_block_info;

      return split(node_ptr, size);
    }
    sf_errno = ENOMEM;
    return NULL;
  }

  //THERE IS A FREELIST OF THAT SIZE
  else {
    node_ptr->prev->next = node_ptr->next;
    node_ptr->next->prev = node_ptr->prev;


    sf_block_info newInfo;
    newInfo.allocated = 1;
    newInfo.prev_allocated = node_ptr->head.links.next->info.prev_allocated;
    newInfo.two_zeroes = 0;
    newInfo.block_size = rqSize >> 4;
    newInfo.requested_size = size;

    if ((node_ptr->head.links.next) == &(node_ptr->head) && node_ptr->head.links.prev == &(node_ptr->head)) {
      node_ptr->next = NULL;
      node_ptr->prev = NULL;
    }
    node_ptr->head.links.next->info = newInfo;

    return (void*)(node_ptr->head.links.next->payload);

  }

  //Use a function to traverse freelist and obtain
  return NULL;
}

void sf_free(void *pp) {
  //payloadpointer of the block they want to free

  int checkfree = checkFree(pp);

  if(checkfree == 0) {
    abort();
  }

  sf_header* freeHeader = (pp - sizeof(sf_block_info));
  sf_footer* freeFooter = (pp + (freeHeader->info.block_size << 4) - 2*sizeof(sf_footer));

  sf_header* nextHeader = (pp - sizeof(sf_block_info) + (freeHeader->info.block_size << 4));
  sf_footer* nextFooter = ((void*)nextHeader + (nextHeader->info.block_size << 4) - sizeof(sf_footer));

  //surrounding blocks are allocated
  // if (freeHeader->info.prev_allocated == 1 && nextHeader->info.allocated == 1) {
    //new block info for the now freed block

    sf_block_info newInfo;
    newInfo.allocated = 0;
    newInfo.prev_allocated = freeHeader->info.prev_allocated;
    newInfo.two_zeroes = 0;
    newInfo.block_size = freeHeader->info.block_size;
    newInfo.requested_size = 0;
    freeHeader->info = newInfo;
    freeFooter->info = newInfo;

    //change the blockinfo for the next block
    nextHeader->info.prev_allocated = 0;
    // printf("nextHeader info prevallocated = %d\n", nextHeader->info.prev_allocated);
    nextFooter->info.prev_allocated = 0;

    sf_free_list_node* node_ptr = (sf_free_list_head.next);
    while((node_ptr->size != 0) && (node_ptr->size != freeHeader->info.block_size << 4)) {
      node_ptr = node_ptr->next;
    }

    //no free list of free block sizea
    if(node_ptr->size == 0) {

      //find where the freelist belongs before inserting
      while((newInfo.block_size << 4)> node_ptr->size) {
        if(node_ptr->size == 0 && node_ptr->next->size == 0)
          break;
        node_ptr = node_ptr->next;
      }

      sf_free_list_node* prevNode = node_ptr->prev;
      sf_free_list_node* new_node_ptr = sf_add_free_list((size_t)(freeHeader->info.block_size << 4), node_ptr);
      prevNode->next = new_node_ptr;
      new_node_ptr->prev = prevNode;

      new_node_ptr->head.links.next = freeHeader;
      freeHeader->links.next = &(new_node_ptr->head);
      freeHeader->links.prev = &(new_node_ptr->head);
      new_node_ptr->head.links.prev = freeHeader;
    } else {
      freeHeader->links.next = node_ptr->head.links.next;
      freeHeader->links.prev = node_ptr->head.links.prev->links.next;
      node_ptr->head.links.next->links.prev = freeHeader;
      node_ptr->head.links.next = freeHeader;
    }

  // } else { //there is an adjacent free block, need to coalsce
    coalesce((void*)freeHeader);
  // }
  return;
}

void *sf_realloc(void *pp, size_t rsize) {

  //VALIDATE ARGS
  int valid = checkFree(pp);
  if(valid == 0) {
    sf_errno = EINVAL;
    return NULL;
  } else {
    if (rsize == 0)
      return NULL;
  }

  sf_header* currentHeader = pp - sizeof(sf_block_info);
  int realSize = get_blockSize((int)rsize);
  //REALLOC TO BIGGER SIZE
  if(realSize > (currentHeader->info.block_size << 4)) {
    void* ptr = sf_malloc(rsize);
    if (ptr == NULL) {
      return NULL;
    }
    memcpy(ptr, pp, currentHeader->info.requested_size);

    sf_free(pp);
    return ptr;
  } else if (realSize < (currentHeader->info.block_size << 4)){
    //REALLOC TO SMALLER SIZE

    //case splinter
    if((currentHeader->info.block_size << 4) - realSize < 32) {
      currentHeader->info.requested_size = rsize;
      return pp;
    } else { //case no splinter

      sf_header* newHeader = (sf_header*)((void*)currentHeader + realSize);
      sf_block_info newInfo;
      newInfo.allocated = 0;
      newInfo.prev_allocated = 1;
      newInfo.two_zeroes = 0;
      newInfo.block_size = ((currentHeader->info.block_size << 4) - realSize) >> 4;

      newHeader->info.requested_size = 0;
      newHeader->info = newInfo;

      currentHeader->info.requested_size = rsize;
      currentHeader->info.block_size = realSize >> 4;

      sf_footer* newFooter = (sf_footer*) ((void*)newHeader + (newInfo.block_size << 4) - sizeof(sf_footer));
      newFooter->info = newInfo;

      sf_free_list_node* node_ptr = (sf_free_list_head.next);
      while(node_ptr->size != (newHeader->info.block_size << 4) && node_ptr->size != 0) {
        node_ptr = node_ptr->next;
      }

      if(node_ptr->size == 0) {
        while(node_ptr->size < (newHeader->info.block_size << 4)) {
          if(node_ptr->next->size == 0 && (node_ptr->size < (newHeader->info.block_size << 4))) {
            break;
          }
          node_ptr = node_ptr->next;
        }

        sf_free_list_node* new_node_ptr = sf_add_free_list(newHeader->info.block_size << 4, node_ptr);
        new_node_ptr->head.links.next = newHeader;
        newHeader->links.next = &new_node_ptr->head;
        newHeader->links.prev = &new_node_ptr->head;
        new_node_ptr->head.links.prev = newHeader;
        // sf_show_blocks();
        // sf_show_free_lists();
      }

      sf_header* nextHeader = (sf_header*)((void*)newHeader + (newInfo.block_size << 4));
      if(nextHeader->info.allocated == 0) { //need to coalesce
        coalesce(newHeader);
        return pp;
      } else { //just need to update prev allocated
        nextHeader->info.prev_allocated = 1;
        return pp;
      }

    }

  } else {
    return pp;
  }
    return NULL;
}

int set_logues() {
  sf_prologue pl;
  sf_epilogue el;
  sf_prologue* plp = &pl;
  sf_epilogue* elp = &el;

  sf_block_info hfInfo;
  hfInfo.allocated = 1;
  hfInfo.prev_allocated = 0;
  hfInfo.two_zeroes = 0;
  hfInfo.block_size = 0;
  hfInfo.requested_size = 0;
  plp->padding = 0;

  sf_header* plheaderp = &(plp->header);
  sf_footer* plfooterp = &(plp->footer);
  plheaderp->info = hfInfo;
  plheaderp->links.next = 0;
  plheaderp->links.prev = 0;
  plfooterp->info = hfInfo;

  sf_footer* elfooterp = &(elp->footer);
  elfooterp->info = hfInfo;

  *((sf_prologue*)((void*)(sf_mem_start()))) = pl;
  *((sf_epilogue*)(sf_mem_end() - sizeof(sf_footer))) = el;
  return 1;
}

void generateFBfromFL() {
  sf_header fbHeader;
  sf_footer fbFooter;

  sf_free_list_node* firstBlockPointer = sf_add_free_list(((sf_mem_end() - sf_mem_start()) - sizeof(sf_prologue) - sizeof(sf_epilogue)), &(sf_free_list_head));

  //block info creation
  sf_block_info fbInfo;
  fbInfo.allocated = 0;
  fbInfo.two_zeroes = 0;
  fbInfo.prev_allocated = 1;
  fbInfo.block_size = ((sf_mem_end() - sf_mem_start()) - sizeof(sf_prologue) - sizeof(sf_epilogue)) >> 4;
  fbInfo.requested_size = 0;

  //header and footer locations
  fbHeader.info = fbInfo;
  (fbHeader.links.prev) = &(firstBlockPointer->head);
  (fbHeader.links.next) = &(firstBlockPointer->head);
  firstBlockPointer->head.links.next = ((sf_header*)(sf_mem_start() + sizeof(sf_prologue)));
  firstBlockPointer->head.links.prev = ((sf_header*)(sf_mem_start() + sizeof(sf_prologue)));
  fbFooter.info = fbInfo;

  *((sf_header*)(sf_mem_start() + sizeof(sf_prologue))) = fbHeader;
  *((sf_footer*)(sf_mem_end() - (2 * sizeof(sf_footer)))) = fbFooter;
}

int get_blockSize(int size) {
  if ((8 + size) > 32)
    if ((8 + size) % 16 == 0)
      return 8 + size;
    else
      return 8 + size + 16 - ((8 + size) % 16);
  else
    return 32;
}

void* split(sf_free_list_node* node_ptr, int rqsize) {

  int rqSize = get_blockSize(rqsize);

  //SPLIT THE FREEBLOCK
  sf_header newHeader;
  sf_footer newFooter;

  //update header of old free block
  sf_block_info prevInfo;
  prevInfo.allocated = 1;

  if(node_ptr->prev->size == 0) {
    prevInfo.prev_allocated = 0;
  } else {
    prevInfo.prev_allocated = node_ptr->prev->head.links.next->info.prev_allocated;
  }

  prevInfo.two_zeroes = 0;
  prevInfo.block_size = rqSize >> 4;
  prevInfo.requested_size = rqsize;

  //update footer of old free block
  void* temp = (node_ptr->prev->head.links.next);
  ((sf_footer*)(temp + node_ptr->prev->size - sizeof(sf_footer)))->info.block_size = (node_ptr->prev->size - rqSize) >> 4;
  ((sf_footer*)(temp + node_ptr->prev->size - sizeof(sf_footer)))->info.prev_allocated = 1;
  node_ptr->prev->head.links.next->info = prevInfo;

  int newSize = (node_ptr->prev->size - rqSize);
  //create new freelist for freeblock
  sf_free_list_node* nodeSizeChecker = &sf_free_list_head;
  nodeSizeChecker = nodeSizeChecker->next;

  //check the correct size
  while(nodeSizeChecker->size != 0 && nodeSizeChecker->size != newSize) {
    nodeSizeChecker = nodeSizeChecker->next;
  }

  sf_free_list_node* new_node_ptr;
  //IF THE SPLIT BLOCK NEEDS A NEW FREELIST OF THAT SIZE
  if(nodeSizeChecker->size == 0) {
    nodeSizeChecker = nodeSizeChecker->next;
    while(nodeSizeChecker->size < newSize && nodeSizeChecker->size != 0) {
      nodeSizeChecker = nodeSizeChecker->next;
    }

    new_node_ptr = sf_add_free_list(newSize, nodeSizeChecker->next);
    new_node_ptr->prev = nodeSizeChecker->prev;
    nodeSizeChecker->prev->next = new_node_ptr;

    //set new block info for new footer/header
    sf_block_info newInfo;
    newInfo.allocated = 0;
    newInfo.two_zeroes = 0;
    newInfo.prev_allocated = 1;
    newInfo.block_size = newSize >> 4;
    newInfo.requested_size = 0;

    //update links
    newHeader.info = newInfo;
    (newHeader.links.prev) = &(new_node_ptr->head);
    (newHeader.links.next) = &(new_node_ptr->head);
    new_node_ptr->head.links.next = ((sf_header*)(temp + rqSize));
    new_node_ptr->head.links.prev = ((sf_header*)(temp + rqSize));
    newFooter.info = newInfo;

    //set new header for the free block
    //set new footer for the free block
    *((sf_header*)((void*)(temp + rqSize))) = newHeader;
    *((sf_footer*)((void*)(temp + rqSize + new_node_ptr->size - sizeof(sf_footer)))) = newFooter;

  } else {  //the split block already has a freelist of that size

    new_node_ptr = nodeSizeChecker; //new_node_ptr is the freelist node of the correct size

    //set new block info for new footer/header
    sf_block_info newInfo;
    newInfo.allocated = 0;
    newInfo.two_zeroes = 0;
    newInfo.prev_allocated = 1;
    newInfo.block_size = newSize >> 4;
    newInfo.requested_size = 0;

    //UPDATE LINKS
    newHeader.info = newInfo;
    new_node_ptr->head.links.prev->links.next = ((sf_header*)(temp + rqSize));
    new_node_ptr->head.links.prev = ((sf_header*)(temp + rqSize));
    newFooter.info = newInfo;

    *((sf_header*)((void*)(temp + rqSize))) = newHeader;
    *((sf_footer*)((void*)(temp + rqSize + nodeSizeChecker->size - sizeof(sf_footer)))) = newFooter;

  }
  void* answer = temp + 8;
  return answer;
}

int coalesce(void* freeBlockHeader) {
  //CHECK FOR ADJACENT BLOCKS

  sf_header* nextHeader = (freeBlockHeader + (((sf_header*) freeBlockHeader)->info.block_size << 4));
  sf_footer* nextFooter = ((void*)nextHeader + (nextHeader->info.block_size << 4) - sizeof(sf_footer));

  if(((sf_header*)freeBlockHeader)->info.prev_allocated == 0) {
    sf_footer* prevBlockFooter = freeBlockHeader - sizeof(sf_footer);
    sf_header* prevBlockHeader = freeBlockHeader - (prevBlockFooter->info.block_size << 4);
    sf_free_list_node* node_ptr = &sf_free_list_head;
    while(node_ptr->size != (prevBlockFooter->info.block_size << 4)) {
      node_ptr = node_ptr->next;
    }

    //search for prevBlockHeader in specific freelist of prev size, indicated by node_ptr
    sf_header* header_ptr = &node_ptr->head;
    while((void*)header_ptr != (void*)prevBlockHeader) {
      header_ptr = header_ptr->links.next;
    }

    sf_header* tempNext = header_ptr->links.next;
    sf_header* tempPrev = header_ptr->links.prev;

    //updating links between prev and next
    tempNext->links.prev = tempPrev;
    tempPrev->links.next = tempNext;

    //update freelists
    if((void*)tempNext == &node_ptr->head && (void*)tempPrev == &node_ptr->head) {
      node_ptr->prev->next = node_ptr->next;
      node_ptr->next->prev = node_ptr->prev;
    }

    //update header_ptr's header and footer
    header_ptr->info.block_size = (((header_ptr->info.block_size << 4) + (((sf_header*)freeBlockHeader)->info.block_size << 4)) >> 4);
    ((sf_footer*) (freeBlockHeader + (((sf_header*)freeBlockHeader)->info.block_size << 4) - sizeof(sf_footer)))->info.block_size = header_ptr->info.block_size;
    ((sf_footer*) (freeBlockHeader + (((sf_header*)freeBlockHeader)->info.block_size << 4) - sizeof(sf_footer)))->info.prev_allocated = header_ptr->info.prev_allocated;

    //search freelists for freelist node of new blocksize, otherwise add new freelist of the new blocksize;
    node_ptr = sf_free_list_head.next;
    while(node_ptr->size != (header_ptr->info.block_size << 4) && node_ptr->size != 0) {
      node_ptr = node_ptr->next;
    }

    //add a new freelist of new freeblocksize
    if(node_ptr->size == 0) {
      while(node_ptr->size < (header_ptr->info.block_size << 4)) {
        if(node_ptr->next->size == 0 && (node_ptr->size < (header_ptr->info.block_size << 4))) {
          break;
        }
        node_ptr = node_ptr->next;
      }

      sf_free_list_node* new_node_ptr = sf_add_free_list((header_ptr->info.block_size << 4), node_ptr);
      new_node_ptr->head.links.next = header_ptr;
      header_ptr->links.next = &new_node_ptr->head;
      header_ptr->links.prev = &new_node_ptr->head;
      new_node_ptr->head.links.prev = header_ptr;
    } else {  //insert block into freelist idicated by node_ptr

      header_ptr->links.next = node_ptr->head.links.next;
      node_ptr->head.links.next->links.prev = header_ptr;
      node_ptr->head.links.next = header_ptr;
      header_ptr->links.prev = &node_ptr->head;

    }
    nextHeader->info.prev_allocated = 0;
    if(nextHeader->info.allocated == 0) {
      nextFooter->info.prev_allocated = 0;
    }

    //do the same for freelist referring to currentBlockHeader (freeBlockHeader)
    sf_header* currentBlockHeader = (sf_header*)freeBlockHeader;

    node_ptr = &sf_free_list_head;
    while(node_ptr->size != (currentBlockHeader->info.block_size << 4)) {
      node_ptr = node_ptr->next;
    }

    //search for currentBlockHeader in specific freelist of size indicated by new_node_ptr
    header_ptr = &node_ptr->head;
    while((void*)header_ptr != (void*)currentBlockHeader) {
      header_ptr = header_ptr->links.next;
    }

    tempNext = header_ptr->links.next;
    tempPrev = header_ptr->links.prev;

    //updating links between prev and next
    tempNext->links.prev = tempPrev;
    tempPrev->links.next = tempNext;

    //update freelists
    if((void*)tempNext == &node_ptr->head && (void*)tempPrev == &node_ptr->head) {
      node_ptr->prev->next = node_ptr->next;
      node_ptr->next->prev = node_ptr->prev;
    }

  }

  if(nextHeader->info.allocated == 0) {
    sf_footer* nextFooter = (void*)nextHeader + (nextHeader->info.block_size << 4) - sizeof(sf_footer);
    sf_footer* prevBlockFooter = (void*)nextHeader - sizeof(sf_footer);
    sf_header* prevBlockHeader = (void*)nextHeader - (prevBlockFooter->info.block_size << 4);
    sf_free_list_node* node_ptr = &sf_free_list_head;

    //search for freelist referring to prevBlockHeader
    while(node_ptr->size != (prevBlockFooter->info.block_size << 4)) {
      node_ptr = node_ptr->next;
    }

    sf_header* header_ptr = &node_ptr->head;
    while((void*)header_ptr != (void*)prevBlockHeader) {
      header_ptr = header_ptr->links.next;
    }

    sf_header* tempNext = header_ptr->links.next;
    sf_header* tempPrev = header_ptr->links.prev;

    //updating links between prev and next
    tempNext->links.prev = tempPrev;
    tempPrev->links.next = tempNext;

    //update freelists
    if((void*)tempNext == &node_ptr->head && (void*)tempPrev == &node_ptr->head) {
      node_ptr->prev->next = node_ptr->next;
      node_ptr->next->prev = node_ptr->prev;
    }

    node_ptr = &sf_free_list_head;

    //do the same for freelist referring to currentBlockHeader (nextHeader)
    while(node_ptr->size != (nextHeader->info.block_size << 4)) {
      node_ptr = node_ptr->next;
    }

    header_ptr = &node_ptr->head;
    while((void*)header_ptr != (void*)nextHeader) {
      header_ptr = header_ptr->links.next;
    }

    tempNext = header_ptr->links.next;
    tempPrev = header_ptr->links.prev;

    tempNext->links.prev = tempPrev;
    tempPrev->links.next = tempNext;

    if((void*)tempNext == &node_ptr->head && (void*)tempPrev == &node_ptr->head) {
      node_ptr->prev->next = node_ptr->next;
      node_ptr->next->prev = node_ptr->prev;
    }

    //update prevBlockHeader and nextHeaderFooter blockSizeInfo
    prevBlockHeader->info.block_size = (((prevBlockHeader->info.block_size << 4) + (nextHeader->info.block_size << 4)) >> 4);
    nextFooter->info.block_size = prevBlockHeader->info.block_size;
    nextFooter->info.prev_allocated = prevBlockHeader->info.prev_allocated;

    //search freelists for freelist node of new blocksize, otherwise add new freelist for new blocksize,
    node_ptr = sf_free_list_head.next;
    while(node_ptr->size != (header_ptr->info.block_size << 4) && node_ptr->size != 0) {
      node_ptr = node_ptr->next;
    }

    //add a new freelist of newfreeblocksize, which should be prevBlockHeader
    if(node_ptr->size == 0) {
      while(node_ptr->size < (prevBlockHeader->info.block_size << 4)) {
        if(node_ptr->next->size == 0 && (node_ptr->size < (prevBlockHeader->info.block_size << 4))) {
          break;
        }
        node_ptr = node_ptr->next;
      }

        sf_free_list_node* new_node_ptr = sf_add_free_list((prevBlockHeader->info.block_size << 4), node_ptr);
        new_node_ptr->head.links.next = prevBlockHeader;
        prevBlockHeader->links.next = &new_node_ptr->head;
        prevBlockHeader->links.prev = &new_node_ptr->head;
        new_node_ptr->head.links.prev = prevBlockHeader;

    } else { //insert block into freelist indicated by node_ptr
      prevBlockHeader->links.next = node_ptr->head.links.next;
      node_ptr->head.links.next->links.prev = prevBlockHeader;
      node_ptr->head.links.next = prevBlockHeader;
      prevBlockHeader->links.prev = &node_ptr->head;
    }
  }

  return 1;
}

int checkFree(void* pp) {
  if(pp == NULL) {
    return 0;
  }

  if ((pp - sizeof(sf_footer)) < (sf_mem_start() + sizeof(sf_prologue)))
    return 0;

  if ((pp - sizeof(sf_footer)) > (sf_mem_end() - sizeof(sf_epilogue)))
    return 0;

  sf_header* tempHeader = (sf_header*)(pp - sizeof(sf_block_info));

  if(tempHeader->info.prev_allocated == 0) {
    sf_footer* prevFooter = ((void*)tempHeader) - sizeof(sf_footer);
    sf_header* prevHeader = ((void*)tempHeader) - (prevFooter->info.block_size << 4);

    if(prevFooter->info.allocated == 1 || prevHeader->info.allocated == 1) {
      return 0;
    }
  }

  if(tempHeader->info.allocated == 0 ||
    ((tempHeader->info.block_size << 4 ) % 16 != 0) ||
    ((tempHeader->info.block_size << 4 ) < 32) ||
    (tempHeader->info.requested_size + sizeof(sf_block_info) ) > (tempHeader->info.block_size << 4)
  )
    return 0;

  return 1;
}
