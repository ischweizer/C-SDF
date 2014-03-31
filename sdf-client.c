#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "net/queuebuf.h"

#include "sdf-config.h"
#include "battery.h"
#include "energymeter.h"
#include "consumptionrate.h"
#include "co-sensor.h"
#include "co2-sensor.h"
#include "gps-sensor.h"
#include "udphelper.h"
#include "time.h"
#include "samplingrate.h"

// number of packets SDF may sent in a loop
// (will only use 3/4 of buffer to make space for csma/routing messages)
#define SENDQUEUE ((QUEUEBUF_NUM * 3) / 4)

// udp socket
static struct uip_udp_conn* udp;

// static instance of SDF sink ip and parent ip
static uip_ipaddr_t ip_sink, ip_parent;

// starting time of SDF algorithm
static unsigned long time_init;

// initial samplingrate
static int samplingrate;

// number of children the samplingrate has yet been tranmitted to
static int samplingrate_children_transmitted = 0, samplingrate_children_count = 0;

// number of sampled samples in each interval
static int sampled;

// last samplingrate received from parent (keep on sending with minimal rate on missing update from parent after init phase)
static int last_parent_samlingrate = SDF_SAMPLINGRATE_MINIMAL;

// last time an energysample for samplingrate has been taken
static unsigned long last_samplingrate_energysample = 0;

// number of childs in samplingrate inverval
static int last_samplingrate_childs;

// function for updating sampling rate
static void update_sampling_rate(struct etimer* sampletimer, struct etimer* timer_samplingrate_transmit, int real_calculation);

// backoff timers for sending UDP messages
static struct ctimer backofftimer_send_sample, backofftimer_send_samplingrate;

/**
 * simple function for string to int conversion
 *
 * atoi() needed two much firmware size ;)
 */
static int str2int(char* str) {
	int sum = 0, i = 0;
	while(str[i] != '\0') {
		sum = sum * 10 + (str[i] - '0');
		i++;
	}

	return sum;
}

/**
 * samples sensors and sends packet
 */
static void send_packet(void* ptr) {
	// get sensor data
	static gps_position gps;
	co2_value();
	co_value();
	gps_value(&gps);
	
	// send dummy sample (there's actually no firmware space for building real message)
	udphelper_send(udp, &ip_sink, "dummy-sample-with-26-chars", strlen("dummy-sample-with-26-chars") + 1);

	// take another sample
	if(sampled < samplingrate)
		etimer_reset((struct etimer*) ptr);
}

/**
 * sends samplingrate information to childs
 */
static void send_samplingrate(void* ptr) {
	// save samplingrate message
	static char message[4];
	sprintf(message, "%d", samplingrate);

	// send message to childs
	int sent = 0;
	static uip_ipaddr_t ip;
	while(++sent <= SENDQUEUE && samplingrate_children_transmitted < samplingrate_children_count) {
		if(udphelper_childs_direct_get(samplingrate_children_transmitted++, &ip) != NULL)
			udphelper_send(udp, &ip, message, strlen(message) + 1);
	}

	// restart timer for remaining childs
	if(samplingrate_children_transmitted < samplingrate_children_count)
		etimer_reset((struct etimer*) ptr);
}

/**
 * SDF-Client process
 */
