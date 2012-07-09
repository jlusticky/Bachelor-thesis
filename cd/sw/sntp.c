/*
 * Testing program for sending NTP messages to a specified host
 * by IPv6 or IPv4 address.
 *
 * Program send client, server or broadcast NTP message.
 * In client mode program waits for NTP server response.
 * Client mode is default.
 * Broadcast mode message is sent only to the specified host.
 *
 * Unprivileged port is used by default unless -p given,
 * which uses NTP_PORT (123) then.
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


/*
 * Numbers and conversions according to RFC 5905
 */
#define NTP_PORT             123
#define	JAN_1970             2208988800UL    /* 1970 - 1900 in seconds */

#define	NTP_VERSION	4
#define	NTP_MAXSTRATUM	15


/*
 * NTP Packet sizes
 */
#define	NTP_DIGESTSIZE		16
#define	NTP_MSGSIZE_NOAUTH	48
#define	NTP_MSGSIZE		(NTP_MSGSIZE_NOAUTH + 4 + NTP_DIGESTSIZE)

/*
 * NTP structures
 */
struct l_fixedpt {
	uint32_t int_partl;
	uint32_t fractionl;
};

struct s_fixedpt {
	uint16_t int_parts;
	uint16_t fractions;
};

/*
 * NTP Packet structure - we are not dealing with digest
 */
struct ntp_msg {
	uint8_t status; /* status of local clock and leap info */
	uint8_t stratum;	/* Stratum level */
	uint8_t ppoll;		/* poll value */
	int8_t precision;
	struct s_fixedpt rootdelay;
	struct s_fixedpt dispersion;
	uint32_t refid;
	struct l_fixedpt reftime;
	struct l_fixedpt orgtime;
	struct l_fixedpt rectime;
	struct l_fixedpt xmttime;
};


/*
 * Macros for packet
 */

/* Leap Second Codes (high order two bits) */
#define	LI_NOWARNING	(0 << 6)	/* no warning */
#define	LI_PLUSSEC	(1 << 6)	/* add a second (61 seconds) */
#define	LI_MINUSSEC	(2 << 6)	/* minus a second (59 seconds) */
#define	LI_ALARM	(3 << 6)	/* alarm condition */

/* Status Masks */
#define	MODEMASK	(7 << 0)
#define	VERSIONMASK	(7 << 3)
#define LIMASK		(3 << 6)

/* Mode values */
#define	MODE_RES0	0	/* reserved */
#define	MODE_SYM_ACT	1	/* symmetric active */
#define	MODE_SYM_PAS	2	/* symmetric passive */
#define	MODE_CLIENT	3	/* client */
#define	MODE_SERVER	4	/* server */
#define	MODE_BROADCAST	5	/* broadcast */
#define	MODE_RES1	6	/* reserved for NTP control message */
#define	MODE_RES2	7	/* reserved for private use */


int bflag, cflag, ival, pflag, sflag;
struct ntp_msg ntpmsg;

