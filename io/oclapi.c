/*
TODO: This library should handle all things opencl.

Register functions by passing their source code, name, and
input / output parameters.

Call the function using its name and passing its parameters.

Automatic clean-up
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "args.h"
#include "oclapi.h"

cl_platform_id cpPlatform;
cl_device_id device_id;
cl_context context;
cl_command_queue queue;

Klist *kernels = NULL;

int ocinit() {
    int err;
    
    err = clGetPlatformIDs(1, &cpPlatform, NULL);
    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    // If no GPU was found, use the CPU instead. Cant see how this would ever
    // fail, since the docs state the CPU is the host device - meaning, if this
    // code is running, there must be a CPU to run it. 
    if (err == CL_DEVICE_NOT_FOUND) {
        if (chkset(sets, DB))
            puts("Failed to get a GPU device, running un-accelerated.");
        clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
    }
    
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    queue = clCreateCommandQueue(context, device_id, 0, &err);
    
    return OCLNO_ERR;
}

int occln() {
    Klist *k = kernels;
    
    while (k != NULL) {
        // TODO: Programs will repeat if multiple kernels are added at once.
        // This will cause clReleaseProgram to return an error. This is not
        // problematic per say but not best practice and should be avoided.
        cl_program prog;
        clGetKernelInfo(k->kernel, CL_KERNEL_PROGRAM, sizeof(cl_program), &prog, NULL);
        clReleaseProgram(prog);
        clReleaseKernel(k->kernel);
        
        Klist *oldk = k;
        k = k->next;
        free(oldk);
    }
    
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    if (chkset(sets, DB)) puts("OCLAPI_C: Cleanup successful.");
    
    return OCLNO_ERR;
}

/*
Before a function is ran using the ocl api, it must first be
registered. The user needs to supply a source program and any
number of kernels in that program. All names must be unique.
*/
int register_from_src(const char **src, int kerneln, ...) {
    int err;
    
    cl_program prog = clCreateProgramWithSource(context, 1, src, NULL, &err);
    // Build with kernel arg info flag to retrive it later. This solution thankfully
    // allows for building programs and having access to some of the information form
    // the compilation process, allowing for this whole library to feasably exist.
    err = clBuildProgram(prog, 0, NULL, "-cl-kernel-arg-info", NULL, NULL);
    
    va_list valist;
    va_start(valist, kerneln);
    
    for (int kernel = 0; kernel < kerneln; kernel++) {
        char *name = va_arg(valist, char *);
        
        Klist *k = kernels;
        
        if (k == NULL) {
            kernels = (Klist *) malloc(sizeof(Klist));
            k = kernels;
            k->next = NULL;
        } else {
            while (k->next != NULL) {
                if (strcmp(k->name, name) == 0) return OCLINVALID_NAME;
                k = k->next;
            }
            
            k->next = (Klist *) malloc(sizeof(Klist));
            k->next->next = NULL;
            k = k->next;
        }
        
        k->kernel = clCreateKernel(prog, name, &err);
        k->name = name;
        clGetKernelInfo(k->kernel, CL_KERNEL_NUM_ARGS, sizeof(int), &(k->argc), NULL);
        k->argv = (Karg *) malloc(sizeof(Karg) * k->argc);
        
        // Fill out arguments
        for (int argument = 0; argument < k->argc; argument++) {
            clGetKernelArgInfo(k->kernel, argument, CL_KERNEL_ARG_TYPE_NAME, RETTYPE_SIZE, k->argv[argument].rettype, NULL);
            k->argv[argument].isptr = strchr(k->argv[argument].rettype, '*') != 0;
            
            if (strstr(k->argv[argument].rettype, "char")) {
                k->argv[argument].asize = sizeof(char);
            } else if (strstr(k->argv[argument].rettype, "int")) {
                k->argv[argument].asize = sizeof(int);
            } else if (strstr(k->argv[argument].rettype, "float")) {
                k->argv[argument].asize = sizeof(float);
            } else if (strstr(k->argv[argument].rettype, "double")) {
                k->argv[argument].asize = sizeof(double);
            } else {
                va_end(valist);
                puts("INVALID ARG");
                return OCLINVALID_ARG;
            }
        }
    }
    
    va_end(valist);
    
    return OCLNO_ERR;
}

