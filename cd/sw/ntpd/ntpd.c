/*
 * NTP client implementation for Contiki
 *
 * NTPv4 - RFC 5905
 *
 * Copyright (c) 2011, 2012 Josef Lusticky
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

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

/* Change ports to non-standard values - NTP_PORT is defined in ntpd.h */
#define REMOTE_PORT NTP_PORT
#define LOCAL_PORT NTP_PORT

/* NTP client uses clock_set_time if the local clock offset is
 * equal or greater than ADJUST_THRESHOLD seconds.
 */
#define ADJUST_THRESHOLD 3

/* If remote host is defined, assuming NTP unicast mode.
 * Client is active and sends ntp_msg to NTP server, otherwise no ntp_msg is needed.
 */
#ifdef REMOTE_HOST
static struct ntp_msg msg;

struct time_spec ts;

/* NTP Poll interval in seconds = 2^TAU
 * In NTPv4 TAU ranges from 4 (Poll interval 16 s) to 17 ( Poll interval 36 h)
 */
/// CAUTION: etimer is limited in Contiki to cca 500s (platform specific)
/// so do not use TAU > 8
#ifndef TAU
#define TAU 4
#endif /* TAU */

#else
#warning "No REMOTE_HOST defined - only NTP broadcast messages will be processed!"
#endif /* REMOTE_HOST */

static struct uip_udp_conn *udpconn;

/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  struct ntp_msg *pkt;          // pointer to incomming packet

  /* timestamps for offset calculation */
  // t1 == ts
#ifdef REMOTE_HOST              // variables needed only for NTP unicast mode
  struct time_spec rects;       // t2

  struct time_spec xmtts;       // t3
