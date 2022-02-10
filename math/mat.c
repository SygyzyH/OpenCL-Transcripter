#include "mat.h"

// Hardware acceleration.
// The functions ARE NOT AVAILABLE IF __CUDACC__ IS NOT DEFINED AT COMPILE TIME
#include "../hware/mathw.h"

int matdot(Mat a, Mat b, double *res) {
#ifdef __CUDACC__
    // GPU Implementation
    return amatdot(a, b, res);
#else
    // CPU Implementation
    
    if (a.width != b.width || a.height != b.height)
		return TSIZE_MISMATCH;
    
    for (int i = 0; i < a.height; i++)
        for (int j = 0; j < a.width; j++)
    (*res) += a.data[j + i * a.width] * b.data[j + i * a.width];
    
    return TNO_ERR;
#endif
}

int matadd(Mat a, Mat b, Mat *res) {
#ifdef __CUDACC__
    // GPU Implementation
    return amatadd(a, b, res);
#else
    // CPU Implementation
    
    if (a.width != b.width || a.height != b.height)
		return TSIZE_MISMATCH;
    
    res->width = a.width;
    res->height = a.height;
	
    for (int i = 0; i < a.height; i++)
        for (int j = 0; j < a.width; j++)
        res->data[j + i * a.width] = a.data[j + i * a.width] + b.data[j + i * a.width];
    
	return TNO_ERR;
#endif
}

int matsub(Mat a, Mat b, Mat *res) {
#ifdef __CUDACC__
    // GPU Implementation
    return amatsub(a, b, res);
#else
    // CPU Implementation
    
    if (a.width != b.width || a.height != b.height)
		return TSIZE_MISMATCH;
    
    res->width = a.width;
    res->height = a.height;
	
    for (int i = 0; i < a.height; i++)
        for (int j = 0; j < a.width; j++)
        res->data[j + i * a.width] = a.data[j + i * a.width] - b.data[j + i * a.width];
    
	return TNO_ERR;
#endif
}