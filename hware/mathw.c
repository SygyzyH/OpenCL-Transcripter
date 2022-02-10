#ifdef __CUDACC__
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mathw.h"
#include "../math/mat.h"

__device__ 
double atomicAddD(double* address, double val) {
    unsigned long long int* address_as_ull = (unsigned long long int*)address;
    unsigned long long int old = *address_as_ull, assumed;
    
    do {
        assumed = old;
        old = atomicCAS(address_as_ull, assumed, __double_as_longlong(val + __longlong_as_double(assumed)));
        // Note: uses integer comparison to avoid hang in case of NaN (since NaN != NaN)
    } while (assumed != old);
    
    return __longlong_as_double(old);
}

__global__
void Nmatmul(Mat a, Mat b, Mat *res, int commonsz) {
    for (int i = blockIdx.y * blockDim.y + threadIdx.y; i < res->height; i += blockDim.y * gridDim.y) {
        for (int j = blockIdx.x * blockDim.x + threadIdx.x; j < res->width; j += blockDim.x * gridDim.x) {
            double sum = 0;
            for (int k = 0; k < commonsz; k++)
                sum += a.data[k + i * a.width] * b.data[j + k * b.width];
            
            res->data[j + i * res->width] = sum;
        }
    }
}

__global__
void Nmatdot(Mat a, Mat b, double *res) {
    for (int i = blockIdx.y * blockDim.y + threadIdx.y; i < a.height; i += blockDim.y * gridDim.y) {
        for (int j = blockIdx.x * blockDim.x + threadIdx.x; j < a.width; j += blockDim.x * gridDim.x) {
            double sum = a.data[j + i * a.width] * b.data[j + i * a.width];
			atomicAddD(res, sum);
        }
    }
}

__global__
void Nmatadd(Mat a, Mat b, Mat *res) {
	for (int i = blockIdx.y * blockDim.y + threadIdx.y; i < res->height; i += blockDim.y * gridDim.y) {
        for (int j = blockIdx.x * blockDim.x + threadIdx.x; j < res->width; j += blockDim.x * gridDim.x) {
			res->data[j + i * res->width] = a.data[j + i * res->width] + b.data[j + i * res->width];
		}
	}
}

__global__
void Nmatsub(Mat a, Mat b, Mat *res) {
	for (int i = blockIdx.y * blockDim.y + threadIdx.y; i < res->height; i += blockDim.y * gridDim.y) {
        for (int j = blockIdx.x * blockDim.x + threadIdx.x; j < res->width; j += blockDim.x * gridDim.x) {
			res->data[j + i * res->width] = a.data[j + i * res->width] - b.data[j + i * res->width];
		}
	}
}

int amatmul(Mat a, Mat b, Mat *res) {
	Mat tres;
	
	int commonsz;
	
	if (a.width == b.height) {
        commonsz = a.width;
		tres.width = b.width;
        tres.height = a.height;
    } else if (a.height == b.width) {
        commonsz = a.height;
		tres.width = a.width;
        tres.height = b.height;
    } else {
        return TSIZE_MISMATCH;
    }
	
	/* Cuda seg */
    int e = 0;
	
    double *aD, *bD, *resD;
    e += cudaMalloc(&aD, a.width * a.height * sizeof(double));
    e += cudaMalloc(&bD, b.width * b.height * sizeof(double));
    e += cudaMalloc(&resD, tres.width * tres.height * sizeof(double));
    e += cudaMemcpy(aD, a.data, a.width * a.height * sizeof(double), cudaMemcpyHostToDevice);
    e += cudaMemcpy(bD, b.data, b.width * b.height * sizeof(double), cudaMemcpyHostToDevice);
    a.data = aD;
    b.data = bD;
    tres.data = resD;
    
    Mat *resC;
    e += cudaMalloc(&resC, sizeof(Mat));
    e += cudaMemcpy(resC, &tres, sizeof(Mat), cudaMemcpyHostToDevice);
    
	if (e) return e;
	
    Nmatmul<<<ceil((double) tres.width * tres.height / 1024), 1024>>>(a, b, resC, commonsz);
    
    e += cudaMemcpy(res, resC, sizeof(Mat), cudaMemcpyDeviceToHost);
	double *resd = (double *) malloc(tres.width * tres.height * sizeof(double));
	e += cudaMemcpy(resd, resD, tres.width * tres.height * sizeof(double), cudaMemcpyDeviceToHost);
	res->data = resd;
	e += cudaFree(aD);
	e += cudaFree(bD);
    e += cudaFree(resD);
    e += cudaFree(resC);
	
	/* End Cuda seg */
	
	if (e) return e;
	
	return TNO_ERR;
}

