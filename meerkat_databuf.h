#include <stdint.h>
#include <stdio.h>
#include "hashpipe.h"
#include "hashpipe_databuf.h"

#define CACHE_ALIGNMENT         512
#define N_INPUT_BLOCKS          3
#define N_OUTPUT_BLOCKS         3

#define NANTS                   64
#define NCHANS_IN               1
#define NUPCHAN                 4  //should be 2**19, use a smaller number now for testing
#define NSAMPS_IN               8*NUPCHAN
#define NPOLS                   2
#define NBEAMS                  65  //64 coherent plus 1 incoherent
#define NCHANS_OUT              NUPCHAN
#define NSAMPS_OUT              NSAMPS_IN/NUPCHAN

#define SIZEOF_INPUT_DATA_BUF   NANTS*NCHANS_IN*NSAMPS_IN*NPOLS*2
#define SIZEOF_OUTPUT_DATA_BUF  NBEAMS*NCHANS_OUT*NSAMPS_OUT

/* INPUT BUFFER STRUCTURES   
   64 antennas x 1 frequency x 8x2**19 time x 2 pol x complex , in 8bit
   TO DO: make it dynamic for the 3 F-engine modes, currrently hardcoded for 1k mode.
  */
typedef struct input_block_header {
   uint64_t mcnt;                    // mcount of first packet
} input_block_header_t;

typedef uint8_t hashpipe_databuf_cache_alignment[
   CACHE_ALIGNMENT - (sizeof(hashpipe_databuf_t)%CACHE_ALIGNMENT)
];

typedef uint8_t input_header_cache_alignment[
   CACHE_ALIGNMENT - (sizeof(input_block_header_t)%CACHE_ALIGNMENT)
];

typedef struct input_block {
   input_block_header_t header;
   input_header_cache_alignment padding; // Maintain cache alignment
   char data_block[SIZEOF_INPUT_DATA_BUF];
} input_block_t;

typedef struct input_databuf {
   hashpipe_databuf_t header;
   input_header_cache_alignment padding;
   //hashpipe_databuf_cache_alignment padding; // Maintain cache alignment
   input_block_t block[N_INPUT_BLOCKS];
} input_databuf_t;


/*
  * OUTPUT BUFFER STRUCTURES
  65 beams x 2**19 freq x 8 times, in float
  */
typedef struct output_block_header {
   uint64_t mcnt;
} output_block_header_t;

typedef uint8_t output_header_cache_alignment[
   CACHE_ALIGNMENT - (sizeof(output_block_header_t)%CACHE_ALIGNMENT)
];

typedef struct output_block {
   output_block_header_t header;
   output_header_cache_alignment padding; // Maintain cache alignment
   float output[SIZEOF_OUTPUT_DATA_BUF*sizeof(float)];
} output_block_t;

typedef struct output_databuf {
   hashpipe_databuf_t header;
   hashpipe_databuf_cache_alignment padding; // Maintain cache alignment
   output_block_t block[N_OUTPUT_BLOCKS];
} output_databuf_t;

/*
 * INPUT BUFFER FUNCTIONS
 */
hashpipe_databuf_t *input_databuf_create(int instance_id, int databuf_id);

static inline input_databuf_t *input_databuf_attach(int instance_id, int databuf_id)
{
    return (input_databuf_t *)hashpipe_databuf_attach(instance_id, databuf_id);
}

static inline int input_databuf_detach(input_databuf_t *d)
{
    return hashpipe_databuf_detach((hashpipe_databuf_t *)d);
}

static inline void input_databuf_clear(input_databuf_t *d)
{
    hashpipe_databuf_clear((hashpipe_databuf_t *)d);
}

static inline int input_databuf_block_status(input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_block_status((hashpipe_databuf_t *)d, block_id);
}

static inline int input_databuf_total_status(input_databuf_t *d)
{
    return hashpipe_databuf_total_status((hashpipe_databuf_t *)d);
}

static inline int input_databuf_wait_free(input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int input_databuf_busywait_free(input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int input_databuf_wait_filled(input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int input_databuf_busywait_filled(input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int input_databuf_set_free(input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_free((hashpipe_databuf_t *)d, block_id);
}

static inline int input_databuf_set_filled(input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_filled((hashpipe_databuf_t *)d, block_id);
}

/*
 * OUTPUT BUFFER FUNCTIONS
 */

hashpipe_databuf_t *output_databuf_create(int instance_id, int databuf_id);

static inline void output_databuf_clear(output_databuf_t *d)
{
    hashpipe_databuf_clear((hashpipe_databuf_t *)d);
}

static inline output_databuf_t *output_databuf_attach(int instance_id, int databuf_id)
{
    return (output_databuf_t *)hashpipe_databuf_attach(instance_id, databuf_id);
}

static inline int output_databuf_detach(output_databuf_t *d)
{
    return hashpipe_databuf_detach((hashpipe_databuf_t *)d);
}

static inline int output_databuf_block_status(output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_block_status((hashpipe_databuf_t *)d, block_id);
}

static inline int output_databuf_total_status(output_databuf_t *d)
{
    return hashpipe_databuf_total_status((hashpipe_databuf_t *)d);
}

static inline int output_databuf_wait_free(output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int output_databuf_busywait_free(output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_free((hashpipe_databuf_t *)d, block_id);
}
static inline int output_databuf_wait_filled(output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int output_databuf_busywait_filled(output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int output_databuf_set_free(output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_free((hashpipe_databuf_t *)d, block_id);
}

static inline int output_databuf_set_filled(output_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_filled((hashpipe_databuf_t *)d, block_id);
}


