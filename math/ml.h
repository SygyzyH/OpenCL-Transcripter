/* date = February 12th 2022 4:09 pm */

#ifndef ML_H
#define ML_H

#define unpkmat(mat) (mat).data, (mat).width, (mat).height
#define unpkmatp(mat) (mat)->data, (mat)->width, (mat)->height

enum ML_ERR { MLNO_ERR=0, MLINVALID_ARG, MLLAYER_ERR, MLSIZE_MISMATCH };
enum PAD_TYPE { SAME, NONE };

typedef struct {
    double *data;
    unsigned int width;
    unsigned int height;
} Mat;

typedef struct Layer {
    int inw; 
    int inh;
    Mat params;
    /* 
Params: layer parameter (usually weights), input, output ptr.
 Returns: output of function.
*/
    int (*transform)(double *, int, int, double *, int, int, Mat **);
    // Maybe ill implement
    /*
Prams: pointer to layer parameters, next layer's error, width
*/
    void (*learn)(Mat *, double *, int);
    struct Layer *prev;
    struct Layer *next;
} Layer;

void addlayer(Layer **machine, Layer **newl);
int forwardpass(Layer machine, Mat input, Mat **output);

int softmax(double *unused0, int unused1, int unused2,
            double *in, int iw, int ih, Mat **out);
int relu(double *unused0, int unused1, int unused2,
         double *in, int iw, int ih, Mat **out);
int dropout(double *prob, int unused0, int unused1,
            double *in, int iw, int ih, Mat **out);
int bnorm(double *params, int paramsw, int unused0,
          double *in, int inw, int inh, Mat **out);
int maxpool(double *params, int paramsw, int unused0,
            double *in, int inw, int inh, Mat **out);
int conv2d(double *params, int paramsw, int unused0,
           double *in, int inw, int inh, Mat **out);

#endif //ML_H
