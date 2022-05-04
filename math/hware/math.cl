inline double hamming(int x, int windowsize, double alpha) {
	/* Hamming window (includes Hanning) for windowing */
	if (windowsize == 1) return 1;
	else return alpha - (1 - alpha) * cos(2 * M_PI * x / windowsize);
}

__kernel void stft(__global double *src, int framesize, int windowsize, int hopsize,
int fbins, int sides, __global double *res) {
	
	int frame = get_global_id(0);
	int freq = get_global_id(1);

	// ceil(fbins / 2 + 1)
	// 	   sum
	//  -ceil(fbins / 2)
	if (freq > ceil((double) fbins / 2 + 1)) freq = freq - fbins;

	double sumr = 0;
	double sumi = 0;

	for (int n = 0; n < framesize; n++) {
		double r = src[n + frame * hopsize] * hamming(n, windowsize, 0.54);
        double phi = -2 * M_PI * n * freq / fbins;
		sumr += r * cos(phi);
		sumi += r * sin(phi);
	}

	if (sides <= 2)
		res[freq + frame * fbins] = sumr * sumr + sumi * sumi;
	else {
		int offset = (int) freq + fbins - ceil((double) fbins / 2 + 1) + 1;
		res[offset % fbins + frame * fbins] = sumr * sumr + sumi * sumi;
	}
}