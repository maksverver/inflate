#ifndef INFLATE_H_INCLUDED
#define INFLATE_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>

/* Constants returned by inflate_run() */
#define INFLATE_ERROR   (-1)
#define INFLATE_DONE    ( 0)
#define INFLATE_INPUT   ( 1)
#define INFLATE_OUTPUT  ( 2)

/* The Inflate structure contains the context information required by the
   decoder. You may allocate it yourself (on the stack, or dynamically) and
   initialize it with inflate_init(), or let the library allocate one for you
   (with inflate_create()) which is initialized automatically and must be
   destroyed with inflate_destroy() after you are done with it.

   The Inflate structure starts with two members, in and out, representing
   the input and output buffers. The structure contains more fields which are
   used internally and should not be modified by the caller.
*/
struct Inflate;

/* Dynamically allocates an Inflate structure.
   Returns NULL if allocation fails; otherwise the structure must be freed
   with inflate_destroy() after use.
*/
struct Inflate *inflate_create();

/* Frees an Inflate structure created by inflate_create(). */
void inflate_destroy(struct Inflate *infl);

/* Initializes an Inflate structure. This function must be called if the
   structure was not created with inflate_create(). It may also be called
   to reset a structure (even if created with inflate_create()) in order to
   reuse the structure withour reallocating it.

   Note that this function does not initialize the input and output buffers!
*/
void inflate_init(struct Inflate *infl);

/* Runs the main inflate algorithm.
   The caller is responsible for setting the input and output buffers to
   sensible values. The algorithm wil modify the positions of the buffers.

   Return value is one of:
    -1 INFLATE_ERROR    A processing error occured; this only occurs if the
                        input data violates the DEFLATE file format.
     0 INFLATE_DONE     Processing completed succesfully.
                        TODO: define input buffer position
     1 INFLATE_INPUT    Input buffer empty. (in.pos == in.len)
     2 INFLATE_OUTPUT   Output buffer full. (out.pos == out.len)

  This function is usually called in a loop, where the caller is responsible
  for providing new input when INFLATE_INPUT is returned, and emptying the
  output buffer when INFLATE_OUTPUT is returned, until INFLATE_DONE is
  returned to indicate end of input, or INFLATE_ERROR if an error occurs.
*/
int inflate_run(struct Inflate *infl);


/* A representation of a buffer, consisting of a byte pointer (buf),
   a buffer length (len), and a position (pos).
   In a valid buffer, buf is an array of at least len (0 <= len) elements
   and 0 <= pos <= len.

   The inflate algorithm reads from buf[pos] (for input buffers) or writes to
   buf[len] (for output buffers) and then increments pos. The caller typically
   has to reset pos to zero (or a different value) to reuse a buffer.
*/
struct InflateBuffer
{
    uint8_t *buf;
    size_t pos, len;
};


/* The following structures are for internal use only! */
struct HuffmanEntry
{
    unsigned short bits : 4, value : 12;
};

/* Huffman decoding table (~64KB) */
struct HuffmanTable
{
    uint32_t mask;
    struct HuffmanEntry e[1<<15];
};

/* The main Inflate structure. Takes around 160KB of space! */
struct Inflate
{
    struct InflateBuffer in, out;

    /* Following members are for internal use only! */

    /* Co-routine implementation */
    int line, old_line, param;

    /* Local variables that are preserved through calls */
    int bfinal;
    int hlit, hdist, hclen;
    int n, last;
    int l, d;

    /* Currently cached word */
    int word_size;
    uint32_t word;

    uint window_pos;
    uint8_t window[32768];

    uint8_t litlen_lengths[288], dist_lengths[32], code_lengths[19];

    struct HuffmanTable ht1, ht2;
};


#endif /* ndef INFLATE_H_INCLUDED */
