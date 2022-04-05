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
        clReleaseProgram(k->prog);
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
    err = clBuildProgram(prog, 0, NULL, "-cl-kernel-arg-info", NULL, NULL);
    
    va_list valist;
    va_start(valist, kerneln);
    for (int kernel = 0; kernel < kerneln; kernel++) {
        char *name = va_arg(valist, char *);
        
        Klist *k = kernels;
        
        if (k == NULL) {
            k = (Klist *) malloc(sizeof(Klist));
            k->next = NULL;
        } else {
            while (k->next != NULL) {
                if (strcmp(k->name, name) == 0) return OCLINVALID_NAME;
                k = kernels->next;
            }
            
            k->next = (Klist *) malloc(sizeof(Klist));
            k->next->next = NULL;
            k = k->next;
        }
        
        k->prog = prog;
        k->name = name;
        k->kernel = clCreateKernel(prog, name, &err);
    }
    
    va_end(valist);
    
    return OCLNO_ERR;
}

int run_kernel(const char *name, size_t globalSize[3], ...) {
    Klist *k = kernels;
    
    while (k != NULL) {
        if (strcmp(k->name, name) == 0) break;
        k = k->next;
    }
    
    if (k == NULL) return OCLINVALID_NAME;
    
    // Number of arguments in kernel
    int argcount;
    clGetKernelInfo(k->kernel, CL_KERNEL_NUM_ARGS, sizeof(int), &argcount, NULL);
    
    Arguments *kernelargs = (Arguments *) malloc(sizeof(Arguments));
    kernelargs->next = NULL;
    
    va_list valist;
    va_start(valist, globalSize);
    
    for (int i = 0; i < argcount; i++) {
        // Variable size
        int argsz = -1;
        // Get variable type
        char rettype[16];
        size_t lastindex;
        clGetKernelArgInfo(k->kernel, i, CL_KERNEL_ARG_TYPE_NAME, 16, rettype, &lastindex);
        
        // TODO: Speed improvement: Clacluate and store things like parameter count,
        // parameter types, parameter type sizes and so on ahead of time (preferably
        // when registering the kernel)
        void *data;
        if (rettype[lastindex - 1] == '*') {
            
            int dsize = va_arg(valist, int);
            int flags = va_arg(valist, int);
            cl_mem_flags clflags;
            switch (flags & ~OCLCPY) {
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
                return OCLINVALID_ARG;
            }
            
            data = &clCreateBuffer(context, clflags, sizeof_st(rettype) * dsize, NULL, NULL);
            
            if (flags & OCLCPY)
                clEnqueueWriteBuffer(queue, (cl_mem) *data, CL_TRUE, 0, sizeof_st(rettype) * dsize, host_data, 0, NULL, NULL);
            
        } else
            data = &va_arg_st(valist, rettype);
        clSetKernelArg(k->kernel, i, sizeof_st(rettype), data);
        
        kernelargs->arg = data;
        kernelargs->type = rettype;
        
        
        /*// TODO: This NEEDS some refractoring
        if (strstr(rettype, "int") == 0) {
            int data;
            if (rettype[lastindex - 1] == '*') {
                int *host_data = va_arg(valist, int *);
                
                int dsize = va_arg(valist, int);
                int flags = va_arg(valist, int);
                cl_mem_flags clflags;
                switch (flags & ~OCLCPY) {
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
                    return OCLINVALID_ARG;
                }
                
                cl_mem data = clCreateBuffer(context, clflags, sizeof(int) * dsize, NULL, NULL);
                
                if (flags & OCLCPY)
                    clEnqueueWriteBuffer(queue, data, CL_TRUE, 0, sizeof(int) * dsize, host_data, 0, NULL, NULL);
                
            } else {
                data = va_arg(valist, int);
            }
            
            clSetKernelArg(k->kernel, i, sizeof(data), &data);
        } else if (strstr(rettype, "char") == 0) {
            char data;
            if (rettype[lastindex - 1] == '*') {
                char *host_data = va_arg(valist, char *);
                
                int dsize = va_arg(valist, int);
                int flags = va_arg(valist, int);
                cl_mem_flags clflags;
                switch (flags & ~OCLCPY) {
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
                    return OCLINVALID_ARG;
                }
                
                cl_mem data = clCreateBuffer(context, clflags, sizeof(char) * dsize, NULL, NULL);
                
                if (flags & OCLCPY)
                    clEnqueueWriteBuffer(queue, data, CL_TRUE, 0, sizeof(char) * dsize, host_data, 0, NULL, NULL);
                
            } else {
                data = (char) va_arg(valist, int);
            }
            
            clSetKernelArg(k->kernel, i, sizeof(data), &data);
        } else if (strstr(rettype, "float") == 0) {
            float data;
            if (rettype[lastindex - 1] == '*') {
                float *host_data = va_arg(valist, float *);
                
                int dsize = va_arg(valist, int);
                int flags = va_arg(valist, int);
                cl_mem_flags clflags;
                switch (flags & ~OCLCPY) {
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
                    return OCLINVALID_ARG;
                }
                
                cl_mem data = clCreateBuffer(context, clflags, sizeof(float) * dsize, NULL, NULL);
                
                if (flags & OCLCPY)
                    clEnqueueWriteBuffer(queue, data, CL_TRUE, 0, sizeof(float) * dsize, host_data, 0, NULL, NULL);
                
            } else {
                data = (float) va_arg(valist, double);
            }
            
            clSetKernelArg(k->kernel, i, sizeof(data), &data);
        } else if (strstr(rettype, "double") == 0) {
            double data;
            if (rettype[lastindex - 1] == '*') {
                double *host_data = va_arg(valist, double *);
                
                int dsize = va_arg(valist, int);
                int flags = va_arg(valist, int);
                cl_mem_flags clflags;
                switch (flags & ~OCLCPY) {
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
                    return OCLINVALID_ARG;
                }
                
                cl_mem data = clCreateBuffer(context, clflags, sizeof(double) * dsize, NULL, NULL);
                
                if (flags & OCLCPY)
                    clEnqueueWriteBuffer(queue, data, CL_TRUE, 0, sizeof(double) * dsize, host_data, 0, NULL, NULL);
                
            } else {
                data = va_arg(valist, double);
            }
            
            clSetKernelArg(k->kernel, i, sizeof(data), &data);
        } else return OCLUNKNOWN_SIZE;*/
        
    }
    
    va_end(valist);
    
    // Actually run the kernel
    clEnqueueNDRangeKernel(queue, k->kernel, 3, NULL, globalSize, NULL, 0, NULL, NULL);
    clFinish(queue);
    
    return OCLNO_ERR;
}