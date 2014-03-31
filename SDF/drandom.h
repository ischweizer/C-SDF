#ifndef DRANDOM_H_
#define DRANDOM_H_

/**
 * maximum random value
 */
#define DRANDOM_RAND_MAX RANDOM_RAND_MAX

/**
 * creates a deterministic random number between 0 and DRANDOM_RAND_MAX
 */
int drandom_rand();

/**
 * creates a deterministic random number within in range of min and max
 */
int drandom_rand_minmax(int min, int max);

#endif /* DRANDOM_H_ */
