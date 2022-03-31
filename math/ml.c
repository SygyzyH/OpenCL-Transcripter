#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "math.h"
#include "ml.h"

/*
Multidimensionality in memory is handled as:
for a tensor with 2nd degree dimensions of
w - 2
h - 2

and c extra dimensions, the final dimensions would be
c - 3
w - 2
h - 2 * c = 6

and the data would be laid out as such:

d1 d1
d1 d1
-----
d2 d2
d2 d2
-----
d3 d3
d3 d3
*/

// TODO: This may require some testing...

int conv2d(double *params, int paramsw, int unused0,
           double *in, int inw, int inh, Mat **out) {
    /*
Parameters order:
- stridew
- strideh

- filterw
- filterh
- filtern

- pad
- channels

filtern * filterw * filterh times:
- weights

filtern times:
- bias
*/
#pragma pack(push, 1)
    typedef struct {
        double stridew;
        double strideh;
        double filterw;
        double filterh;
        double filtern;
        double pad;
        double channels;
    } PARAMS;
#pragma pack(pop)
    
    if (paramsw != sizeof(PARAMS)) return MLINVALID_ARG;
    PARAMS *param = (PARAMS *) params;
    if (paramsw != sizeof(PARAMS) + param->filterw * param->filterh * param->filtern + param->filtern) return MLINVALID_ARG;
    
    // ooo spooky pointer math, im so scared let me call mommy java to pick me up
    double *weights = (double *) params + sizeof(PARAMS);
    int weightn = (int) param->filterw * param->filterh * param->filtern;
    double *bias = weights + weightn;
    int biasn = (int) param->filtern;
    
    int ptop, pbot, pleft, pright;
    switch ((int) param->pad) {
        case SAME: {
            // Matlab pads with the ceil on bottom and right side
            int pw = (int) param->filterw - param->stridew;
            int ph = (int) param->filterh - param->strideh;
            
            ptop = floor(ph / 2);
            pbot = ph - ptop;
            pleft = floor(pw / 2);
            pright = pw - pleft;
            
            break;
        }
        
        case NONE: {
            ptop = 0;
            pbot = 0;
            pright = 0;
            pleft = 0;
            
            break;
        }
        
        default:
        return MLINVALID_ARG;
    }
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = floor((inw - param->filterw + 2 * pright) / param->stridew + 1);
    o->height = floor(param->filtern * (inh - param->filterh + 2 * pbot) / param->strideh + 1);
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    // TODO: This also HAS to run on the GPU.
    for (int filter = 0; filter < param->filtern; filter++) {
        for (int i = -pleft; i < inw + pright; i += param->stridew) {
            for (int j = -ptop; j < inh / param->channels + pbot; j += param->strideh) {
                double sum = bias[filter];
                for (int c = 0; c < param->channels; c++) {
                    for (int k = i; k < param->filterw; k++) {
                        for (int l = j; j < param->filterh; l++) {
                            
                            // Usually when padding you'd create a new array and add the
                            // padding zeros, however, in this direct implementation we
                            // can just check if were in padding space and not add the
                            // result to the total sum. Saves time copying the array and
                            // saves on memory usage.
                            if (k + l * param->filterw > 0 && k + l * param->filterw < inw * inh)
                                sum += in[(int) (k + l * param->filterw + c * inw * inh / param->channels)] *
                                weights[(int) (k + l * param->filterw + c * param->filterw * param->filterh / param->channels)];
                        }
                    }
                }
                
                o->data[(int) (ceil((double) (i + pleft) / param->stridew) + ceil((double) (j + ptop) * o->width / param->strideh) + filter * o->width * o->height / param->filtern)] = sum;
            }
        }
    }
    
    *out = o;
    
    return MLNO_ERR;
}

