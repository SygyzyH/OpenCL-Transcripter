#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "math.h"
#include "../io/args.h"
#include "../io/oclapi.h"
#include "../std.h"

#define noerr(e) { if ((e)) { puts("MATH_H: Initialized math UNSUCCESSFULY."); fflush(stdout); return e; } }

bool minit = false;

// Initilize kerenels for math operations
/*
returns 0 on success
*/
int mainit() {
    int err;
    char *mprog = ldfile(MAT_PROG);
    err = register_from_src(&(const char *) { mprog }, 5, "matadd", "matsub", "matmul", "matadds", "matmuls");
    free(mprog);
    noerr(err);
    
    mprog = ldfile(MATH_PROG);
    err = register_from_src(&(const char *) { mprog }, 1, "stft");
    free(mprog);
    noerr(err);
    
    puts("MATH_H: Initialized math successfuly.");
    
    minit = true;
    
    return MNO_ERR;
}

/* Math functions */
/*
a - first matrix data
aw - matrix width
ah - matrix height
b - second matrix data or single value
bw - matrix width
bh - matrix height
res - pointer to reslut (allocates needed memory automaticly)
returns 0 on success
*/

int matmul(double *a, unsigned int aw, unsigned int ah,
           double *b, unsigned int bw, unsigned int bh, double **res) {
    if (!minit) return MUNINITIALIZED;
    
    unsigned int k, az, bz;
    
    if (aw == bh) {
        k = aw;
        az = ah;
        bz = bw;
    } else if (ah == bw) {
        k = ah;
        az = aw;
        bz = bh;
    } else {
        return MINVALID_ARG;
    }
    
    size_t gz[] = { az, bz };
    
    double *r = (double *) malloc(sizeof(double) * az * bz);
    run_kernel("matmul", 2, gz, NULL, a, aw * ah, OCLREAD | OCLCPY,
               b, bw * bh, OCLREAD | OCLCPY, bz, az, k, r, az * bz, OCLWRITE | OCLOUT);
    
    *res = r;
    
    return MNO_ERR;
}

int matadd(double *a, unsigned int aw, unsigned int ah, 
           double *b, unsigned int bw, unsigned int bh, double **res) {
    if (!minit) return MUNINITIALIZED;
    
    if (aw != bw || ah != bh) return MINVALID_ARG;
    
    size_t gz[] = { aw, ah };
    
    int sz = aw * ah;
    
    double *r = (double *) malloc(sizeof(double) * aw * ah);
    run_kernel("matadd", 2, gz, NULL, a, sz, OCLREAD | OCLCPY,
               b, sz, OCLREAD | OCLCPY, aw, ah, r, sz, OCLWRITE | OCLOUT);
    
    *res = r;
    
    return MNO_ERR;
}

int matsub(double *a, unsigned int aw, unsigned int ah, 
           double *b, unsigned int bw, unsigned int bh, double **res) {
    if (!minit) return MUNINITIALIZED;
    
    if (aw != bw || ah != bh) return MINVALID_ARG;
    
    size_t gz[] = { aw, ah };
    
    int sz = aw * ah;
    
    double *r = (double *) malloc(sizeof(double) * aw * ah);
    run_kernel("matsub", 2, gz, NULL, a, sz, OCLREAD | OCLCPY,
               b, sz, OCLREAD | OCLCPY, aw, ah, r, sz, OCLWRITE | OCLOUT);
    
    *res = r;
    
    return MNO_ERR;
}

int matmuls(double *a, unsigned int aw, unsigned int ah, double b, double **res) {
    // assert(a != *res)
    if (!minit) return MUNINITIALIZED;
    
    double *r = (double *) malloc(sizeof(double) * aw * ah);
    
    size_t gz[] = { aw, ah };
    
    run_kernel("matmuls", 2, gz, NULL, a, aw * ah, OCLREAD | OCLCPY,
               b, aw, ah, r, aw * ah, OCLWRITE | OCLOUT);
    
    *res = r;
    
    return MNO_ERR;
}

