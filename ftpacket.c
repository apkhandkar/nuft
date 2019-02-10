/*

  A 'packet' is a pointer to a block of allocated memory that
  is initialised in a particular way. It is a composite type,
  however, unlike any composite offered by the C language, it
  can vary in size depending upon the size of its payload.

  It consists of two parts: a header and the payload.
  The header consists of three fields: packet type; which is
  an unsigned integer with minimum width of 8 bits, sequence
  number; an unsigned integer with minimum width of 32 bits,
  and packet size; a signed size value which stores the size
  of the payload that succeeds the header.

  +------+------+------+-----------
  |      |      |      |
  | type | snum | size |    data  ......
  |      |      |      |
  +------+------+------+-----------
   uint8  uint32 ssize

  The minimum size of one such packet works out to be:
  sizeof(uint8_t) +
  sizeof(uint32_t) +
  sizeof(ssize_t) +
  1 byte (packet cannot be empty)
  
  The functions this file provides can be used to allocate 
  and initialise such a packet in memory (ftpack_create),
  free an allocated packet (ftpack_free) and return the values
  for fields in the packet header (ftpack_dsize, 
  ftpack_snum and ftpack_ptype), return size of the entire 
  packet (ftpack_psize) and return a pointer to the data 
  (payload) of the packet (ftpack_pdata).  

*/

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

/*
 * these functions are fancy wrappers for pointer arithmetic and
 * somewhat complicated casts, as such, they could be reworked
 * into macro functions sometime in the future
 *
 * 10/02/19: macros for the functions have been provided in 
 * ftpacket.h
 */

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