int amatdot(Mat a, Mat b, double *res) {
	if (a.width != b.width || a.height != b.height)
		return TSIZE_MISMATCH;
	
	int e = 0;
	
	/* Cuda seg */
	double *aD, *bD;
    e += cudaMalloc(&aD, a.width * a.height * sizeof(double));
    e += cudaMalloc(&bD, b.width * b.height * sizeof(double));
    e += cudaMemcpy(aD, a.data, a.width * a.height * sizeof(double), cudaMemcpyHostToDevice);
    e += cudaMemcpy(bD, b.data, b.width * b.height * sizeof(double), cudaMemcpyHostToDevice);
    a.data = aD;
    b.data = bD;
	
	double t = 0;
	double *ores = res;
    e += cudaMalloc(&res, sizeof(double));
    e += cudaMemcpy(res, &t, sizeof(double), cudaMemcpyHostToDevice);
	
	if (e) return e;
	
    Nmatdot<<<ceil((double)  a.width * a.height / 1024), 1024>>>(a, b, res);
	
	e += cudaMemcpy(ores, res, sizeof(double), cudaMemcpyDeviceToHost);
	e += cudaFree(aD);
	e += cudaFree(bD);
    e += cudaFree(res);
	
	/* End Cuda seg */
	
	if (e) return e;
	
	return TNO_ERR;
}

int amatadd(Mat a, Mat b, Mat *res) {
	if (a.width != b.width || a.height != b.height)
		return TSIZE_MISMATCH;
	
	Mat tres;
	tres.width = a.width;
	tres.height = a.height;
	
	int e = 0;
	
	/* Cuda seg */
	double *aD, *bD, *resD;
    e += cudaMalloc(&aD, a.width * a.height * sizeof(double));
    e += cudaMalloc(&bD, b.width * b.height * sizeof(double));
    e += cudaMalloc(&resD, tres.width * tres.height * sizeof(double));
    e += cudaMemcpy(aD, a.data, a.width * a.height * sizeof(double), cudaMemcpyHostToDevice);
    e += cudaMemcpy(bD, b.data, b.width * b.height * sizeof(double), cudaMemcpyHostToDevice);
    a.data = aD;
    b.data = bD;
    tres.data = resD;
	
	Mat *resC;
    e += cudaMalloc(&resC, sizeof(Mat));
    e += cudaMemcpy(resC, &tres, sizeof(Mat), cudaMemcpyHostToDevice);
	
	if (e) return e;
	
    Nmatadd<<<ceil((double) tres.width * tres.height / 1024), 1024>>>(a, b, resC);
	
	e += cudaMemcpy(res, resC, sizeof(Mat), cudaMemcpyDeviceToHost);
	double *resd = (double *) malloc(tres.width * tres.height * sizeof(double));
	e += cudaMemcpy(resd, resD, tres.width * tres.height * sizeof(double), cudaMemcpyDeviceToHost);
	res->data = resd;
	e += cudaFree(aD);
	e += cudaFree(bD);
    e += cudaFree(resD);
    e += cudaFree(resC);
	
	/* End Cuda seg */
	
	if (e) return e;
	
	return TNO_ERR;
}

int amatsub(Mat a, Mat b, Mat *res) {
	if (a.width != b.width || a.height != b.height)
		return TSIZE_MISMATCH;
	
	Mat tres;
	tres.width = a.width;
	tres.height = a.height;
	
	int e = 0;
	
	/* Cuda seg */
	double *aD, *bD, *resD;
    e += cudaMalloc(&aD, a.width * a.height * sizeof(double));
    e += cudaMalloc(&bD, b.width * b.height * sizeof(double));
    e += cudaMalloc(&resD, tres.width * tres.height * sizeof(double));
    e += cudaMemcpy(aD, a.data, a.width * a.height * sizeof(double), cudaMemcpyHostToDevice);
    e += cudaMemcpy(bD, b.data, b.width * b.height * sizeof(double), cudaMemcpyHostToDevice);
    a.data = aD;
    b.data = bD;
    tres.data = resD;
	
	Mat *resC;
    e += cudaMalloc(&resC, sizeof(Mat));
    e += cudaMemcpy(resC, &tres, sizeof(Mat), cudaMemcpyHostToDevice);
	
	if (e) return e;
	
    Nmatsub<<<ceil((double) tres.width * tres.height / 1024), 1024>>>(a, b, resC);
	
	e += cudaMemcpy(res, resC, sizeof(Mat), cudaMemcpyDeviceToHost);
	double *resd = (double *) malloc(tres.width * tres.height * sizeof(double));
	e += cudaMemcpy(resd, resD, tres.width * tres.height * sizeof(double), cudaMemcpyDeviceToHost);
	res->data = resd;
	e += cudaFree(aD);
	e += cudaFree(bD);
    e += cudaFree(resD);
    e += cudaFree(resC);
	
	/* End Cuda seg */
	
	if (e) return e;
	
	return TNO_ERR;
}
#endif