#include "contiki.h"
#include "sdf-config.h"

#include "energymeter.h"

#include "co2-sensor.h"
#include "gps-sensor.h"
#include "co-sensor.h"
#include "drandom.h"
#include "gccbugs.h"
#include "fpint.h"

/**
 * lifetime energymeter tickcounter
 */
static energymeter_sample lifetime = {0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL};

/**
 * last tickcounter of energest modules
 */
static unsigned long last_energest_cpu_active     = 0;
static unsigned long last_energest_cpu_sleep      = 0;
static unsigned long last_energest_radio_transmit = 0;
static unsigned long last_energest_radio_listen   = 0;

/**
 * saves new energest tick information to lifetime structure (param)
 */
static void updateEnergest(unsigned long energest, unsigned long long* lifetime, unsigned long* last) {
	unsigned long diff = energest  - *last;
	if(diff > 0) {
		*lifetime = *lifetime + diff;
		*last = energest;
	}
}

/**
 * on tmote sky the (2^32)-1 energest value has a resolution of 1.5 days (32768 ticks/second),
 * so every 1.5 days the value will overflow and it's not possible to calculate energy drains
 * for a longer period of 1.5 days. Therefore the energest values are saved to an unsigned 64bit
 * integer for infinte long energy measurement periods.
 * The energymeter_sampling() function has to be called at least once every 1.5 days to save the
 * values. In the actual implementation the battery is the component ensuring this periodic call,
 * when this should not be not given in the future a separate energymeter process is required to
 * meet the requirements
 */
void energymeter_sampling(energymeter_sample* fill) {
    // save actual energy data to internal energest structure:
    // energest data is only automatically flushed after _ON() _OFF() sequence,
    // calling energest_flush() allows to have accurate data in between.
    energest_flush();

    // save energest values
    updateEnergest(energest_type_time(ENERGEST_TYPE_CPU),      &lifetime.cpu_active,     &last_energest_cpu_active);
    updateEnergest(energest_type_time(ENERGEST_TYPE_LPM),      &lifetime.cpu_sleep,      &last_energest_cpu_sleep);
    updateEnergest(energest_type_time(ENERGEST_TYPE_TRANSMIT), &lifetime.radio_transmit, &last_energest_radio_transmit);
    updateEnergest(energest_type_time(ENERGEST_TYPE_LISTEN),   &lifetime.radio_listen,   &last_energest_radio_listen);

    // copy values of lifetime sample to sampled sample
    memcpy(fill, &lifetime, sizeof(energymeter_sample));
}

fpint energymeter_calculate_drain(const energymeter_sample* last, const energymeter_sample* now, int only_full_timeframes) {
	static energymeter_sample temp;
	memcpy(&temp, last, sizeof(energymeter_sample));

	return energymeter_calculate_drain_update_last(&temp, now, only_full_timeframes);
}

/**
 * calculates drain with given drain function and adds to absolute drain
 */
static void update_drain_generic(unsigned long long (*drainfunc)(const unsigned long long*, const unsigned long long*, fpint, fpint*, int), const unsigned long long* now, unsigned long long* last, fpint fp_energy, fpint* fp_drain, int timeframe_option) {
	fpint fp_drain_mote;
	*last += (*drainfunc)(last, now, fp_energy, &fp_drain_mote, timeframe_option);
	*fp_drain = fpint_add(*fp_drain, fp_drain_mote);
}