int matadds(double *a, unsigned int aw, unsigned int ah, double b, double **res) {
    // assert(a != *res)
    if (!minit) return MUNINITIALIZED;
    
    double *r = (double *) malloc(sizeof(double) * aw * ah);
    
    size_t gz[] = { aw, ah };
    
    run_kernel("matadds", 2, gz, NULL, a, aw * ah, OCLREAD | OCLCPY, 
               b, aw, ah, r, aw * ah, OCLWRITE | OCLOUT);
    
    *res = r;
    
    return MNO_ERR;
}

// Compute the stft
/*
src - source data
sz - size of the source data
framesize - size of each frame
windowsize - size of the windowing function
hopsize - size of each hop between DFTs
res - result (automaticly allocated)
op - either 
returns 0 on success
*/
int stft(double *src, int sz, int framesize, int windowsize, int fftlen,
         int hopsize, int sides, Mat **res) {
    if (!minit) return MUNINITIALIZED;
    
    int err;
    
    /* NOTE: as to avoid messing with returning complex numbers, the result is either only the complex
                solutions, only the real solutions, or their magnitude squared. This equates to the wave's amplitude,
                the waves phase, or the wave's power spectral density. */
    
    if (hopsize == 0) return MZERO_DIV;
    if (framesize == 0) return MZERO_DIV;
    if (fftlen < framesize) return MINVALID_ARG;
    
    int fbins;
    switch (sides) {
        case ONE_SIDED:
        fbins = ceil(fftlen / 2 + 1);
        break;
        
        case TWO_SIDED:
        case TWO_SIDED_CENTERED:
        fbins = fftlen;
        break;
        
        default:
        return MINVALID_ARG;
    }
    int frames = ceil(((sz - framesize) / hopsize) + 1);
    /*int fbins, frames;
	stftwh(sz, framesize, hopsize, &fbins, &frames);*/
    
    double *r = (double *) malloc(sizeof(double) * fbins * frames);
    
    size_t gz[] = { frames, fbins };
    
    printf("framesize: %d, windowsize: %d, fftlen: %d, hopsize: %d, fbins: %d, frames: %d\n",
           framesize, windowsize, fftlen, hopsize, fbins, frames);
    puts("stft input:"); for (int i = 0; i < sz; i++) printfu("%lf, ", src[i]); putsu();
    
    run_kernel("stft", 2, gz, NULL, src, sz, OCLREAD | OCLCPY,
               framesize, windowsize, hopsize, fbins, sides,
               r, fbins * frames, OCLWRITE | OCLOUT);
    
    *res = (Mat *) malloc(sizeof(Mat));
    (*res)->width = fbins; 
    (*res)->height = frames;
    (*res)->data = r;
    
    return MNO_ERR;
}

// Convert amps to decibels
/*
src - data source
sz - size of data
topdb - top decible allowed
op - operation type
returns 0 on success
*/
int amptodb(double **src, int sz, double topdb, int op) {	
	double *dsrc = *src;
	
	if (topdb < 0 && topdb != -1.0) return MINVALID_ARG;
	if (sz < 1) return MINVALID_ARG;
    
	double max = 0;
	double ref = 0;
	
    for (int i = 0; i < sz; i++) 
        if (dsrc[i] > max)
        max = dsrc[i];
    
	switch (op) {
		case SCALE_MAX:
		if (max == 0) return MZERO_DIV;
        ref = max;
		break;
		
		case SCALE_ONE:
		ref = 1;
		break;
        
        case SCALE_FIRST:
        ref = dsrc[0];
        break;
		
		default:
		return MINVALID_ARG;
	}
    
    for (int i = 0; i < sz; i++) {
		double conv = 10 * log10(MAX(dsrc[i], EPSIL)) - 10 * log10(MAX(ref, EPSIL));
		dsrc[i] = (topdb == -1.0)? conv : MAX(conv, 10 * log10(MAX(max, EPSIL)) - topdb);
    }
    
    return MNO_ERR;
}

// Converts frequency to mel
/*
f - frequency
returns frequency in mel
*/
double ftomel(double f) {
    return 2595.0 * log10(1.0 + (f / 700.0));
}

