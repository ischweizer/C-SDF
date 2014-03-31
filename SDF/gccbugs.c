/**
 * DO NOT REMOVE THIS MACRO !!!!
 * MACRO DISABLES GCC "OPTIMIZATION" WHICH ARE INCORRECT, READ GCCBUGS.H
 */
#pragma GCC optimize ("O0")

#include "gccbugs.h"

unsigned long long gccbugs_ulldiv(unsigned long long a, unsigned long long b) {
	return a / b;
}

long long gccbugs_lldiv(long long a, long long b) {
	return a / b;
}

unsigned long long gccbugs_ullmul(unsigned long long a, unsigned long long b) {
	return a * b;
}

long long gccbugs_llmul(long long a, long long b) {
	return a * b;
}
