#include <string.h>

#include "sdf-config.h"
#include "fpint.h"
#include "energymeter.h"
#include "samplingrate.h"
#include "circularbuffer.h"
#include "consumptionrate.h"
#include "udphelper.h"
#include "gccbugs.h"

#define DEBUG DEBUG_OFF
#include "debug.h"

/**
 * samples of energy drain for radio transmiting a message
 */
static fpint tx_samples[SDF_SAMPLINGRATE_ENERGYSAMPLES];
static int   tx_nextpos = 0, tx_saved = 0;

/**
 * samples of energy drain for radio receiving a message
 */
static fpint rx_samples[SDF_SAMPLINGRATE_ENERGYSAMPLES];
static int   rx_nextpos = 0, rx_saved = 0;

/**
 * samples of energy drain for sensing values for a single message
 */
static fpint sense_samples[SDF_SAMPLINGRATE_ENERGYSAMPLES];
static int   sense_nextpos = 0, sense_saved = 0;

/**
 * last energymeter sample
 */
static energymeter_sample last_energymeter_sample;

/**
 * whether an energymeter sample has already been taken
 */
static int energymeter_sample_taken = 0;

/**
 * calculte drain of a soure
 */
static fpint drain(unsigned long long* last, unsigned long long* now, fpint fp_energy) {
	fpint fp_drain;

	energymeter_drain_seconds(last, now, fp_energy, &fp_drain, ENERGYMETER_DRAIN_WITH_INCOMPLETE_TIMEFRAMES);
	return fp_drain;
}

int samplingrate_calculate(int max_messages) {
	// average energy needed for operations
	fpint fp_energy_rx    = fpint_avg(rx_samples,    rx_saved);
	fpint fp_energy_tx    = fpint_avg(tx_samples,    tx_saved);
	fpint fp_energy_sense = fpint_avg(sense_samples, sense_saved);

	// available energy
	fpint fp_energy = consumptionrate_energy(SDF_SAMPLINGRATE_UPDATEINTERVAL);

	// get child count
	fpint fp_childs = fpint_to(udphelper_childs_all_count());

	// calculate messages
	fpint fp_drain_childs = fpint_mul(fp_childs, fpint_add(fp_energy_rx, fp_energy_tx));
	fpint fp_drain_self   = fpint_add(fp_energy_tx, fp_energy_sense);
	fpint fp_messages     = fpint_div(fp_energy, fpint_add(fp_drain_childs, fp_drain_self));

	// set sampling rate
	int samplingrate = fpint_from(fpint_floor(fp_messages));
	if(max_messages != -1 && samplingrate > max_messages)
		samplingrate = max_messages;
	int min_samplingrate = fpint_from(fpint_round(fpint_div(fpint_to(SDF_SAMPLINGRATE_MINIMAL), fpint_to(SPEEDMULTIPLIER))));
	if(samplingrate < min_samplingrate)
		samplingrate = min_samplingrate;

	// debug calculation results
	debug("[SAMPLINGRATE] ");
	debug("avg-receive=%smAh ",      debug_fpint(fp_energy_rx));
	debug("avg-transmit=%smAh ",     debug_fpint(fp_energy_tx));
	debug("avg-sensors=%smAh ",      debug_fpint(fp_energy_sense));
	debug("available-energy=%smAh ", debug_fpint(fp_energy));
	debug("childs=%d ",              udphelper_childs_all_count());
	debug("messages=%s ",            debug_fpint(fp_messages));
	debug("max-messages=%d | ",      max_messages);
	debug("samplingrate=%d\n",       samplingrate);

	return samplingrate;
}

void samplingrate_sample_energy_drain(int last_samplingrate, int last_childcount) {
	// sample actual energy
	static energymeter_sample now;
	energymeter_sampling(&now);

	// prevent calculation for first interval: no last sample is available
	if(energymeter_sample_taken) {
		// last sampling rate as fpint
		fpint fp_lastsamplingrate = fpint_to(last_samplingrate);

		// calc tx drain
		fpint fp_transmitted    = fpint_add(fp_lastsamplingrate, fpint_mul(fp_lastsamplingrate, fpint_to(last_childcount)));
		fpint fp_drain_transmit = fpint_div(drain(&last_energymeter_sample.radio_transmit, &now.radio_transmit, ENERGYMETER_DRAIN_SECONDS_RADIO_TRANSMIT), fp_transmitted);
		circularbuffer_save(tx_samples, SDF_SAMPLINGRATE_ENERGYSAMPLES, fp_drain_transmit, &tx_nextpos, &tx_saved);

		// calc rx drain
		fpint fp_drain_receive;
		if(last_childcount > 0) {
			fpint fp_received = fpint_mul(fp_lastsamplingrate, fpint_to(last_childcount));
			fp_drain_receive  = fpint_div(drain(&last_energymeter_sample.radio_listen, &now.radio_listen, ENERGYMETER_DRAIN_SECONDS_RADIO_LISTEN), fp_received);
			circularbuffer_save(rx_samples, SDF_SAMPLINGRATE_ENERGYSAMPLES, fp_drain_receive, &rx_nextpos, &rx_saved);
		} else {
			fp_drain_receive = FPINT_ZERO;
		}

		// calc sensor drains
		fpint fp_drain_co    = drain(&last_energymeter_sample.sensor_co,  &now.sensor_co,  ENERGYMETER_DRAIN_SECONDS_SENSOR_CO);
		fpint fp_drain_co2   = drain(&last_energymeter_sample.sensor_co2, &now.sensor_co2, ENERGYMETER_DRAIN_SECONDS_SENSOR_CO2);
		fpint fp_drain_gps   = drain(&last_energymeter_sample.sensor_gps, &now.sensor_gps, ENERGYMETER_DRAIN_SECONDS_SENSOR_GPS);
		fpint fp_drain_sense = fpint_div(fpint_add(fp_drain_co, fpint_add(fp_drain_co2, fp_drain_gps)), fp_lastsamplingrate);
		circularbuffer_save(sense_samples, SDF_SAMPLINGRATE_ENERGYSAMPLES, fp_drain_sense, &sense_nextpos, &sense_saved);

		// uncomment code block if you're really interested in (saved firmware size can be used for other debugging purposes)
		//debug("[SAMPLINGRATE] samplingrate=%d | ", last_samplingrate);
		//debug("receive=%smAh ",  debug_fpint(fp_drain_receive));
		//debug("transmit=%smAh ", debug_fpint(fp_drain_transmit));
		//debug("co=%smAh ",       debug_fpint(fp_drain_co));
		//debug("co2=%smAh ",      debug_fpint(fp_drain_co2));
		//debug("gps=%smAh\n",     debug_fpint(fp_drain_gps));
	}

	// save actual sample as last sample
	memcpy(&last_energymeter_sample, &now, sizeof(energymeter_sample));
	energymeter_sample_taken = 1;
}
