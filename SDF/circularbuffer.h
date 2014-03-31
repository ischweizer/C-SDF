#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_

#include "fpint.h"

void circularbuffer_save(fpint* samples, int size, fpint sample, int* nextpos, int* saved);

#endif /* CIRCULARBUFFER_H_ */
