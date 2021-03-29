#define MAX_HDR_SIZE (256000)

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
#include "hpguppi_databuf.h"
#include "rawspec_rawutils.h"

int get_header_size(char * header_buf, size_t len)
{
  int i;
  //Read header loop over the 80-byte records
  for (i=0; i<len; i += 80) {
    // If we found the "END " record
    if(!strncmp(header_buf+i, "END ", 4)) {
      // Move to just after END record
      i += 80;
      break;
    }
  }
  return i;
}

int get_block_size(char * header_buf, size_t len)
{
  int i;
  char bs_str[32];
  int blocsize;
  //Read header loop over the 80-byte records
  for (i=0; i<len; i += 80) {
    if(!strncmp(header_buf+i, "BLOCSIZE", 8)) {
      strncpy(bs_str,header_buf+i+16, 32);
      //bs_str[31] = '\0';
      blocsize = strtoul(bs_str,NULL,0);
      break;
    }
  }
  return blocsize;
}
  
void set_output_path(char * header_buf, size_t len, char outpath)
{
  int i;
  char datadir[1024];
  //Read header loop over the 80-byte records
  for (i=0; i<len; i += 80) {
    if(!strncmp(header_buf+i, "DATADIR", 7)) {
      //strncpy(datadir,header_buf+i+16, 32);
      //printf("current path %s\n",datadir);
      //strncpy(header_buf+i+16, "/scratch/Cherry/test/\0", 30);
      //header_buf[i+16+29] = '\0';
      hputs(header_buf, "DATADIR", "/buf0/scratch/Cherry/test/");
      //strncpy(datadir,header_buf+i+16, 32);
      //printf("new path %s\n",datadir);
      //strncpy(datadir, outpath, 1023);
      break;
    }
  }
  return 0;
}

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
    hpguppi_input_databuf_t *db  = (hpguppi_input_databuf_t *)args->obuf;
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
    char *ptr;
    int len;
    int directio = 0;
    char header_buf[MAX_HDR_SIZE];
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
        while ((rv=hpguppi_input_databuf_wait_free(db, block_idx)) 
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
	//fdin = open("/datax/20200626/commensal/Unknown/GUPPI/guppi_59025_41038_003850_Unknown_0001.0000.raw", O_RDONLY);
	fdin = open("/buf0/scratch/Cherry/Data/guppi_59204_70813_002321__0001.0000.raw", O_RDONLY);
	if(fdin == -1) {
	  hashpipe_error(__FUNCTION__, "cannot open raw file");
	  pthread_exit(NULL);
	  break;
	}
	printf("\n");

	read(fdin, header_buf, MAX_HDR_SIZE);
	int pos = get_header_size(header_buf, MAX_HDR_SIZE);
	printf("pos is %d\n", pos);

        set_output_path(header_buf, MAX_HDR_SIZE, "test");

	//Write header info to buf
	char *header = hpguppi_databuf_header(db, block_idx);
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "receiving");
	memcpy(header, &header_buf, pos);
        hashpipe_status_unlock_safe(&st);

	
	//Read data
	int blocsize = get_block_size(header_buf, MAX_HDR_SIZE);
	printf("Block size %d\n",blocsize);
	ptr = hpguppi_databuf_data(db, block_idx);
	bytes_read = read_fully(fdin, ptr, blocsize);
	close(fdin);

	//display some values
	hashpipe_status_lock_safe(&st);
	//hputi8(st.buf,"data[0]",db[0]);
	//hputi8(st.buf,"data[1]",db[1]);
	//hputi8(st.buf,"data[-2]",input_data[input_len-2]);
	//hputi8(st.buf,"data[-1]",input_data[input_len-1]);

	hashpipe_status_unlock_safe(&st);
        // Mark block as full
        hpguppi_input_databuf_set_filled(db, block_idx);

        // Setup for next block
        block_idx = (block_idx + 1) % N_INPUT_BLOCKS;
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
    obuf_desc: {hpguppi_input_databuf_create}
};

static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&read_raw_file);
}
