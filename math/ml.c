/*
Layers needed to be implemented (in ascending difficulty):

MaxPooling
BatchNorm
Convelution


Implemented:

Dropout
ReLu
Softmax
*/

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "math.h"
#include "ml.h"

int maxpool(double *params, int paramsw, int unused0,
            double *in, int inw, int inh, Mat **out) {
    if (paramsw != 4) return MLINVALID_ARG;
    
    int poolw = params[0];
    int poolh = params[1];
    int stridew = params[2];
    int strideh = parmas[3];
}

void addlayer(Layer **machine, Layer **newl) {
    Layer *p = *machine;
    
    while (p != NULL)
        p = p->next;
    
    *newl->prev = p;
    p->next = *newl;
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

int forwardpass(Layer machine, Mat input, Mat **output) {
    Layer *p = machine.next;
    *output = &input;
    Mat *data = *output;
    
    while (p != NULL) {
        // Refrance ;)
        Mat *old_data = data;
        
        if (p.inw != data->width || p.inhg != data->height) return MLSIZE_MISMATCH;
        if((*p.transform)(unpckmat(p.params), unpkmatp(data), &data)) return MLLAYER_ERR;
        
        free(old_data->data);
        free(old_data);
        
        p = p.next;
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