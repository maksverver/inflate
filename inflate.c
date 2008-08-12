#include "inflate.h"
#include <stdlib.h>
#include <string.h>

#define HUFF_MAX_LEN    15

int init_huffman_table(struct HuffmanTable *ht, const uint8_t *lengths, int lengths_size)
{
    int len_count[HUFF_MAX_LEN + 1] = { };
    int next_code[HUFF_MAX_LEN + 1];
    int n, m, x, len;

    /* Figure out code lengths */
    len = 0;
    for (n = 0; n < lengths_size; ++n)
    {
        if (lengths[n] > HUFF_MAX_LEN)
            return -1;
        if (lengths[n] > len)
            len = lengths[n];
        if (lengths[n] > 0)
            ++len_count[(int)lengths[n]];
    }
    ht->mask = (1<<len)-1;

    /* Figure out starting codes for lengths */
    next_code[0] = 0;
    for (n = 1; n <= len; ++n)
        next_code[n] = (next_code[n - 1] + len_count[n - 1]) << 1;

    /* Put values in Huffman table */
    memset(ht->e, 0, (1<<len)*sizeof(struct HuffmanEntry));
    for (n = 0; n < lengths_size; ++n)
    {
        if (lengths[n] == 0)
            continue;

        x = 0;
        for (m = 0; m < lengths[n]; ++m)
        {
            x <<= 1;
            x |= (next_code[lengths[n]] >> m)&1;
        }
        next_code[lengths[n]] += 1;

        for ( ; x < (1<<len); x += (1<<lengths[n]))
        {
            ht->e[x].bits  = lengths[n];
            ht->e[x].value = n;
        }
    }
    return 0;
}

#define CHECK(c) while(!(c)) { goto error; }

#define RESERVE(n) \
    do { I->param = n; \
         if (I->word_size < I->param) { \
             I->line = __LINE__; \
             goto shift; \
             case __LINE__:; \
         } \
    } while(0)

#define SHIFT(n) \
    do { I->word >>= n; \
         I->word_size -= n; \
    } while(0)

#define SKIP(n) \
    do { RESERVE(n); \
         SHIFT(I->param); \
    } while(0)

#define GRAB(n,e) \
    do { RESERVE(n); \
         e = (I->word)&((1 << I->param) - 1); \
         SHIFT(I->param); \
    } while(0)

#define DECODE(ht,v) \
    do { struct HuffmanEntry *he; \
         while ((he = (ht).e + (I->word&(ht).mask))->bits > I->word_size) \
            RESERVE(he->bits); \
        CHECK(he->bits != 0); \
        SHIFT(he->bits); \
        v = he->value; \
    } while(0)

#define OUTPUT(c) \
    do { I->window[I->window_pos] = (uint8_t)c; \
         while (I->out.pos >= I->out.len) \
            YIELD(INFLATE_OUTPUT); \
         I->out.buf[I->out.pos] = I->window[I->window_pos]; \
         I->window_pos = (I->window_pos + 1)&32767; \
         I->out.pos += 1; \
    } while(0);

#define YIELD(n) \
    do { I->line = __LINE__; \
         return (n); \
         case __LINE__:; \
    } while(0)

