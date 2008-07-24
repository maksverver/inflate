#include "inflate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Example code demonstrating the use of the Inflate library.
   Reads a deflate-compressed gzip chunks from standard input,
   and writes decompressed output to standard output. */

uint32 crc32, isize;
uint32 crc_table[256];

static struct Inflate infl;
uint8 input[1024], output[1024];

void init_crc_table()
{
    uint32 c;
    int n, k;

    for (n = 0; n < 256; n++)
    {
        c = (uint32)n;
        for (k = 0; k < 8; k++)
        {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
}

void update_crc(const uint8 *buf, int len)
{
    while (len-- > 0)
        crc32 = crc_table[(crc32 ^ *buf++)&0xff] ^ (crc32 >> 8);
}

void inflate()
{
    /* Initiaize Inflate structure */
    inflate_init(&infl);

    infl.in.buf = input;
    infl.in.len = 0;
    infl.in.pos = 0;

    infl.out.buf = output;
    infl.out.len = sizeof(output);
    infl.out.pos = 0;

    for (;;)
    {
        switch(inflate_run(&infl))
        {
        case INFLATE_ERROR:
            fprintf(stderr, "Decoding error - check your input!\n");
            exit(1);

        case INFLATE_DONE:
            /* Don't forget to write data remaining in the buffer! */
            fwrite(infl.out.buf, 1, infl.out.pos, stdout);
            isize += infl.out.pos;
            update_crc(infl.out.buf, infl.out.pos);
            return;

        case INFLATE_INPUT:
            /* Input buffer is empty */
            infl.in.pos = 0;
            infl.in.len = fread(input, 1, sizeof(input), stdin);
            if (infl.in.len == 0)
            {
                fprintf(stderr, "Premature end of input!\n");
                exit(2);
            }
            break;

        case INFLATE_OUTPUT:
            /* Output buffer is full */
            fwrite(infl.out.buf, 1, infl.out.pos, stdout);
            isize += infl.out.pos;
            update_crc(infl.out.buf, infl.out.pos);
            infl.out.pos = 0;
            break;
        }
    }
}

int main()
{
    /* The GZIP chunk header */
    struct GZIP_Header
    {
        uint8  id1, id2, cm, flg;
        uint32 mtime;
        uint8  xfl, os;
    } header;

    /* The GZIP footer */
    uint8 footer[8];

    init_crc_table();

    while (fread(&header, 10, 1, stdin) != 0)
    {
        if (header.id1 != 31 || header.id2 != 139)
        {
            fprintf(stderr, "Input not recognized as a GZIP file!\n");
            return 1;
        }

        if (header.cm != 8)     /* 8 == DEFLATE compression */
        {
            fprintf(stderr, "GZIP file uses unknown compression method!\n");
            return 1;
        }

        if (header.flg &~((1<<5)-1))
        {
            fprintf(stderr, "GZIP file uses unknown flags!\n");
            return 1;
        }

        if (header.flg & (1<<2))
        {
            /* Skip extra data section */
            int len = getchar();
            len += 256*getchar();
            while (len--)
                getchar();
        }

        if (header.flg & (1<<3))
        {
            /* Skip original file name */
            while (getchar()&255) { };
        }

        if (header.flg & (1<<4))
        {
            /* Skip file comment */
            while (getchar()&255) { };
        }

        if (header.flg & (1<<1))
        {
            /* Skip 16-bit CRC */
            getchar();
            getchar();
        }

        /* Finally, we can start decompressing the deflate data! */
        crc32 = 0xffffffff;
        isize = 0;
        inflate();
        crc32 ^= 0xffffffff;

        /* Read GZIP footer, taking any remaining data from the buffer */
        infl.in.len -= infl.in.pos;
        if (infl.in.len >= sizeof(footer))
        {
            memcpy(footer, infl.in.buf + infl.in.pos, 8);
        }
        else
        {
            memcpy(footer, infl.in.buf + infl.in.pos, infl.in.len);
            if (fread(footer + infl.in.len, 8 - infl.in.len, 1, stdin) != 1)
            {
                fprintf(stderr, "Unable to read footer!\n");
                exit(3);
            }
        }

        /* Verify the integrity of the output */
        if (crc32 != (footer[0] | footer[1]<<8 | footer[2]<<16 | footer[3]<<24))
        {
            fprintf(stderr, "Calculaed CRC does not match stored CRC!\n");
            exit(5);
        }
        if (isize != (footer[4] | footer[5]<<8 | footer[6]<<16 | footer[7]<<24))
        {
            fprintf(stderr, "Output size does not match stored size!\n");
            exit(4);
        }
    }

    return 0;
}