// Converts mel to frequency
/*
mel - mel
returns mel in frequency
*/
double meltof(double mel) {
    return 700 * (pow(10, mel / 2595) - 1);
}

// Make mel spectogram
/*
src - data source
sz - size of data
framesize - size of each frame in stft
windowsize - size of each window in stft (hann windowing by default)
hopsize - size of each hop in stft
bands - number of bands
fstart - starting frequency of filters
fend - ending frequency of filters
res - resulting spectogram (memory automaticly allocated)
returns 0 on success
*/
int melspec(double *src, int sz, int fs, int framesize, int windowsize, int fftlen, 
            int hopsize, int bands, double fstart, double fend, Mat **res) {
    int e = 0;
    
    if (framesize < 1 || windowsize < 1 || hopsize < 1 || bands < 1) return MINVALID_ARG;
    
    /* Extract STFT */
    
    Mat *stftr;
    if ((e = stft(src, sz, framesize, windowsize, fftlen, hopsize, TWO_SIDED, &stftr))) 
        return e;
    
    double *ft = stftr->data;
    int fbins = stftr->width, frames = stftr->height;
    
    if (fend == 0.0) fend = fbins;
    
    /* Convert amplitude to Decibels */
    
    amptodb(&ft, fbins * frames, -1, SCALE_ONE);
    
    /* Build the filter bank */
    
    double *fban = (double *) calloc(fftlen * bands, sizeof(double));
    
    double spacing = (double) (ftomel(fend) - ftomel(fstart)) / (bands + 1);
    double *bedges = (double *) malloc(sizeof(double) * (bands + 2));
    for (int i = 0; i < bands + 2; i++)
        bedges[i] = meltof(i * spacing);
    
    double *linf = (double *) malloc(sizeof(double) * fftlen);
    for (int i = 0; i < fftlen; i++) linf[i] = (double) fs * i / fftlen;
    puts("linf:");
    for (int i = 0; i < fftlen; i++) printfu("%lf, ", linf[i]);
    puts();
    
    int valide = 0;
    for (int i = 0; i < bands + 2; i++) if (bedges[i] - fs / 2 < sqrt(EPSIL)) valide++;
    printf("Valid edges: %d, total edges: %d\n", valide, bands + 2);
    
    double *p = (double *) calloc(valide, sizeof(double));
    for (int i = 0; i < valide; i++) {
        for (int j = 0; j < fftlen; j++) {
            if(linf[j] > bedges[i]) {
                p[i] = j;
                break;
            }
        }
    }
    puts("P:");
    for (int i = 0; i < valide; i++) printfu("%lf, ", p[i]);
    puts();
    //exit(0);
    
    double *bw = (double *) malloc(sizeof(double) * bands + 1);
    for (int i = 0; i < bands + 1; i++) bw[i] = bedges[i + 1] - bedges[i];
    
    puts("Computing filters...");
    for (int i = 0; i < valide - 2; i++) {
        // Rising side
        for (int j = p[i]; j < p[i + 1]; j++)
            fban[i + j * bands] = (linf[j] - bedges[i]) / bw[i];
        
        // Falling side
        for (int j = p[i + 1]; j < p[i + 2]; j++)
            fban[i + j * bands] = (bedges[i + 2] - linf[j]) / bw[i + 1];
    }
    
    free(bedges);
    free(linf);
    free(p);
    free(bw);
    
    puts("Filter bank...");
    for (int i = 0; i < fbins; i++) {
        for (int j = 0; j < bands; j++) {
            printfu("%lf, ", fban[j + i * bands]);
        } putsu();
    } puts("... Done");
    
    /* Apply filter bank to signal */
    
    *res = (Mat *) malloc(sizeof(Mat));
    (*res)->width = bands;
    (*res)->height = frames;
    matmul(fban, bands, fbins, ft, fbins, frames, &((*res)->data));
    printf("melspec: rw: %d, rh: %d\n", bands, frames);
    
    /* Cleanup */
    
    free(ft);
    free(stftr);
    free(fban);
    
    return MNO_ERR;
}