/*
 * meerkat_net_thread.c
 *
 * Routine to write fake data into shared memory blocks.  This allows the
 * processing pipelines to be tested without the network portion of SERENDIP6.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include "hashpipe.h"
#include "meerkat_databuf.h"

static void *run(hashpipe_thread_args_t * args)
{
    input_databuf_t *db  = (input_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char * status_key = args->thread_desc->skey;

    /* Main loop */
    int i, rv,input;
    uint64_t mcnt = 0;
    int block_idx = 0;
    unsigned long header; // 64 bit counter

    int nants=64;
    int npols=2;
    int nchans=4; //2**19;
    int nsamps=8;
    int input_len = nants*nchans*nsamps*npols*2;
    unsigned char input_data[input_len];

    while (run_threads()) {

        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "NETBKOUT", block_idx);
	hputi8(st.buf,"NETMCNT",mcnt);
        hashpipe_status_unlock_safe(&st);
	sleep(1); 
        // Wait for data
        /* Wait for new block to be free, then clear it
         * if necessary and fill its header with new values.
         */
        while ((rv=input_databuf_wait_free(db, block_idx)) 
                != HASHPIPE_OK) {
            if (rv==HASHPIPE_TIMEOUT) {
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "blocked");
                hashpipe_status_unlock_safe(&st);
                continue;
            } else {
                hashpipe_error(__FUNCTION__, "error waiting for free databuf");
                pthread_exit(NULL);
                break;
            }
        }

        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "receiving");
        hashpipe_status_unlock_safe(&st);
	

	for(int i=0;i<input_len;i++){
	    input_data[i] = 1;
	}
	//move input data to buffer
	db->block[block_idx].header.mcnt = mcnt;  
	memcpy(db->block[block_idx].data_block, input_data, input_len*sizeof(unsigned char));

	//display first two values 
	hashpipe_status_lock_safe(&st);
	hputi8(st.buf,"Input[0]",input_data[0]);
	hputi8(st.buf,"Input[1]",input_data[1]);
	hputi8(st.buf,"Input[2]",input_data[2]);
	hputi8(st.buf,"Input[3]",input_data[3]);
	hputi8(st.buf,"Input[4]",input_data[4]);
	hashpipe_status_unlock_safe(&st);
	
        // Mark block as full
        input_databuf_set_filled(db, block_idx);

        // Setup for next block
        block_idx = (block_idx + 1) % db->header.n_block;
        mcnt++;

        /* Will exit if thread has been cancelled */
        pthread_testcancel();
    }

    // Thread success!
    return THREAD_OK;
}

static hashpipe_thread_desc_t meerkat_net_thread = {
    name: "meerkat_net_thread",
    skey: "NETSTAT",
    init: NULL,
    run:  run,
    ibuf_desc: {NULL},
    obuf_desc: {input_databuf_create}
};

static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&meerkat_net_thread);
}
