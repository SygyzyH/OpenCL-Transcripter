#define __CL_ENABLE_EXCEPTIONS
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "math.h"
#include "../io/args.h"

#define noerr(e) { if ((e)) { puts("MATH_H: Initialized math UNSUCCESSFULY."); fflush(stdout); return e; } }

cl_platform_id cpPlatform;        // OpenCL platform
cl_device_id device_id;           // device ID
cl_context context;               // context
cl_command_queue queue;           // command queue

// Programs
cl_program matprog;
cl_program mathprog;

// Kernels
cl_kernel kmatmul;
cl_kernel kmatadd;
cl_kernel kmatsub;
cl_kernel kmatmuls;
cl_kernel kmatadds;

int mainit() {
    int err;
    
    /* Init matrix lib */
    err = clGetPlatformIDs(1, &cpPlatform, NULL);
    noerr(err);
    // TODO: If no GPU, use CPU
    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    noerr(err);
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    noerr(err);
    queue = clCreateCommandQueue(context, device_id, 0, &err);
    noerr(err);
    
    // Get the compute matprog from the source code
    char *matSource = ldfile(MAT_PROG);
    matprog = clCreateProgramWithSource(context, 1, (const char **) &matSource, NULL, &err);
    free(matSource);
    noerr(err);
    char *mathSource = ldfile(MATH_PROG);
    mathprog = clCreateProgramWithSource(context, 1, (const char **) &mathSource, NULL, &err);
    free(mathSource);
    noerr(err);
    // TODO: In debug mode, print if CL_BUILD_ERROR
    err = clBuildProgram(matprog, 0, NULL, NULL, NULL, NULL);
    noerr(err);
    err = clBuildProgram(mathprog, 0, NULL, NULL, NULL, NULL);
    noerr(err);
    
    /* Make all kernels */
    kmatmul = clCreateKernel(matprog, "matmul", &err);
    noerr(err);
    kmatadd = clCreateKernel(matprog, "matadd", &err);
    noerr(err);
    kmatsub = clCreateKernel(matprog, "matsub", &err);
    noerr(err);
    kmatmuls = clCreateKernel(matprog, "matmuls", &err);
    noerr(err);
    kmatadds = clCreateKernel(matprog, "matadds", &err);
    noerr(err);
    
    if (chkset(sets, DB))
        puts("MATH_H: Initialized math successfuly.");
    
    return MNO_ERR;
}

int macln() {
    clReleaseProgram(matprog);
    clReleaseProgram(mathprog);
    clReleaseKernel(kmatmul);
    clReleaseKernel(kmatadd);
    clReleaseKernel(kmatsub);
    clReleaseKernel(kmatmuls);
    clReleaseKernel(kmatadds);
    
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    if (chkset(sets, DB))
        puts("MATH_H: Cleanup successful.");
    
    return MNO_ERR;
}

int matmul(double *a, double *b, unsigned int aw, unsigned int ah,
           unsigned int bw, unsigned int bh, double **res) {
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
    
    double *h_r = (double *) malloc(sizeof(double) * az * bz);
    
    int err;
    
    // Prepare the data in the device
    cl_mem d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(double) * aw * ah, NULL, NULL);
    cl_mem d_b = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(double) * bw * bh, NULL, NULL);
    cl_mem d_r = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(double) * az * bz, NULL, NULL);
    
    // Copy the data to the device
    err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0, sizeof(double) * aw * ah, a, 0, NULL, NULL);
    err |= clEnqueueWriteBuffer(queue, d_b, CL_TRUE, 0, sizeof(double) * bw * bh, b, 0, NULL, NULL);
    noerr(err);
    
    // Set the kernel args
    err  = clSetKernelArg(kmatmul, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kmatmul, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(kmatmul, 2, sizeof(unsigned int), &bz);
    err |= clSetKernelArg(kmatmul, 3, sizeof(unsigned int), &az);
    err |= clSetKernelArg(kmatmul, 4, sizeof(unsigned int), &k);
    err |= clSetKernelArg(kmatmul, 5, sizeof(cl_mem), &d_r);
    noerr(err);
    
    // Number of total work items - localSize must be devisor
    size_t globalSize[3] = { az, bz, 0 };
    
    // Execute kernel
    err = clEnqueueNDRangeKernel(queue, kmatmul, 2, NULL, globalSize, NULL, 0, NULL, NULL);
    noerr(err);
    
    // Wait for the command queue to get serviced before reading back results
    clFinish(queue);
    
    // Copy results back
    clEnqueueReadBuffer(queue, d_r, CL_TRUE, 0, sizeof(double) * az * bz, h_r, 0, NULL, NULL );
    
    // Free
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_b);
    clReleaseMemObject(d_r);
    
    *res = h_r;
    
    return MNO_ERR;
}

