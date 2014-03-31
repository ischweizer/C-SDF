#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdio.h>

#include "udphelper.h"

/**
 * disabled debugging
 */
#define DEBUG_OFF 0

/**
 * enabled debugging
 */
#define DEBUG_ON 1

#if DEBUG & DEBUG_ON
	#define debug(...) printf(__VA_ARGS__)
	#define debug_address(addr) udphelper_print_address(addr)
	#define debug_fpint(fp) fpint_str(fp, fpint_strbuf)
#else
	#define debug(...)
	#define debug_address(addr)
	#define debug_fpint(fp)
#endif

#endif /* DEBUG_H_ */
