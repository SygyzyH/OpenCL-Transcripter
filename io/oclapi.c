/*
Register functions by passing their source code, name, and
input / output parameters.

Call the function using its name and passing its parameters.

Automatic clean-up.

TODO: This can be refractored. Error checking, at the very least.
Maybe also safe calls to run_kernel? (type checking, variable count checks, NULL ptr)
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "args.h"
#include "../std.h"
#include "oclapi.h"

cl_platform_id cpPlatform;
cl_device_id device_id;
cl_context context;
cl_command_queue queue;

bool oclinit = false;

Klist *kernels = NULL;

// Initialize OpenCL and prepare registery
/*
returns 0 on success
*/
int ocinit() {
    int err;
    
    err = clGetPlatformIDs(1, &cpPlatform, NULL);
    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    // If no GPU was found, use the CPU instead. Cant see how this would ever
    // fail, since the docs state the CPU is the host device - meaning, if this
    // code is running, there must be a CPU to run it. 
    if (err == CL_DEVICE_NOT_FOUND) {
        putsc(chkset(sets, DB), "Failed to get a GPU device, running un-accelerated.");
        clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
    }
    
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    queue = clCreateCommandQueue(context, device_id, 0, &err);
    
    putsc(chkset(sets, DB), "Initialized OpenCL API successfuly.");
    
    oclinit = true;
    
    return OCLNO_ERR;
}

// Cleanup registery
/*
returns 0 on success
*/
int occln() {
    if (!oclinit) return OCLUNINITIALIZED;
    
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
        free(k->argv);
        k = k->next;
        free(oldk);
    }
    
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    putsc(chkset(sets, DB), "Cleanup successful.");
    
    return OCLNO_ERR;
}

// Returns error as string
/*
error - error int
returns string describing the error name
*/
const char *clGetErrorString(cl_int error)
{
    switch(error){
        // run-time and JIT compiler errors
        case 0: return "CL_SUCCESS";
        case -1: return "CL_DEVICE_NOT_FOUND";
        case -2: return "CL_DEVICE_NOT_AVAILABLE";
        case -3: return "CL_COMPILER_NOT_AVAILABLE";
        case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case -5: return "CL_OUT_OF_RESOURCES";
        case -6: return "CL_OUT_OF_HOST_MEMORY";
        case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case -8: return "CL_MEM_COPY_OVERLAP";
        case -9: return "CL_IMAGE_FORMAT_MISMATCH";
        case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case -11: return "CL_BUILD_PROGRAM_FAILURE";
        case -12: return "CL_MAP_FAILURE";
        case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
        case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
        case -15: return "CL_COMPILE_PROGRAM_FAILURE";
        case -16: return "CL_LINKER_NOT_AVAILABLE";
        case -17: return "CL_LINK_PROGRAM_FAILURE";
        case -18: return "CL_DEVICE_PARTITION_FAILED";
        case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
        
        // compile-time errors
        case -30: return "CL_INVALID_VALUE";
        case -31: return "CL_INVALID_DEVICE_TYPE";
        case -32: return "CL_INVALID_PLATFORM";
        case -33: return "CL_INVALID_DEVICE";
        case -34: return "CL_INVALID_CONTEXT";
        case -35: return "CL_INVALID_QUEUE_PROPERTIES";
        case -36: return "CL_INVALID_COMMAND_QUEUE";
        case -37: return "CL_INVALID_HOST_PTR";
        case -38: return "CL_INVALID_MEM_OBJECT";
        case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case -40: return "CL_INVALID_IMAGE_SIZE";
        case -41: return "CL_INVALID_SAMPLER";
        case -42: return "CL_INVALID_BINARY";
        case -43: return "CL_INVALID_BUILD_OPTIONS";
        case -44: return "CL_INVALID_PROGRAM";
        case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
        case -46: return "CL_INVALID_KERNEL_NAME";
        case -47: return "CL_INVALID_KERNEL_DEFINITION";
        case -48: return "CL_INVALID_KERNEL";
        case -49: return "CL_INVALID_ARG_INDEX";
        case -50: return "CL_INVALID_ARG_VALUE";
        case -51: return "CL_INVALID_ARG_SIZE";
        case -52: return "CL_INVALID_KERNEL_ARGS";
        case -53: return "CL_INVALID_WORK_DIMENSION";
        case -54: return "CL_INVALID_WORK_GROUP_SIZE";
        case -55: return "CL_INVALID_WORK_ITEM_SIZE";
        case -56: return "CL_INVALID_GLOBAL_OFFSET";
        case -57: return "CL_INVALID_EVENT_WAIT_LIST";
        case -58: return "CL_INVALID_EVENT";
        case -59: return "CL_INVALID_OPERATION";
        case -60: return "CL_INVALID_GL_OBJECT";
        case -61: return "CL_INVALID_BUFFER_SIZE";
        case -62: return "CL_INVALID_MIP_LEVEL";
        case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
        case -64: return "CL_INVALID_PROPERTY";
        case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
        case -66: return "CL_INVALID_COMPILER_OPTIONS";
        case -67: return "CL_INVALID_LINKER_OPTIONS";
        case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";
        
        // extension errors
        case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
        case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
        case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
        case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
        case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
        case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
        default: return "Unknown OpenCL error";
    }
}

