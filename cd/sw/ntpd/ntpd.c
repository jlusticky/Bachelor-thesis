/* 
 * NTP client implementation for Contiki
 * 
 * NTPv4 - RFC 5905
 *
 * Copyright (c) 2011, 2012 Josef Lusticky <xlusti00@stud.fit.vutbr.cz>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *		may be used to endorse or promote products derived from this software
 *		without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
//#include "sys/clock.h"

#include "ntpd.h"

#include <string.h>
#include <inttypes.h>

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

/*
#if CONTIKI_TARGET_AVR_RAVEN
#include <avr/pgmspace.h>
#else
#define PROGMEM
#endif
*/


static const char * host = "aaaa::1"; // NTP server
static uint16_t port = NTP_PORT; // NTP port

static struct ntp_msg msg = { .padding = 0x23 }; // client
/*{
	.padding = 0x23, // client
	.rootdelay = 0,
	.dispersion = 0,
	.refid = 0,
	.reftime.int_partl = 0xEEEE,
	.reftime.fractionl = 0xFFFF,
	.orgtime = 0,
	.rectime = 0,
	.xmttime = 0
};*/

static struct simple_udp_connection connection;

static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  printf("Data received on port %d from port %d with length %d\n",
         receiver_port, sender_port, datalen);
}


static struct uip_udp_conn *udpconn;

PROCESS(ntpd_process, "ntpd");
AUTOSTART_PROCESSES(&ntpd_process);

PROCESS_THREAD(ntpd_process, ev, data)
{
	/*static struct etimer et;
	static uip_ipaddr_t ipaddr;*/
	static uip_ipaddr_t *serveraddr;
	static clock_time_t systime; /* Unsigned (long/short) int */
	
	static uip_ipaddr_t numaddr;
	///uip_ipaddr(&numaddr, 217,31,205,226); // ntp.nic.cz
	uip_ipaddr(&numaddr, 192,168,56,1);
	
	static uip_ipaddr_t dnsserver;
	dnsserver.u8[0] = 4;
	dnsserver.u8[1] = 4;
	dnsserver.u8[2] = 4;
	dnsserver.u8[3] = 4;
	
	PROCESS_BEGIN();
	PRINTF("ntpd process started\n");
	
	clock_init();           /* Initialize system clock */
	systime = clock_time(); /* Get the current clock time */
	
	static struct timer timer;
	
	/*resolv_conf(&dnsserver);  
	
	
	resolv_query(host);
	serveraddr = resolv_lookup(host);
	if (serveraddr == NULL)
	{
		printf("ERROR RESOLVING HOSTNAME\n");
	}
	*/
	
	/// call, after the uip_udp_new() function has been 
	
	//uiplib_ipaddrconv(server, &serveraddr);
	///udpconn = udp_new((const uip_ipaddr_t *) &serveraddr, uip_htons(port), NULL);
	udpconn = udp_new((const uip_ipaddr_t *) &numaddr, uip_htons(port), NULL);
	
	uip_udp_bind(udpconn, UDP_PORT);
	
for (;;)
{
	systime = clock_time(); /* Get the current clock time */
	printf("%lu\n", systime);
	
	uip_udp_packet_send(udpconn, (const void *) &msg, sizeof(msg));

	/*static clock_time_t time;
	init_simple_udp();
	
	simple_udp_register(&unicast_connection, UDP_PORT, NULL, UDP_PORT, receiver);*/

	printf("packet sent\n");
	
	timer_set(&timer, CLOCK_SECOND);
	
	while(!timer_expired(&timer))
	{}
	//PROCESS_WAIT_UNTIL();
	//PROCESS_WAIT_EVENT();
	//PROCESS_YIELD_UNTIL(timer_expired(&timer));
}
	
	//simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, addr);

	//set_connection_address(&ipaddr);

	/* find the IP of router */
	/*etimer_set(&et, CLOCK_SECOND);
	while(1){
		if(uip_ds6_defrt_choose()){
			uip_ipaddr_copy(&ipaddr, uip_ds6_defrt_choose());
			break;
		}
		etimer_set(&et, CLOCK_SECOND);
		PROCESS_YIELD_UNTIL(etimer_expired(&et));
	}*/

	/* new connection with remote host */
	/*ntp_conn = udp_new(&ipaddr, UIP_HTONS(NTPD_PORT), NULL);

	etimer_set(&et, SEND_INTERVAL * CLOCK_SECOND);
	while(1) {
		PROCESS_YIELD();
		if(etimer_expired(&et)) {
			timeout_handler();
			
			if((clock_seconds() > 4294967290U) || (clock_seconds() < 20)){
	SEND_INTERVAL = 2 * CLOCK_SECOND;
	etimer_set(&et, SEND_INTERVAL);
			} else {
	if(SEND_INTERVAL <= 512 && (getCurrTime() != 0)) {
		SEND_INTERVAL = 2 * SEND_INTERVAL;
	}
	etimer_set(&et, SEND_INTERVAL * CLOCK_SECOND);
			}
		} else if(ev == tcpip_event) {
			tcpip_handler();
		}
	}*/

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
