/*upchan_bf_cpu_thread.c
 *
 * Beamforming in CPU
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <fftw3.h>
#include <unistd.h>
#include "hashpipe.h"
#include "meerkat_databuf.h"

static void *run(hashpipe_thread_args_t * args)
{
    // Local aliases to shorten access to args fields
    input_databuf_t *db_in = (input_databuf_t *)args->ibuf;
    output_databuf_t *db_out = (output_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char * status_key = args->thread_desc->skey;

    int rv;
    int nbeams=65;
    int nants=64;
    int npols=2;
    int nupchan = 4;
    int nchans_in = 1;
    int nchans_out = nchans_in*nupchan; //2**19;
    int nsamps_in = 8*nupchan;
    int nsamps_out = nsamps_in/nupchan;

    int input_len = nants*nchans_in*nsamps_in*npols*2;
    int output_len = nbeams*nchans_out*nsamps_out;
    float* host_output;
    char* host_input;
    float* host_phase;
    float* upchan_output;
    host_output = (float*)malloc(output_len * sizeof(float));
    host_input = (char*)malloc(input_len * sizeof(char));
    host_phase = (float*)malloc(nbeams*nants*2*sizeof(float));
    upchan_output = (float*)malloc(input_len*sizeof(float));
    uint64_t mcnt=0;
    int curblock_in=0;
    int curblock_out=0;
    while (run_threads()) {

        hashpipe_status_lock_safe(&st);
        hputi4(st.buf, "CPUBLKIN", curblock_in);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "CPUBKOUT", curblock_out);
	hputi8(st.buf,"CPUMCNT",mcnt);
        hashpipe_status_unlock_safe(&st);
	sleep(1);
        // Wait for new input block to be filled
        while ((rv=input_databuf_wait_filled(db_in, curblock_in)) != HASHPIPE_OK) {
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

        // Wait for new output block to be free
        while ((rv=output_databuf_wait_free(db_out, curblock_out)) != HASHPIPE_OK) {
            if (rv==HASHPIPE_TIMEOUT) {
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "blocked cpu out");
                hashpipe_status_unlock_safe(&st);
                continue;
            } else {
                hashpipe_error(__FUNCTION__, "error waiting for free databuf");
                pthread_exit(NULL);
                break;
            }
        }

        // Note processing status
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "processing cpu");
        hashpipe_status_unlock_safe(&st);

	//Calcuate phase------------------------------------------------------------
	//Do we want it here or as a separate thread? will need to update every second.
	for (int b = 0; b < nbeams; b++) {
	  for (int i = 0; i < nants; i++) {
	    host_phase[(b*nants+i)*2] = b;   //Real
	    host_phase[(b*nants+i)*2+1] = b; //Imag
	  }
	}
	  
	//Read input  [ant-freq-time-pol]
        host_input =db_in->block[curblock_in].data_block;
	hputi4(st.buf,"in[0]",host_input[0]);
	hputi4(st.buf,"in[1]",host_input[1]);
	hputi4(st.buf,"in[2]",host_input[2]);
	hputi4(st.buf,"in[-3]",host_input[input_len-3]);
	hputi4(st.buf,"in[-2]",host_input[input_len-2]);	
	hputi4(st.buf,"in[-1]",host_input[input_len-1]);


	//Upchannelize with an FFT---------------------------------------------------
	fftwf_complex *fftbuf;
	fftwf_plan fplan;
	fftbuf = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex)*nupchan);
	fplan = fftwf_plan_dft_1d(nupchan, fftbuf, fftbuf, FFTW_FORWARD, FFTW_MEASURE);

	for (int i = 0 ; i < nants*nchans_in*(nsamps_in/nupchan) ; i++) {
	  for (int p = 0 ; p < npols ; p++){
	    for (int j = 0; j < nupchan ; j++){
	      fftbuf[j][0] = host_input[(i*nupchan*npols + j*npols + p)*2]; // real
	      fftbuf[j][1] = host_input[(i*nupchan*npols + j*npols + p)*2+1]; // imag
	    }
	    fftwf_execute(fplan);
	    for (int j = 0; j < nupchan ; j++){
	      upchan_output[(i*nupchan*npols + j*npols + p)*2] = fftbuf[j][0]; //real
	      upchan_output[(i*nupchan*npols + j*npols + p)*2+1] = fftbuf[j][1]; //imag
	    }
	  }
	}
	fftwf_free(fftbuf); 
	fftwf_destroy_plan(fplan); 
	
	hputi4(st.buf,"upch[0]",upchan_output[0]);
	hputi4(st.buf,"upch[1]",upchan_output[1]);
	hputi4(st.buf,"upch[2]",upchan_output[2]);
	hputi4(st.buf,"upch[-3]",upchan_output[input_len-3]);
	hputi4(st.buf,"upch[-2]",upchan_output[input_len-2]);
	hputi4(st.buf,"upch[-1]",upchan_output[input_len-1]);

	float sum_re, sum_im;
	//incoherent beamform by adding all antennas--------------------------------
	for (int ft = 0 ; ft < nchans_out*nsamps_out ; ft++) {
	    for (int p = 0 ; p < npols ; p++) {
	      sum_re = 0.0;
	      sum_im = 0.0;
	      for (int i = 0 ; i < nants ; i++) {
		int in_id = (i*nchans_out*nsamps_out*npols + ft*npols + p)*2;
		sum_re += upchan_output[in_id];
		sum_im += upchan_output[in_id+1];
	      }
	      host_output[ft] += sum_re*sum_re + sum_im*sum_im;
	    }
	}
	
	//coherent beamform by multiplying phase
	for (int ft = 0 ; ft < nchans_out*nsamps_out ; ft++) {
	  for (int p = 0 ; p < npols ; p++) {
	    for (int b = 1 ; b < nbeams ; b++) { //starting from b1 since b0 is incoherent
	      sum_re = 0.0;
	      sum_im = 0.0;
	      int out_id = b*nchans_out*nsamps_out + ft;
	      for (int i = 0 ; i < nants ; i++) {
		int ph_id = (b*nants+i)*2;
		int in_id = (i*nchans_out*nsamps_out*npols + ft*npols + p)*2;
		sum_re += upchan_output[in_id] * host_phase[ph_id] + upchan_output[in_id+1]*host_phase[ph_id+1];
		sum_im += upchan_output[in_id+1]*host_phase[ph_id] - upchan_output[in_id]*host_phase[ph_id+1];
	      }
	      host_output[out_id] += sum_re*sum_re + sum_im*sum_im; //sum Real+Imag and pol
	    }
	  }
	}
	  
	
	sleep(1);
	memcpy(db_out->block[curblock_out].output, host_output, output_len*sizeof(float));
	
        // Mark output block as full and advance
        output_databuf_set_filled(db_out, curblock_out);
        curblock_out = (curblock_out + 1) % db_out->header.n_block;

        // Mark input block as free and advance
        input_databuf_set_free(db_in, curblock_in);
        curblock_in = (curblock_in + 1) % db_in->header.n_block;
	mcnt++;
	//display sum in status
	hashpipe_status_lock_safe(&st);
	hputi4(st.buf,"CPU b0",host_output[0]);
	hputi4(st.buf,"CPU b1",host_output[nchans_out*nsamps_out]);
	hputi4(st.buf,"CPU b2",host_output[2*nchans_out*nsamps_out]);
	hashpipe_status_unlock_safe(&st);
        /* Check for cancel */
        pthread_testcancel();


    }
    return NULL;
    free(host_input);
    free(host_output);
    free(host_phase);
    free(upchan_output);

}

static hashpipe_thread_desc_t upchan_bf_cpu_thread = {
    name: "upchan_bf_cpu_thread",
    skey: "CPUSTAT",
    init: NULL,
    run:  run,
    ibuf_desc: {input_databuf_create},
    obuf_desc: {output_databuf_create}
};

static __attribute__((constructor)) void ctor()
{
  register_hashpipe_thread(&upchan_bf_cpu_thread);
}

