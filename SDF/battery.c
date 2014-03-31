#include "contiki.h"
#include "sdf-config.h"

#include "time.h"
#include "fpint.h"
#include "battery.h"
#include "solarpanel.h"
#include "energymeter.h"

#define DEBUG DEBUG_OFF
#include "debug.h"

/**
 * battery capacity
 */
static fpint fp_capacity;

/**
 * drain of battery for a complete days
 */
static fpint fp_drain_day = 0;

/**
 * information when battery was last updated for specific energy drain sources
 */
static energymeter_sample lastsample = {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL};

/**
 * process for periodically battery updates
 */
PROCESS(battery_process, "Battery-Process");

/**
 * process for periodically energy harvesting with solar panel
 */
PROCESS(solarpanel_process, "Solarpanel-Process");

/**
 * helper function for updating the battery
 */
static fpint update_battery() {
    // get sample
    static energymeter_sample sample;
    energymeter_sampling(&sample);

    // calculate drain
    fpint fp_drain = energymeter_calculate_drain_update_last(&lastsample, &sample, ENERGYMETER_DRAIN_ONLY_FULL_TIMEFRAMES);
    fp_drain_day   = fpint_add(fp_drain_day, fp_drain);

    // update battery
    return fp_capacity = fpint_max(fpint_to(0), fpint_sub(fp_capacity, fp_drain));
}

void battery_init() {
    #if BATTERY_EMULATE
        process_start(&battery_process, NULL);
    #endif
	#if SOLARPANEL_EMULATE
		process_start(&solarpanel_process, NULL);
	#endif
}

int battery_level() {
	fpint fp_percent = fpint_div(fpint_mul(battery_capacity(), fpint_to(100)), battery_maxcapacity());
    return fpint_from(fp_percent);
}

fpint battery_capacity() {
    #if BATTERY_EMULATE
        return update_battery();
    #else
        // read comment at implementation of energymeter_sampling() !!!
        #error no real battery implemented
    #endif
}

fpint battery_maxcapacity() {
    #if BATTERY_EMULATE
        return fpint_to(BATTERY_MAXCAPACITY);
    #else
        #error no real battery implemented
    #endif
}

/**
 * updates periodically the battery capacity
 */
PROCESS_THREAD(battery_process, ev, data) {
	PROCESS_BEGIN();

	// set battery to initial capacity
	fpint fp_load = fpint_div(fpint_to(BATTERY_INITIALCAPACITY), fpint_to(100));
	fp_capacity = fpint_mul(battery_maxcapacity(), fp_load);

	// timer for updating the battery periodically
	static struct etimer timer_update_battery;
	etimer_set(&timer_update_battery, CLOCK_SECOND * 300 / SPEEDMULTIPLIER); // every 5 minutes

	// last day of battery calculation
	static int last_day = TIME_DAY;

	// update loop
	while(1) {
		PROCESS_WAIT_UNTIL(etimer_expired(&timer_update_battery));

		// update battery
		update_battery();

		// calculate debug day drainage
		if(last_day != time_day()) {
			debug("[BATTERY] %smAh drain\n", debug_fpint(fp_drain_day));
			fp_drain_day = 0;
			last_day = time_day();
		}

		// restart timer
		etimer_reset(&timer_update_battery);
	}

	PROCESS_END();
}

/**
 *
 *
 * Solarpanel implementation
 *
 *
 */

/**
 * updates periodically the battery capacity with enegergy harvested by solar panel
 */
PROCESS_THREAD(solarpanel_process, ev, data) {
	PROCESS_BEGIN();

	// timer for updating  solar calculation periodically
	static struct etimer timer_update_solar;
	etimer_set(&timer_update_solar, CLOCK_SECOND * 60 / SPEEDMULTIPLIER);

	// cummulated solar energy of complete day
	static fpint fp_solarenergy_day = 0;

	// last day of battery calculation
	static int last_day = TIME_DAY;

	// update loop
	while(1) {
		PROCESS_WAIT_UNTIL(etimer_expired(&timer_update_solar));

		// a new day begins...
		if(last_day != time_day()) {
			debug("[SOLARPANEL] harvested %smAh\n", debug_fpint(fp_solarenergy_day));
			fp_solarenergy_day = 0;
			last_day = time_day();
		}

		// calculate energy flow
		fpint fp_additional_capacity = solarpanel_capacity(60);

		// update battery
		fp_capacity = fpint_min(battery_maxcapacity(), fpint_add(fp_additional_capacity, battery_capacity()));

		// add to cummulated
		fp_solarenergy_day = fpint_add(fp_solarenergy_day, fp_additional_capacity);

		// restart timer
		etimer_reset(&timer_update_solar);
	}

	PROCESS_END();
}
