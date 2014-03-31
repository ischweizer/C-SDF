#ifndef TIME_H_
#define TIME_H_

/**
 * time in seconds since start of mote
 */
unsigned long time();

/**
 * actual day of year
 */
unsigned int time_day();

/**
 * actual hour of day
 */
unsigned int time_hour();

/**
 * actual minute of day
 */
unsigned int time_minute();

/**
 * actual seconds of day
 */
unsigned int time_seconds();

#endif /* TIME_H_ */
