#ifdef __CUDACC__
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>

#include "../math/math.h"

__device__
double Nhann(int x, int windowsize) {
    return pow(sin(M_PI * x / windowsize), 2);
}

__global__
void Nstft(double *src, int fbins, int frames, int framesize, int windowsize, int hopsize, double *res, int op) {
    for (int frame = blockIdx.y * blockDim.y + threadIdx.y; frame < frames; frame += blockDim.y * gridDim.y) {
        for (int freqbi = blockIdx.x * blockDim.x + threadIdx.x; freqbi < framesize; freqbi += blockDim.x * gridDim.x) {
            int freq = (freqbi < ceil((double) framesize / 2))? freqbi : freqbi - framesize;
            
            double sumr = 0;
            double sumi = 0;
            
            for (int n = 0; n < framesize; n++) {
                double r = src[n + frame * hopsize] * Nhann(n, windowsize);
                double phi = -2 * M_PI * n * freq / framesize;
                sumr += r * cos(phi);
                sumi += r * sin(phi);
            }
            
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
            }
            
            if (freqbi < fbins)
                res[freqbi + frame * fbins] = opres;
        }
    }
}

int astft(double *src, int sz, int framesize, int windowsize, int hopsize, double **res, int szr, int op) {
    if (hopsize == 0) return MZERO_DIV;
    if (framesize == 0) return MZERO_DIV;
    
    // User should call this to get result size
	int fbins, frames;
	stftwh(sz, framesize, hopsize, &fbins, &frames);
    
    if (szr < fbins * frames) return MSMALL_BUF;
    
    /* Cuda Seg */
    
    double *csrc;
    cudaMalloc(&csrc, sizeof(double) * sz);
    cudaMemcpy(csrc, src, sizeof(double) * sz, cudaMemcpyHostToDevice);
    
    double *cres;
    cudaMalloc(&cres, sizeof(double) * fbins * frames);
    
    
    Nstft<<<ceil((double) fbins * frames / 1024), 1024>>>(csrc, fbins, frames, framesize, windowsize, hopsize, cres, op);
    
    cudaMemcpy(*res, cres, sizeof(double) * fbins * frames, cudaMemcpyDeviceToHost);
    
    cudaFree(csrc);
    cudaFree(cres);
    
    /* End Cuda seg*/
    
    return MNO_ERR;
}
#endif