/* date = February 1st 2022 1:48 pm */
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef MATH_H
#define MATH_H

// TODO: These should not be absolute! but its the only thing that seems to work
// when the ldfile function is in a diffrent file...
#define MAT_PROG "./math/hware/mat.cl"
#define MATH_PROG "./math/hware/math.cl"

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

#define EPSIL 1e-10

enum STFT_OPTYPE { REAL, COMPLEX, MAGSQ };
enum DB_OPTYPE { SCALE_MAX, SCALE_ONE, SCALE_FIRST };
enum MATH_ERR { MNO_ERR=0, MINVALID_ARG, MZERO_DIV, MSMALL_BUF };

#define stftw(framesize) ceil(framesize / 2 + 1)
#define stfth(sz, framesize, hopsize) ceil(((sz - framesize) / hopsize) + 1);
#define stftwh(sz, framesize, hopsize, w, h) { *w = stftw(framesize); *h = stfth(sz, framesize, hopsize); } 

int amptodb(double **src, int sz, double topdb, int op);
int stft(double *src, int sz, int framesize, int windowsize, int hopsize, double **res, int szr, int op);
int melspec(double *src, int sz, int framesize, int windowsize, int hopsize, int bands, double fstart, double fend, double **res);

int matmul(double *a, unsigned int aw, unsigned int ah, 
           double *b, unsigned int bw, unsigned int bh, double **res);
int matadd(double *a, unsigned int aw, unsigned int ah,
           double *b, unsigned int bw, unsigned int bh, double **res);
int matsub(double *a, unsigned int aw, unsigned int ah,
           double *b, unsigned int bw, unsigned int bh, double **res);
int matmuls(double *a, unsigned int aw, unsigned int ah,
            double b, double **res);
int matadds(double *a, unsigned int aw, unsigned int ah,
            double b, double **res);

int mainit();
int macln();

#endif //MATH_H
