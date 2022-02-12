/* date = February 12th 2022 4:09 pm */

#ifndef ML_H
#define ML_H

enum ML_ERR { MLNO_ERR=0 };

typedef struct {
    double *data;
    unsigned int width;
    unsigned int height;
} Mat;

#define unpkmat(mat) (mat).data, (mat).width, (mat).height

#endif //ML_H
