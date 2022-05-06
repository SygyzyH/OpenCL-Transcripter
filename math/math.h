/* date = February 1st 2022 1:48 pm */
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef MATH_H
#define MATH_H

#define MAT_PROG "./math/hware/mat.cl"
#define MATH_PROG "./math/hware/math.cl"

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define CLAMP(BOT, A, TOP) (MIN(MAX(BOT, A), TOP))

#define EPSIL 1e-6

enum STFT_SIDES { ONE_SIDED=1, TWO_SIDED=2, TWO_SIDED_CENTERED=3 };
enum DB_OPTYPE { SCALE_MAX, SCALE_ONE, SCALE_FIRST };
enum MATH_ERR { MNO_ERR=0, MINVALID_ARG, MZERO_DIV, MSMALL_BUF, MUNINITIALIZED };

#define stftw(framesize) ceil(framesize / 2 + 1)
#define stfth(sz, framesize, hopsize) ceil(((sz - framesize) / hopsize) + 1)
#define stftwh(sz, framesize, hopsize, w, h) { *w = stftw(framesize); *h = stfth(sz, framesize, hopsize); } 

#define unpkmat(mat) (mat).data, (mat).width, (mat).height
#define unpkmatp(mat) (mat)->data, (mat)->width, (mat)->height

typedef struct {
    double *data;
    unsigned int width;
    unsigned int height;
} Mat;

int amptodb(double **src, int sz, double topdb, int op);
int stft(double *src, int sz, int framesize, int windowsize,
         int fftlen, int hopsize, int sides, Mat **res);
int melspec(double *src, int sz, int fs, int framesize, int windowsize, int fftlen, 
            int hopsize, int bands, double fstart, double fend, Mat **res);

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

int ensuredims(Mat inp, int tw, int th, Mat **res);

int mainit();

#endif //MATH_H
