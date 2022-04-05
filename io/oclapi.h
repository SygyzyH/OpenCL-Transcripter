/* date = March 31st 2022 2:22 pm */

#ifndef OCLAPI_H
#define OCLAPI_H

#define __CL_ENABLE_EXCEPTIONS
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define va_arg_st(valist, st) ( (!strstr(st, "char") || !strstr(st, "int"))? va_arg(valist, int) : (!strstr(st, "float") || !strstr(st, "double"))? va_arg(valist, double) : NULL )
#define sizeof_st(st) ( !strstr(st, "char")? sizeof(char) : !strstr(st, "int")? sizeof(int) : !strstr(st, "float")? sizeof(float) : !strstr(st, "double")? sizeof(double) : 0 )
#define cast_st(arg, st) ( !strstr(st, "char")? (char) arg : !strstr(st, "int")? (int) arg : !strstr(st, "float")? (float) arg : !strstr(st, "double")? (double) arg : 0 )

enum _OCLAPI_MEM_OP { _OCLCPY, _OCLREAD, _OCLWRITE, _OCLOUT };
enum OCLAPI_MEM { OCLCPY=1 << _OCLCPY, OCLREAD=1 << _OCLREAD, OCLWRITE=1 << _OCLWRITE, OCLOUT=1 << _OCLOUT };

enum OCLAPI_ERR { OCLNO_ERR=0, OCLINVALID_NAME, OCLUNKNOWN_SIZE, OCLINVALID_ARG };

/*typedef struct klist {
    cl_program prog;
    cl_kernel kernel;
    const char *name;
    struct klist *next;
} Klist;*/

typedef struct klist {
    cl_kernel kernel;
    // Name char limit is 128
    const char name[128];
    
    int argc;
    struct arg {
        int rettype[16];
        int isptr;
        size_t asize;
        struct arg *nexta;
    };
    
    struct klist *next;
} Klist;

typedef struct arg {
    int size;
    void *arg;
    int outputarg;
    void *outputto;
    const char *type;
    struct arg *next;
} Arguments;

int ocinit();
int occln();
int register_from_src(const char **src, int kerneln, ...);
int run_kernel(const char *name, size_t globalSize[3], ...);

#endif //OCLAPI_H
