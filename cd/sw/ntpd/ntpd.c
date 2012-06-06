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

#include <stdlib.h>

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "sys/clock.h"

#include "ntpd.h"

//#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"


// error if remote NTP server not defined in Makefile
#ifndef REMOTE_HOST
/// TODO : broadcast mode - move to assigning addresses + remove TAU and POLL INTERVAL
	#warning "No REMOTE_HOST defined in Makefile - only broadcast mode will work!"
#endif

// change ports to non-standard values - NTP_PORT is defined in ntpd.h
#define REMOTE_PORT NTP_PORT
#define LOCAL_PORT NTP_PORT

static struct ntp_msg msg;

struct time_spec ts;

// NTP Poll interval in seconds = 2^TAU
/// TAU ranges from 4 (Poll interval 16 s) to 17 ( Poll interval 36 h) - multiply as in RFC1361?
/// - timer is limited in Contiki to xx s - use etimer vs. stimer
#define TAU 4
#define POLL_INTERVAL (1 << TAU)

// Send interval in clock ticks
#define SEND_INTERVAL POLL_INTERVAL * CLOCK_SECOND

static struct uip_udp_conn *udpconn;

/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  struct ntp_msg *pkt; // pointer to incomming packet
  
  /* timestamps for offset calculation */
  ///struct time_spec orgts; // t1 == ts
  struct time_spec rects; // t2
  struct time_spec xmtts; // t3
  struct time_spec dstts; // t4
  
  /* timestamp for local clock adjustment */
  struct time_spec adjts;

  if(uip_newdata())
  {
	// get destination (t4) timestamp
	clock_get_time(&dstts);
	
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
		// set server mode and send our time
		pkt->status = MODE_SERVER | (NTP_VERSION << 3) | LI_ALARM;
		pkt->xmttime.int_partl = uip_htonl(dstts.sec + JAN_1970);
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
    
    /* Compute adjustment */
    adjts.nsec = 0;
    if ((pkt->status & MODEMASK) == MODE_BROADCAST) // in broadcast mode set time to xmttime
    {
	    // local clock offset THETA = t3 - t4
	    adjts.sec = (uip_ntohl(pkt->xmttime.int_partl) - JAN_1970) - dstts.sec;
	    
	    if (adjts.sec == 0) // if seconds are the same calculate nsec offset
	    {
			adjts.nsec = fractionl_to_nsec(uip_htonl(pkt->xmttime.fractionl)) - dstts.nsec;
		}
	}
	else // in client-server mode calculate local clock offset
	{
		if (ts.sec != (uip_ntohl(pkt->orgtime.int_partl) - JAN_1970))
		{
			PRINTF("Orgtime mismatch between received NTP packet and timestamp sent by us\n");
			return;
		}
		
		/* Compute local clock offset THETA = ((t2 - t1) + (t3 - t4)) / 2
		 * only for seconds part.
		 * If seconds offset is zero, compute nsec offset later.
		 */
		rects.sec = uip_htonl(pkt->rectime.int_partl) - JAN_1970;
		xmtts.sec = uip_htonl(pkt->xmttime.int_partl) - JAN_1970;
		
		PRINTF("SECONDS: org = %ld, rec = %ld, xmt = %ld, dst = %lu\n", ts.sec, rects.sec, xmtts.sec, dstts.sec); 
		PRINTF("THETA = ((%ld - %ld) + (%ld - %ld)) / 2\n", rects.sec, ts.sec, xmtts.sec, dstts.sec);
		
		adjts.sec = ((rects.sec - ts.sec) // TODO look at assembly for DIV vs. shift
					+ (xmtts.sec - dstts.sec)) / 2; // dstts.sec + 1 = 0
		
		PRINTF("Local clock offset = %ld sec\n",  adjts.sec);
		
		if (adjts.sec == 0) // if seconds offset is zero calculate nsec offset
		{
			rects.nsec = fractionl_to_nsec(uip_htonl(pkt->rectime.fractionl));
			xmtts.nsec = fractionl_to_nsec(uip_htonl(pkt->xmttime.fractionl));
			
			PRINTF("THETA = ((%ld - %ld) + (%ld - %ld)) / 2\n", rects.nsec, ts.nsec, xmtts.nsec, dstts.nsec);
			
			adjts.nsec = ((rects.nsec - ts.nsec) + (xmtts.nsec - dstts.nsec)) / 2; // TODO as above
			PRINTF("Local clock offset = %ld nsec\n", adjts.nsec);
		}
	}

	/* Set or adjust local clock */
    if (labs(adjts.sec) > 3) /// do this only IF difference > 36min use settime, otherwise adjtime
    {
		PRINTF("Setting the time to xmttime from server\n");
		clock_set_time(uip_ntohl(pkt->xmttime.int_partl) - JAN_1970);
	}
	else
	{
		printf("Adjusting the time for %ld and %ld\n", adjts.sec, adjts.nsec);
		clock_adjust_time(&adjts);
	}
  }
}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(void)
{
	msg.status = MODE_CLIENT | (NTP_VERSION << 3) | LI_ALARM;
	
	clock_get_time(&ts);
	msg.xmttime.int_partl = uip_htonl(ts.sec + JAN_1970);
	
	PRINTF("Sending NTP packet to server ");
#ifdef UIP_CONF_IPV6
	PRINT6ADDR(&udpconn->ripaddr);
#else
	PRINTLLADDR(&udpconn->ripaddr);
#endif /* UIP_CONF_IPV6 */
	PRINTF("\n");
	
	PRINTF("Sent timestamp: %ld sec %ld nsec\n", ts.sec, ts.nsec);
	
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
	/**int b = CLOCK_SECOND;// * (OCR2A + 1); // CLOCK_SECOND * OCR2A
	int a;
	for (a = 0; b > 1; a--, b >>= 1)
		{}
	msg.precision = a;*/
	
#if 1 // initial setting of time after startup
	// wait 6s for ip to settle
	etimer_set(&et, 6 * CLOCK_SECOND);
	PROCESS_WAIT_EVENT();
	
	// ask for time
	msg.refid = UIP_HTONL(0x494e4954); // INIT string in ASCII
	timeout_handler();
	msg.refid = 0;
#endif

	etimer_set(&et, SEND_INTERVAL);
	for(;;) {
		PROCESS_WAIT_EVENT();
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
