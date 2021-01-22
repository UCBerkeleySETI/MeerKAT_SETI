#include <stdint.h>
#include <stdio.h>
#include "hashpipe.h"
#include "hashpipe_databuf.h"

#define CACHE_ALIGNMENT         8
#define N_INPUT_BLOCKS          3 
#define N_OUTPUT_BLOCKS         3
#define SIZEOF_INPUT_DATA_BUF   200

/* INPUT BUFFER STRUCTURES
  */
typedef struct readfile_input_block_header {
   uint64_t mcnt;                    // mcount of first packet
} readfile_input_block_header_t;

typedef uint8_t readfile_input_header_cache_alignment[
   CACHE_ALIGNMENT - (sizeof(readfile_input_block_header_t)%CACHE_ALIGNMENT)
];

typedef struct readfile_input_block {
   readfile_input_block_header_t header;
   readfile_input_header_cache_alignment padding; // Maintain cache alignment
   char data_block[SIZEOF_INPUT_DATA_BUF*sizeof(char)]; // define input buffer
} readfile_input_block_t;

typedef struct readfile_input_databuf {
   hashpipe_databuf_t header;
   readfile_input_header_cache_alignment padding;
   //hashpipe_databuf_cache_alignment padding; // Maintain cache alignment
   readfile_input_block_t block[N_INPUT_BLOCKS];
} readfile_input_databuf_t;


/*
 * INPUT BUFFER FUNCTIONS
 */
hashpipe_databuf_t *readfile_input_databuf_create(int instance_id, int databuf_id);

static inline readfile_input_databuf_t *readfile_input_databuf_attach(int instance_id, int databuf_id)
{
    return (readfile_input_databuf_t *)hashpipe_databuf_attach(instance_id, databuf_id);
}

static inline int readfile_input_databuf_detach(readfile_input_databuf_t *d)
{
    return hashpipe_databuf_detach((hashpipe_databuf_t *)d);
}

static inline void readfile_input_databuf_clear(readfile_input_databuf_t *d)
{
    hashpipe_databuf_clear((hashpipe_databuf_t *)d);
}

static inline int readfile_input_databuf_block_status(readfile_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_block_status((hashpipe_databuf_t *)d, block_id);
}

static inline int readfile_input_databuf_total_status(readfile_input_databuf_t *d)
{
    return hashpipe_databuf_total_status((hashpipe_databuf_t *)d);
}

static inline int readfile_input_databuf_wait_free(readfile_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int readfile_input_databuf_busywait_free(readfile_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_free((hashpipe_databuf_t *)d, block_id);
}

static inline int readfile_input_databuf_wait_filled(readfile_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_wait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int readfile_input_databuf_busywait_filled(readfile_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_busywait_filled((hashpipe_databuf_t *)d, block_id);
}

static inline int readfile_input_databuf_set_free(readfile_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_free((hashpipe_databuf_t *)d, block_id);
}

static inline int readfile_input_databuf_set_filled(readfile_input_databuf_t *d, int block_id)
{
    return hashpipe_databuf_set_filled((hashpipe_databuf_t *)d, block_id);
}