int inflate_run(struct Inflate *I)
{
start:
    switch(I->line)
    {
default:
    SKIP(0);
    do {
        int btype;
        GRAB(1, I->bfinal);
        GRAB(2, btype);
        CHECK(btype < 3);

        if (btype == 0)
        {
            /* Seek to next byte boundary */
            SKIP(I->word_size%8);
            GRAB(16, I->n);
            GRAB(16, I->last);
            CHECK(I->n == (I->last^0xffff));

            /* Ouput n characters */
            while (I->n-- > 0)
            {
                int tmp;
                GRAB(8, tmp);
                OUTPUT(tmp);
            }
        }
        else
        {
            if (btype == 1)
            {
                /* Load fixed Huffman tables */
                static const uint8_t fixed_litlen_lengths[288] = {
                    8,8,8,8,8,8,8,8,  8,8,8,8,8,8,8,8,
                    8,8,8,8,8,8,8,8,  8,8,8,8,8,8,8,8,
                    8,8,8,8,8,8,8,8,  8,8,8,8,8,8,8,8,
                    8,8,8,8,8,8,8,8,  8,8,8,8,8,8,8,8,
                    8,8,8,8,8,8,8,8,  8,8,8,8,8,8,8,8,
                    8,8,8,8,8,8,8,8,  8,8,8,8,8,8,8,8,
                    8,8,8,8,8,8,8,8,  8,8,8,8,8,8,8,8,
                    8,8,8,8,8,8,8,8,  8,8,8,8,8,8,8,8,
                    8,8,8,8,8,8,8,8,  8,8,8,8,8,8,8,8,
                    9,9,9,9,9,9,9,9,  9,9,9,9,9,9,9,9,
                    9,9,9,9,9,9,9,9,  9,9,9,9,9,9,9,9,
                    9,9,9,9,9,9,9,9,  9,9,9,9,9,9,9,9,
                    9,9,9,9,9,9,9,9,  9,9,9,9,9,9,9,9,
                    9,9,9,9,9,9,9,9,  9,9,9,9,9,9,9,9,
                    9,9,9,9,9,9,9,9,  9,9,9,9,9,9,9,9,
                    9,9,9,9,9,9,9,9,  9,9,9,9,9,9,9,9,
                    7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,
                    7,7,7,7,7,7,7,7,  8,8,8,8,8,8,8,8 };
                static const uint8_t fixed_dist_lengths[32] = {
                    5,5,5,5,5,5,5,5,  5,5,5,5,5,5,5,5,
                    5,5,5,5,5,5,5,5,  5,5,5,5,5,5,5,5 };

                CHECK(init_huffman_table(&I->ht1, fixed_litlen_lengths, 288) == 0);
                CHECK(init_huffman_table(&I->ht2, fixed_dist_lengths, 32) == 0);
            }
            else
            /* btype == 2 */
            {
                /* Read dynamic Huffman tables */
                static const int code_list[19] = {
                    16, 17, 18, 0, 8, 7, 9, 6, 10, 5,
                    11, 4, 12, 3, 13, 2, 14, 1, 15 };

                /* Read sizes */
                GRAB(5, I->hlit);       /* Number of lit/len codes */
                GRAB(5, I->hdist);      /* Number of distance codes */
                GRAB(4, I->hclen);      /* Number of code lengths */
                I->hlit  += 257;
                I->hdist += 1;
                I->hclen += 4;

                /* Read code lengths Huffman table */
                memset(I->code_lengths, 0, sizeof(I->code_lengths));
                for (I->n = 0; I->n < I->hclen; ++I->n)
                    GRAB(3, I->code_lengths[code_list[I->n]]);
                CHECK(init_huffman_table(&I->ht1, I->code_lengths, 19) == 0);

                /* Read lit/len lengths and dist lengths */
                I->last = 0;
                for (I->n = 0; I->n < I->hlit + I->hdist; )
                {
                    int c, repeat;

                    DECODE(I->ht1, c);

                    if (c < 16)
                    {
                        if (I->n < I->hlit)
                            I->litlen_lengths[I->n] = c;
                        else
                            I->dist_lengths[I->n - I->hlit] = c;
                        I->last = c;
                        ++I->n;
                    }
                    else
                    {
                        if (c == 16)
                        {
                            GRAB(2, repeat);
                            repeat += 3;
                        }
                        else
                        {
                            if (c == 17)
                            {
                                GRAB(3, repeat);
                                repeat += 3;
                            }
                            else
                            /* c == 18 */
                            {
                                GRAB(7, repeat);
                                repeat += 11;
                            }
                            I->last = 0;
                        }

                        /* Check if repeat sequence would exceed buffer */
                        CHECK(repeat <= I->hlit + I->hdist - I->n);

                        while (repeat > 0)
                        {
                            if (I->n < I->hlit)
                                I->litlen_lengths[I->n] = I->last;
                            else
                                I->dist_lengths[I->n - I->hlit] = I->last;
                            ++I->n;
                            --repeat;
                        }
                    }
                }

                CHECK(init_huffman_table(&I->ht1, I->litlen_lengths, I->hlit ) == 0);
                CHECK(init_huffman_table(&I->ht2, I->dist_lengths,   I->hdist) == 0);
            }

            for(;;) {
                int c, n;

                DECODE(I->ht1, c);

                if (c < 256)
                {
                    OUTPUT(c);
                }
                else
                /* c >= 256 */
                {
                    static const int base_len[29] = {
                        3,   4,   5,   6,   7,   8,   9,  10,  11,  13,
                        15,  17,  19,  23,  27,  31,  35,  43,  51,  59,
                        67,  83,  99, 115, 131, 163, 195, 227, 258 };

                    static const int len_bits[29] = {
                        0,  0,  0,  0,  0,  0,  0,  0,  1,  1,
                        1,  1,  2,  2,  2,  2,  3,  3,  3,  3,
                        4,  4,  4,  4,  5,  5,  5,  5,  0 };

                    static const int base_dist[30] = {
                        1,   2,   3,   4,   5,   7,   9,  13,  17,  25,
                        33,  49,  65,  97, 129, 193, 257, 385, 513, 769,
                    1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };

                    static const int dist_bits[30] = {
                        0,  0,  0,  0,  1,  1,  2,  2,  3,  3,
                        4,  4,  5,  5,  6,  6,  7,  7,  8,  8,
                        9,  9, 10, 10, 11, 11, 12, 12, 13, 13 };

                    if (c == 256)
                        break;
                    CHECK(c - 257 < 29);
                    I->l = base_len[c - 257];
                    GRAB(len_bits[c - 257], n);
                    I->l += n;

                    DECODE(I->ht2, c);
                    CHECK(c < 30);

                    I->d = base_dist[c];
                    GRAB(dist_bits[c], n);
                    I->d += n;

                    while (I->l-- > 0)
                        OUTPUT(I->window[(I->window_pos + 32768 - I->d)&32767]);
                }
            }
        }
    } while(!I->bfinal);

    for (;;) YIELD(INFLATE_DONE);

    /* Never reached */

shift:
    /* Fills the word buffer with I->param bits of input */
    while (I->word_size < I->param)
    {
        while (I->in.pos >= I->in.len)
        {
            I->old_line = I->line;
            YIELD(INFLATE_INPUT);
            I->line = I->old_line;
        }
        I->word |= I->in.buf[I->in.pos++] << (I->word_size);
        I->word_size += 8;
    }
    goto start;

error:
    for (;;) YIELD(INFLATE_ERROR);

    }   /* end of switch */

    /* Never reached */
    return INFLATE_ERROR;
}


struct Inflate *inflate_create()
{
    struct Inflate *I = (struct Inflate *)malloc(sizeof(struct Inflate));
    if (I != NULL)
        inflate_init(I);
    return I;
}

void inflate_destroy(struct Inflate *I)
{
    free(I);
}

void inflate_init(struct Inflate *I)
{
    I->word = 0;
    I->word_size = 0;
    memset(I->window, 0, sizeof(I->window));
    I->window_pos = 0;

    I->line = 0;
}
