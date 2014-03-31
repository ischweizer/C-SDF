#ifndef GCCBUGS_H_
#define GCCBUGS_H_

/**
 * These functions only exist to have workarounds for gcc bugs at a common place.
 * You could add "#pragma GCC optimize ("O0")" to every file but this would disable
 * optimizations for all code in the source file. With gccbugs functions only the real
 * operation is not optimized
 *
 * gccbugs_ulldiv() and gccbugs_lldiv() are fixing incorrect division results
 * gccbugs_ullmul() and gccbugs_llmul() preventing gcc compiler from inlining small
 *   function bodies using 64 bit multiplication (e.g. fpint_mul)
 */

/**
 * divides two unsigned long long
 */
unsigned long long gccbugs_ulldiv(unsigned long long a, unsigned long long b);

/**
 * divides two long long
 */
long long gccbugs_lldiv(long long a, long long b);

/**
 * multiplies two unsigned long long
 */
unsigned long long gccbugs_ullmul(unsigned long long a, unsigned long long b);

/**
 * multiplies two long long
 */
long long gccbugs_llmul(long long a, long long b);

#endif /* GCCBUGS_H_ */
