// No header guards on purpse
// Insure ""parameter"" is defined
#ifndef FILENAME
#error "No FILENAME defined for static source"
#endif
#ifndef OCLAPI_H
#error "oclapi not included"
#endif
// Ensure OCLAPI_FREE_IF_STATIC is free to define
#ifdef OCLAPI_FREE_IF_STATIC
#undef OCLAPI_FREE_IF_STATIC
#endif
#ifdef OCLAPI_STATIC_SOURCE
#define OCLAPI_FREE_IF_STATIC
// If static source, use include. (OCLAPI_HEADER must be in the start of the file!)
#include FILENAME 
#else
	// Otherwise, load the file at runtime
	ldfile(FILENAME)
	// And free it (at runtime)
#define OCLAPI_FREE_IF_STATIC free(FILENAME);
#endif
#undef FILENAME
;