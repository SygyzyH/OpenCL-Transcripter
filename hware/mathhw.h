#ifdef __CUDACC__
/* date = February 6th 2022 11:50 pm */

#ifndef MATHHW_H
#define MATHHW_H

int astft(double *src, int sz, int framesize, int windowsize, int hopsize, double **res, int szr, int op);

#endif //MATHHW_H
#endif