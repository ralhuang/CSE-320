#include "csapp.h"
#include "data.h"
#include <pthread.h>
#include <debug.h>
#include "transaction.h"

BLOB *blob_create(char *content, size_t size) {
  if (size < 0)
    return NULL;
  BLOB* newBlob = (BLOB*)Calloc(1, sizeof(BLOB));
  newBlob->refcnt = 1;
  newBlob->size = size;
  int initpthread = pthread_mutex_init(&(newBlob->mutex), NULL);
  if (initpthread != 0) {
    Free(newBlob);
    return NULL;
  }
  if (content != NULL) {
    newBlob->content = Calloc(1, size+1);
    memcpy(newBlob->content, content, size);
    newBlob->prefix = newBlob->content;
  } else {
    newBlob->content = NULL;
    newBlob->prefix = NULL;
  }

  return newBlob;
}

BLOB *blob_ref(BLOB *bp, char *why) {
  if (bp == NULL) {
    return NULL;
  }
  pthread_mutex_lock(&(bp->mutex));
  bp->refcnt++;
  // debug("%s", why);
  pthread_mutex_unlock(&(bp->mutex));
  return bp;
}

void blob_unref(BLOB *bp, char *why) {
  pthread_mutex_lock(&(bp->mutex));
  if (bp != NULL) {
  bp->refcnt--;
  pthread_mutex_unlock(&(bp->mutex));
  // debug("%s", why);
  if (bp->refcnt == 0) {
      if (bp->content != NULL)
        Free(bp->content);
      Free(bp);
    }
  }
  pthread_mutex_unlock(&(bp->mutex));
}

int blob_compare(BLOB *bp1, BLOB *bp2) {
  // if ((bp1 == NULL && bp2 != NULL) || (bp2 == NULL && bp1 != NULL))
  //   return -1;
  pthread_mutex_lock(&(bp1->mutex));
  pthread_mutex_lock(&(bp2->mutex));
  size_t sizeToCompare = 0;
  if (bp1->size > bp2->size) {
    sizeToCompare = bp1->size;
  } else if (bp1->size < bp2->size) {
    sizeToCompare = bp2->size;
  } else {
    sizeToCompare = bp1->size;
  }
  if((memcmp(bp1->content, bp2->content, sizeToCompare) == 0) && (bp1->size == bp2->size)) {
    pthread_mutex_unlock(&(bp1->mutex));
    pthread_mutex_unlock(&(bp2->mutex));
    return 0;
  } else {
    pthread_mutex_unlock(&(bp1->mutex));
    pthread_mutex_unlock(&(bp2->mutex));
    return -1;
  }
}

int blob_hash(BLOB *bp) {

  int sumOfChars = 0;
  for(int i = 0; i < bp->size; i++){
      sumOfChars+= ((bp->content)[i]);
  }
  int hashInt = sumOfChars % 8;
  return hashInt;
}

KEY *key_create(BLOB *bp) {
  KEY* newKey = (KEY*)Calloc(1, sizeof(KEY));
  newKey->hash = blob_hash(bp);
  newKey->blob = bp;

  blob_ref(newKey->blob, "Creating key.");
  return newKey;
}

void key_dispose(KEY *kp) {
  blob_unref(kp->blob, "Disposing key.");
  Free(kp);
  kp = NULL;
}

int key_compare(KEY *kp1, KEY *kp2) {
  if(kp1->hash == kp2->hash) {
    if(blob_compare(kp1->blob, kp2->blob) == 0)
      return 0;
    else
      return -1;
    }
  else
    return -1;
}

VERSION *version_create(TRANSACTION *tp, BLOB *bp) {
  VERSION* newVersion = (VERSION*)Calloc(1, sizeof(VERSION));
  newVersion->creator = tp;
  newVersion->blob = bp;
  trans_ref(tp, "Creating version, referencing transaction.");
  //
  blob_ref(newVersion->blob, "Creating version, referencing blob.");

  return newVersion;
}

void version_dispose(VERSION *vp) {
  blob_unref(vp->blob, "Disposing version.");
  trans_unref(vp->creator, "Disposing version transaction.");
  Free(vp);
}
