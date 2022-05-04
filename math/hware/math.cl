inline double win(int x, int windowsize) {    
	/* Hann window function to be used as windowing function for stft */

	// Special case for when W = 1, so as to not zero the only point,
	// return 1.
	if (windowsize == 1) return 1;
    //else return pow(sin(M_PI * x / windowsize), 2);
	// Hamming window
	else return 0.54 - 0.46 * cos(2 * M_PI * x / windowsize);
}

__kernel void stft(__global double *src, int framesize, int windowsize, int hopsize,
int fbins, int sides, __global double *res) {
	
	int frame = get_global_id(0);
	int freq = get_global_id(1);

	if (freq > ceil((double) fbins / 2 + 1)) freq = freq - fbins;

	double sumr = 0;
	double sumi = 0;

	for (int n = 0; n < framesize; n++) {
		double r = src[n + frame * hopsize] * win(n, windowsize);
        double phi = -2 * M_PI * n * freq / fbins;
		sumr += r * cos(phi);
		sumi += r * sin(phi);
	}

	if (sides <= 2)
		res[freq + frame * fbins] = sumr * sumr + sumi * sumi;
	else
		res[(int) (freq + fbins - ceil((double) fbins / 2 + 1) + 1) % fbins + frame * fbins] = sumr * sumr + sumi * sumi;
}