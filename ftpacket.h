/*

  ftpack - typedef'd data type and associated functions to
  manipulate and read packets used by nu-ft

  note that:
    ftpack_create calls malloc()  internally to  perform  allocation
    and should set errno on failure

*/

#ifndef FTPACKET_H
#define FTPACKET_H

typedef void * ftpack;

ftpack ftpack_create(uint8_t type, uint32_t snum, void *data, ssize_t size);
ssize_t ftpack_psize(ftpack packet);
ssize_t ftpack_dsize(ftpack packet);
uint8_t ftpack_ptype(ftpack packet);
void *ftpack_pdata(ftpack packet);
void ftpack_free(ftpack packet);

#endif
