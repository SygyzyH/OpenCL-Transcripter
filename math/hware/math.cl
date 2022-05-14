#ifndef _OCLAPI_INCLUDE_SOURCE
#define _OCLAPI_INCLUDE_SOURCE(source) source
#endif
_OCLAPI_INCLUDE_SOURCE(
inline double hamming(int x, int windowsize, double alpha) {
	/* Hamming window (includes Hanning) for windowing */
	if (windowsize == 1) return 1;
	else return alpha - (1 - alpha) * cos(2 * M_PI * x / windowsize);
}

__kernel void stft(__global double *src, int framesize, int windowsize, int hopsize,
int fbins, int frames, int sides, __global double *res) {
	
	int frame = get_global_id(0);
	int freq = get_global_id(1);

	// ceil(fbins / 2 + 1)
	// 	   sum
	//  -ceil(fbins / 2)
	int freqn = freq;
	if (freq > ceil((double) fbins / 2 + 1)) freqn = freq - fbins;

	double sumr = 0;
	double sumi = 0;

	for (int n = 0; n < framesize; n++) {
		double r = src[n + frame * hopsize] * hamming(n, windowsize, 0.54);
        double phi = -2 * M_PI * n * freqn / fbins;
		sumr += r * cos(phi);
		sumi += r * sin(phi);
	}

	if (sides <= 2)
		res[frame + freq * frames] = sumr * sumr + sumi * sumi;
	else {
		int offset = (int) freq + ceil((double) fbins / 2 + 1) + 1;
		res[frame + (offset % fbins) * frames] = sumr * sumr + sumi * sumi;
	}
}
)