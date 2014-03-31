#ifndef CONSUMPTIONRATE_H_
#define CONSUMPTIONRATE_H_

#include "sdf-config.h"
#include "fpint.h"

/**
 * init consumptionrate functionality
 */
void consumptionrate_init();

/**
 * takes an energy neutral consumption rate sample
 */
void consumptionrate_sample();

/**
 * energy neutral consumption energy for a given timeframe in seconds
 */
fpint consumptionrate_energy(long timeframe);

#endif /* CONSUMPTIONRATE_H_ */
