inline double win(int x, int windowsize) {
    /* Hann window function to be used as windowing function for stft */
    return pow(sin(M_PI * x / windowsize), 2);
}

__kernel void stft(__global double *src, int framesize, int windowsize, int hopsize, __global double *res, int op) {