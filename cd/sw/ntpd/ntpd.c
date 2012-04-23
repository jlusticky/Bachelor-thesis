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
#include "sys/clock.h"

#include "ntpd.h"

#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"


static const char * host = "aaaa::1"; // NTP server
#define REMOTE_PORT NTP_PORT
#define LOCAL_PORT NTP_PORT

static struct ntp_msg msg;
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

// NTP Poll interval in seconds = 2^TAU
/// TAU ranges from 4 (Poll interval 16 s) to 17 ( Poll interval 36 h)
/// - timer is limited in Contiki to xx s - use etimer vs. stimer
///#ifndef NTP_TAU
	#define TAU 3
	#define POLL_INTERVAL (1 << TAU)
///#endif // TAU

// Send interval in clock ticks
#define SEND_INTERVAL POLL_INTERVAL * CLOCK_SECOND

static struct uip_udp_conn *udpconn;
static clock_time_t clocktime;
static clock_time_t clockseconds;
		
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  struct ntp_msg *pkt;
  static struct timer t;

  if(uip_newdata())
  {
    pkt = uip_appdata;
    
    // check if received packet is complete
    if ((uip_datalen() != NTP_MSGSIZE_NOAUTH) && (uip_datalen() != NTP_MSGSIZE))
    {
		printf("Received malformed NTP packet\n");
		return;
	}

#ifdef NTP_SERVER_SUPPORT
    if ((pkt->status & MODEMASK) == MODE_CLIENT) // we have recieved a query from NTP client
    {
		// set server mode
		msg.status = MODE_SERVER | (NTP_VERSION << 3) | LI_NOWARNING;
		/// TODO switch IP address
		/// ENTER THE TIME WE HAVE AND SEND
		uip_udp_packet_send(udpconn, &msg, sizeof(struct ntp_msg));
		return;
	}
#endif // NTP_SERVER_SUPPORT

    printf("Seconds got from server: %" PRIu32 "\n", pkt->xmttime.int_partl);
    
	/*clocktime = clock_time(); // get the current clock time
	printf("clock_time: %u\n", clocktime);*/
	clockseconds = clock_seconds(); // get the current clock seconds
	printf("clock_seconds: %u\n", clockseconds);
  }
}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(void)
{
	msg.status = MODE_CLIENT | (NTP_VERSION << 3) | LI_NOWARNING;
	msg.ppoll = TAU;
	printf("Sending NTP packet to server\n");
	//PRINT6ADDR(&udpconn->ripaddr);
	uip_udp_packet_send(udpconn, &msg, sizeof(struct ntp_msg));
}
/*---------------------------------------------------------------------------*/
PROCESS(ntpd_process, "ntpd");
AUTOSTART_PROCESSES(&ntpd_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ntpd_process, ev, data)
{	
	static struct etimer et;
	uip_ipaddr_t ipaddr;	
	
	PROCESS_BEGIN();

	// set the NTP server address
#ifdef UIP_CONF_IPV6	
	uip_ip6addr(&ipaddr,0xaaaa,0,0,0,0,0,0,0x0001);
#else
	uip_ipaddr(&ipaddr, 10, 18, 48, 75);
#endif /* UIP_CONF_IPV6 */
	
	/* new connection with remote host */
	udpconn = udp_new(&ipaddr, UIP_HTONS(REMOTE_PORT), NULL); // remote server port
	udp_bind(udpconn, UIP_HTONS(LOCAL_PORT)); // local client port
	
	etimer_set(&et, SEND_INTERVAL);
	for(;;) {
		PROCESS_YIELD();
		if(etimer_expired(&et))
		{
			timeout_handler();
			etimer_restart(&et);
		}
		else if(ev == tcpip_event)
		{
			tcpip_handler();
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
