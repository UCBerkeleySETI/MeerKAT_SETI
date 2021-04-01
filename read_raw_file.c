#define MAX_HDR_SIZE (6000)

#define _GNU_SOURCE 1
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
//#include "hpguppi_params.h"
//#include "hpguppi_util.h"
#include "rawspec_rawutils.h"
#include <time.h>

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
  
void set_output_path(char * header_buf, size_t len)
{
  int i;
  char datadir[1024];
  //Read header loop over the 80-byte records
  for (i=0; i<len; i += 80) {
    if(!strncmp(header_buf+i, "DATADIR", 7)) {
      hputs(header_buf, "DATADIR", "/buf0/scratch/Cherry/test/");
      break;
    }
  }
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
    //uint64_t mcnt = 0;
    int block_idx = 0;
    int block_count=0, blocks_per_file=128, filenum=0;

    size_t pos;

    rawspec_raw_hdr_t raw_hdr;
    size_t bytes_read;
    char *ptr;
    int len;
    int directio = 0;
    char header_buf[MAX_HDR_SIZE];
    int open_flags = 0;
    clock_t start, end, start_loop, end_loop;
    double cpu_time_used;
    int blocsize;

    /* Init output file descriptor (-1 means no file open) */
    static int fdin = -1;

    directio = 1; //hpguppi_read_directio_mode(ptr);
    char basefilename[200];
    char fname[256];
    sprintf(basefilename, "%s","/buf0/scratch/Cherry/Data/guppi_59204_70813_002321__0001");

    while (run_threads()) {
        start = clock();
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "NETBKOUT", block_idx);
	//hputi8(st.buf,"NETMCNT",mcnt);
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
        end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("    TIME: waiting for data %f\n", cpu_time_used);

	//Read raw files
	if (fdin == -1) { //no file opened
	  start_loop = clock();
	  start = clock();
	  sprintf(fname, "%s.%04d.raw", basefilename, filenum);
	  printf(stderr, "Opening first raw file '%s' (directio=%d)\n", fname, directio);
	  
	  open_flags = O_CREAT|O_RDWR|O_SYNC;
	  /*if(directio) {
	    open_flags |= O_DIRECT;
	    }*/
	  fdin = open(fname, open_flags, 0644);
	  if (fdin==-1) {
	    hashpipe_error(__FUNCTION__,"Error opening file.");
	    pthread_exit(NULL);
	  }
	  end = clock();
	  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	  //printf("    TIME: open file %f\n", cpu_time_used);
	}

	
	//Find out header size
	start = clock();
	pos = lseek(fdin, 0, SEEK_CUR);
	//printf("  Before reading anything, at pos=%ld\n",pos);
	read(fdin, header_buf, MAX_HDR_SIZE);
	int headersize= get_header_size(header_buf, MAX_HDR_SIZE);
	//printf("Header size before adjustment %d\n", headersize);
	// Adjust length for any padding required for DirectIO
	if(directio) {
	  // Round up to next multiple of 512
	  headersize = (headersize+511) & ~511;
	}
	printf("Reading block_count=%d idx=%d header size=%d------------------\n", block_count, block_idx, headersize);
        set_output_path(header_buf, MAX_HDR_SIZE);

	//Write header info from header_buf to buf
	char *header = hpguppi_databuf_header(db, block_idx);
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "receiving");
	memcpy(header, &header_buf, headersize);
        hashpipe_status_unlock_safe(&st);

	//Read data
	pos = lseek(fdin, 0, SEEK_CUR);
	printf("  After read header, at pos=%ld, need to seek by %d\n", pos, headersize-MAX_HDR_SIZE);
	pos = lseek(fdin, headersize-MAX_HDR_SIZE, SEEK_CUR);
	//	printf("  After seek back, at %ld\n", pos);
	blocsize = get_block_size(header_buf, MAX_HDR_SIZE);
	ptr = hpguppi_databuf_data(db, block_idx);
	bytes_read = read_fully(fdin, ptr, blocsize);
	pos = lseek(fdin, 0, SEEK_CUR);
	//printf(  "  After reading data block, at %ld\n", pos);
	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	//printf("    TIME: read header and data %f\n", cpu_time_used);
	
	start = clock();
	// Mark block as full
	hpguppi_input_databuf_set_filled(db, block_idx);

        // Setup for next block
        block_idx = (block_idx + 1) % N_INPUT_BLOCKS;
        block_count++;
	printf("  Filled db, increment to block_idx=%d block_count=%d \n", block_idx, block_count);
	
	/* See if we need to open next file */
	if (block_count>= blocks_per_file) {
	  close(fdin);
	  end_loop= clock();
	  cpu_time_used = ((double) (end_loop - start_loop)) / CLOCKS_PER_SEC;
	  printf("    TIME: one file %f\n", cpu_time_used);
	  filenum++;
	  sprintf(fname, "%s.%4.4d.raw", basefilename, filenum);
	  open_flags = O_CREAT|O_RDWR|O_SYNC;
	  /*if(directio) {
	    open_flags |= O_DIRECT;
	    }*/
	  start_loop = clock();
	  printf(stderr, "Opening next raw file '%s' (directio=%d)\n", fname, directio);
	  fdin = open(fname, open_flags, 0644);
	  if (fdin==-1) {
	    hashpipe_error(__FUNCTION__,"Error opening file.");
	    pthread_exit(NULL);
	  }
	  block_count=0;
	}

        /* Will exit if thread has been cancelled */
        pthread_testcancel();

	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	//printf("    TIME: tidy up %f\n", cpu_time_used);
	
    }
    
    // Thread success!
    if (fdin!=-1) {
      close(fdin);
    }
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
