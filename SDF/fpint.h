#ifndef __FPINT_H__
#define __FPINT_H__

#include <stdint.h>

/**
 * Q15.16 floating point integers
 */
typedef long fpint;

/**
 * fpint constants
 */
#define FPINT_MIN      0x80000001L
#define FPINT_MAX      0x7FFFFFFFL
#define FPINT_ZERO     0x00000000L
#define FPINT_ONE      0x00010000L
#define FPINT_PI       0x0003243FL
#define FPINT_MINUSONE 0xFFFF0000L

/**
 * buffer for converting fpint values to strings
 *
 * free to use for every component! but be aware that it's
 * an single buffer which can only hold one value at a time
 * other functions may overwrite the content.
 */
char fpint_strbuf[24];

/**
 * converting a long to fpint
 */
#define fpint_to(a) (((long) (a)) << 16)

/**
 * converting fpint to long
 */
#define fpint_from(a) ((a) >> 16)

/**
 * converts an fpint to a string
 *
 * a 24 byte buffer is needed (23 chars + NULL byte)
 */
char* fpint_str(fpint a, char* buf);

/**
 * add one fpint to another
 */
#define fpint_add(a, b) ((a) + (b))

/**
 * subtract one fpint from another
 */
#define fpint_sub(a, b) ((a) - (b))

/**
 * multiply one fpint with another
 */
fpint fpint_mul(fpint a, fpint b);

/**
 * multiply one fpint multiple times
 */
fpint fpint_multimes(fpint a, unsigned long times);

/**
 * divide one fpint from another
 */
fpint fpint_div(fpint a, fpint b);

/**
 * floor an fpint
 */
#define fpint_floor(a) ((a) & 0xFFFF0000)

/**
 * ceil an fpint
 */
fpint fpint_ceil(fpint a);

/**
 * round an fpint
 */
fpint fpint_round(fpint a);

/**
 * returns greater fpint value
 */
fpint fpint_max(fpint a, fpint b);

/**
 * returns smaller fpint value
 */
fpint fpint_min(fpint a, fpint b);

/**
 * makes fpint value positive
 */
fpint fpint_abs(fpint a);

/**
 * sine of fpint
 */
fpint fpint_sin(fpint x);

/**
 * cosine of fpint
 */
fpint fpint_cos(fpint x);

/**
 * average of a set of fpints
 */
fpint fpint_avg(const fpint* set, int count);

/**
 * quantile of a set of fpints
 */
fpint fpint_quantile(const fpint* set, int count, fpint q);

/**
 * calculates square root of a number
 */
fpint fpint_sqrt(fpint a);

/**
 * calculates square root of a number
 */
fpint fpint_sqrt_epsilon(fpint a, fpint epsilon);

/**
 * compares two fpint values
 *
 * returns:
 *  0 as a is equal to b
 *  1 as a is greater than b
 * -1 as a is less than ab
 */
int fpint_compare(fpint a, fpint b);

/**
 * compares two fpint values
 *
 * returns:
 *  0 as a is equal to b
 *  1 as a is greater than b
 * -1 as a is less than ab
 */
int fpint_compare_epsilon(fpint a, fpint b, fpint epsilon);

#endif /* __FPINT_H__ */