int run_kernel(const char *name, int wdim, size_t *gsz, size_t *lsz, ...) {
    int err;
    
    Klist *k = kernels;
    
    while (k != NULL) {
        if (strcmp(k->name, name) == 0) break;
        k = k->next;
    }
    
    if (k == NULL) return OCLINVALID_NAME;
    
    va_list valist;
    va_start(valist, lsz);
    
    for (int i = 0; i < k->argc; i++) {
        void *data;
        size_t device_dsize = k->argv[i].asize;
        
        if (k->argv[i].isptr) {
            if (strstr(k->argv[i].rettype, "char")) 
                k->argv[i]._host_data = (char *) (va_arg(valist, int *));
            else if (strstr(k->argv[i].rettype, "int"))
                k->argv[i]._host_data = (int *) va_arg(valist, int *);
            else if (strstr(k->argv[i].rettype, "float"))
                k->argv[i]._host_data = (float *) (va_arg(valist, double *));
            else if (strstr(k->argv[i].rettype, "double"))
                k->argv[i]._host_data = (double *) va_arg(valist, double *);
            else {
                va_end(valist);
                // TODO: Error message
                puts("TODO: Error message INVALID ARG");
                return OCLINVALID_ARG;
            }
            
            // Store the data size to know how much data to return
            int dsize = va_arg(valist, int);
            k->argv[i]._dsize = dsize;
            
            // Store the flags in-case this argument needs to be copied out
            int flags = va_arg(valist, int);
            k->argv[i]._flags = flags;
            
            cl_mem_flags clflags;
            switch ((flags & ~OCLCPY) & ~OCLOUT) {
                case OCLREAD:
                clflags = CL_MEM_READ_ONLY;
                break;
                
                case OCLWRITE:
                clflags = CL_MEM_WRITE_ONLY;
                break;
                
                case OCLREAD | OCLWRITE:
                clflags = CL_MEM_READ_WRITE;
                break;
                
                default:
                va_end(valist);
                return OCLINVALID_ARG;
            }
            
            // This will be freed later
            k->argv[i]._device_data = clCreateBuffer(context, clflags, k->argv[i].asize * dsize, NULL, NULL);
            
            data = &(k->argv[i]._device_data);
            device_dsize = sizeof(cl_mem);
            
            // Copy data from given pointer to buffer pointed to by data
            if (flags & OCLCPY)
                err = clEnqueueWriteBuffer(queue, *(cl_mem *) data, CL_TRUE, 0, k->argv[i].asize * dsize, k->argv[i]._host_data, 0, NULL, NULL);
        } else {
            if (strstr(k->argv[i].rettype, "char")) 
                data = &(char) { va_arg(valist, int) };
            else if (strstr(k->argv[i].rettype, "int")) 
                data = &(int) { va_arg(valist, int) };
            else if (strstr(k->argv[i].rettype, "float"))
                data = &(float) { va_arg(valist, double) };
            else if (strstr(k->argv[i].rettype, "double")) 
                data = &(double) { va_arg(valist, double) };
            else {
                va_end(valist);
                // TODO: Error message
                puts("TODO: Error message INVALID ARG");
                return OCLINVALID_ARG;
            }
        }
        
        err = clSetKernelArg(k->kernel, i, device_dsize, data);
        printf("e: %d\n", err);
    }
    
    va_end(valist);
    
    // Run the kernel
    clEnqueueNDRangeKernel(queue, k->kernel, wdim, NULL, gsz, lsz, 0, NULL, NULL);
    
    // Copy out the data and free it
    for (int i = 0; i < k->argc; i++) {
        if (k->argv[i]._flags & OCLOUT) {
            clEnqueueReadBuffer(queue, k->argv[i]._device_data, CL_TRUE, 0, k->argv[i].asize * k->argv[i]._dsize, k->argv[i]._host_data, 0, NULL, NULL);
            clReleaseMemObject(k->argv[i]._device_data);
        }
    }
    
    clFinish(queue);
    
    return OCLNO_ERR;
}