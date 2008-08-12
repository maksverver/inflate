#ifndef CRC32_H_INCLUDED
#define CRC32_H_INCLUDED

#include <stdint.h>

/* Initializes the CRC32 module.
   Must be called once before any other functions are called. */
void crc32_init();

void crc32_begin(uint32_t *crc);
void crc32_update(uint32_t *crc, const uint8_t *buf, int len);
void crc32_end(uint32_t *crc);

#endif /* ndef CRC32_H_INCLUDED */
