#include "ftpacket.h"

ftpack 
ftpack_create(uint8_t type, void *data, ssize_t size)
{
  void *packdata;

  if(((packdata = malloc(sizeof(uint8_t) + size + sizeof(ssize_t))) == NULL) || (size < 0)) {
    return NULL;
  }

  uint8_t *ptype = (uint8_t*)packdata;
  ssize_t *psize = (ssize_t*)(packdata + sizeof(uint8_t));
  void *pdata = (packdata + sizeof(uint8_t) + sizeof(ssize_t));

  *ptype = type;
  *psize = size;

  memcpy(pdata, data, size);

  return packdata;
}

ssize_t 
ftpack_psize(ftpack packet) 
{ 
  return ((*(ssize_t*)(packet + sizeof(uint8_t))) + sizeof(uint8_t) + sizeof(ssize_t)); 
}

ssize_t 
ftpack_dsize(ftpack packet) { return (*(ssize_t*)(packet + sizeof(uint8_t))); }

uint8_t 
ftpack_ptype(ftpack packet) { return (uint8_t)*((uint8_t*) packet); }

void 
*ftpack_pdata(ftpack packet) { return (packet + sizeof(uint8_t) + sizeof(ssize_t)); }

void 
ftpack_free(ftpack packet) { return free(packet); }
