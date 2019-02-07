#include <stdlib.h>
#include <string.h>

#define S(t) sizeof(t)

/* Seems fair to use a typedef here */ 
typedef void ftpack;
 
/* Byte offset corresponding to packet number */
int SNUM_OFFSET = sizeof(uint8_t);
/* Byte offset corresponding to packet size */
int SIZE_OFFSET = sizeof(uint8_t) + sizeof(uint32_t); 
/* Byte offset corresponding to packet data */
int DATA_OFFSET = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(ssize_t);

ftpack 
*ftpack_create(uint8_t type, uint32_t num, void *data, ssize_t size)
{
  void *packdata;

  if(((packdata = malloc(S(uint8_t) + size + S(ssize_t))) == NULL) || (size < 0)) {
    return NULL;
  }

  uint8_t *ptype = (uint8_t*)packdata;
  uint32_t *snum = (uint32_t*)(packdata + SNUM_OFFSET);
  ssize_t *psize = (ssize_t*)(packdata + SIZE_OFFSET);
  void *pdata = packdata + DATA_OFFSET;

  *ptype = type;
  *snum = num;
  *psize = size;

  memcpy(pdata, data, size);

  return packdata;
}

ssize_t 
ftpack_psize(ftpack *packet) 
{ 
  return ((*(ssize_t*)(packet + SIZE_OFFSET)) + S(uint8_t) + S(uint32_t) + S(ssize_t)); 
}

ssize_t 
ftpack_dsize(ftpack *packet) { return (*(ssize_t*)(packet + SIZE_OFFSET)); }

uint8_t 
ftpack_ptype(ftpack *packet) { return (uint8_t)*((uint8_t*) packet); }

uint32_t
ftpack_snum(ftpack *packet) { return (uint32_t)*((uint32_t*)(packet + SNUM_OFFSET)); }

void 
*ftpack_pdata(ftpack *packet) { return (packet + DATA_OFFSET); }

void 
ftpack_free(ftpack *packet) { return free(packet); }
