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
#include <stdlib.h>

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"


// error if remote NTP server not defined in Makefile
#ifndef REMOTE_HOST
	#error "No REMOTE_HOST defined in Makefile!"
#endif

// change ports to non-standard values - NTP_PORT is defined in ntpd.h
#define REMOTE_PORT NTP_PORT
#define LOCAL_PORT NTP_PORT


// Pointer to a single global uIP buffer for packets
// UIP_LLH_LEN is Lower-Layer-Header Length (14 for ethernet)
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

static struct ntp_msg msg;

struct time_spec ts;

// NTP Poll interval in seconds = 2^TAU
/// TAU ranges from 4 (Poll interval 16 s) to 17 ( Poll interval 36 h) - multiply as in RFC1361?
/// - timer is limited in Contiki to xx s - use etimer vs. stimer
///#ifndef NTP_TAU
	#define TAU 4
	#define POLL_INTERVAL (1 << TAU)
///#endif // TAU

// Send interval in clock ticks
#define SEND_INTERVAL POLL_INTERVAL * CLOCK_SECOND

static struct uip_udp_conn *udpconn;

static void timeout_handler(void);

/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  struct ntp_msg *pkt;
  struct time_spec tmpts;

  if(uip_newdata())
  {
    // check if received packet is complete
    if ((uip_datalen() != NTP_MSGSIZE_NOAUTH) && (uip_datalen() != NTP_MSGSIZE))
    {
		PRINTF("Received malformed NTP packet\n");
		return;
	}
	
    pkt = uip_appdata;
#if 0 // NTP_SERVER_SUPPORT - can only communicate with REMOTE_HOST
	if ((pkt->status & MODEMASK) == MODE_CLIENT)
	{
		pkt->status = MODE_SERVER | (NTP_VERSION << 3) | LI_ALARM;
		clock_get_time(&tmpts);
		pkt->xmttime.int_partl = uip_htonl(tmpts.sec + JAN_1970);
		uip_udp_packet_send(udpconn, pkt, sizeof(struct ntp_msg));
		return;
	}
#endif

	// check if the server is synchronised
#if 0 // change to 1 for strict check
    if (((pkt->status & LI_ALARM) == LI_ALARM) || (pkt->stratum > NTP_MAXSTRATUM) ||
		(pkt->stratum == 0) || ((pkt->xmttime.int_partl) == (uint32_t) 0))
#else
    if ((pkt->stratum > NTP_MAXSTRATUM) || (pkt->xmttime.int_partl) == (uint32_t) 0)
#endif
    {
		PRINTF("Received NTP packet from unsynchronised server\n");
		return;
	}
    
    clock_get_time(&tmpts);
    
    /* Compute adjustment */
    long sec_diff;
    if ((pkt->status & MODEMASK) == MODE_BROADCAST)
    {
	    /* Substract and cast to signed type.
	     * This will work until 2038 when wrap around can occur,
	     * but as NTP Era 0 ends 2036 this code must be in the future changed anyway.
	     */
	    // local clock offset THETA = t3 - t4
	    sec_diff = (long) ((uip_ntohl(pkt->xmttime.int_partl) - JAN_1970) - tmpts.sec);
	}
	else
	{
		if (ts.sec != (uip_ntohl(pkt->orgtime.int_partl) - JAN_1970))
		{
			PRINTF("Received NTP packet is not for us\n");
			return;
		}
		PRINTF("\n");
		// local clock offset THETA = ((t2 - t1) + (t3 - t4)) / 2
		PRINTF("THETA = ((%lu - %lu) + (%lu - %lu)) / 2\n",
		(uip_ntohl(pkt->rectime.int_partl) - JAN_1970), ts.sec,
		(uip_ntohl(pkt->xmttime.int_partl) - JAN_1970), tmpts.sec);
		
		sec_diff = (long) ((long)(uip_ntohl(pkt->rectime.int_partl) - JAN_1970 - ts.sec)
		+ (long)(uip_ntohl(pkt->xmttime.int_partl) - JAN_1970 - tmpts.sec)) >> 1;
		
		PRINTF("Local clock offset = %ld\n",  sec_diff);
	}
	
	/* Set or adjust local clock */
    if (labs(sec_diff) > 5)
    {
	/// do this only IF difference > 36min use settime, otherwise adjtime
		PRINTF("Setting the time to xmttime from server = %lu\n", tmpts.sec, sec_diff);
		clock_set_time(uip_ntohl(pkt->xmttime.int_partl) - JAN_1970);
		
		///msg.xmttime.int_partl = uip_htonl(0x534554);
		///uip_udp_packet_send(udpconn, &msg, sizeof(struct ntp_msg));
	}
	else
	{
		PRINTF("Adjusting the time\n");
		clock_adjust_time(sec_diff);
	}
  }
}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(void)
{
	msg.status = MODE_CLIENT | (NTP_VERSION << 3) | LI_ALARM; ///LI_NOWARNING; - NOT SYNCHRONISED
	
	clock_get_time(&ts);
	msg.xmttime.int_partl = uip_htonl(ts.sec + JAN_1970);
	
	PRINTF("Sending NTP packet to server ");
	PRINT6ADDR(&udpconn->ripaddr); // PRINTLLADDR for ipv4
	PRINTF("\n");
	
	uip_udp_packet_send(udpconn, &msg, sizeof(struct ntp_msg));
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("NTP Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
    }
  }
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

	print_local_addresses();

	// set the NTP server address
#ifdef UIP_CONF_IPV6
	//uip_ip6addr(&ipaddr,0xaaaa,0,0,0,0,0,0,0x1);
	uiplib_ipaddrconv(REMOTE_HOST, &ipaddr);
	//#error "WTF"
#else
	#error "IPv4 support not implemented"
#endif /* UIP_CONF_IPV6 */

	/* new connection with remote host */
	udpconn = udp_new(&ipaddr, UIP_HTONS(REMOTE_PORT), NULL); // remote server port

	udp_bind(udpconn, UIP_HTONS(LOCAL_PORT)); // local client port

	msg.ppoll = TAU; // log2(poll_interval)
	
	// set clock precision - convert Hz to log2 - borrowed from OpenNTPD
	int b = CLOCK_SECOND;// * (OCR2A + 1); // CLOCK_SECOND * OCR2A - HOW IS IT COMPILED?
	int a;
	for (a = 0; b > 1; a--, b >>= 1)
		{}
	msg.precision = a;
	
	/*
	// ask for time and wait for response
	msg.refid = UIP_HTONL(0x494e4954); // INIT string in ASCII
	timeout_handler();
	PROCESS_YIELD();
	if (ev == tcpip_event)
	{
		tcpip_handler();
	}
	*/
	
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
