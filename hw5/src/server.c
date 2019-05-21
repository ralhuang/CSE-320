#include <time.h>
#include "client_registry.h"
#include "server.h"
#include "protocol.h"
#include "transaction.h"
#include "data.h"
#include "store.h"
#include "debug.h"
#include "csapp.h"

CLIENT_REGISTRY *client_registry;

void *xacto_client_service(void *arg) {

  /* TABLE OF CONTENTS
    1. Retrieve file descriptor
    2. Area where file descriptor must be freed
    3. Detach thread
    4. Register client file descriptor
    5. Create transaction
    6. Thread should enter a service loop to
        a. Receives a request packet sent by the client
        b. Carries out request
        c. Sends a reply packet
           i. Possibly followed by a data packet
        d. If it commits or aborts then...
    7. Terminate
  */

  if (arg == NULL) {
    return NULL;
  }

  // 1. Retrieve file descriptor
  int fd = *((int*)arg);

  // 2. Free
  free(arg);
  debug("%d", fd);

  // 3. Detach thread
  pthread_detach(pthread_self());

  // 4. Register client file descriptor
  creg_register(client_registry, fd);

  // 5. Create transaction
  TRANSACTION* newTrans = trans_create();

  // 6. Service Loop
  TRANS_STATUS tempStatus = trans_get_status(newTrans);


  while (tempStatus == TRANS_PENDING) {

    XACTO_PACKET* tempPacket = Malloc(sizeof(XACTO_PACKET));
    int firstPacket = proto_recv_packet(fd, tempPacket, NULL);
    if (firstPacket != -1) {
      switch(tempPacket->type) {
        case XACTO_PUT_PKT: ;
          XACTO_PACKET tempPutPacket;
          void** keyDataP = Malloc(sizeof(void*));
          int secondPacket = proto_recv_packet(fd, &tempPutPacket, keyDataP);
          if (secondPacket != -1) {
            size_t keySize = tempPutPacket.size;
            void** valueDataP = Malloc(sizeof(void*));
            int thirdPacket = proto_recv_packet(fd, &tempPutPacket, valueDataP);
            if (thirdPacket != -1) {
              //create key
              BLOB* keyBlob = blob_create((char*)*keyDataP, keySize);
              KEY* putKey = key_create(keyBlob);

              //store value
              size_t valueSize = tempPutPacket.size;
              BLOB* valueBlob = blob_create((char*)*valueDataP, valueSize);

              //store put
              TRANS_STATUS testStorePut = store_put(newTrans, putKey, valueBlob);
              store_show();
              trans_show_all();
              if (testStorePut == TRANS_ABORTED) { // HANDLE ABORT
                Free(tempPacket);
                Free(*keyDataP);
                Free(keyDataP);
                Free(*valueDataP);
                Free(valueDataP);
                tempStatus = trans_abort(newTrans);

              } else {
                //SEND REPLY
                tempPacket->type = XACTO_REPLY_PKT;
                struct timespec send_pkt_time;
                clock_gettime(CLOCK_MONOTONIC, &send_pkt_time);
                tempPacket->timestamp_sec = send_pkt_time.tv_sec;
                tempPacket->timestamp_nsec = send_pkt_time.tv_nsec;
                int sendPacket = proto_send_packet(fd, tempPacket, NULL);

                //Then free
                Free(tempPacket);
                Free(*keyDataP);
                Free(keyDataP);
                Free(*valueDataP);
                Free(valueDataP);

                if (sendPacket == -1)
                  tempStatus = trans_abort(newTrans);
              }

            } else { //handle failure, must abort
              Free(*keyDataP);
              Free(keyDataP);
              Free(*valueDataP);
              Free(valueDataP);
              tempStatus = trans_abort(newTrans);
            }

          } else { //handle failure, must abort
            Free(*keyDataP);
            Free(keyDataP);
            tempStatus = trans_abort(newTrans);
          }
          break;


        case XACTO_GET_PKT: ;
          XACTO_PACKET tempGetPacket;
          void** getDataP = Malloc(sizeof(void*));
          int keyPacket = proto_recv_packet(fd, &tempGetPacket, getDataP);
            if (keyPacket != -1) {
              BLOB* keyBlob = blob_create((char*)*getDataP, tempGetPacket.size);
              KEY* getKey = key_create(keyBlob);
              BLOB* replyBlob;
              TRANS_STATUS testStoreGet = store_get(newTrans, getKey, &replyBlob);

              store_show();
              trans_show_all();
              if (testStoreGet != TRANS_ABORTED) {

                tempPacket->type = XACTO_REPLY_PKT;
                struct timespec send_pkt_time;
                clock_gettime(CLOCK_MONOTONIC, &send_pkt_time);
                tempPacket->timestamp_sec = send_pkt_time.tv_sec;
                tempPacket->timestamp_nsec = send_pkt_time.tv_nsec;
                int sendPacket = proto_send_packet(fd, tempPacket, NULL);
                if (sendPacket != -1) {
                  tempPacket->type = XACTO_DATA_PKT;
                  if(replyBlob == NULL)
                    tempPacket->size = 0;
                  else
                    tempPacket->size = replyBlob->size;
                  int sendGetPacket = 0;
                  clock_gettime(CLOCK_MONOTONIC, &send_pkt_time);
                  tempPacket->timestamp_sec = send_pkt_time.tv_sec;
                  tempPacket->timestamp_nsec = send_pkt_time.tv_nsec;
                  if (replyBlob != NULL) {
                    sendGetPacket = proto_send_packet(fd, tempPacket, replyBlob->content);
                  }
                  else {
                    tempPacket->null = 1;
                    sendGetPacket = proto_send_packet(fd, tempPacket, NULL);
                  }
                  Free(*getDataP);
                  Free(getDataP);
                  Free(tempPacket);
                  if (sendGetPacket == -1)
                    tempStatus = trans_abort(newTrans);
                } else {
                  Free(*getDataP);
                  Free(getDataP);
                  Free(tempPacket);
                  tempStatus = trans_abort(newTrans);
                }

              } else {
                debug("ABORTED GET");

                tempPacket->type = XACTO_REPLY_PKT;
                tempPacket->status = 2;
                tempPacket->null = 0;
                tempPacket->size = 0;
                struct timespec send_pkt_time;
                clock_gettime(CLOCK_MONOTONIC, &send_pkt_time);
                tempPacket->timestamp_sec = send_pkt_time.tv_sec;
                tempPacket->timestamp_nsec = send_pkt_time.tv_nsec;

                proto_send_packet(fd, tempPacket, NULL);

                Free(*getDataP);
                Free(getDataP);
                Free(tempPacket);
                tempStatus = trans_abort(newTrans);
                break;
              }
            } else {

              XACTO_PACKET* tempAbortPacket = Malloc(sizeof(XACTO_PACKET));
              tempAbortPacket->type = XACTO_REPLY_PKT;
              tempAbortPacket->status = 2;
              struct timespec send_pkt_time;
              clock_gettime(CLOCK_MONOTONIC, &send_pkt_time);
              tempAbortPacket->timestamp_sec = send_pkt_time.tv_sec;
              tempAbortPacket->timestamp_nsec = send_pkt_time.tv_nsec;

              proto_send_packet(fd, tempAbortPacket, NULL);

              Free(*getDataP);
              Free(getDataP);
              Free(tempPacket);
              Free(tempAbortPacket);
              tempStatus = trans_abort(newTrans);

            }


          break;

        case XACTO_COMMIT_PKT: ;
          XACTO_PACKET tempCommitPacket;
          tempCommitPacket.type = XACTO_REPLY_PKT;
          tempCommitPacket.status = 1;
          tempCommitPacket.size = 0;
          tempCommitPacket.null = 0;

          struct timespec send_pkt_time;
          clock_gettime(CLOCK_MONOTONIC, &send_pkt_time);
          tempCommitPacket.timestamp_sec = send_pkt_time.tv_sec;
          tempCommitPacket.timestamp_nsec = send_pkt_time.tv_nsec;

          int sendPacket = proto_send_packet(fd, &tempCommitPacket, NULL);

          Free(tempPacket);
          if (sendPacket != -1) {
            tempStatus = trans_commit(newTrans);
            store_show();
            trans_show_all();
          }
          else
            tempStatus = trans_abort(newTrans);
          break;



        default: //ABORT

          Free(tempPacket);
          tempStatus = trans_abort(newTrans);
          break;
      }
    }
    else {
      // tempStatus = TRANS_ABORTED;
      if (tempStatus != TRANS_ABORTED) {
        Free(tempPacket);
        tempStatus = trans_abort(newTrans);
        break;
      }
    }
  }

  creg_unregister(client_registry, fd);
  close(fd);

  return NULL;
}
