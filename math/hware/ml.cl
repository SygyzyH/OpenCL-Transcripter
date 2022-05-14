#ifndef _OCLAPI_INCLUDE_SOURCE
#define _OCLAPI_INCLUDE_SOURCE(source) source
#endif
_OCLAPI_INCLUDE_SOURCE(
void sumArray(__local double **tmp, int bsize, int li);

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define CLAMP(BOT, A, TOP) (MIN(MAX(BOT, A), TOP))

__kernel void conv2d(__global double *in, int inw, int inh, int stridew, int strideh,
                     int pleft, int pright, int ptop, int pbot, __local double *sum, 
                     __global double *bias, __global double *weights, __global double *res) {
    int channels = get_local_size(0);
    int filterw = get_local_size(1);
    int filterh = get_local_size(2);
    
    int c = get_local_id(0);
    int lfilterw = get_local_id(1);
    int lfilterh = get_local_id(2);
    
    int filtern = (int) get_global_size(0) / channels;
    int resh = get_global_size(2) * filtern;
    int resw = get_global_size(1);
    
    int n = (int) (get_global_id(0) / channels);
    int i = get_global_id(2);
    int j = get_global_id(1);
    
    resw /= filterw;
    resh /= filterh;
    
    i = (int) i / filterh;
    j = (int) j / filterw;
    
    int einw = inw;
    int einh = inh / channels;
    
    int ecinhs = c * einh;
    int ecinhe = (c + 1) * einh;
    
    int startw = j * stridew - pleft;
    int starth = i * strideh - ptop + ecinhs;
    
    int l = startw + lfilterw;
    int k = starth + lfilterh;
    
    int weindex = (l - startw) + (k - starth) * filterw + 
        c * filterw * filterh +
        n * filterw * filterh * channels;
    if (l >= 0 && l < einw && k >= ecinhs && k < ecinhe)
        sum[lfilterw + lfilterh * filterw + c * filterw * filterh] = in[l + k * einw] * weights[weindex];
    else
        sum[lfilterw + lfilterh * filterw + c * filterw * filterh] = 0;
    
    int fsize = filterw * filterh * channels;
    sumArray(&sum, fsize, lfilterw + lfilterh * filterw + c * filterw * filterh);
    
    int eoc = (int) n * resw * resh / filtern;
    if (lfilterw + lfilterh == 0)
        res[j + i * resw + eoc] = sum[0] + bias[n];
    
}

__kernel void maxpool(__global double *in, int inw, int inh, int stridew, int strideh,
                      int pleft, int pright, int ptop, int pbot, __local double *max, __global double *res) {
    int resw = get_global_size(0);
    int channels = get_global_size(2);
    int resh = get_global_size(1) * channels;
    
    int i = get_global_id(1);
    int j = get_global_id(0);
    int c = get_global_id(2);
    
    int poolw = get_local_size(0);
    int poolh = get_local_size(1);
    
    resw /= poolw;
    resh /= poolh;
    
    i /= poolh;
    j /= poolw;
    
    int einw = inw;
    int einh = inh / channels;
    
    int ecinhs = c * einh;
    int ecinhe = (c + 1) * einh;
    
    int startw = j * stridew - pleft;
    int starth = i * strideh - ptop + ecinhs;
    
    int lpoolw = get_local_id(0);
    int lpoolh = get_local_id(1);
    
    int l = startw + lpoolw;
    int k = starth + lpoolh;
    
    int inindex = CLAMP(0, l, einw - 1) + CLAMP(ecinhs, k, ecinhe - 1) * einw;
    max[lpoolw + lpoolh * poolw] = in[inindex];
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    int psize = poolw * poolh;
    int halfpsize = (int) psize / 2;
    while (halfpsize > 0) {
        if (lpoolw + lpoolh * poolw < halfpsize) {
            int index = lpoolw + lpoolh * poolw;
            max[index] = MAX(max[index], max[index + halfpsize]);
            
            if (psize % 2 == 1 && lpoolw + lpoolh == 0)
                max[index] = MAX(max[index], max[index + psize - 1]);
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
        psize = halfpsize;
        halfpsize = (int) psize / 2;
    }
    
    int eoc = (int) c * resw * resh / channels;
    if (lpoolw + lpoolh == 0)
        res[j + i * resw + eoc] = max[0];
}

__kernel void bnorm(__global double *in, int einsz, int channels, __global double *params,
                    __global double *res) {
    
    typedef __attribute__((packed)) struct {
        double mean;
        double variance;
        double offset;
        double scale;
    } PARAMS;
    
    PARAMS *param = (PARAMS *) (params + 1);
    
    int i = get_global_id(0);
    int c = get_global_id(1);
    
    res[i + (int) (c * einsz)] =
        param[c].scale * (in[(int) i + c * einsz] - param[c].mean) / 
        sqrt(param[c].variance + 1e-6) + param[c].offset;
}

__kernel void softmax(__global double *in, __local double *temp, __global double *res) {
    int gi = get_global_id(0);
    
    // Copy array part to local memory
    temp[gi] = exp(in[gi]);
    
    // Sum exp(array)
    int bsize = get_local_size(0);
    sumArray(&temp, bsize, gi);
    
    // Devide by sum
    res[gi] = exp(in[gi]) / temp[0];
}

__kernel void relu(__global double *in, __global double *res) {
    int i = get_global_id(0);
    
    res[i] = (in[i] > 0)? in[i] : 0;
}

void sumArray(__local double **tmp, int bsize, int li) {
    // Ensure input is ready before function runs
    barrier(CLK_LOCAL_MEM_FENCE);
    __local double *temp = *tmp;
    
    int halfbsize = (int) bsize / 2;
    while (halfbsize > 0) {
        if (li < halfbsize) {
            temp[li] += temp[li + halfbsize];
            if (bsize % 2 == 1 && li == 0)
                temp[li] += temp[li + bsize - 1];
        }
        
        // Proceed to next half
        barrier(CLK_LOCAL_MEM_FENCE);
        bsize = halfbsize;
        halfbsize = (int) bsize / 2;
    }
    
    // Ensure output is ready once the function returns
    barrier(CLK_LOCAL_MEM_FENCE);
}
)