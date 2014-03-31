#include <string.h>

#include "sdf-config.h"
#include "consumptionrate.h"
#include "circularbuffer.h"
#include "energymeter.h"
#include "solarpanel.h"
#include "battery.h"
#include "fpint.h"
#include "time.h"

#define DEBUG DEBUG_OFF
#include "debug.h"

/**
 * last energymeter sample
 */
static energymeter_sample last_energymeter_sample;

/**
 * last battery capacity
 */
static fpint fp_last_battery_capacity;

/**
 * next position in consumptionrate buffer
 */
static int nextpos = 0;

/**
 * number of consumptionrate samples saved
 */
int consumptionrate_saved = 0;

/**
 * saved consumptionrate samples
 */
fpint consumptionrate_samples[CONSUMPTIONRATE_SAMPLES];

/**
 * process for periodically polling solar harvested energy
 */
PROCESS(consumptionrate_solarpanel_process, "Consumptionrate-Solarpanel-Process");

/**
 * solar harvested energy since last sample
 */
static fpint fp_solar_energy = 0;

void consumptionrate_init() {
    energymeter_sampling(&last_energymeter_sample);
    fp_last_battery_capacity = battery_capacity();
	#if CONSUMPTIONRATE_SOLARENERGY_BATTERYPREDICTION == 0
		process_start(&consumptionrate_solarpanel_process, NULL);
	#endif

}

void consumptionrate_sample() {
	// take energymeter sample
	static energymeter_sample now;
	energymeter_sampling(&now);

	// calculate energy consumption rate and save in buffer
	#if CONSUMPTIONRATE_SOLARENERGY_BATTERYPREDICTION
		fpint fp_drain = energymeter_calculate_drain(&last_energymeter_sample, &now, ENERGYMETER_DRAIN_WITH_INCOMPLETE_TIMEFRAMES);
		fpint fp_battery = fpint_sub(battery_capacity(), fp_last_battery_capacity);
		fpint fp_consumptionrate = fpint_max(fpint_to(0), fpint_add(fp_battery, fp_drain));
		debug("[CONSUMPTIONRATE] drain=%smAh, batterydiff=%smAh, consumptionrate=%smAh\n", debug_fpint(fp_drain), debug_fpint(fp_battery), debug_fpint(fp_consumptionrate));
	#else
		fpint fp_consumptionrate = fp_solar_energy;
		fp_solar_energy = 0;
		debug("[CONSUMPTIONRATE] consumptionrate=%smAh\n", debug_fpint(fp_consumptionrate));
	#endif
	circularbuffer_save(consumptionrate_samples, CONSUMPTIONRATE_SAMPLES, fp_consumptionrate, &nextpos, &consumptionrate_saved);

	#if DEBUG & DEBUG_ON
		debug("[CONSUMPTIONRATE] Samples=[");
		int i = 0;
		while(i < consumptionrate_saved) {
			debug("%s", debug_fpint(consumptionrate_samples[i]));
			if(++i < consumptionrate_saved)
				debug(", ");
		}
		debug("]\n");
	#endif

	// save actual samples
	memcpy(&last_energymeter_sample, &now, sizeof(energymeter_sample));
	fp_last_battery_capacity = battery_capacity();
}

static fpint samples_confidence(int s, int s_max, fpint fp_c_start, fpint fp_cl_max) {
	if(s < 1)
		return fpint_to(0);

	fpint fp_clmax_mul_smax = fpint_mul(fpint_to(s_max), fp_cl_max);
	if(fpint_compare(fpint_to(s), fp_clmax_mul_smax) == 1)
		return fpint_to(1);

	fpint fp_m_dividend = fpint_sub(fp_c_start, FPINT_ONE);
	fpint fp_m_divisor  = fpint_sub(fp_clmax_mul_smax, fpint_to(2));
	fpint fp_m          = fpint_sub(FPINT_ZERO, fpint_div(fp_m_dividend, fp_m_divisor));
	fpint fp_b_dividend = fpint_sub(fpint_mul(fp_clmax_mul_smax, fp_c_start), fpint_to(2));
	fpint fp_b_divisor  = fpint_sub(fp_clmax_mul_smax, fpint_to(2));
	fpint fp_b          = fpint_div(fp_b_dividend, fp_b_divisor);

	return fpint_add(fpint_mul(fp_m, fpint_to(s)), fp_b);
}

static fpint consumptionrate_calc(fpint fp_c_start, fpint fp_cl_max) {
	// calculate samples confidence
	fpint fp_confidence = samples_confidence(consumptionrate_saved, CONSUMPTIONRATE_SAMPLES, fp_c_start, fp_cl_max);
	debug("[CONSUMPTIONRATE] sample-confidence=%s\n", debug_fpint(fp_confidence));

	// calculate quantile energy
	fpint fp_q = 0xFFFEB7EC;
	fpint fp_quantile = fpint_quantile(consumptionrate_samples, consumptionrate_saved, fp_q);
	debug("[CONSUMPTIONRATE] quantile-energy=%smAh\n", debug_fpint(fp_quantile));

	fpint fp_consumptionrate = fpint_mul(fp_confidence, fp_quantile);
	debug("[CONSUMPTIONRATE] consumptionrate=%smAh\n", debug_fpint(fp_consumptionrate));

	return fp_consumptionrate;
}

fpint consumptionrate_energy(long timeframe) {
	fpint fp_energy_interval = consumptionrate_calc(0x8000, 0x8000);
	fpint fp_interval_factor = fpint_div(FPINT_ONE, fpint_to(86400L / timeframe));
	fpint fp_energy = fpint_mul(fp_interval_factor, fp_energy_interval);
	debug("[CONSUMPTIONRATE] timeframe-energy=%smAh\n", debug_fpint(fp_energy));

	return fp_energy;
}

/**
 * updates periodically energy harvested by solar panel
 */
PROCESS_THREAD(consumptionrate_solarpanel_process, ev, data) {
	PROCESS_BEGIN();

	// timer for updating  solar calculation periodically
	static struct etimer timer_update_solar;
	etimer_set(&timer_update_solar, CLOCK_SECOND * 60 / SPEEDMULTIPLIER);

	// update loop
	while(1) {
		PROCESS_WAIT_UNTIL(etimer_expired(&timer_update_solar));

		// calculate energy flow & save
		fpint fp_capacity = solarpanel_capacity(60);
		fp_solar_energy = fpint_add(fp_solar_energy, fp_capacity);

		// restart timer
		etimer_reset(&timer_update_solar);
	}

	PROCESS_END();
}
