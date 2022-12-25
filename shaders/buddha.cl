__constant sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;

__constant ulong pcg_shift = 6364136223846793005ULL;
__constant float pcg_max_1 = 4294967296.0;

#define PDF_SIDES 100
#define PDF_SIZE 201

#define UNIFORM_LOW 0.02425
#define UNIFORM_HIGH 0.97575

__constant float a[] = {
    -3.969683028665376e+01,
     2.209460984245205e+02,
    -2.759285104469687e+02,
     1.383577518672690e+02,
    -3.066479806614716e+01,
     2.506628277459239e+00
};

__constant float b[] = {
    -5.447609879822406e+01,
     1.615858368580409e+02,
    -1.556989798598866e+02,
     6.680131188771972e+01,
    -1.328068155288572e+01
};

__constant float c[] = {
    -7.784894002430293e-03,
    -3.223964580411365e-01,
    -2.400758277161838e+00,
    -2.549732539343734e+00,
     4.374664141464968e+00,
     2.938163982698783e+00
};

__constant float d[] = {
    7.784695709041462e-03,
    3.224671290700398e-01,
    2.445134137142996e+00,
    3.754408661907416e+00
};

double inverseNormalCdf(double uniform) {
	double q, r;

	if (uniform <= 0) {
		return -HUGE_VAL;
	}
	else if (uniform < UNIFORM_LOW) {
		q = sqrt(-2 * log(uniform));
		return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
			((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
	}
	else if (uniform <= UNIFORM_HIGH) {
        q = uniform - 0.5;
        r = q * q;
        
        return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
            (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1);
	}
	else if (uniform < 1) {
		q  = sqrt(-2 * log(1 - uniform));

		return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
			((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
	}
	else {
		return HUGE_VAL;
	}
}

inline ulong pcg32_random_r(global ulong *state, global ulong *inc, int x) {
    ulong oldstate = state[x];
    state[x] = oldstate * pcg_shift + inc[x];
    uint xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint rot = oldstate >> 59u;
    uint pcg = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));

    return pcg;
}

__kernel void seed_noise(global ulong *state, global ulong *inc, global ulong *init_state, global ulong *init_seq) {
    const int x = get_global_id(0);

    state[x] = 0U;
    inc[x] = (init_seq[x] << 1u) | 1u;
    pcg32_random_r(state, inc, x);
    state[x] += init_state[x];
    pcg32_random_r(state, inc, x);
}

__kernel void fill_noise(global float *q_list, global ulong *state, global ulong *inc) {
    const int x = get_global_id(0);

    uint pcg = pcg32_random_r(state, inc, x);

    q_list[x] = (float)pcg / pcg_max_1;
} 