int matadd(double *a, double *b, unsigned int aw, unsigned int ah, 
           unsigned int bw, unsigned int bh, double **res) {
    if (aw != bw || ah != bh) return MINVALID_ARG;
    
    double *h_r = (double *) malloc(sizeof(double) * aw * ah);
    
    int err;
    
    // Prepare the data in the device
    cl_mem d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(double) * aw * ah, NULL, NULL);
    cl_mem d_b = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(double) * bw * bh, NULL, NULL);
    cl_mem d_r = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(double) * aw * ah, NULL, NULL);
    
    // Copy the data to the device
    err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0, sizeof(double) * aw * ah, a, 0, NULL, NULL);
    err |= clEnqueueWriteBuffer(queue, d_b, CL_TRUE, 0, sizeof(double) * bw * bh, b, 0, NULL, NULL);
    noerr(err);
    
    // Set the kernel args
    err  = clSetKernelArg(kmatadd, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kmatadd, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(kmatadd, 2, sizeof(unsigned int), &aw);
    err |= clSetKernelArg(kmatadd, 3, sizeof(unsigned int), &ah);
    err |= clSetKernelArg(kmatadd, 4, sizeof(cl_mem), &d_r);
    noerr(err);
    
    // Number of total work items - localSize must be devisor
    size_t globalSize[3] = { aw, ah, 0 };
    
    // Execute kernel
    err = clEnqueueNDRangeKernel(queue, kmatadd, 2, NULL, globalSize, NULL, 0, NULL, NULL);
    noerr(err);
    
    // Wait for the command queue to get serviced before reading back results
    clFinish(queue);
    
    // Copy results back
    clEnqueueReadBuffer(queue, d_r, CL_TRUE, 0, sizeof(double) * aw * ah, h_r, 0, NULL, NULL );
    
    // Free
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_b);
    clReleaseMemObject(d_r);
    
    *res = h_r;
    
    return MNO_ERR;
}

int matsub(double *a, double *b, unsigned int aw, unsigned int ah, 
           unsigned int bw, unsigned int bh, double **res) {
    if (aw != bw || ah != bh) return MINVALID_ARG;
    
    double *h_r = (double *) malloc(sizeof(double) * aw * ah);
    
    int err;
    
    // Prepare the data in the device
    cl_mem d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(double) * aw * ah, NULL, NULL);
    cl_mem d_b = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(double) * bw * bh, NULL, NULL);
    cl_mem d_r = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(double) * aw * ah, NULL, NULL);
    
    // Copy the data to the device
    err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0, sizeof(double) * aw * ah, a, 0, NULL, NULL);
    err |= clEnqueueWriteBuffer(queue, d_b, CL_TRUE, 0, sizeof(double) * bw * bh, b, 0, NULL, NULL);
    noerr(err);
    
    // Set the kernel args
    err  = clSetKernelArg(kmatsub, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kmatsub, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(kmatsub, 2, sizeof(unsigned int), &aw);
    err |= clSetKernelArg(kmatsub, 3, sizeof(unsigned int), &ah);
    err |= clSetKernelArg(kmatsub, 4, sizeof(cl_mem), &d_r);
    noerr(err);
    
    // Number of total work items - localSize must be devisor
    size_t globalSize[3] = { aw, ah, 0 };
    
    // Execute kernel
    err = clEnqueueNDRangeKernel(queue, kmatsub, 2, NULL, globalSize, NULL, 0, NULL, NULL);
    noerr(err);
    
    // Wait for the command queue to get serviced before reading back results
    clFinish(queue);
    
    // Copy results back
    clEnqueueReadBuffer(queue, d_r, CL_TRUE, 0, sizeof(double) * aw * ah, h_r, 0, NULL, NULL );
    
    // Free
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_b);
    clReleaseMemObject(d_r);
    
    *res = h_r;
    
    return MNO_ERR;
}

