#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "../io/args.h"
#include "math.h"
#include "ml.h"

/* 
Internal ML functions, implemenation may vary in the future.
 For the user end, use the function names to specify layer type 
when making a machine.
 */
// TODO: Layer functions need to run on the GPU, most of 
// this can be easily done using the math library.
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

filtern * filterw * filterh * channels times:
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
    
    if (paramsw < sizeof(PARAMS) / sizeof(double)) return MLINVALID_ARG;
    PARAMS *param = (PARAMS *) params;
    if (paramsw != sizeof(PARAMS) / sizeof(double) + (param->filterw * param->filterh * param->channels + 1) * param->filtern) return MLINVALID_ARG;
    
    // ooo spooky pointer math, im so scared let me call mommy java to pick me up
    double *weights = (double *) params + sizeof(PARAMS) / sizeof(double);
    int weightn = (int) param->filterw * param->filterh * param->channels * param->filtern;
    double *bias = weights + weightn;
    int biasn = (int) param->filtern;
    for (int i = 0; i < biasn; i++) {
        printf("%lf, ", bias[i]);
    } puts("");
    
    int pw, ph;
    int ptop, pbot, pleft, pright;
    switch ((int) param->pad) {
        case SAME: {
            // In same padding mode, the output size would be input size / stride.
            // Using the formula (input size - filter size + 2 * padding) / stride + 1 = output
            // Solving for 2 * padding gives padding = filter size - stride.
            // Than the padding of each dimension is padding / 2. Padding doesn't have to
            // be devisable by 2, so to solve that one side is given the reminder.
            // Matlab pads with the ceil on bottom and right side
            pw = (int) abs(param->filterw - param->stridew);
            ph = (int) abs(param->filterh - param->strideh);
            
            // If there is a need for more padding than the filter size, there would
            // be iterations over padding only. No point in those.
            if (pw > param->filterw || ph > param->filterh) {
                pw = ph = 0;
                
                ptop = 0;
                pbot = 0;
                pright = 0;
                pleft = 0;
                
                break;
            }
            
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
        if (chkset(sets, DB)) puts("ML_C: Convelution layer FAILURE - Invalid padding type.");
        return MLINVALID_ARG;
    }
    
    int einw = inw, einh = (int) inh / param->channels;
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = floor((einw - param->filterw + pw) / param->stridew + 1);
    o->height = floor(param->filtern * ((einh - param->filterh + ph) / param->strideh + 1));
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    for (int n = 0; n < param->filtern; n++) {
        // Iterating over the output is clearer than over the input
        for (int i = 0; i < o->height / param->filtern; i++) {
            for (int j = 0; j < o->width; j++) {
                // Starting sum with bias would save an addition later
                double sum = bias[n];
                
                // Convelution goes three dimensionaly
                for (int c = 0; c < param->channels; c++) {
                    // effective channel height start (and end) is the starting and ending 
                    // heights when considering the layout of channels in memory.
                    int ecinhs = c * einh;
                    int ecinhe = (c + 1) * einh;
                    
                    // Calculate the starting position. Filter will be iterated starting from
                    // these indexes.
                    int startw = j * param->stridew - pleft;
                    int starth = i * param->strideh - ptop + ecinhs;
                    
                    // Iterate over the filter. Padding cannot be ignored by clamping
                    // since itll be multiplied by a potentially non-zero value.
                    for (int k = starth; k < param->filterh + starth; k++) {
                        for (int l = startw; l < param->filterw + startw; l++) {
                            if (l >= 0 && l < einw && k >= ecinhs && k < ecinhe) {
                                int weindex = (l - startw) + (k - starth) * param->filterw + 
                                    c * param->filterw * param->filterh +
                                    n * param->filterw * param->filterh * param->channels;
                                sum += in[l + k * einw] * weights[weindex];
                            }
                        }
                    }
                }
                
                // Save the result
                int eoc = (int) n * o->width * o->height / param->filtern;
                o->data[j + i * o->width + eoc] = sum;
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
        double stridew;
        double strideh;
        double poolw;
        double poolh;
        double pad;
        double channels;
    } PARAMS;
#pragma pack(pop)
    
    if (paramsw != sizeof(PARAMS) / sizeof(double)) {
        if (chkset(sets, DB))
            puts("ML_C: Maxpool layer FAILURE - Parameter size mistmatch.");
        return MLINVALID_ARG;
    }
    
    PARAMS *param = (PARAMS *) params;
    
    int pw, ph;
    int ptop, pbot, pleft, pright;
    switch ((int) param->pad) {
        case SAME: {
            // In same padding mode, the output size would be input size / stride.
            // Using the formula (input size - pool size + 2 * padding) / stride + 1 = output
            // Solving for 2 * padding gives padding = pool size - stride.
            // Than the padding of each dimension is padding / 2. Padding doesn't have to
            // be devisable by 2, so to solve that one side is given the reminder.
            // Matlab pads with the ceil on bottom and right side
            pw = (int) abs(param->poolw - param->stridew);
            ph = (int) abs(param->poolh - param->strideh);
            
            // If there is a need for more padding than the pool size, there would
            // be iterations over padding only. No point in those.
            if (pw > param->poolw || ph > param->poolh) {
                pw = ph = 0;
                
                ptop = 0;
                pbot = 0;
                pright = 0;
                pleft = 0;
                
                break;
            }
            
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
        if (chkset(sets, DB)) puts("ML_C: Maxpool layer FAILURE - Invalid padding type.");
        return MLINVALID_ARG;
    }
    
    int einw = inw, einh = (int) inh / param->channels;
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = floor((einw - param->poolw + pw) / param->stridew + 1);
    o->height = floor(param->channels * ((einh - param->poolh + ph) / param->strideh + 1));
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    // Its clearer to represent the three dimensional input as a triple
    // nested array, even though iterating through i -> o->hieght and 
    // figuring out c is possible.
    for (int c = 0; c < param->channels; c++) {
        // effective channel height start and end is the starting and ending 
        // height  when considering the layout of channels in memory
        int ecinhs = c * einh;
        int ecinhe = (c + 1) * einh;
        
        // Iterating over the output is clearer than over the input
        for (int i = 0; i < o->height / param->channels; i++) {
            for (int j = 0; j < o->width; j++) {
                // Calculate the starting position. Pool will be iterated starting from
                // these indexes
                int startw = j * param->stridew - pleft;
                int starth = i * param->strideh - ptop + ecinhs;
                
                // Starting max is the first in the pool
                double max = in[CLAMP(0, startw, einw - 1) + CLAMP(ecinhs, starth, ecinhe - 1) * einw];
                
                // Iterate over the pool and find the max value. Padding is ignored by
                // clamping the value to the effective range
                printf("Testing (%d, %d)\n", startw, starth);
                for (int k = starth; k < param->poolh + starth; k++) {
                    for (int l = startw; l < param->poolw + startw; l++) {
                        max = MAX(max, in[CLAMP(0, l, einw - 1) + CLAMP(ecinhs, k, ecinhe - 1) * einw]);
                    }
                }
                
                // Save the result
                int eoc = (int) c * o->width * o->height / param->channels;
                o->data[j + i * o->width + eoc] = max;
            }
        }
    }
    
    *out = o;
    
    return MLNO_ERR;
}

int fullyc(double *params, int paramsw, int unused0,
           double *in, int inw, int inh, Mat **out) {
    // TODO: Could be done using matmul and matadds.
    /*
Parameters order:
- in size
- out size

insz * outsz times:
 - weight

outsz times:
- bias
*/
    
#pragma pack(push, 1)
    typedef struct {
        double insz;
        double outsz;
    } PARAMS;
#pragma pack(pop)
    
    if (paramsw < sizeof(PARAMS) / sizeof(double)) return MLSIZE_MISMATCH;
    PARAMS *param = (PARAMS *) params;
    if (paramsw != (param->insz + 1)* param->outsz + sizeof(PARAMS) / sizeof(double)) return MLSIZE_MISMATCH;
    
    if (inw * inh != param->insz) return MLINVALID_ARG;
    
    double *weights = (double *) params + sizeof(PARAMS);
    int weightn = param->insz * param->outsz;
    double *bias = (double *) weights + weightn;
    int biasn = param->outsz;
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = param->outsz;
    o->height = 1;
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    for (int i = 0; i < param->outsz; i++) {
        double sum = bias[i];
        
        for (int j = 0; j < param->insz; j++)
            sum += in[j] * weights[(int) (j + i * param->outsz)];
        
        o->data[i] = sum;
    }
    
    *out = o;
    
    return MLNO_ERR;
}

int zcenter(double *params, int paramsw, int unused0,
            double *in, int inw, int inh, Mat **out) {
    // TODO: Could be done using matsub.
    /*
Parameters order:
insz times
- mean
*/
    
    int insz = inw * inh;
    if (paramsw != insz) return MLINVALID_ARG;
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = inw;
    o->height = inh;
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    for (int i = 0; i < insz; i++) o->data[i] = in[i] - params[i];
    
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
    
#pragma pack(push, 1)
    typedef struct {
        double mean;
        double variance;
        double offset;
        double scale;
    } PARAMS;
#pragma pack(pop)
    
    int insz = inw * inh;
    if (paramsw != insz * sizeof(PARAMS) / sizeof(double)) return MLSIZE_MISMATCH;
    
    PARAMS *param = (PARAMS*) params;
    
    Mat *o = (Mat *) malloc(sizeof(Mat));
    o->width = inw;
    o->height = inh;
    o->data = (double *) malloc(sizeof(double) * o->width * o->height);
    
    // https://www.mathworks.com/help/deeplearning/ref/nnet.cnn.layer.batchnormalizationlayer.html
    for (int i = 0; i < insz; i++)
        o->data[i] = param[i].scale * (in[i] - param[i].mean) / sqrt(param[i].variance + EPSIL) + param[i].offset;
    
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

// Pass an input through a machine
/*
machine - machine to run the input on
input - input matrix
oputput - resulting output (memory allocated automaticly)
returns 0 on success
*/
int forwardpass(Layer machine, Mat input, Mat **output) {
    Layer *p = &machine;
    Mat *datai = &input;
    Mat *prevout = NULL;
    
    while (p != NULL) {
        // TODO: Does size checking need to happen before the layer is called? why not in the
        // layer implementation itself?
        
        // If there needs to be size checking
        if(p->inw != -1 && p->inh != -1)
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

// Adds a layer to a machine
/*
machine - machine to add the layer to
layer - layer to be added
*/
void addlayer(Layer **machine, Layer **newl) {
    Layer *p = *machine;
    
    while (p->next != NULL)
        p = p->next;
    
    (*newl)->prev = p;
    p->next = *newl;
}