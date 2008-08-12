#include "crc32.h"

#define QUOTIENT 0xedb88320L

static uint32_t crc_table[256];

void crc32_init()
{
    uint32_t c;
    int n, k;

    for (n = 0; n < 256; n++)
    {
        c = (uint32_t)n;
        for (k = 0; k < 8; k++)
        {
            if (c & 1)
                c = QUOTIENT ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
}

void crc32_begin(uint32_t *crc32)
{
    *crc32 = 0xffffffffu;
}

void crc32_update(uint32_t *crc32, const uint8_t *buf, int len)
{
    while (len-- > 0)
        *crc32 = crc_table[(*crc32 ^ *buf++)&0xff] ^ (*crc32 >> 8);
}

void crc32_end(uint32_t *crc32)
{
    *crc32 ^= 0xffffffffu;
}
