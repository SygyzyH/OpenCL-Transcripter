/* date = March 31st 2022 2:22 pm */

#ifndef OCLAPI_H
#define OCLAPI_H

#define __CL_ENABLE_EXCEPTIONS
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

enum _OCLAPI_MEM_OP { _OCLCPY, _OCLREAD, _OCLWRITE };
enum OCLAPI_MEM { OCLCPY=1 << _OCLCPY, OCLREAD=1 << _OCLREAD, OCLWRITE=1 << _OCLWRITE };

enum OCLAPI_ERR { OCLNO_ERR=0, OCLINVALID_NAME, OCLUNKNOWN_SIZE, OCLINVALID_ARG };

typedef struct klist {
    cl_program prog;
    cl_kernel kernel;
    const char *name;
    struct klist *next;
} Klist;

#endif //OCLAPI_H
