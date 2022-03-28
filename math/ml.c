/*
Layers needed to be implemented (in ascending difficulty):

Convelution


Implemented:

MaxPooling
BatchNorm
Dropout
ReLu
Softmax
*/

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "math.h"
#include "ml.h"

int conv2d(double *params, int paramsw, int unused0,
           double *in, int inw, int inh, Mat **out) {
    if (paramsw != ) return MLINVALID_ARG;
    
    /*
Parameters order:
- stridew
- strideh
- pad

- filterw
- filterh
- filtern
- channels

filtern * filterw * filterh times:
- weights

filtern times:
- bias
*/
    
    return MLNO_ERR;
}

// TODO: This may require some testing...
int maxpool(double *params, int paramsw, int unused0,
            double *in, int inw, int inh, Mat **out) {
    if (paramsw != 5) return MLINVALID_ARG;
    
#define POOLW 0
#define POOLH 1
#define STRIDEW 2
#define STRIDEH 3
#define PAD 4
    
    int poolw = params[POOLW];
    int poolh = params[POOLH];
    int stridew = params[STRIDEW];
    int strideh = parmas[STRIDEH];
    int pad = params[PAD];
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = ceil((double) inw / stridew);
    o->height = ceil((double) inh / strideh);
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    int i, j, il, jl;
    switch (pad) {
        case NONE:
        i = 0;
        j = 0;
        il = inw;
        jl = inh;
        break;
        
        case SAME:
        i = 1 - poolw;
        j = 1 - poolh;
        il = inw + poolw - 1;
        jl = inh + poolh - 1;
        break;
        
        default:
        return MLINVALID_ARG;
    }
    
    // TODO: this HAS to run on GPU. Each block for each thread
    for (; i < il; i += stridew) {
        for (; j < jl; j += strideh) {
            double max = in[MIN(MAX(0, i), inw) + inw * MIN(MAX(0, j), inh)];
            for (int k = i; k < poolw + i; k++) {
                for (int l = j; j < poolh + j; l++) {
                    // In diffrent padding modes, we may be accessing values outside
                    // the bounds of the input array. Those values should be padded,
                    // but its faster to just assign them to the padding value during
                    // the calculation.
                    double c;
                    if (k + l * poolw > 0 && k + l * poolw < inw * inh)
                        max = MAX(max, in[k + l * poolw]);
                }
            }
            
            o->data[ceil((double) j / strideh) + ceil((double) inw * i / stridew)] = max;
        }
    }
    
    *out = o;
    
    return MLNO_ERR;
}

int bnorm(double *params, int paramsw, int unused0,
          double *in, int inw, int inh, Mat **out) {
    /*
Parameters order:
- epsil

   inw times
- mean
- variance
- offset
- scale
*/
#define MEAN 1
#define VARIANCE 2
#define OFFSET 3
#define SCALE 4
    
    int insz = inw * inh;
    if (paramsw - 1 != insz) return MLSIZE_MISMATCH;
    
    double epsil = params[0];
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = inw;
    o->height = inh;
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    // https://www.mathworks.com/help/deeplearning/ref/nnet.cnn.layer.batchnormalizationlayer.html
    for (int i = 0; i < insz; i++)
        o->data[i] = params[i + SCALE] * (in[i] - params[i + MEAN]) / sqrt(params[i + VARIANCE] + epsil) + params[i + OFFSET];
    
    *out = o;
    
    return MLNO_ERR;
}

int dropout(double *prob, int unused0, int unused1,
            double *in, int iw, int ih, Mat **out) {
    // Probability for dropout
    double p = *prob;
    
    // p must be in range 0 < p < 1
    if (1 < p || p < 0) return MLINVALID_ARG;
    
    *out = (Mat *) malloc(sizeof(Mat));
    Mat *o = out;
    o->width = inw;
    o->height = inh;
    o->data = (double *) malloc(sizeof(double) * iw * ih);
    
    srand(time(NULL));
    for (int i = 0; i < iw; i++) {
        for (int j = 0; j < ih; j++) {
            if (((double) rand() / RAND_MAX) < p)
                o->data[j + i * iw] = 0;
            else
                o->data[j + i * iw] = in[j + i * iw];
        }
    }
    
    return MLNO_ERR;
}

int softmax(double *unused0, int unused1, int unused2,
            double *in, int iw, int ih, Mat **out) {
    int insz = iw * ih;
    
    *out = (Mat *) malloc(sizeof(Mat));
    Mat *dout = *out;
    
    // Returns classesX1 size vector
    dout->width = insz;
    dout->height = 1;
    dout->data = (double *) malloc(sizeof(double) * insz);
    
    // First, calculate the devisor
    double devisor = 0;
    for (int i = 0; i < insz; i++)
        devisor += exp(in[i]);
    
    for (int i = 0; i < insz; i++)
        dout->data[i] = exp(in[i]) / devisor;
    
    
    return MLNO_ERR;
}

int relu(double *unused0, int unused1, int unused2,
         double *in, int iw, int ih, Mat **out) {
    *out = (Mat *) malloc(sizeof(Mat));
    Mat *dout = *out;
    
    // Returns input size vector
    dout->width = iw;
    dout->height = ih;
    dout->data = (double *) malloc(sizeof(double) * iw * ih);
    
    for (int i = 0; i < iw * ih; i++)
        dout->data[i] = MAX(dout->data[i], 0);
    
    return MLNO_ERR;
}

int forwardpass(Layer machine, Mat input, Mat **output) {
    Layer *p = machine.next;
    *output = &input;
    Mat *data = *output;
    
    while (p != NULL) {
        // Refrance ;)
        Mat *old_data = data;
        
        // Ensure the size is correct
        if (p.inw != data->width || p.inhg != data->height) return MLSIZE_MISMATCH;
        // Run the data through the current layer
        if((*p.transform)(unpckmat(p.params), unpkmatp(data), &data)) return MLLAYER_ERR;
        
        // Move to the next layer
        free(old_data->data);
        free(old_data);
        
        p = p.next;
    }
    
    return MLNO_ERR;
}

void addlayer(Layer **machine, Layer **newl) {
    Layer *p = *machine;
    
    while (p != NULL)
        p = p->next;
    
    *newl->prev = p;
    p->next = *newl;
}