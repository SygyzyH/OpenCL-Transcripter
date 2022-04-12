__kernel void convolve_same(__global double *in, int inw, int inh, int filtern, 
                            int pleft, int pright, int ptop, int pbot,
                            int filterw, int filterh, int filtern,
                            int stridew, int strideh,
                            int channels, int ow, int oh,
                            __global double *bias, __global double *weights,
                            __global double *out) {
    /*
Implementing

for (int i = -pleft; i < inw + pright; i += param->stridew) {
for (int j = -ptop; j < inh + pbot; j += param->strideh) {
                // Block entry
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
                                sum += in[k + l * param->filterw + c * inw * inh] *
                                weights[k + l * param->filterw + c * param->filterw * param->filterh];
                        }
                    }
                }
                
                o->data[ceil((double) (i + pleft) / param->stridew) + ceil((double) (j + ptop) * o->width / param->strideh) + filter * o->width * o->height] = sum;
}
}
*/
    
    int filter = get_global_id(0);
    int i = (get_global_id(1) - pleft) * stridew;
    int j = (get_global_id(2) - ptop) * strideh;
    
    double sum = bias[filter];
    for (int c = 0; c < channels; c++) {
        for (int k = i; k < filterw; k++) {
            for (int l = j; l < filterh; l++) {
                if (k + l * filterw > 0 && k + l * filterw < inw * inh)
                    sum += in[k + l * filterw + c * inw * inh] *
                    weights[k + l * filterw + c * filterw * filterh];
            }
        }
    }
    
    out[ceil((double) (i + pleft) / stridew) + ceil((double) (j + ptop) * ow / strideh) + filtern * ow * oh] = sum;
}