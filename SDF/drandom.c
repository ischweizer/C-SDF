#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"

#include "sdf-config.h"
#include "drandom.h"
#include "udphelper.h"

//whether dranom has been seeded
static int seeded = 0;

// seed and counter for every random operation
static unsigned short seed = 0, counter = 0;

static unsigned short seeding() {
	// generate seed for first time
	if(seeded == 0) {
		static uip_ipaddr_t ip;
		udphelper_address_local(&ip);

		// first part of seed: IPv6 address
		int i;
		for(i = 0; i < 8; i++)
			seed ^= ip.u16[i];

		// second part of seed: config
		seed ^= DRANDOM_SEED;

		seeded = 1;
	}

	return seed ^ ++counter;
}

/**
 * creates random number
 */
int drandom_rand() {
	// random generator has to be seeded for every call while contiki
	// may use random_rand() too and would therefore change meaning
	// when it would be used in an unreliable behaviour like for
	// example on radio transmit collisions for retrying
	random_init(seeding());
	return (int) random_rand();
}

int drandom_rand_minmax(int min, int max) {
	unsigned short rand = drandom_rand();

	return min + rand % (max - min + 1);
}
