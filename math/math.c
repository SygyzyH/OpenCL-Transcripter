#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "math.h"
#include "mat.h"
// Hardware acceleration.
// The functions ARE NOT AVAILABLE IF __CUDACC__ IS NOT DEFINED AT COMPILE TIME
#include "../hware/mathhw.h"
#include "../hware/mathw.h"


double hann(int x, int windowsize) {
    /* Hann window function to be used as windowing function for stft */
    return pow(sin(M_PI * x / windowsize), 2);
}

int stft(double *src, int sz, int framesize, int windowsize, int hopsize, double (*w)(int, int), double **res, int szr, int op) {
#ifdef __CUDACC__
    // GPU Implementation
    return astft(src, sz, framesize, windowsize, hopsize, res, szr, op);
#else
    // CPU Implementation
    
    /* NOTE: as to avoid messing with returning complex numbers, the result is either only the complex
            solutions, only the real solutions, or their magnitude squared. This equates to the wave's amplitude,
            the waves phase, or the wave's power spectral density. */
    
    if (hopsize == 0) return MZERO_DIV;
    if (framesize == 0) return MZERO_DIV;
    
    // User should call this to get result size
	int fbins, frames;
	stftwh(sz, framesize, hopsize, &fbins, &frames);
    
    if (szr < fbins * frames) return MSMALL_BUF;
	
    // This is the part that runs on the GPU on accelerated mode
	for (int frame = 0; frame < frames; frame++) {
		for (int freqbi = 0; freqbi < framesize; freqbi++) {
			int freq = (freqbi < ceil(framesize / 2))? freqbi : freqbi - framesize;
			
			double sumr = 0;
			double sumi = 0;
            
            for (int n = 0; n < framesize; n++) {
                double r = src[n + frame * hopsize] * (*w)(n, windowsize);
                double phi = -2 * M_PI * n * freq / framesize;
				sumr += r * cos(phi);
				sumi += r * sin(phi);
            }
            
            // The usuall use case for this is with MAGSQ, but the other
            // options do exist.
            double opres;
            switch (op) {
                case REAL:
                opres = sumr;
                break;
                
                case COMPLEX:
                opres = sumi;
                break;
                
                case MAGSQ:
                opres = sumr * sumr + sumi * sumi;
                break;
                
                default:
                return MINVALID_ARG;
            }
            
			if (freqbi < fbins)
            (*res)[freqbi + frame * fbins] = opres;
        }
    }
    
    return MNO_ERR;
#endif
}

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

double ftomel(double f) {
    return 2595.0 * log10(1.0 + (f / 700.0));
}

double meltof(double mel) {
    return 700 * (pow(10, mel / 2595) - 1);
}

int melspec(double *src, int sz, int framesize, int windowsize, int hopsize, int bands,
            double fstart, double fend, double **res) {
    int e = 0;
    
    /* Extract STFT */
    
	int fbins, frames;
	stftwh(sz, framesize, hopsize, &fbins, &frames);
    
    if (fend == 0.0) fend = fbins;
    
    double *ft = (double *) malloc(fbins * frames * sizeof(double));
    if ((e = stft(src, sz, framesize, windowsize, hopsize, hann, &ft, fbins * frames, MAGSQ)))
        return e;
    
    /* Convert amplitude to Decibels */
    
    amptodb(&ft, fbins * frames, -1, SCALE_ONE);
    
    /* Build the filter bank */
    
    double minmel = ftomel(fstart);
    double maxmel = ftomel(fend);
    double spacing = (double) (maxmel - minmel) / (bands - 1);
    
    // Transcribed from Apple
    /* https://developer.apple.com/documentation/accelerate/computing_the_mel_spectrum_using_linear_algebra
   */
    int *filters = (int *) malloc(sizeof(double) * bands);
    // 'i' goes up to maxmel + spacing to include the last iteration
    for (double i = minmel; i < maxmel + spacing; i += spacing)
        filters[(int) i] = (int) fbins * meltof(i) / fend;
    
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
    
    Mat a, b, r;
    a.width = fbins;
    a.height = frames;
    b.width = fbins;
    b.height = bands;
    
    matmul(a, b, &r);
    
    /* Cleanup */
    
    *res = r.data;
    
    free(ft);
    free(filters);
    free(fban);
    
    return MNO_ERR;
}

int matmul(Mat a, Mat b, Mat *res) {
#ifdef __CUDACC__
    // GPU Implementation
    return amatmul(a, b, res);
#else
    // CPU Implementation
	
    int commonsz;
    
	if (a.width == b.height) {
        commonsz = a.width;
        res->width = b.width;
        res->height = a.height;
    } else if (a.height == b.width) {
        commonsz = a.height;
        res->width = a.width;
        res->height = b.height;
    } else {
        return TSIZE_MISMATCH;
    }
    
    for (int i = 0; i < res->height; i++) {
        for (int j = 0; j < res->width; j++) {
            double sum = 0.0;
            for (int k = 0; k < commonsz; k++) 
                sum += a.data[k + i * a.width] * b.data[j + k * b.width];
            
            res->data[j + i * res->width] = sum;
        }
    }
    
    return TNO_ERR;
#endif
}