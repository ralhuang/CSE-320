#include "protocol.h"
#include "csapp.h"
#include <debug.h>

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data) {
  //get size of payload, depending
  uint32_t sizeBeforeConvert;
  if (data == NULL) {
    sizeBeforeConvert = 0;
  } else {
    sizeBeforeConvert = pkt->size;
  }

  pkt->size = htonl((pkt->size));
  pkt->timestamp_sec = htonl((pkt->timestamp_sec));
  pkt->timestamp_nsec = htonl((pkt->timestamp_nsec));

  if (rio_writen(fd, (void*) pkt, sizeof(XACTO_PACKET)) < 0)
    return -1;

  if(sizeBeforeConvert > 0)
    if(rio_writen(fd, data, sizeBeforeConvert) < 0)
      return -1;

  return 0;
}

int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **datap) {

  if (rio_readn(fd, pkt, sizeof(XACTO_PACKET)) <= 0)
    return -1;

  pkt->size = ntohl(pkt->size);
  pkt->timestamp_sec = ntohl(pkt->timestamp_sec);
  pkt->timestamp_nsec = ntohl(pkt->timestamp_nsec);

  if(pkt->size != 0) {
    *datap = Malloc(pkt->size);
    if (rio_readn(fd, *datap, pkt->size) <= 0) {
      return -1;

      }
      // free(*datap);
    }
  return 0;
}