#endif /* REMOTE_HOST */
  struct time_spec dstts;       // t4

  /* timestamp for local clock adjustment */
  struct time_spec adjts;

  if(uip_newdata()) {
    // get destination (t4) timestamp
    clock_get_time(&dstts);

    // check if received packet is complete
    if((uip_datalen() != NTP_MSGSIZE_NOAUTH)
       && (uip_datalen() != NTP_MSGSIZE)) {
      PRINTF("Received malformed NTP packet\n");
      return;
    }

    pkt = uip_appdata;

    // check if the server is synchronised
#if 0                           // change to 1 for strict check
    if(((pkt->status & LI_ALARM) == LI_ALARM)
       || (pkt->stratum > NTP_MAXSTRATUM) || (pkt->stratum == 0)
       || ((pkt->xmttime.int_partl) == (uint32_t) 0))
#else
    if((pkt->stratum > NTP_MAXSTRATUM)
       || (pkt->xmttime.int_partl) == (uint32_t) 0)
#endif
    {
      PRINTF("Received NTP packet from unsynchronised server\n");
      return;
    }

    /* Compute adjustment */
    if((pkt->status & MODEMASK) == MODE_BROADCAST)      // in broadcast mode compute time from xmt and dst
    {
      // local clock offset THETA = t3 - t4
      adjts.sec = (uip_ntohl(pkt->xmttime.int_partl) - JAN_1970) - dstts.sec;
      adjts.nsec =
        fractionl_to_nsec(uip_htonl(pkt->xmttime.fractionl)) - dstts.nsec;
    }
#ifndef REMOTE_HOST             // if only NTP broadcast mode supported
    else                        // in broadcast only mode, no other calcualtion is possible
    {
      PRINTF("Received NTP non-broadcast mode message\n");
      return;
    }
#else
    else                        // in client-server mode calculate local clock offset
    {
      if(ts.sec != (uip_ntohl(pkt->orgtime.int_partl) - JAN_1970)) {
        PRINTF
          ("Orgtime mismatch between received NTP packet and timestamp sent by us\n");
        return;
      }

      /* Compute local clock offset THETA = ((t2 - t1) + (t3 - t4)) / 2
       * for seconds part
       */
      rects.sec = uip_htonl(pkt->rectime.int_partl) - JAN_1970;
      xmtts.sec = uip_htonl(pkt->xmttime.int_partl) - JAN_1970;
      adjts.sec = ((rects.sec - ts.sec) + (xmtts.sec - dstts.sec)) / 2;

      /* Compute local clock offset for fraction part */
      rects.nsec = fractionl_to_nsec(uip_htonl(pkt->rectime.fractionl));
      xmtts.nsec = fractionl_to_nsec(uip_htonl(pkt->xmttime.fractionl));

      /* Correct fraction parts if seconds are adjacent */
      if(adjts.sec == 0) {
        if(ts.sec < rects.sec)  // server received packet in other second
        {
          ts.nsec -= 1000000000;
        }
        if(xmtts.sec < dstts.sec)       // our client received packet in other second
        {
          dstts.nsec += 1000000000;
        }
      }

      adjts.nsec = ((rects.nsec - ts.nsec) + (xmtts.nsec - dstts.nsec)) / 2;
    }

    /* Set our timestamp to zero to avoid processing the same packet more than once */
    ts.sec = 0;
#endif /* ! REMOTE_HOST */

#ifdef DEBUG                    // correct plus minus for printing
    if((adjts.sec < 0) && (adjts.nsec > 0)) {
      adjts.sec = adjts.sec + 1;
      adjts.nsec = adjts.nsec - 1000000000;
    } else if((adjts.sec > 0) && (adjts.nsec < 0)) {
      adjts.sec = adjts.sec - 1;
      adjts.nsec = adjts.nsec + 1000000000;
    }
#endif

    PRINTF("Local clock offset = %ld sec %ld nsec\n", adjts.sec, adjts.nsec);

    /* Set or adjust local clock */
    if(labs(adjts.sec) >= ADJUST_THRESHOLD) {
      PRINTF("Setting the time to xmttime from server\n");
      clock_set_time(uip_ntohl(pkt->xmttime.int_partl) - JAN_1970);
    } else {
      PRINTF("Adjusting the time for %ld and %ld\n", adjts.sec, adjts.nsec);
      clock_adjust_time(&adjts);
    }
  }
}
/*---------------------------------------------------------------------------*/
#ifdef REMOTE_HOST              // this function sends NTP client message to REMOTE_HOST
static void
timeout_handler(void)
{
  msg.status = MODE_CLIENT | (NTP_VERSION << 3) | LI_ALARM;

  clock_get_time(&ts);
  msg.xmttime.int_partl = uip_htonl(ts.sec + JAN_1970);

  uip_udp_packet_send(udpconn, &msg, sizeof(struct ntp_msg));

  PRINTF("Sent timestamp: %ld sec %ld nsec to ", ts.sec, ts.nsec);
#ifdef UIP_CONF_IPV6
  PRINT6ADDR(&udpconn->ripaddr);
#else
  PRINTLLADDR(&udpconn->ripaddr);
#endif /* UIP_CONF_IPV6 */
  PRINTF("\n");
}
#endif /* REMOTE_HOST */
/*---------------------------------------------------------------------------*/
PROCESS(ntpd_process, "ntpd");
AUTOSTART_PROCESSES(&ntpd_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ntpd_process, ev, data)
{
  PROCESS_BEGIN();

#ifndef REMOTE_HOST
  udpconn = udp_new(NULL, UIP_HTONS(REMOTE_PORT), NULL);        // remote server port
#else
  // set the NTP server address
  uip_ipaddr_t ipaddr;

  uiplib_ipaddrconv(REMOTE_HOST, &ipaddr);

  /* new connection with remote host */
  udpconn = udp_new(&ipaddr, UIP_HTONS(REMOTE_PORT), NULL);     // remote server port

#define POLL_INTERVAL (1 << TAU)
  msg.ppoll = TAU;              // log2(poll_interval)

  // Send interval in clock ticks
#define SEND_INTERVAL POLL_INTERVAL * CLOCK_SECOND
#endif /* ! REMOTE_HOST */

  udp_bind(udpconn, UIP_HTONS(LOCAL_PORT));     // local client port

#ifdef REMOTE_HOST              // initial setting of time after startup
  // wait 6s for ip to settle
  static struct etimer et;

  etimer_set(&et, 6 * CLOCK_SECOND);
  PROCESS_WAIT_EVENT();

  // ask for time
  msg.refid = UIP_HTONL(0x494e4954);    // INIT string in ASCII
  timeout_handler();
  msg.refid = 0;

  etimer_set(&et, SEND_INTERVAL);       // wait SEND_INTERVAL before sending next request
#endif

  for(;;) {
    PROCESS_WAIT_EVENT();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
#ifdef REMOTE_HOST
    else if(etimer_expired(&et)) {
      timeout_handler();
      etimer_restart(&et);      // wait again SEND_INTERVAL seconds
    } else if(ev == PROCESS_EVENT_MSG)  // another application wants us to synchronise
    {
      timeout_handler();
    }
#endif
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
