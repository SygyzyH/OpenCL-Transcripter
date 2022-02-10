__kernel void matmul(__global double *a, __global double *b, unsigned int w, unsigned int h, unsigned int k, __global double *res) {
	int gw = get_global_id(0);
	int gh = get_global_id(1);

	double acc = 0;
	for (int i = 0; i < k; i++) {
		acc += a[i + gh * h] * b[gw + i * w];
	}

	res[gw + gh * w] = acc;
}

__kernel void matadd(__global double *a, __global double *b, unsigned int w, unsigned int h, __global double *res) {
	int gw = get_global_id(0);
	int gh = get_global_id(1);

	res[gw + gh * w] = a[gw + gh * w] + b[gw + gh * w];
}

__kernel void matsub(__global double *a, __global double *b, unsigned int w, unsigned int h, __global double *res) {
	int gw = get_global_id(0);
	int gh = get_global_id(1);

	res[gw + gh * w] = a[gw + gh * w] + b[gw + gh * w];
}

__kernel void matmuls(__global double *a, double s, unsigned int w, unsigned int h, __global double *res) {
	int gw = get_global_id(0);
	int gh = get_global_id(1);

	res[gw + gh * w] = a[gw + gh * w] * s;
}

__kernel void matadds(__global double *a, double s, unsigned int w, unsigned int h, __global double *res) {
	int gw = get_global_id(0);
	int gh = get_global_id(1);

	res[gw + gh * w] = a[gw + gh * w] + s;
}