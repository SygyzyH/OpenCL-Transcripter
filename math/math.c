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
res - pointer to result (allocates needed memory automaticly, aw * bh)
returns 0 on success
*/

int matmul(double *a, unsigned int aw, unsigned int ah,
           double *b, unsigned int bw, unsigned int bh, double **res) {
    if (!minit) return MUNINITIALIZED;
    
    if (ah != bw)
        return MINVALID_ARG;
    
    size_t gz[] = { aw, bh };
    
    double *r = (double *) malloc(sizeof(double) * aw * bh);
    run_kernel("matmul", 2, gz, NULL, 
               a, aw * ah, OCLREAD | OCLCPY,
               b, bw * bh, OCLREAD | OCLCPY, 
               aw, ah, bw,
               r, aw * bh, OCLWRITE | OCLOUT);
    
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
    
    double *r = (double *) malloc(sizeof(double) * fbins * frames);
    
    size_t gz[] = { frames, fbins };
    
    run_kernel("stft", 2, gz, NULL, src, sz, OCLREAD | OCLCPY,
               framesize, windowsize, hopsize, fbins, frames, sides,
               r, fbins * frames, OCLWRITE | OCLOUT);
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = frames;
    o->height = fbins;
    o->data = r;
    
    *res = o;
    
    return MNO_ERR;
}

// Convert amps to decibels
/*
src - data source
sz - size of data
topdb - top decible allowed (-1 if DC)
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

// Convert mel spectrograms to log-mel spectrograms.
/*
src - source data
sz - size of source data
*/
void log10spec(double **src, int sz) {
    double *dsrc = *src;
    
    for (int i = 0; i < sz; i++)
        dsrc[i] = log10(dsrc[i] + EPSIL);
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
    int fbins = stftr->height, frames = stftr->width;
    
    if (fend == 0.0) fend = fbins;
    
    /* Build the filter bank */
    
    double *fban = (double *) calloc(fbins * bands, sizeof(double));
    
    double spacing = (double) (ftomel(fend) - ftomel(fstart)) / (bands + 1);
    double *bedges = (double *) malloc(sizeof(double) * (bands + 2));
    for (int i = 0; i < bands + 2; i++)
        bedges[i] = meltof(ftomel(fstart) + i * spacing);
    
    double *linf = (double *) malloc(sizeof(double) * fbins);
    for (int i = 0; i < fftlen; i++) linf[i] = (double) fs * i / fftlen;
    
    int valide = 0;
    for (int i = 0; i < bands + 2; i++) if (bedges[i] - fs / 2 < sqrt(EPSIL)) valide++;
    
    double *p = (double *) calloc(valide, sizeof(double));
    for (int i = 0; i < valide; i++) {
        for (int j = 0; j < fftlen; j++) {
            if(linf[j] > bedges[i]) {
                p[i] = j;
                break;
            }
        }
    }
    
    double *bw = (double *) malloc(sizeof(double) * bands + 1);
    for (int i = 0; i < bands + 1; i++) bw[i] = bedges[i + 1] - bedges[i];
    
    for (int i = 0; i < valide - 2; i++) {
        // Rising side
        for (int j = p[i]; j < p[i + 1]; j++)
            fban[j + i * fbins] = (linf[j] - bedges[i]) / bw[i];
        
        // Falling side
        for (int j = p[i + 1]; j < p[i + 2]; j++)
            fban[j + i * fbins] = (bedges[i + 2] - linf[j]) / bw[i + 1];
    }
    
    /* Filter bank normalization */
    // Bandwidth normalization
    
    double *weightb = (double *) malloc(sizeof(double) * bands);
    for (int i = 0; i < bands; i++) weightb[i] = 2 / (bedges[i + 2] - bedges[i]);
    
    for (int i = 0; i < bands; i++) {
        for (int j = 0; j < fftlen; j++) {
            fban[j + i * fbins] *= weightb[i];
        }
    }
    
    free(weightb);
    free(bedges);
    free(linf);
    free(p);
    free(bw);
    
    /* Apply filter bank to signal */
    
    *res = (Mat *) malloc(sizeof(Mat));
    (*res)->width = frames;
    (*res)->height = bands;
    
    matmul(ft, frames, fbins, fban, fbins, bands, &((*res)->data));
    
    /* Convert amplitude to Decibels */
    
    // amptodb(&ft, fbins * frames, -1, SCALE_ONE);
    // amptodb(&((*res)->data), (*res)->width * (*res)->height, -1, SCALE_ONE);
    
    /* Cleanup */
    
    free(ft);
    free(stftr);
    free(fban);
    
    return MNO_ERR;
}

// Ensure matrix dimensions match
/* 
 inp - input matrix
tw - target width
th - target height
res - pointer to result OR NULL (automaticly allocated)
returns 0 if res isn't NULL, otherwise returns MINVALID_ARG if dimensions don't match, else 0
*/
int ensuredims(Mat inp, int tw, int th, Mat **res) {
    int pleft = 0, pright = 0, ptop = 0, pbot = 0;
    
    if (inp.width < tw || inp.height < th) {
        if (res == NULL) return MINVALID_ARG;
        
        pleft = ceil((tw - inp.width) / 2);
        pright = tw - pleft;
        ptop = ceil((th - inp.height) / 2);
        pbot = th - ptop;
    }
    
    if (res == NULL) return MNO_ERR;
    
    Mat *r = (Mat *) malloc(sizeof(Mat));
    r->width = tw;
    r->height = th;
    r->data = (double *) malloc(sizeof(double) * tw * th);
    
    for (int i = 0; i < th; i++) {
        for (int j = 0; j < tw; j++) {
            if (j < pleft || j > inp.width + pright || i < ptop || i > inp.height + pbot)
                r->data[j + i * tw] = 0;
            else
                r->data[j + i * tw] = inp.data[j - pleft + (i - ptop) * inp.width];
        }
    }
    
    *res = r;
    
    return MNO_ERR;
}