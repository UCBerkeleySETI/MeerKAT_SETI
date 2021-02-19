/*
 * output_thread.c
 */

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "hashpipe.h"
#include "hpguppi_databuf.h"

static void *run(hashpipe_thread_args_t * args)
{
    // Local aliases to shorten access to args fields
    hashpipe_databuf_t *db;
    hashpipe_status_t st = args->st;
    const char * status_key = args->thread_desc->skey;
    int rv;
    int block_idx = 0;

    /* Main loop */
    while (run_threads()) {

        hashpipe_status_lock_safe(&st);
        hputi4(st.buf, "OUTBLKIN", block_idx);
        hputs(st.buf, status_key, "waiting");
        hashpipe_status_unlock_safe(&st);
	sleep(1);
       // get new data
       while ((rv=hashpipe_databuf_wait_filled(db, block_idx))
                != HASHPIPE_OK) {
            if (rv==HASHPIPE_TIMEOUT) {
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "blocked");
                hashpipe_status_unlock_safe(&st);
                continue;
            } else {
                hashpipe_error(__FUNCTION__, "error waiting for filled databuf");
                pthread_exit(NULL);
                break;
            }
        }

       // Note processing status, current input block
       hashpipe_status_lock_safe(&st);
       hputs(st.buf, status_key, "processing");
       hputi4(st.buf, "NULBLKIN", block_idx);
       hashpipe_status_unlock_safe(&st);

       // Mark block as free
       hashpipe_databuf_set_free(db, block_idx);

       // Setup for next block
       block_idx = (block_idx + 1) % db->n_block;

       /* Will exit if thread has been cancelled */
       pthread_testcancel();
    }

    // Detach from databuf
    hashpipe_databuf_detach(db);
    //pthread_cleanup_pop(0); // databuf detach

    // Thread success!
    return THREAD_OK;
}

static hashpipe_thread_desc_t null_thread = {
    name: "null_output_thread",
    skey: "OUTSTAT",
    init: NULL, 
    run:  run,
    ibuf_desc: {NULL},
    obuf_desc: {NULL}
};

static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&null_thread);
}

