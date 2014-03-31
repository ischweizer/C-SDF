#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include "sdf-config.h"
#include "energymeter.h"
#include "co-sensor.h"
#include "co2-sensor.h"
#include "gps-sensor.h"
#include "udphelper.h"

// udp socket
static struct uip_udp_conn* udp;

PROCESS(sdfserver, "SDF-Server");
AUTOSTART_PROCESSES(&sdfserver);
PROCESS_THREAD(sdfserver, ev, data) {
    PROCESS_BEGIN();

    // server runs at 100% radio dutycycle:
    // server will no more loose messages from clients when radio is turned off,
    // so clients can everytime reach the sink with no packet loss due to turned
    // of radio decreasing energy consumption on clients for resent packets
    NETSTACK_MAC.off(1);

    // registers local ipv6 address
    udphelper_registerlocaladdress(1);

    // delay for initializing rpl routing tree
    static struct etimer initdelay;
    etimer_set(&initdelay, CLOCK_SECOND * RPLINITTIME);

    // print start information
    printf("Started SDF-Server at %dx (", SPEEDMULTIPLIER);
    uip_ipaddr_t ip;
    udphelper_print_address(udphelper_address_local(&ip));
    printf(")\n");

    // bind sdf server to udp port
    udp = udphelper_bind(5678);

    while(1) {
    	PROCESS_WAIT_EVENT();

        // new UDP data
        if(ev == tcpip_event && uip_newdata()) {
			#if PRINTSAMPLES
				printf("received '%s' from ", (char*) udphelper_packet_data());
				static uip_ipaddr_t sender;
				udphelper_print_address(udphelper_packet_senderaddress(&sender));
				printf("\n");
			#endif
        }
    }

    PROCESS_END();
}