fpint energymeter_calculate_drain_update_last(energymeter_sample* last, const energymeter_sample* now, int timeframe_option) {
	// summed drain
	fpint fp_drain = fpint_to(0);

    // update drains
	// (some have to be calculated only for full hours due to very small drain)
	update_drain_generic(&energymeter_drain_seconds, &now->cpu_active,     &last->cpu_active,     ENERGYMETER_DRAIN_SECONDS_CPU_ACTIVE,     &fp_drain, timeframe_option);
	update_drain_generic(&energymeter_drain_seconds, &now->radio_transmit, &last->radio_transmit, ENERGYMETER_DRAIN_SECONDS_RADIO_TRANSMIT, &fp_drain, timeframe_option);
	update_drain_generic(&energymeter_drain_seconds, &now->radio_listen,   &last->radio_listen,   ENERGYMETER_DRAIN_SECONDS_RADIO_LISTEN,   &fp_drain, timeframe_option);
	update_drain_generic(&energymeter_drain_seconds, &now->sensor_co,      &last->sensor_co,      ENERGYMETER_DRAIN_SECONDS_SENSOR_CO,      &fp_drain, timeframe_option);
	update_drain_generic(&energymeter_drain_seconds, &now->sensor_co2,     &last->sensor_co2,     ENERGYMETER_DRAIN_SECONDS_SENSOR_CO2,     &fp_drain, timeframe_option);
	update_drain_generic(&energymeter_drain_seconds, &now->sensor_gps,     &last->sensor_gps,     ENERGYMETER_DRAIN_SECONDS_SENSOR_GPS,     &fp_drain, timeframe_option);
	update_drain_generic(&energymeter_drain_hours,   &now->cpu_sleep,      &last->cpu_sleep,      ENERGYMETER_DRAIN_HOURS_CPU_SLEEP,        &fp_drain, timeframe_option);

	return fp_drain;
}

/**
 * generic calculation of drain
 */
static unsigned long long drain_generic(const unsigned long long* last, const unsigned long long* now, fpint fp_energy, fpint* fp_drain, unsigned long long timeframe, int timeframe_option) {
	unsigned long long ticks_calculated = 0;

	// save empty drain in "return" (maybe no drain will be calculated so default to zero)
	*fp_drain = fpint_to(0);

	// calculate tick difference
	unsigned long long tickdiff = *now - *last;

	// calculate number of timeframes between last and now
	unsigned long long timeframes = gccbugs_ulldiv(tickdiff, timeframe);

	// save calculated ticks and and reduce tickdiff
	ticks_calculated = timeframes * timeframe;
	tickdiff -= ticks_calculated;

	// calculate needed energy for timeframes
	if(timeframes > 0)
		*fp_drain = fpint_multimes(fp_energy, (unsigned long) timeframes);

	// calculate energy drain for incomplete timeframes
	if(timeframe_option == ENERGYMETER_DRAIN_WITH_INCOMPLETE_TIMEFRAMES && tickdiff > 0) {
		long l_tickdiff  = (long) tickdiff;
		long l_timeframe = (long) timeframe;

		// calculate seconds the drain has to be calculated for
		// timeframe may overflow fpint on platform, workaround:
		long multiple = l_timeframe / l_tickdiff;
		fpint fp_seconds = fpint_div(FPINT_ONE, fpint_to(multiple));

		// calculate drain
		*fp_drain = fpint_add(*fp_drain, fpint_mul(fp_seconds, fp_energy));

		// save calculated ticks
		ticks_calculated += tickdiff;
	}

	return ticks_calculated;
}

unsigned long long energymeter_drain_seconds(const unsigned long long* last, const unsigned long long* now, fpint fp_energy, fpint* fp_drain, int timeframe_option) {
	return drain_generic(last, now, fp_energy, fp_drain, ENERGYMETER_TICKS_PER_SECOND, timeframe_option);
}

unsigned long long energymeter_drain_hours(const unsigned long long* last, const unsigned long long* now, fpint fp_energy, fpint* fp_drain, int timeframe_option) {
	return drain_generic(last, now, fp_energy, fp_drain, ENERGYMETER_TICKS_PER_SECOND * 3600ULL, timeframe_option);
}

int co2_value() {
	// co2 sensor runs 0.5s-1s
	int seconddiff = drandom_rand_minmax(1, 2);
    lifetime.sensor_co2 += gccbugs_ulldiv(ENERGYMETER_TICKS_PER_SECOND, seconddiff);

    // TGS4161 is specified for 350~10000ppm
    return drandom_rand_minmax(350, 10000);
}

int co_value() {
	// co sensor runs 10s-30s
	int seconds = drandom_rand_minmax(10, 30);
	lifetime.sensor_co += ENERGYMETER_TICKS_PER_SECOND * seconds;

    // TGS2442 is specified for 30~1000ppm
    return drandom_rand_minmax(30, 1000);
}

void gps_value(gps_position* position) {
	// gps sensor runs 1s-5s
	int seconds = drandom_rand_minmax(1, 5);
	lifetime.sensor_gps += ENERGYMETER_TICKS_PER_SECOND * seconds;

    position->latitude = 0x31E0A0;
    position->latitude = 0x8A789;
}
