#include "csapp.h"
#include "client_registry.h"
#include <semaphore.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

typedef struct client_registry {
  fd_set connectedFDs;
  pthread_mutex_t masterThread;
  sem_t threadCtrMutex;
  sem_t test;
  int threadCtr;
} CLIENT_REGISTRY;

CLIENT_REGISTRY* creg_init() {
  CLIENT_REGISTRY* cr_ptr = (CLIENT_REGISTRY*)(Calloc(1, sizeof(CLIENT_REGISTRY)));
  FD_ZERO(&(cr_ptr->connectedFDs));
  pthread_mutex_init(&(cr_ptr->masterThread), NULL);
  sem_init(&(cr_ptr->threadCtrMutex), 0, 0);
  // sem_init(&(cr_ptr->test), 0, 0);
  cr_ptr->threadCtr = 0;
  return cr_ptr;
}

void creg_fini(CLIENT_REGISTRY *cr) {
  free(cr);
}

void creg_register(CLIENT_REGISTRY *cr, int fd) {
  pthread_mutex_lock(&(cr->masterThread));

  FD_SET(fd, &(cr->connectedFDs));
  // P(&(cr->test));
  cr->threadCtr++;
  // V(&(cr->test));
  pthread_mutex_unlock(&(cr->masterThread));
}

void creg_unregister(CLIENT_REGISTRY *cr, int fd) {
  pthread_mutex_lock(&(cr->masterThread));
  FD_CLR(fd, &(cr->connectedFDs));
  // P(&(cr->test));
  cr->threadCtr--;
  // V(&(cr->test));
  if(cr->threadCtr == 0)
    V(&(cr->threadCtrMutex));
  pthread_mutex_unlock(&(cr->masterThread));
}

//USE SEMAPHORE TO BLOCK
void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
  P(&(cr->threadCtrMutex));
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {
  for(int i = 0; i < FD_SETSIZE; i++) {
    if(FD_ISSET(i, &(cr->connectedFDs)))
      shutdown(i, SHUT_RD);
  }
}
