/* date = Febuary 9th 2022 20:21 pm */

#ifndef MAT_H
#define MAT_H

enum MAT_ERR { TNO_ERR=0, TSIZE_MISMATCH };

typedef struct {
    double *data;
    int width, height;
} Mat;

int matmul(Mat a, Mat b, Mat *res);
int matdot(Mat a, Mat b, double *res);
int matadd(Mat a, Mat b, Mat *res);
int matsub(Mat a, Mat b, Mat *res);

#endif //MAT_H