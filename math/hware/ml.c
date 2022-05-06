__kernel void bnorm(__global double *in, int insz, int channels, __global double *params,
                    __global double *res) {
    
    typedef __attribute__((packed)) struct {
        double mean;
        double variance;
        double offset;
        double scale;
    } PARAMS;
    
    PARAMS *param = (PARAMS *) params;
    
    int i = get_global_id(0);
    int j = get_global_id(1);
    int c = get_global_id(2);
    
    res[(int) i + c * insz / channels] = 
        param[c].scale * (in[i + c * insz / channels] - param[c].mean) / 
        sqrt(param[c].variance + 1e-6) + param[c].offset;
}

__kernel void relu(__global double *in, __global double *res) {
    int i = get_global_id(0);
    
    res[i] = (in[i] > 0)? in[i] : 0;
}
