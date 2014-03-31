#include "circularbuffer.h"
#include "fpint.h"

void circularbuffer_save(fpint* samples, int size, fpint sample, int* nextpos, int* saved) {
	samples[*nextpos] = sample;

	// increment saved variable but stop at maximum size of circular buffer
	if(++*saved > size)
		*saved = size;

	// increment nextpos and wrap at size of circular buffer
	*nextpos = ++*nextpos % size;
}