int maxpool(double *params, int paramsw, int unused0,
            double *in, int inw, int inh, Mat **out) {
    
#pragma pack(push, 1)
    typedef struct {
        double poolw;
        double poolh;
        double stridew;
        double strideh;
        double pad;
        double channels;
    } PARAMS;
#pragma pack(pop)
    
    if (paramsw != sizeof(PARAMS)) return MLINVALID_ARG;
    PARAMS *param = (PARAMS *) params;
    
    int ptop, pbot, pleft, pright;
    switch ((int) param->pad) {
        case SAME: {
            // Matlab pads with the ceil on bottom and right side
            int pw = (int) param->poolw - param->stridew;
            int ph = (int) param->poolh - param->strideh;
            
            ptop = floor(ph / 2);
            pbot = ph - ptop;
            pleft = floor(pw / 2);
            pright = pw - pleft;
            
            break;
        }
        
        case NONE: {
            ptop = 0;
            pbot = 0;
            pright = 0;
            pleft = 0;
            
            break;
        }
        
        default:
        return MLINVALID_ARG;
    }
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = floor((inw - param->poolw + 2 * pright) / param->stridew + 1);
    o->height = floor(param->channels * (inh - param->poolh + 2 * pbot) / param->strideh + 1);
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    // TODO: This HAS to run on GPU. Each block for each thread
    for (int c = 0; c < param->channels; c++) {
        for (int i = -pleft; i < inw + pright; i += param->stridew) {
            for (int j = -ptop; j < inh / param->channels + pbot; j += param->strideh) {
                double max = in[(int) (MIN(MAX(0, i), inw) + MIN(MAX(0, j), inh) * inw + c * inw * inh / param->channels)];
                for (int k = i; k < param->poolw + i; k++) {
                    for (int l = j; j < param->poolh + j; l++) {
                        
                        // In diffrent padding modes, we may be accessing values outside
                        // the bounds of the input array. Those values should be padded,
                        // but its faster to just assign them to the padding value during
                        // the calculation.
                        if (k + l * param->poolw > 0 && k + l * param->poolw < inw * inh / param->channels)
                            max = MAX(max, in[(int) (k + l * param->poolw + c * inw * inh / param->channels)]);
                    }
                }
                
                o->data[(int) (ceil((double) i / param->stridew) + ceil((double) j / param->strideh) * o->width + c * o->width * o->height / param->channels)] = max;
            }
        }
    }
    
    *out = o;
    
    return MLNO_ERR;
}

// TODO: Maybe this can also use a struct (?)
int bnorm(double *params, int paramsw, int unused0,
          double *in, int inw, int inh, Mat **out) {
    /*
Parameters order:
   insz times
- mean
- variance
- offset
- scale
*/
#define MEAN 0
#define VARIANCE 1
#define OFFSET 2
#define SCALE 3
    
    int insz = inw * inh;
    if (paramsw != insz * 4) return MLSIZE_MISMATCH;
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = inw;
    o->height = inh;
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    // https://www.mathworks.com/help/deeplearning/ref/nnet.cnn.layer.batchnormalizationlayer.html
    for (int i = 0; i < insz; i++)
        o->data[i] = params[i + SCALE] * (in[i] - params[i + MEAN]) / sqrt(params[i + VARIANCE] + EPSIL) + params[i + OFFSET];
    
    *out = o;
    
    return MLNO_ERR;
}

int dropout(double *prob, int unused0, int unused1,
            double *in, int iw, int ih, Mat **out) {
    // Probability for dropout
    double p = *prob;
    
    // p must be in range 0 < p < 1
    if (1 < p || p < 0) return MLINVALID_ARG;
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = iw;
    o->height = ih;
    o->data = (double *) malloc(sizeof(double) * iw * ih);
    
    srand(time(NULL));
    for (int i = 0; i < ih; i++) {
        for (int j = 0; j < iw; j++) {
            if (((double) rand() / RAND_MAX) < p)
                o->data[j + i * iw] = 0;
            else
                o->data[j + i * iw] = in[j + i * iw];
        }
    }
    
    *out = o;
    
    return MLNO_ERR;
}

int softmax(double *unused0, int unused1, int unused2,
            double *in, int iw, int ih, Mat **out) {
    int insz = iw * ih;
    
    // Returns classesX1 size vector
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = insz;
    o->height = 1;
    o->data = (double *) malloc(sizeof(double) * insz);
    
    // First, calculate the devisor
    double devisor = 0;
    for (int i = 0; i < insz; i++)
        devisor += exp(in[i]);
    
    for (int i = 0; i < insz; i++)
        o->data[i] = exp(in[i]) / devisor;
    
    *out = o;
    
    return MLNO_ERR;
}

int relu(double *unused0, int unused1, int unused2,
         double *in, int iw, int ih, Mat **out) {
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = iw;
    o->height = ih;
    o->data = (double *) malloc(sizeof(double) * iw * ih);
    
    for (int i = 0; i < iw * ih; i++)
        o->data[i] = MAX(in[i], 0);
    
    *out = o;
    
    return MLNO_ERR;
}

int forwardpass(Layer machine, Mat input, Mat **output) {
    Layer *p = &machine;
    Mat *datai = &input;
    Mat *prevout = NULL;
    
    while (p != NULL) {
        // Ensure the size is correct
        if (p->inw != datai->width || p->inh != datai->height) return MLSIZE_MISMATCH;
        // Run the data through the current layer
        if((*p->transform)(unpkmat(p->params), unpkmatp(datai), output)) return MLLAYER_ERR;
        
        // Move to the next layer
        datai = *output;
        p = p->next;
        
        if (prevout != NULL)
            free(prevout->data);
        // free-ing NULL is allowed
        free(prevout);
        
        prevout = *output;
    }
    
    return MLNO_ERR;
}

void addlayer(Layer **machine, Layer **newl) {
    Layer *p = *machine;
    
    while (p != NULL)
        p = p->next;
    
    (*newl)->prev = p;
    p->next = *newl;
}