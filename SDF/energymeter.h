#ifndef __ENERGYMETER_H__
#define __ENERGYMETER_H__

#include "fpint.h"

/**
 * energy drain information
 */
#define ENERGYMETER_DRAIN_SECONDS_CPU_ACTIVE     0x021L // 01.8000mAh = 0.0005mAs
#define ENERGYMETER_DRAIN_SECONDS_RADIO_TRANSMIT 0x16CL // 20.0000mAh = 0.0056mAs
#define ENERGYMETER_DRAIN_SECONDS_RADIO_LISTEN   0x13BL // 17.4000mAh = 0.0048mAs
#define ENERGYMETER_DRAIN_SECONDS_SENSOR_CO      0x034L // 03.0000mAh = 0.0008mAs
#define ENERGYMETER_DRAIN_SECONDS_SENSOR_CO2     0x388L // 50.0000mAh = 0.0138mAs
#define ENERGYMETER_DRAIN_SECONDS_SENSOR_GPS     0x175L // 20.5000mAh = 0.0057mAs
#define ENERGYMETER_DRAIN_HOURS_CPU_SLEEP        0xDF4L // 00.0545mAh

/**
 * flags for drain calculation
 */
#define ENERGYMETER_DRAIN_WITH_INCOMPLETE_TIMEFRAMES 0
#define ENERGYMETER_DRAIN_ONLY_FULL_TIMEFRAMES       1

/**
 * number of energymeter ticks forming an second
 */
#define ENERGYMETER_TICKS_PER_SECOND (unsigned long long) RTIMER_SECOND

/**
 * datastructure of an energymeter sample
 */
typedef struct {
    unsigned long long cpu_active;
    unsigned long long cpu_sleep;
    unsigned long long radio_transmit;
    unsigned long long radio_listen;
    unsigned long long sensor_co;
    unsigned long long sensor_co2;
    unsigned long long sensor_gps;
} energymeter_sample;

/**
 * takes an energymeter sample
 */
void energymeter_sampling(energymeter_sample* fill);

/**
 * calculates the energy drain between two energymeter samples
 */
fpint energymeter_calculate_drain(const energymeter_sample* last, const energymeter_sample* now, int timeframe_option);

/**
 * calculates the energy drain between two energymeter samples
 *
 * function does update the "last" sample by adding all energymeter ticks from "now" sample
 * which have been used for drain calculation
 *
 * This method is usefull whenever you need highly accurate long time drain calculation, while
 * it does not loose any precision: Whenever a drainage is to small to be calculated the drain will
 * be ignored until it's safe to calculate. Calling this method of a long term and feeding exactly
 * the same "last" sample every time will return the most accurant long term drain. But drainage
 * between two samples may not be the most accurate whenever the threshold of an small drain is
 * reached and can be calculated
 */
fpint energymeter_calculate_drain_update_last(energymeter_sample* last, const energymeter_sample* now, int timeframe_option);

/**
 * calculates drain of a source in second resolution
 *
 * returns number of ticks used in calculation
 */
unsigned long long energymeter_drain_seconds(const unsigned long long* last, const unsigned long long* now, fpint fp_energy, fpint* fp_drain, int timeframe_option);

/**
 * calculates drain of a source in hour resolution
 *
 * returns number of ticks used in calculation
 */
unsigned long long energymeter_drain_hours(const unsigned long long* last, const unsigned long long* now, fpint fp_energy, fpint* fp_drain, int timeframe_option);

#endif /* __ENERGYMETER_H__ */
