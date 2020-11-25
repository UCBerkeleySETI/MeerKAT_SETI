/*upchan_bf_cpu_thread.c
 *
 * Beamforming in CPU
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <fftw3.h>
#include <unistd.h>
#include "hashpipe.h"
#include "meerkat_databuf.h"
#include "metadata.h" 

#define PI 3.14159265
#define light 299792458.
#define one_over_c 0.0033356
#define R2D (180. / PI)
#define D2R (PI / 180.)
#define TAU (2 * PI)
#define inst_long 20.4439
#define inst_lat 30.7111  //Do we need to take altitude into account?

void calculate_phase(int nbeams, int nants, uint32_t nchans_in, struct timespec time_now, float* freq_now, struct beamCoord beam_coord, struct antCoord ant_coord, double* phase)
{
  struct tm* timeinfo;
  timeinfo = localtime(&time_now.tv_sec);
  uint year = timeinfo->tm_year + 1900;
  uint month = timeinfo->tm_mon + 1;
  if (month < 3) {
    month = month + 12;
    year = year - 1;
  }
  uint day = timeinfo->tm_mday;
  float JD = 2 - (int)(year / 100.) + (int)(int)((year / 100.) / 4.) + (int)(365.25 * year)
    + (int)(30.6001 * (month + 1)) + day + 1720994.5;
  double T = (JD - 2451545.0)
    / 36525.0; // Works if time after year 2000, otherwise T is -ve and might break
  double T0 = fmod((6.697374558 + (2400.051336 * T) + (0.000025862 * T * T)), 24.);
  double UT = (timeinfo->tm_hour) + (timeinfo->tm_min / 60.)
    + (timeinfo->tm_sec + time_now.tv_nsec / 1.e9) / 3600.;
  double GST = fmod((T0 + UT * 1.002737909), 24.);
  double LST = GST + inst_long / 15.;
  while (LST < 0) {
    LST = LST + 24;
  }
  LST = fmod(LST, 24);

  for (int b = 0; b < nbeams; b++) {
    double hour_angle = LST * 15. - beam_coord.ra[b];
    double alt = sin(beam_coord.dec[b] * D2R) * sin(inst_lat * D2R)
                 + cos(beam_coord.dec[b] * D2R) * cos(inst_lat * D2R) * cos(hour_angle * D2R);
    alt = asin(alt);
    double az = (sin(beam_coord.dec[b] * D2R) - sin(alt) * sin(inst_lat * D2R))
      / (cos(alt) * cos(inst_lat * D2R));
    az = acos(az);
    if (sin(hour_angle * D2R) >= 0) {
      az = TAU - az;
    }
    double projection_angle, effective_angle, offset_distance;
    for (int i = 0; i < nants ; i++){
      float dist_y = ant_coord.north[i];
      float dist_x = ant_coord.east[i];
      projection_angle = 90 * D2R - atan2(dist_y, dist_x); //Need to check this
      offset_distance = sqrt(pow(dist_y, 2) + pow(dist_x, 2));
      effective_angle = projection_angle - az;
      for (int f = 0; f < nchans_in ; f++ ) {
	//phase [freq-beam-ant]
	int ph_id = f*nbeams*nants + b*nants + i;
	phase[ph_id*2] = cos(TAU * cos(effective_angle) * cos(alt) * offset_distance
			     * freq_now[f] * one_over_c);
	phase[ph_id*2+1] = -sin(TAU * cos(effective_angle) * cos(-alt) * offset_distance
				* freq_now[f] * one_over_c);
      }
    }
  }
}

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
    int nupchan = 4; //should be 2**19 for 1k mode
    int nchans_in = 1;
    int nchans_out = nchans_in*nupchan; 
    int nsamps_in = 8*nupchan;
    int nsamps_out = nsamps_in/nupchan;

    int input_len = nants*nchans_in*nsamps_in*npols*2;
    int output_len = nbeams*nchans_out*nsamps_out;
    float* host_output;
    char* host_input;
    double* host_phase;
    float* upchan_output;
    host_output = (float*)malloc(output_len * sizeof(float));
    host_input = (char*)malloc(input_len * sizeof(char));
    host_phase = (double*)malloc(nchans_in*(nbeams-1)*nants*2*sizeof(double)); //assume one phase correction for each coarse freq
    upchan_output = (float*)malloc(input_len*sizeof(float));
    float* freq_now = (float*)malloc(nchans_in*sizeof(float));
    struct timespec time_now;
    struct beamCoord beam_coord;
    struct antCoord ant_coord;
    
    fftwf_complex *fftbuf;
    fftwf_plan fplan;
    fftbuf = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex)*nupchan);
    fplan = fftwf_plan_dft_1d(nupchan, fftbuf, fftbuf, FFTW_FORWARD, FFTW_MEASURE);

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

	//Parse info from metadata (hardcoded for now)-------------------------------
	//Time: Do we want it here or as a separate thread? will need to update every second.
	time_now.tv_sec = 100.;
	time_now.tv_nsec = 200.;
	//Freq
	for (int f = 0 ; f < nchans_in ; f++ ) {
	  freq_now[f] = 700.0 + f*0.00836;
	}
	//Antenna position (read from lookup table, only need to do it once)
	for (int i = 0 ; i < nants; i++ ) {
	  scanf("%f %f %f", &ant_coord.north[i], &ant_coord.east[i], &ant_coord.up[i] ;)
	}
	//Beam coordinatees
	for (int b = 0; b< nbeams-1; b++){
	  beam_coord.ra[b] = 128.83588121;
	  beam_coord.dec[b] = -45.17635419;
	}
	
	//Calcuate phase------------------------------------------------------------
	calculate_phase(nbeams-1, nants, nchans_in, time_now, freq_now, beam_coord, ant_coord, host_phase);
	  
	//Read input  [ant-freq-time-pol]
        host_input =db_in->block[curblock_in].data_block;
	hputi4(st.buf,"in[0]",host_input[0]);
	hputi4(st.buf,"in[1]",host_input[1]);
	hputi4(st.buf,"in[2]",host_input[2]);
	hputi4(st.buf,"in[-3]",host_input[input_len-3]);
	hputi4(st.buf,"in[-2]",host_input[input_len-2]);	
	hputi4(st.buf,"in[-1]",host_input[input_len-1]);

	//Upchannelize with an FFT---------------------------------------------------
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
	
	hputi4(st.buf,"upch[0]",upchan_output[0]);
	hputi4(st.buf,"upch[1]",upchan_output[1]);
	hputi4(st.buf,"upch[2]",upchan_output[2]);
	hputi4(st.buf,"upch[-3]",upchan_output[input_len-3]);
	hputi4(st.buf,"upch[-2]",upchan_output[input_len-2]);
	hputi4(st.buf,"upch[-1]",upchan_output[input_len-1]);

	//incoherent beamform by adding all antennas--------------------------------
	for (uint32_t ft = 0 ; ft < nchans_out*nsamps_out ; ft++) {
	  float tmp_real = 0.0;
	  float tmp_imag = 0.0;
	  float out_sq = 0.0;
	  for (int p = 0 ; p < npols ; p++) {
	    for (int i = 0 ; i < nants ; i++) {
	      int in_id = (i*nchans_out*nsamps_out*npols + ft*npols + p)*2;
	      tmp_real = upchan_output[in_id];
	      tmp_imag = upchan_output[in_id+1];
	      out_sq += tmp_real*tmp_real + tmp_imag*tmp_imag;		    
	    }
	  }
	  //host_output [beam->freq->time]
	  host_output[ft] = out_sq;
	}

	hputi4(st.buf,"incoh[0]",host_output[0]);
	hputi4(st.buf,"incoh[-1]",host_output[nchans_out*nsamps_out-1]);
	
	//coherent beamform by multiplying phase
	for (uint32_t f = 0 ; f < nchans_out ; f++) {
	  int f_coarse = (int)f/nupchan;
	  for (uint32_t t = 0 ; t < nsamps_out ; t++ ) {
	    uint32_t ft = f*t;
	    for (int b = 1 ; b < nbeams ; b++) { //starting from b1 since b0 is incoherent
	      float out_sq = 0.0;
	      int out_id = b*nchans_out*nsamps_out + ft;
	      for (int p = 0 ; p < npols ; p++) {
		float tmp_real = 0.0;
		float tmp_imag = 0.0;
		//multiply-add the 64 antennas with phase offset
		for (int i = 0 ; i < nants ; i++) {
		  uint32_t ph_id = (f_coarse*(nbeams-1)*nants + (b-1)*nants+i)*2;
		  uint32_t in_id = (i*nchans_out*nsamps_out*npols + ft*npols + p)*2;
		  tmp_real += upchan_output[in_id] * host_phase[ph_id] + upchan_output[in_id+1]*host_phase[ph_id+1];
		  tmp_imag += upchan_output[in_id+1]*host_phase[ph_id] - upchan_output[in_id]*host_phase[ph_id+1];
		}
		out_sq += tmp_real*tmp_real + tmp_imag*tmp_imag; //sum Real+Imag and pol
	      }
	      //host_output [beam->freq->time]
	      host_output[out_id] = out_sq;
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
	hputi4(st.buf,"out[0]",host_output[0]);
	hputi4(st.buf,"out[1]",host_output[1]);
	hputi4(st.buf,"out[2]",host_output[2]);
	hputi4(st.buf,"out[-3]",host_output[nbeams*nchans_out*nsamps_out-3]);
	hputi4(st.buf,"out[-2]",host_output[nbeams*nchans_out*nsamps_out-2]);
	hputi4(st.buf,"out[-1]",host_output[nbeams*nchans_out*nsamps_out-1]);
	hashpipe_status_unlock_safe(&st);
        /* Check for cancel */
        pthread_testcancel();


    }
    return NULL;
    free(host_input);
    free(host_output);
    free(host_phase);
    free(upchan_output);
    free(freq_now);
    fftwf_free(fftbuf); 
    fftwf_destroy_plan(fplan); 

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

