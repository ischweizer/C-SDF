#include <stdio.h>
#include <string.h>
#include "contiki-net.h"
#include "net/rpl/rpl.h"
#include "uip-debug.h"

#include "udphelper.h"

#define DEBUG DEBUG_OFF
#include "debug.h"

/**
 * import rpl routing table
 */
extern uip_ds6_route_t uip_ds6_routing_table[UIP_DS6_ROUTE_NB];

/**
 * uIP UDP packet buffer
 */
#define UIP_IP_BUF ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

/**
 * whether mote is sink
 */
static int is_sink;

/**
 * temporary ip for copy operations etc.
 */
static uip_ipaddr_t temp_ip, temp_ip2;

/**
 * normalizes an ipv6 address by setting the first 16bits to aaaa
 */
uip_ipaddr_t* normalize_address(uip_ipaddr_t* addr) {
    addr->u16[0] = 0xaaaa;
    return addr;
}

struct uip_udp_conn* udphelper_bind(unsigned short port) {
	struct uip_udp_conn* udp = udp_new(NULL, UIP_HTONS(port), NULL);
	if(udp != NULL)
		udp_bind(udp, UIP_HTONS(port));

	return udp;
}

void udphelper_send(struct uip_udp_conn* udp, const uip_ipaddr_t* to, const void* data, unsigned short datalen) {
    debug("[UDPHELPER] sent packet '%s' to ", (char*) data);
	debug_address(to);
	debug("\n");

	uip_ipaddr_copy(&udp->ripaddr, to);
    uip_udp_packet_send(udp, data, datalen);
	uip_create_unspecified(&udp->ripaddr);
}

uip_ipaddr_t* udphelper_packet_senderaddress(uip_ipaddr_t* addr) {
	uip_ipaddr_copy(addr, &UIP_IP_BUF->srcipaddr);
	return normalize_address(addr);
}

void* udphelper_packet_data() {
	return (void*) uip_appdata;
}

uint16_t udphelper_packet_datalen() {
	return uip_datalen();
}

void udphelper_registerlocaladdress(int sink) {
	is_sink = sink;
    if(!is_sink) {
    	/**
    	 * code taken from examples/ipv6/rpl-udp/udp-client.c
    	 */
        uip_ip6addr(&temp_ip, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
        uip_ds6_set_addr_iid(&temp_ip, &uip_lladdr);
        uip_ds6_addr_add(&temp_ip, 0, ADDR_AUTOCONF);
    } else {
    	// add udphelper_address_sink() to possible ip addresses
    	udphelper_address_sink(&temp_ip);
    	temp_ip.u16[0] = 0x80fe;                    // add link-local tentative ip
    	uip_ds6_addr_add(&temp_ip, 0, ADDR_MANUAL); // add link-local tentative ip
    	temp_ip.u16[0] = 0xaaaa;                    // add global preferred ip
    	uip_ds6_addr_add(&temp_ip, 0, ADDR_MANUAL); // add global preferred ip

    	// make udphelper_address_sink() preferred ip
    	int i, state;
    	for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    		state = uip_ds6_if.addr_list[i].state;
    		if(uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
    			if(udphelper_address_equals(&temp_ip, &uip_ds6_if.addr_list[i].ipaddr))
    				uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
    			else
    				uip_ds6_if.addr_list[i].state = ADDR_TENTATIVE;
    		}
    	}

    	/**
    	 * code taken from examples/ipv6/rpl-udp/udp-server.c
    	 */
    	udphelper_address_sink(&temp_ip);
    	struct uip_ds6_addr* root_if = uip_ds6_addr_lookup(&temp_ip);
    	if(root_if != NULL) {
    		rpl_dag_t *dag = rpl_set_root(RPL_DEFAULT_INSTANCE, (uip_ip6addr_t*) &temp_ip);
    		rpl_set_prefix(dag, &temp_ip, 64);
    	} else {
    		printf("[UDPHELPER] FAILED TO CREATE RPL DAG\n");
    	}
    }
}

uip_ipaddr_t* udphelper_address_local(uip_ipaddr_t* addr) {
	/**
	 * code taken from examples/ipv6/rpl-udp/udp-client.c
	 *
	 * +++ modified a little bit to reflect preferring of ips +++
	 */
    int i, state;
    for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
        state = uip_ds6_if.addr_list[i].state;
        if(uip_ds6_if.addr_list[i].isused && (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
            // preferred ip of mote != sink is set on first call
        	if(!is_sink)
        		uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;

        	// ip of sink has no been preferred: skip it
        	if(is_sink && state == ADDR_TENTATIVE)
        		continue;

        	uip_ipaddr_copy(addr, &uip_ds6_if.addr_list[i].ipaddr);
        	return normalize_address(addr);
        }
    }

    return NULL;
}

