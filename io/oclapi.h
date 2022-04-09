/* date = March 31st 2022 2:22 pm */

#ifndef OCLAPI_H
#define OCLAPI_H

#define __CL_ENABLE_EXCEPTIONS
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define DEBUG

#ifdef DEBUG
#define wrap(e) ( e? printf("Err %d\n", e) : 0 )
#define wrapf(func, ...) { int e = func(__VA_ARGS__); warp(e) }
#else
#define wrap(e) 0
#define wrapf(func, ...) func(__VA_ARGS__)
#endif

#define RETTYPE_SIZE 64

enum _OCLAPI_MEM_OP { _OCLCPY, _OCLREAD, _OCLWRITE, _OCLOUT };
enum OCLAPI_MEM { OCLCPY=1 << _OCLCPY, OCLREAD=1 << _OCLREAD, OCLWRITE=1 << _OCLWRITE, OCLOUT=1 << _OCLOUT };

enum OCLAPI_ERR { OCLNO_ERR=0, OCLINVALID_NAME, OCLUNKNOWN_SIZE, OCLINVALID_ARG };

typedef struct arg {
    char rettype[RETTYPE_SIZE];
    int isptr;
    size_t asize;
    // Temporary variable used during execution
    int _dsize;
    int _flags;
    void *_host_data;
    cl_mem _device_data;
} Karg;

typedef struct klist {
    cl_kernel kernel;
    char *name;
    
    int argc;
    // Array of args
    Karg *argv;
    
    struct klist *next;
} Klist;

int ocinit();
int occln();
int register_from_src(const char **src, int kerneln, ...);
int run_kernel(const char *name, int wdim, size_t *gsz, size_t *lsz, ...);

#endif //OCLAPI_H
