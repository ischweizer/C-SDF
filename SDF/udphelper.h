#ifndef __SUDP_H__
#define __SUDP_H__

#include "contiki.h"
#include "contiki-net.h"

/**
 * registers an udp socket to udp server
 */
struct uip_udp_conn* udphelper_bind(unsigned short port);

/**
 * sends data to a udp client
 */
void udphelper_send(struct uip_udp_conn* udp, const uip_ipaddr_t* to, const void* data, unsigned short datalen);

/**
 * get address of last received packet
 */
uip_ipaddr_t* udphelper_packet_senderaddress(uip_ipaddr_t* addr);

/**
 * get data of last received packet
 */
void* udphelper_packet_data();

/**
 * get length of last received packet
 */
uint16_t udphelper_packet_datalen();

/**
 * registers local addresses
 */
void udphelper_registerlocaladdress(int sink);

/**
 * gets local ipv6 address
 *
 * returns pointer of parameters
 */
uip_ipaddr_t* udphelper_address_local(uip_ipaddr_t* addr);

/**
 * gets sink ipv6 address
 *
 * returns pointer of parameters
 */
uip_ipaddr_t* udphelper_address_sink(uip_ipaddr_t* addr);

/**
 * gets rpl ipv6 parent addr
 *
 * returns pointer of parameters
 */
uip_ipaddr_t* udphelper_address_parent(uip_ipaddr_t* addr);

/**
 * comapres two ip addresses
 *
 * returns 1 for equality, 0 when not equals
 */
int udphelper_address_equals(const uip_ipaddr_t* ip1, const uip_ipaddr_t* ip2);

/**
 * number of rpl ipv6 childs
 *
 * returns number
 */
int udphelper_childs_all_count();

/**
 * gets specific rpl ivp6 child
 *
 * returns ip or NULL
 */
uip_ipaddr_t* udphelper_childs_all_get(int pos, uip_ipaddr_t* addr);

/**
 * number of direct rpl ipv6 childs
 *
 * returns number
 */
int udphelper_childs_direct_count();

/**
 * gets specific direct rpl ivp6 child
 *
 * returns ip or NULL
 */
uip_ipaddr_t* udphelper_childs_direct_get(int pos, uip_ipaddr_t* addr);


/**
 * prints RPL routing information
 */
void udphelper_print_routing();

/**
 * prints ipv6 address
 */
void udphelper_print_address(const uip_ipaddr_t* addr);

#endif /* __SUDP_H__ */
