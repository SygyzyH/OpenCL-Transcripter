#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "math.h"
#include "../io/args.h"
#include "../io/oclapi.h"

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
    
    puts("456");
    
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
int stft(double *src, int sz, int framesize, int windowsize, int hopsize,
         double **res, int op) {
    if (!minit) return MUNINITIALIZED;
    
    int err;
    
    /* NOTE: as to avoid messing with returning complex numbers, the result is either only the complex
                solutions, only the real solutions, or their magnitude squared. This equates to the wave's amplitude,
                the waves phase, or the wave's power spectral density. */
    
    if (hopsize == 0) return MZERO_DIV;
    if (framesize == 0) return MZERO_DIV;
    
    int fbins, frames;
	stftwh(sz, framesize, hopsize, &fbins, &frames);
    
    double *r = (double *) malloc(sizeof(double) * fbins * frames);
    
    size_t gz[] = { frames, fbins };
    
    run_kernel("stft", 2, gz, NULL, src, sz, OCLREAD | OCLCPY,
               framesize, windowsize, hopsize, r, fbins * frames, OCLWRITE | OCLOUT);
    
    *res = r;
    
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
int melspec(double *src, int sz, int framesize, int windowsize, int hopsize, int bands,
            double fstart, double fend, double **res) {
    int e = 0;
    
    if (framesize < 1 || windowsize < 1 || hopsize < 1 || bands < 1) return MINVALID_ARG;
    
    int fbins, frames;
	stftwh(sz, framesize, hopsize, &fbins, &frames);
    
    if (fend == 0.0) fend = fbins;
    
    /* Extract STFT */
    
    double *ft;
    if ((e = stft(src, sz, framesize, windowsize, hopsize, &ft, MAGSQ))) 
        return e;
    
    /* Convert amplitude to Decibels */
    
    amptodb(&ft, fbins * frames, -1, SCALE_ONE);
    
    /* Build the filter bank */
    
    double minmel = ftomel(fstart);
    double maxmel = ftomel(fend);
    double spacing = (double) (maxmel - minmel) / (bands - 1);
    
    // Transcribed from Apple
    /* https://developer.apple.com/documentation/accelerate/computing_the_mel_spectrum_using_linear_algebra */
    int *filters = (int *) malloc(sizeof(double) * bands);
    // 'i' goes up to maxmel + spacing to include the last iteration
    for (double i = minmel; i < maxmel + spacing; i += spacing)
        filters[(int) ((i - minmel) / spacing)] = (int) fbins * meltof(i) / fend;
    
    // TODO: This should run on the GPU
    /* Create triangular filters */
    // Filter bank shold have bands rows and fbin columns
    double *fban = (double *) malloc(sizeof(double) * fbins * bands);
    /* Wish I could imbed pictures in my comments... TODO: 4coder MD comments?*/
    for (int i = 0; i < bands; i++) {
        // Special case for the first band - the start of the filter is
        // at frequency bin 0
        int start = 0;
        if (i > 0)
            start = filters[i - 1];
        // Special case for the last band - the end of the filter is
        // at the last frequency bin
        int end = fbins;
        if (i < bands - 1)
            end = filters[i + 1];
        
        for (int j = 0; j < fbins; j++) {
            // If point is behind the current filter
            if (filters[i] > j && j > start)
                fban[j + i * fbins] = (j - start) / (filters[i] - start);
            // If the point is the current filter
            else if (j == filters[i])
                fban[j + i * fbins] = 1;
            // If the point is after the current filter
            else if (filters[i] < j && j < end)
                fban[j + i * fbins] = (j - filters[i]) / (end - filters[i]);
            else
                // Anywhere else in the band
                fban[j + i * fbins] = 0;
        }
    }
    
    /* Apply filter bank to signal */
    
    // TODO: There is a mistake here. fban should be rotated (?)
    matmul(fban, bands, fbins, ft, fbins, frames, res);
    puts("123");
    
    /* Cleanup */
    
    free(ft);
    free(filters);
    free(fban);
    
    return MNO_ERR;
}