int matmuls(double *a, double b, unsigned int aw, unsigned int ah, double **res) {
    // assert(a != *res)
    double *h_r = (double *) malloc(sizeof(double) * aw * ah);
    
    int err;
    
    // Prepare the data in the device
    cl_mem d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(double) * aw * ah, NULL, NULL);
    cl_mem d_r = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(double) * aw * ah, NULL, NULL);
    
    // Copy the data to the device
    err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0, sizeof(double) * aw * ah, a, 0, NULL, NULL);
    noerr(err);
    
    // Set the kernel args
    err  = clSetKernelArg(kmatmuls, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kmatmuls, 1, sizeof(double), &b);
    err |= clSetKernelArg(kmatmuls, 2, sizeof(unsigned int), &aw);
    err |= clSetKernelArg(kmatmuls, 3, sizeof(unsigned int), &ah);
    err |= clSetKernelArg(kmatmuls, 4, sizeof(cl_mem), &d_r);
    noerr(err);
    
    // Number of total work items - localSize must be devisor
    size_t globalSize[3] = { aw, ah, 0 };
    
    // Execute kernel
    err = clEnqueueNDRangeKernel(queue, kmatmuls, 2, NULL, globalSize, NULL, 0, NULL, NULL);
    noerr(err);
    
    // Wait for the command queue to get serviced before reading back results
    clFinish(queue);
    
    // Copy results back
    clEnqueueReadBuffer(queue, d_r, CL_TRUE, 0, sizeof(double) * aw * ah, h_r, 0, NULL, NULL );
    
    // Free
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_r);
    
    *res = h_r;
    
    return MNO_ERR;
}

int matadds(double *a, double b, unsigned int aw, unsigned int ah, double **res) {
    // assert(a != *res)
    double *h_r = (double *) malloc(sizeof(double) * aw * ah);
    
    int err;
    
    // Prepare the data in the device
    cl_mem d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(double) * aw * ah, NULL, NULL);
    cl_mem d_r = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(double) * aw * ah, NULL, NULL);
    
    // Copy the data to the device
    err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0, sizeof(double) * aw * ah, a, 0, NULL, NULL);
    noerr(err);
    
    // Set the kernel args
    err  = clSetKernelArg(kmatadds, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kmatadds, 1, sizeof(double), &b);
    err |= clSetKernelArg(kmatadds, 2, sizeof(unsigned int), &aw);
    err |= clSetKernelArg(kmatadds, 3, sizeof(unsigned int), &ah);
    err |= clSetKernelArg(kmatadds, 4, sizeof(cl_mem), &d_r);
    noerr(err);
    
    // Number of total work items - localSize must be devisor
    size_t globalSize[3] = { aw, ah, 0 };
    
    // Execute kernel
    err = clEnqueueNDRangeKernel(queue, kmatadds, 2, NULL, globalSize, NULL, 0, NULL, NULL);
    noerr(err);
    
    // Wait for the command queue to get serviced before reading back results
    clFinish(queue);
    
    // Copy results back
    clEnqueueReadBuffer(queue, d_r, CL_TRUE, 0, sizeof(double) * aw * ah, h_r, 0, NULL, NULL );
    
    // Free
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_r);
    
    *res = h_r;
    
    return MNO_ERR;
}

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
    
    int fbins, frames;
	stftwh(sz, framesize, hopsize, &fbins, &frames);
    
    if (fend == 0.0) fend = fbins;
    
    /* Extract STFT */
    
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
    
    matmul(fban, ft, fbins, frames, fbins, bands, res);
    
    /* Cleanup */
    
    free(ft);
    free(filters);
    free(fban);
    
    return MNO_ERR;
}