#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "math.h"
#include "ml.h"

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
    int weightn = param->filterw * param->filterh * param->filtern;
    double *bias = weights + weightn;
    int biasn = param->filtern;
    
    /*
Multidimensionality in memory is laid out as:
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
    
    int ptop, pbot, pleft, pright;
    switch (param->pad) {
        case SAME:
        // Matlab pads with the ceil on bottom and right side
        int pw = param->filterw - param->stridew;
        int ph = param->filterh - param->strideh;
        
        ptop = floor(ph / 2);
        pbot = ph - ptop;
        pleft = floor(pw / 2);
        pright = pw - pleft;
        break;
        
        case NONE:
        ptop = 0;
        pbot = 0;
        pright = 0;
        pleft = 0;
        break;
        
        default:
        return MLINVALID_ARG;
    }
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = floor((inw - param->filterw + 2 * pright) / param->stridew + 1);
    o-> height = floor(param->filtern * (inh - param->filterh + 2 * pbot) / param->strideh + 1);
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    int i, j, il, jl;
    
    for (int filter = 0; filter < param->filtern; filter++) {
        j += filter * inw;
        for (; i < il; i += param->stridew) {
            for (; j < jl; j += param->strideh) {
                // TODO: also needs to handle padding
                o->data[j + i * o->width] = 
                    bias[filter] + 
                    matdot((double *) in + j + i * inw, param->filterw, param->filterh,
                           weights, param->filterw, param->filterh);
            }
        }
    }
    
    *out = o;
    
    return MLNO_ERR;
}

// TODO: refractor
// TODO: This may require some testing...
int maxpool(double *params, int paramsw, int unused0,
            double *in, int inw, int inh, Mat **out) {
    if (paramsw != 5) return MLINVALID_ARG;
    
    // TODO: channels
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
    // TODO: padding may effect the size calculation
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
            
            o->data[ceil((double) i / stridew) + ceil((double) j / strideh) o->width] = max;
        }
    }
    
    *out = o;
    
    return MLNO_ERR;
}

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
    if (paramsw != insz) return MLSIZE_MISMATCH;
    
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