uip_ipaddr_t* udphelper_address_sink(uip_ipaddr_t* addr) {
    uip_ip6addr(addr, 0xaaaa, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xaaaa);
    return addr;
}

uip_ipaddr_t* udphelper_address_parent(uip_ipaddr_t* addr) {
    /*
     * code taken from examples/ipv6/rpl-collect/udp-sender.c
     */
    rpl_dag_t *dag = rpl_get_any_dag();
    if(dag->preferred_parent != NULL) {
    	uip_ipaddr_copy(addr, &dag->preferred_parent->addr);
    	return normalize_address(addr);
    }

    return NULL;
}

int udphelper_address_equals(const uip_ipaddr_t* ip1, const uip_ipaddr_t* ip2) {
	uip_ipaddr_copy(&temp_ip,  ip1);
	uip_ipaddr_copy(&temp_ip2, ip2);
	return uip_ipaddr_cmp(normalize_address(&temp_ip), normalize_address(&temp_ip2));
}

int udphelper_childs_count() {
    int count = 0, i;
    for(i = 0; i < UIP_DS6_ROUTE_NB; i++) {
        if(uip_ds6_routing_table[i].isused) {
            count++;
        }
    }

    return count;
}

int udphelper_childs_all_count() {
	int childs = 0, i;
    for(i = 0; i < UIP_DS6_ROUTE_NB; i++)
        if(uip_ds6_routing_table[i].isused)
        	childs++;

    return childs;
}


uip_ipaddr_t* udphelper_childs_all_get(int pos, uip_ipaddr_t* addr) {
	int i, j = 0;
	for(i = 0; i < UIP_DS6_ROUTE_NB; i++) {
        if(uip_ds6_routing_table[i].isused) {
        	if(pos == j) {
        		uip_ipaddr_copy(addr, &uip_ds6_routing_table[i].ipaddr);
        		return normalize_address(addr);
        	}

        	j++;
        }
	}

	return NULL;
}

int udphelper_childs_direct_count() {
	int childs = 0, i;
    for(i = 0; i < UIP_DS6_ROUTE_NB; i++)
        if(uip_ds6_routing_table[i].isused && udphelper_address_equals(&uip_ds6_routing_table[i].ipaddr, &uip_ds6_routing_table[i].nexthop))
        	childs++;

    return childs;
}

uip_ipaddr_t* udphelper_childs_direct_get(int pos, uip_ipaddr_t* addr) {
	int i, j = 0;
	for(i = 0; i < UIP_DS6_ROUTE_NB; i++) {
        if(uip_ds6_routing_table[i].isused && udphelper_address_equals(&uip_ds6_routing_table[i].ipaddr, &uip_ds6_routing_table[i].nexthop)) {
        	if(pos == j) {
        		uip_ipaddr_copy(addr, &uip_ds6_routing_table[i].ipaddr);
        		return normalize_address(addr);
        	}

        	j++;
        }
	}

	return NULL;
}

void udphelper_print_routing() {
	printf("IP: ");
	udphelper_print_address(udphelper_address_local(&temp_ip));
	printf("\n");

    /*
     * code taken from examples/ipv6/rpl-collect/udp-sender.c
     */
    printf("Preferred parent: ");
    if(udphelper_address_parent(&temp_ip) != NULL) {
    	udphelper_print_address(&temp_ip);
    } else {
    	printf("NULL");
    }
    printf("\n");

    /*
     * code taken from examples/ipv6/rpl-collect/udp-sender.c
     */
    int i;
    printf("Route entries:\n");
    for(i = 0; i < UIP_DS6_ROUTE_NB; i++) {
        if(uip_ds6_routing_table[i].isused) {
        	// destination
        	uip_ipaddr_copy(&temp_ip, &uip_ds6_routing_table[i].ipaddr);
        	udphelper_print_address(normalize_address(&temp_ip));

        	// nexthop
        	printf(" (nexthop: ");
        	uip_ipaddr_copy(&temp_ip, &uip_ds6_routing_table[i].nexthop);
        	udphelper_print_address(normalize_address(&temp_ip));
        	printf(")\n");
        }
    }
    printf("---\n");
}

void udphelper_print_address(const uip_ipaddr_t* addr) {
	uip_debug_ipaddr_print(addr);
}
