#include <stdio.h>

#include "fpint.h"
#include "gccbugs.h"

/**
 * algorithm does work by dividing decimal place scled by a factor of
 * ten in every step to fraction bit and adding value whenever bit is set.
 */
char* fpint_str(fpint a, char* buf) {
	short nextpos = 0;

	// handle negative fpint values by making them positive
	if(a < FPINT_ZERO) {
		buf[nextpos++] = '-';
		a = fpint_abs(a);
	}

	// save integer part to string
	nextpos += sprintf(buf + nextpos, "%ld", fpint_from(a));

	// add decimal point
	buf[nextpos++] = '.';

	// split fraction part
	fpint fraction = a & 0xFFFF;

	// save trailing zero (after decimal point) for fpint values without fraction
	if(fraction == 0)
		buf[nextpos++] = '0';

	while(fraction > 0) {
		// multiply fraction by ten to move one fraction digit to integer part and save it
		fraction = fpint_mul(fraction, 0xA0000);
		buf[nextpos++] = '0' + fpint_from(fpint_floor(fraction));

		// remove moved fraction digit for algorithm to continue
		fraction &= 0xFFFF;
	}

	// add string terminator
	buf[nextpos++] = '\0';

	return buf;
}

fpint fpint_mul(fpint a, fpint b) {
    return (fpint) (gccbugs_llmul(a, b) >> 16);
}

fpint fpint_multimes(fpint a, unsigned long times) {
	fpint result = 0;

	long remaining = times, iteration;
	while(remaining > 0) {
		iteration = (remaining > 32767) ? 32767 : remaining;
		remaining -= iteration;
		result = fpint_add(result, fpint_mul(a, fpint_to(iteration)));
	}

	return result;
}

fpint fpint_div(fpint a, fpint b) {
    int64_t scaled = ((long long) a) << 16;
    return (fpint) gccbugs_lldiv(scaled, b);
}

fpint fpint_ceil(fpint a) {
	fpint ceil = fpint_floor(a);
	if((a & 0xFFFF) > 0)
		ceil = fpint_add(ceil, FPINT_ONE);

	return ceil;
}

fpint fpint_round(fpint a) {
	if((a & 0xFFFF) >= 32768)
		return fpint_ceil(a);
	else
		return fpint_floor(a);
}

fpint fpint_max(fpint a, fpint b) {
    return (a > b) ? a : b;
}

fpint fpint_min(fpint a, fpint b) {
    return (a < b) ? a : b;
}

fpint fpint_abs(fpint a) {
	// do not use fpint_mul(a, FPINT_MINUSONE) as msp430-gcc
	// seems to inline function resulting in 200-600 byte
	// bigger firmware size
    return (a >= 0) ? a : fpint_sub(FPINT_ZERO, a);
}

// sin(x) ≈ 0,0375758*x^4 - 0,236096*x^3 + 0,0582877*x^2 + 0,0981968*x
fpint fpint_sin(fpint x) {
    int factor = 1;

    // mirror sine of x < 0 to x > 0
    if(x < 0) {
        x = fpint_abs(x);
        factor *= -1;
    }

    // move x to PI-Interval [0, PI]
    if(x > FPINT_PI) {
    	// space-efficient fixed integer calculations:
    	if((x / FPINT_PI) % 2 == 1)
    		factor *= -1;
    	x %= FPINT_PI;
    }

    /*
     * Sine:
     */
    fpint fp_xit = x;
    fpint fp_sum = 0;
    fpint a = 0x099FL, b = 0x3C71L, c = 0x0EECL, d = 0xFB62; // a*x^4 - b*x^3 + c*x^2 + d*x

    // Polynom 4
    fp_sum = fpint_mul(d, fp_xit);
    // Polynom 3
    fp_xit = fpint_mul(fp_xit, x);
    fp_sum = fpint_add(fp_sum, fpint_mul(c, fp_xit));
    // Polynom 2
    fp_xit = fpint_mul(fp_xit, x);
    fp_sum = fpint_sub(fp_sum, fpint_mul(b, fp_xit));
    // Polynom 1
    fp_xit = fpint_mul(fp_xit, x);
    fp_sum = fpint_add(fp_sum, fpint_mul(a, fp_xit));

    return fpint_mul(fp_sum, fpint_to(factor));
}

fpint fpint_cos(fpint x) {
    return fpint_sin(fpint_add(x, FPINT_PI >> 1));
}

fpint fpint_avg(const fpint* set, int count) {
	if(count == 0)
		return 0;

	fpint sum = fpint_to(0);
	int i;
	for(i = 0; i < count; i++)
		sum = fpint_add(sum, set[i]);

	return fpint_div(sum, fpint_to(count));
}

fpint fpint_quantile(const fpint* set, int count, fpint q) {
	if(count == 0)
		return 0;

	// calculate average
	fpint avg = fpint_avg(set, count);

	// calculate standard derivation
	fpint stdev = fpint_to(0);
	int i;
	for(i = 0; i < count; i++) {
		fpint diff = fpint_sub(set[i], avg);
		stdev = fpint_add(stdev, fpint_mul(diff, diff));
	}
	stdev = fpint_sqrt(fpint_div(stdev, fpint_to(count)));

	// return quantile
	return fpint_add(fpint_mul(q, stdev), avg);
}

fpint fpint_sqrt(fpint a) {
	fpint epsilon = 0x0021; // ~0.0005
	return fpint_sqrt_epsilon(a, epsilon);
}

/**
 * Newton's method
 * x_n = (x_n^2 - x) / (2 * x_n)
 */
fpint fpint_sqrt_epsilon(fpint a, fpint epsilon) {
	if(a < FPINT_ZERO) // no fpint_compare() while there's no epsilon wanted
		return FPINT_MINUSONE;

	// find more precise approximation while difference of x_n and x_n_old is greater than epsilon
	// (when difference is smaller or equals epsilon both fpint will be "equals")
	fpint x_n = a >> 2, x_n_old = a;
	while(fpint_compare_epsilon(x_n, x_n_old, epsilon) != 0) {
		x_n_old = x_n;

		// calculate new x_n:
		// it's not possible to calculate x_n^2 while for every x_n > 181 the square will overflow.
		// modified formula calculates correctly: x_n = x_n - (x_n - x / x_n) / 2
		fpint sub = fpint_sub(x_n, fpint_div(a, x_n)) >> 2; // fast division by 2
		x_n = fpint_sub(x_n, sub);
	}

	return x_n;
}

int fpint_compare(fpint a, fpint b) {
	fpint epsilon = 0x0042; // ~ 0.001
	return fpint_compare_epsilon(a, b, epsilon);
}

int fpint_compare_epsilon(fpint a, fpint b, fpint epsilon) {
	fpint diff = fpint_sub(a, b);

	// check for equality
	if(fpint_abs(diff) <= epsilon)
		return 0;

	return (diff > 0) ? 1 : -1;
}
