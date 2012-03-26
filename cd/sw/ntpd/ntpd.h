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

#ifndef __NTPD_H__
#define __NTPD_H__

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#define NTP_EPOCH            (86400U * (365U * 70U + 17U))
#define NTPD_PORT             123

#define UDP_IP_BUF   ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

/* NTP structures - borrowed from OpenNTPD 4.6 */
struct l_fixedpt {
	uint32_t int_partl;
	uint32_t fractionl;
};

struct s_fixedpt {
	uint16_t int_parts;
	uint16_t fractions;
};

struct ntp_msg {
	u_int8_t status;	/* status of local clock and leap info */
	u_int8_t stratum;	/* Stratum level */
	u_int8_t ppoll;		/* poll value */
	int8_t precision;
	struct s_fixedpt rootdelay;
	struct s_fixedpt dispersion;
	uint32_t refid;
	struct l_fixedpt reftime;
	struct l_fixedpt orgtime;
	struct l_fixedpt rectime;
	struct l_fixedpt xmttime;
};


/* Macros for packet - borrowed from DragonflyBSD's dntpd */

/* Leap Second Codes (high order two bits) */
#define	LI_NOWARNING	(0 << 6)	/* no warning */
#define	LI_PLUSSEC	(1 << 6)	/* add a second (61 seconds) */
#define	LI_MINUSSEC	(2 << 6)	/* minus a second (59 seconds) */
#define	LI_ALARM	(3 << 6)	/* alarm condition */

/*
 *	Mode values
 */
#define	MODE_RES0	0	/* reserved */
#define	MODE_SYM_ACT	1	/* symmetric active */
#define	MODE_SYM_PAS	2	/* symmetric passive */
#define	MODE_CLIENT	3	/* client */
#define	MODE_SERVER	4	/* server */
#define	MODE_BROADCAST	5	/* broadcast */
#define	MODE_RES1	6	/* reserved for NTP control message */
#define	MODE_RES2	7	/* reserved for private use */

#define	JAN_1970	2208988800UL	/* 1970 - 1900 in seconds */

#define	NTP_VERSION	4
#define	NTP_MAXSTRATUM	15

#endif /* __NTPD_H__ */

