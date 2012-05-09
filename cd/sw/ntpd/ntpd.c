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

// Pointer to a single global uIP buffer for packets
// UIP_LLH_LEN is Lower-Layer-Header Length (14 for ethernet)
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

//static const char * host = "aaaa::1"; // NTP server
#define REMOTE_PORT NTP_PORT
#define LOCAL_PORT NTP_PORT

static struct ntp_msg msg;

struct time_spec ts;

// NTP Poll interval in seconds = 2^TAU
/// TAU ranges from 4 (Poll interval 16 s) to 17 ( Poll interval 36 h)
/// - timer is limited in Contiki to xx s - use etimer vs. stimer
///#ifndef NTP_TAU
	#define TAU 4
	#define POLL_INTERVAL (1 << TAU)
///#endif // TAU

// Send interval in clock ticks
#define SEND_INTERVAL POLL_INTERVAL * CLOCK_SECOND

static struct uip_udp_conn *udpconn;
static clock_time_t clocktime;
static clock_time_t clockseconds;

/**
static void
ntp_hton(struct ntp_msg *msg)
{
    msg->refid = htonl(msg->refid);
    s_fixedpt_hton(&msg->rootdelay);
    s_fixedpt_hton(&msg->dispersion);
    l_fixedpt_hton(&msg->reftime);
    l_fixedpt_hton(&msg->orgtime);
    l_fixedpt_hton(&msg->rectime);
    l_fixedpt_hton(&msg->xmttime);
    msg->keyid = htonl(msg->keyid);
}
**/

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
    
    // check if the server is synchronised
    ///if (((pkt->status & LI_ALARM) == LI_ALARM) || (pkt->stratum > NTP_MAXSTRATUM)) /// || (pkt->stratum == 0))
    if (pkt->stratum > NTP_MAXSTRATUM)
    {
		PRINTF("Received NTP packet from unsynchronised server\n");
		return;
	}
    
    ts.sec = uip_htonl(pkt->xmttime.int_partl) - JAN_1970;
    //ts.nsec;
    
    clock_get_time(&tmpts);
    
    if (abs(ts.sec - tmpts.sec) > 1) // uint vs. int
    {
	/// do this only IF difference > 36min use settime
		clock_set_time(ts.sec);
		
		PRINTF("Setting the time\n");
		
		///msg.xmttime.int_partl = uip_htonl(0x534554);
		///uip_udp_packet_send(udpconn, &msg, sizeof(struct ntp_msg));
	}
        
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
	msg.status = MODE_CLIENT | (NTP_VERSION << 3) | LI_ALARM; ///LI_NOWARNING; - NOT SYNCHRONISED
	msg.ppoll = TAU; // log2(poll_interval)
	msg.precision = -7; /// %! 2 na precision = rozliseni hodin // 2**-7 => 1/128 = 1/CLOCK_SECOND
	//msg.refid = UIP_HTONL(0x494e4954); // INIT string in ASCII - only for the first time - delete?
	
	clock_get_time(&ts);
	msg.xmttime.int_partl = uip_htonl(ts.sec + JAN_1970);
	
	PRINTF("Sending NTP packet to server ");
	PRINT6ADDR(&udpconn->ripaddr);
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
	//uip_ip6addr(&ipaddr,0xaaaa,0,0,0,0x0260,0x6eff,0xfe7a,0xd4b8);
	uip_ip6addr(&ipaddr,0xaaaa,0,0,0,0,0,0,0x1);
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
