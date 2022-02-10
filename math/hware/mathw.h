/* date = January 31st 2022 10:50 pm */
#ifdef __CUDACC__

#include "../math/mat.h"

#ifndef MATHW_H
#define MATHW_H

int amatmul(Mat a, Mat b, Mat *res);
int amatdot(Mat a, Mat b, double *res);
int amatadd(Mat a, Mat b, Mat *res);
int amatsub(Mat a, Mat b, Mat *res);

__device__ double atomicAddD(double* address, double val);

#endif //MATHW_H
#endif

/*
network options:
learning rate
learning rate decay factor
learning rate decay period
epochs size
batch size
validation frequency

network layers:
image input
relu
convelution layer
batch normalizing layer
dropout layer
softmax

*/