PROCESS(sdfclient, "SDF-Client");
AUTOSTART_PROCESSES(&sdfclient);
PROCESS_THREAD(sdfclient, ev, data) {
    PROCESS_BEGIN();

    // registers local ipv6 address
    udphelper_registerlocaladdress(0);

    // delay for initializing rpl routing tree
    static struct etimer timer_rpldelay;
    etimer_set(&timer_rpldelay, CLOCK_SECOND * RPLINITTIME);
    PROCESS_WAIT_UNTIL(etimer_expired(&timer_rpldelay));

    /*
     *
     * start SDF !!!
     *
     */
    time_init = time();
    printf("Started SDF-Client at %dx (", SPEEDMULTIPLIER);
    uip_ipaddr_t ip;
    udphelper_print_address(udphelper_address_local(&ip));
    printf(")\n");

    // init (after rpl dag creation!)
    battery_init();
    consumptionrate_init();

    // bind udp socket to port
    udp = udphelper_bind(SDF_PORT);

    // save sink ip
    udphelper_address_sink(&ip_sink);

    // timer for energy neutral consumption rate
    static struct etimer timer_consumptionrate;
    etimer_set(&timer_consumptionrate, CLOCK_SECOND * 86400 / SPEEDMULTIPLIER);

    // timer for updating the sampling rate
    static struct etimer timer_updatesamplingrate;
    etimer_set(&timer_updatesamplingrate, CLOCK_SECOND * SDF_SAMPLINGRATE_UPDATEINTERVAL / SPEEDMULTIPLIER);

    // timer for transmitting sampling rate to children
    static struct etimer timer_samplingrate_transmit;
    etimer_set(&timer_samplingrate_transmit, CLOCK_SECOND * 2 / SPEEDMULTIPLIER);
    etimer_stop(&timer_samplingrate_transmit); // started by update_sampling_rate()

    // timer for taking samples and transmitting them
    static struct etimer timer_samples;
    etimer_stop(&timer_samples); // started by update_sampling_rate()

    // init node with first calculation of sampling rate
    update_sampling_rate(&timer_samples, &timer_samplingrate_transmit, 1);

    while(1) {
    	PROCESS_WAIT_EVENT();

    	// take an energy neutral consumptionrate sample
    	// (this has to be the first conditin! consumptionrate should be taken everytime
    	// before samplingrate is calculated, so samplingrate can use the new consumptionrate
    	// sample to calculate a more accurate sampling rate)
    	if(etimer_expired(&timer_consumptionrate)) {
    		consumptionrate_sample();
    		etimer_restart(&timer_consumptionrate);
    	}

    	// update SDF sampling rate
    	if(etimer_expired(&timer_updatesamplingrate)) {
			// no real calculation when not in init phase and parent is not sink
			// (mote will keep last samplingrate as long as a new samplingrate is received)
			if((time() - time_init) / SDF_INITIALIZATIONPHASE == 0 || udphelper_address_equals(udphelper_address_parent(&ip_parent), &ip_sink)) {
    			update_sampling_rate(&timer_samples, &timer_samplingrate_transmit, 1);
    		} else {
				update_sampling_rate(&timer_samples, &timer_samplingrate_transmit, 0);
			}

    		etimer_restart(&timer_updatesamplingrate);
    	}

    	// new SDF sampling rate control message
    	if(ev == tcpip_event && uip_newdata()) {
			// for some reason the real tmote skys in TUDμNet have routing problems not existent in cooja simulator
			// solution: test if sampling rate update was sent by known parent
			// (prevents multiple recalculations on incorrect routing tables)
			static uip_ipaddr_t ip_sender;
			if(udphelper_address_equals(udphelper_address_parent(&ip_parent), udphelper_packet_senderaddress(&ip_sender))) {
				last_parent_samlingrate = str2int(udphelper_packet_data());
				update_sampling_rate(&timer_samples, &timer_samplingrate_transmit, 1);
				etimer_restart(&timer_updatesamplingrate); // new interval for samplingrate is set by rpl parent
			}
    	}

    	// transmit SDF sampling rate to childs
    	// (etimer_stop() seems to not work, so a condition has been added to filter out
    	// invalid etimer events)
    	if(etimer_expired(&timer_samplingrate_transmit) && samplingrate_children_transmitted < samplingrate_children_count) {
    		// For some reason directly sending the messages within this process block has some
    		// random message losses. The contiki example ipv6/rpl-udp/udp-client.c uses the
    		// backoff timer (ctimer) and magically it works without any message lost.
    		// (backoff timer should not be speed up by SPEEDMULTIPLIER!)
    		ctimer_set(&backofftimer_send_samplingrate, CLOCK_SECOND / 8, send_samplingrate, &timer_samplingrate_transmit);
    	}

    	// take a sample and transmit it
    	// (etimer_stop() seems to not work, so a condition has been added to filter out
    	// invalid etimer events)
    	if(etimer_expired(&timer_samples) && ++sampled <= samplingrate) {
    		// For some reason directly sending the messages within this process block has some
    		// random message losses. The contiki example ipv6/rpl-udp/udp-client.c uses the
    		// backoff timer (ctimer) and magically it works without any message lost.
    		// (backoff timer should not be speed up by SPEEDMULTIPLIER!)
    		ctimer_set(&backofftimer_send_sample, CLOCK_SECOND / 8, send_packet, &timer_samples);
    	}
    }

    PROCESS_END();
}

/**
 * calculates the new sampling rate (the "magic" part of SDF is done here)
 */
static void update_sampling_rate(struct etimer* sample_timer, struct etimer* transmit_timer, int real_calculation) {
	// samplingrate energy sample
	if(last_samplingrate_energysample == 0 || (time() - last_samplingrate_energysample) / SDF_SAMPLINGRATE_UPDATEINTERVAL > 0) {
		// take sample
		// first call with no information on last samplingrate and childs will
		// not do anything: reference energy sample is accquired
		samplingrate_sample_energy_drain(samplingrate, last_samplingrate_childs);

		// set infos for next sample
		last_samplingrate_childs = udphelper_childs_all_count();
		last_samplingrate_energysample = time();
	}

	// calculate samplingrate
	if(real_calculation) {
		// calculate samplingrate
		if((time() - time_init) / SDF_INITIALIZATIONPHASE == 0) {
			fpint fp_samplingrate = fpint_div(fpint_to(SDF_SAMPLINGRATE_MINIMAL), fpint_to(SPEEDMULTIPLIER));
			samplingrate = fpint_from(fpint_round(fp_samplingrate));
		} else {
			int max_samples = (udphelper_address_equals(udphelper_address_parent(&ip_parent), &ip_sink)) ? -1 : last_parent_samlingrate;
			samplingrate = samplingrate_calculate(max_samples);

			// send sampling rate to all childs
			if(udphelper_childs_direct_count() > 0) {
				samplingrate_children_transmitted = 0;
				samplingrate_children_count = udphelper_childs_direct_count();
				etimer_restart(transmit_timer);
			}
		}

		// debug
		printf("[%lds] ", time());
		printf("%d samples (battery=%ldmAh)\n", samplingrate, fpint_from(battery_capacity()));
	}

	// set samplingrate timer (evenly distributed samples with gaps at start and end)
	int sampling_delay = (SDF_SAMPLINGRATE_UPDATEINTERVAL - 60) / samplingrate;
	etimer_set(sample_timer, CLOCK_SECOND * sampling_delay / SPEEDMULTIPLIER);
	etimer_restart(sample_timer);

	// reset old samples counter (new interval begins)
	sampled = 0;
}
