inline double win(int x, int windowsize) {
    /* Hann window function to be used as windowing function for stft */
    return pow(sin(M_PI * x / windowsize), 2);
}

__kernel void stft(__global double *src, int framesize, int windowsize, int hopsize, __global double *res) {
	int fbins = (int) framesize / 2 + 1;

	int frame = get_global_id(0);
	int freqbi = get_global_id(1);

	int freq = (freqbi < (int) ceil((double) framesize / 2))? freqbi : freqbi - framesize;

	double sumr = 0;
	double sumi = 0;
	
	for (int n = 0; n < framesize; n++) {
		double r = src[n + frame * hopsize] * win(n, windowsize);
        double phi = -2 * M_PI * n * freq / framesize;
		sumr += r * cos(phi);
		sumi += r * sin(phi);
	}

	if (freqbi < fbins)
		res[freqbi + frame * fbins] = sumr * sumr + sumi * sumi;
}