// Register a kerenel so it can be run
/*
src - source code string
kerneln - number of kernels in source code
... - string names of the kernels in source code
returns 0 on success
*/
/*
Before a function is ran using the ocl api, it must first be
registered. The user needs to supply a source program and any
number of kernels in that program. All names must be unique.
*/
int register_from_src(const char **src, int kerneln, ...) {
    if (!oclinit) return OCLUNINITIALIZED;
    
    int err;
    
    cl_program prog = clCreateProgramWithSource(context, 1, src, NULL, &err);
    safe(err != CL_SUCCESS, "Failed to create program from source: %s\n", clGetErrorString(err));
    // Build with kernel arg info flag to retrive it later. This solution thankfully
    // allows for building programs and having access to some of the information form
    // the compilation process, allowing for this whole library to feasably exist.
    err = clBuildProgram(prog, 0, NULL, "-cl-kernel-arg-info", NULL, NULL);
    if (err == CL_BUILD_PROGRAM_FAILURE) {
        char buildlog[32768];
        clGetProgramBuildInfo(prog, device_id, CL_PROGRAM_BUILD_LOG, (size_t) 32768, buildlog, NULL);
        printf("Build error: \n%s\n", buildlog);
    }
    safe(err != CL_SUCCESS, "Failed to build program: %s\n", clGetErrorString(err));
    
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
                if (strcmp(k->name, name) == 0) { 
                    va_end(valist);
                    return OCLINVALID_NAME;
                }
                
                k = k->next;
            }
            
            k->next = (Klist *) malloc(sizeof(Klist));
            k->next->next = NULL;
            k = k->next;
        }
        
        k->kernel = clCreateKernel(prog, name, &err);
        safe(err != CL_SUCCESS, "Failed to make kernel \"%s\"\n", name);
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
            
            cl_kernel_arg_address_qualifier addressq = CL_KERNEL_ARG_ADDRESS_PRIVATE;
            clGetKernelArgInfo(k->kernel, argument, CL_KERNEL_ARG_ADDRESS_QUALIFIER, sizeof(addressq), &addressq, NULL);
            k->argv[argument]._islocal = addressq == CL_KERNEL_ARG_ADDRESS_LOCAL;
            
            // Initilize values reset
            k->argv[argument]._dsize = 0;
            k->argv[argument]._flags = 0;
            k->argv[argument]._host_data = NULL;
        }
    }
    
    va_end(valist);
    
    return OCLNO_ERR;
}

// Run registered kernel
/*
name - name of the kernel
wdim - number of dimensions to run the kerenl in the GPU (typically between 1 to 3, see OpenCL docs)
gsz - array of sizes for the global run size. Array length should be dim (see OpenCL docs) 
lsz - array of sizes for the local run size. Array length should be dim (see OpenCL docs)
... - parameters for the function. After every pointer there must follow: size of the pointer, operation flags
returns 0 on success
*/
int run_kernel(const char *name, int wdim, size_t *gsz, size_t *lsz, ...) {
    if (!oclinit) return OCLUNINITIALIZED;
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
            // TODO: Maybe the void *host_ptr argument can be used to save the copy?
            k->argv[i]._device_data = clCreateBuffer(context, clflags, k->argv[i].asize * dsize, NULL, &err);
            safe(err != CL_SUCCESS, "Failed to allocate buffer: %s\n", clGetErrorString(err));
            
            if (!k->argv[i]._islocal) {
                data = &(k->argv[i]._device_data);
                device_dsize = sizeof(cl_mem);
            } else {
                data = NULL;
                // This took a bit too much to figure out... OpenCL docs are not the clearest...
                // Moreover, NVIDIA GPUs seem to be very lax when it comes to out of bounds access
                device_dsize = k->argv[i]._dsize * k->argv[i].asize;
            }
            
            // Copy data from given pointer to buffer pointed to by data
            if (flags & OCLCPY)
                safe((err = clEnqueueWriteBuffer(queue, *(cl_mem *) data, CL_TRUE, 0, k->argv[i].asize * dsize, k->argv[i]._host_data, 0, NULL, NULL)) != CL_SUCCESS,
                     "Failed to write buffer data: %s\n", clGetErrorString(err));
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
        
        safe((err = clSetKernelArg(k->kernel, i, device_dsize, data)) != CL_SUCCESS, 
             "Failed to set kernel value: %s\n", clGetErrorString(err));
    }
    
    va_end(valist);
    
    // Run the kernel
    safe((err = clEnqueueNDRangeKernel(queue, k->kernel, wdim, NULL, gsz, lsz, 0, NULL, NULL)) != CL_SUCCESS,
         "Failed to run kernel: %s\n", clGetErrorString(err));
    clFinish(queue);
    
    // Copy out the data and free it
    for (int i = 0; i < k->argc; i++) {
        if (k->argv[i].isptr) {
            if (k->argv[i]._flags & OCLOUT) {
                err = clEnqueueReadBuffer(queue, k->argv[i]._device_data, CL_TRUE, 0, k->argv[i].asize * k->argv[i]._dsize, k->argv[i]._host_data, 0, NULL, NULL);
                safe(err, "Failed to read buffer: %s\n", clGetErrorString(err));
            }
            
            safe((err = clReleaseMemObject(k->argv[i]._device_data)), "Failed to free buffer: %s\n", clGetErrorString(err));
        }
    }
    
    clFinish(queue);
    
    return OCLNO_ERR;
}