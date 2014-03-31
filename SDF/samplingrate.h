#ifndef SAMPLINGRATE_H_
#define SAMPLINGRATE_H_

/**
 * calculates the sampling rate for an interval
 */
int samplingrate_calculate(int max_messages);

/**
 * takes an sample of the energy drains needed for calculating sampling rate
 */
void samplingrate_sample_energy_drain(int last_samplingrate, int last_childcount);

#endif /* SAMPLINGRATE_H_ */
