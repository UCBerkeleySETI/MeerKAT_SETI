#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include "hashpipe.h"
#include "readfile_databuf.h"
#include "rawspec_rawutils.h"

ssize_t read_fully(int fd, void * buf, size_t bytes_to_read)
{
  ssize_t bytes_read;
  ssize_t total_bytes_read = 0;

  while(bytes_to_read > 0) {
    bytes_read = read(fd, buf, bytes_to_read);
    if(bytes_read <= 0) {
      if(bytes_read == 0) {
	break;
      } else {
	return -1;
      }
    }
    buf += bytes_read;
    bytes_to_read -= bytes_read;
    total_bytes_read += bytes_read;
  }

  return total_bytes_read;
}

static void *run(hashpipe_thread_args_t * args)
{
    readfile_input_databuf_t *db  = (readfile_input_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char * status_key = args->thread_desc->skey;

    /* Main loop */
    int i, rv,input;
    uint64_t mcnt = 0;
    int block_idx = 0;
    off_t pos;
    int fdin;
    rawspec_raw_hdr_t raw_hdr;
    size_t bytes_read;
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
        while ((rv=readfile_input_databuf_wait_free(db, block_idx)) 
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

	//Read raw files
	fdin = open("/datax/20200626/commensal/Unknown/GUPPI/guppi_59025_41038_003850_Unknown_0001.0000.raw", O_RDONLY);
	if(fdin == -1) {
	  hashpipe_error(__FUNCTION__, "cannot open raw file");
	  pthread_exit(NULL);
	  break;
	}
	printf("\n");
	//Read header
	pos = rawspec_raw_read_header(fdin, &raw_hdr);
	if(pos <= 0) {
	  if(pos == -1) {
	    hashpipe_error(__FUNCTION__, "error getting obs params from header");
	  } else {
	    hashpipe_error(__FUNCTION__, "no data found in header");
	  }
	  close(fdin);
	  pthread_exit(NULL);
	  break;
	}
	printf("raw_hdr.obsnchan %d\n", raw_hdr.obsnchan);
	printf("raw_hdr.nants %d\n", raw_hdr.nants);
	printf("Current file position=%ld\n",pos);
	
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "receiving");
        hashpipe_status_unlock_safe(&st);
	

	//Read data (say 200B for now)
	int input_len = 200;
	unsigned char input_data[input_len];
	bytes_read = read_fully(fdin,input_data, input_len);
	close(fdin);

	//move these two numbers to buffer
	db->block[block_idx].header.mcnt = mcnt;
	memcpy(db->block[block_idx].data_block, input_data, input_len*sizeof(unsigned char));

	//display some values
	hashpipe_status_lock_safe(&st);
	hputi8(st.buf,"data[0]",input_data[0]);
	hputi8(st.buf,"data[1]",input_data[1]);
	hputi8(st.buf,"data[-2]",input_data[input_len-2]);
	hputi8(st.buf,"data[-1]",input_data[input_len-1]);

	hashpipe_status_unlock_safe(&st);
        // Mark block as full
        readfile_input_databuf_set_filled(db, block_idx);

        // Setup for next block
        block_idx = (block_idx + 1) % db->header.n_block;
        mcnt++;

        /* Will exit if thread has been cancelled */
        pthread_testcancel();
    }

    // Thread success!
    return THREAD_OK;
}

static hashpipe_thread_desc_t read_raw_file = {
    name: "read_raw_file",
    skey: "NETSTAT",
    init: NULL,
    run:  run,
    ibuf_desc: {NULL},
    obuf_desc: {readfile_input_databuf_create}
};

static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&read_raw_file);
}