void
usage(void)
{
	fprintf(stderr, "%s\n%s\n%s\n%s\n%s\n%s\n",
		"Usage: sntp [-p] [-i n] [-b | -c | -s] address",
		"\t-p\tuse privileged NTP port",
		"\t-i n\tcontinue sending NTP messsage every i seconds",
		"\t-b\tbroadcast NTP message to address",
		"\t-c\tclient NTP message to address (default)",
		"\t-s\tserver NTP message to address");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int ch;
	while ((ch = getopt(argc, argv, "bci:ps")) != -1)
	{
		switch (ch)
		{
			case 'b': // NTP broadcast server mode
				bflag = 1;
				break;
			case 'c': // NTP client mode (default)
				cflag = 1;
				break;
			case 'i': // repeat sending message every i seconds
				ival = (int) strtol(optarg, NULL, 10);
				break;
			case 'p': // privileged port NTP_PORT
				pflag = 1;
				break;
			case 's': // NTP server mode
				sflag = 1;
				break;
			default:
				usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) // only one host can be specified
	{
		usage();
	}
	const char * host = argv[0];

	if (bflag) // NTP broadcast server mode
	{
		ntpmsg.status = MODE_BROADCAST | (NTP_VERSION << 3) | LI_ALARM;
	}
	else if (sflag) // NTP server mode
	{
		ntpmsg.status = MODE_SERVER | (NTP_VERSION << 3) | LI_ALARM;
	}
	else // NTP client mode (default)
	{
		ntpmsg.status = MODE_CLIENT | (NTP_VERSION << 3) | LI_ALARM;
	}

	int family;
	if (strchr(host, ':') != NULL) // IPv6
	{
		family = AF_INET6;
	}
	else // IPv4
	{
		family = AF_INET;
	}

	int sockfd = socket(family, SOCK_DGRAM, 0); // UDP
	if (sockfd == -1)
	{
		perror("socket");
		return 1;
	}

	/* Here can be custom fields written */
	//ntpmsg.stratum = 30;

	/* Packet is about to be sent */
	struct sockaddr_in6 sin6;
	struct sockaddr_in sin4;

	if (pflag)
	{
		if (family == AF_INET6)
		{
			memset(&sin6, 0, sizeof(sin6));
			sin6.sin6_family = AF_INET6;
			sin6.sin6_port = htons(NTP_PORT);
			sin6.sin6_addr = in6addr_any;

			if (bind(sockfd, (const struct sockaddr *) &sin6, sizeof(struct sockaddr_in6)) == -1)
			{
				perror("bind");
				close(sockfd);
				return 1;
			}
		}
		else
		{
			sin4.sin_family = AF_INET;
			sin4.sin_port = htons(NTP_PORT);
			sin4.sin_addr.s_addr = INADDR_ANY;
			if (bind(sockfd, (const struct sockaddr *) &sin4, sizeof(struct sockaddr_in)) == -1)
			{
				perror("bind");
				close(sockfd);
				return 1;
			}
		}
	}

	for (;;) // repeat every i seconds
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);

		ntpmsg.xmttime.int_partl = htonl(ts.tv_sec + JAN_1970);
		ntpmsg.xmttime.fractionl = htonl((double) ts.tv_nsec * 0x100000000UL / 1000000000);

		printf("Sending time %ld sec %ld nsec to %s\n", ts.tv_sec, ts.tv_nsec, host);

		if (family == AF_INET6)
		{
			memset(&sin6, 0, sizeof(sin6));
			sin6.sin6_family = AF_INET6;
			sin6.sin6_port = htons(NTP_PORT);
			inet_pton(AF_INET6, host, &sin6.sin6_addr);

			if (sendto(sockfd, &ntpmsg, sizeof(ntpmsg), 0, (const struct sockaddr *) &sin6, sizeof(struct sockaddr_in6)) == -1)
			{
				perror("sendto");
				close(sockfd);
				return 1;
			}
		}
		else
		{
			memset(&sin4, 0, sizeof(sin4));
			sin4.sin_family = AF_INET;
			sin4.sin_port = htons(NTP_PORT);
			inet_pton(AF_INET, host, &sin4.sin_addr);

			if (sendto(sockfd, &ntpmsg, sizeof(ntpmsg), 0, (const struct sockaddr *) &sin4, sizeof(struct sockaddr_in)) == -1)
			{
				perror("sendto");
				close(sockfd);
				return 1;
			}
		}

		if ((ntpmsg.status & MODEMASK) == MODE_CLIENT) // wait for response
		{
			struct ntp_msg recvmsg;
			if (recv(sockfd, &recvmsg, sizeof(recvmsg), 0) == -1)
			{
				perror("recv");
				close(sockfd);
				return 1;
			}

			printf("NTP time got from %s: %lu sec %lu nsec\n", host, ntohl(recvmsg.xmttime.int_partl) - JAN_1970,
				(unsigned long) (((double) ntohl(recvmsg.xmttime.fractionl) * 1000000000) / 0x100000000UL));
		}

		if (ival == 0) // if i was not specified, terminate after sending one packet
		{
			return 0;
		}
		else
		{
			sleep(ival);
		}
	}
	return 0;
}
