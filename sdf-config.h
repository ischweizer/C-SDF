#ifndef SDFCONFIG_H_
#define SDFCONFIG_H_

/**
 * multiplier for timing of software: runs at factor normal multiplied by x
 *
 * runs at mltiple speed but should behave as running in realtime:
 *   - only 1/x energy  and 1/x samplingrate because realtime(msg/s) = speedtime(msg/s) * x
 */
#define SPEEDMULTIPLIER 8

/**
 * time for RPL initialization (60s seems to be optimal value)
 */
#define RPLINITTIME 60

/**
 * whether server should print received sensor samples
 */
#define PRINTSAMPLES 1

/**
 * emulate battery
 */
#define BATTERY_EMULATE 1

/**
 * maximum capacity of battery
 */
#define BATTERY_MAXCAPACITY 1000

/**
 * maximum capacity of battery
 */
#define BATTERY_INITIALCAPACITY 70

/**
 * number of samples saved of energy neutral consumption rate
 */
#define CONSUMPTIONRATE_SAMPLES 14

/**
 * whether solar harvested energy should be predicted by battery difference
 * or polling of solarpanel
 */
#define CONSUMPTIONRATE_SOLARENERGY_BATTERYPREDICTION 0

/**
 * seed for deterministic random library
 */
#define DRANDOM_SEED 12345

/**
 * port for SDF udp communication
 */
#define SDF_PORT 5678

/**
 * initialization phase of SDF with minimum sampling
 */
#define SDF_INITIALIZATIONPHASE 86400

/**
 * minimum of samples sent during each SDF interval
 */
#define SDF_SAMPLINGRATE_MINIMAL 5

/**
 * interval for updating SDF samplingrate
 */
#define SDF_SAMPLINGRATE_UPDATEINTERVAL 600

/**
 * number of drain samples to keep for calculation of Ptx, Prx and Psense
 */
#define SDF_SAMPLINGRATE_ENERGYSAMPLES 10

/**
 * emulate solarpanel
 */
#define SOLARPANEL_EMULATE 1

/**
 * simple solar radiation formula
 *
 * (same approximated radiation every day, for ROM limited devices)
 */
#define SOLARPANEL_SIMPLECALCULATION 1

/**
 * voltage of solarpanel
 */
#define SOLARPANEL_VOLT 12

/**
 * efficiency of solarpanel
 */
#define SOLARPANEL_EFFICIENCY 4

/**
 * size of solar panel (in cm^2)
 */
#define SOLARPANEL_SIZE 100

/**
 * percent value of range the energy will fluctuate
 */
#define SOLARPANEL_NOISE 20

/**
 * day of year (start of mote)
 */
#define TIME_DAY 127

/**
 * minute of day (start of mote)
 */
#define TIME_MINUTE 0

#endif /* SDFCONFIG_H_ */
