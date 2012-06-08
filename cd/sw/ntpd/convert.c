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

#include <inttypes.h>

#include "sys/clock.h"

#include "ntpd.h"

void
ntp_to_ts(const struct l_fixedpt *ntp, struct time_spec *ts)
{
    ts->sec = ntp->int_partl - JAN_1970;
	ts->nsec = fractionl_to_nsec(ntp->fractionl);
}

unsigned long
fractionl_to_nsec(uint32_t fractionl)
{
	unsigned long nsec;
	nsec = fractionl;
#if 0
	/*
	 * We need to compute i * 1000000000 / 2^32.
	 * Greatest common divisor of 1000000000 and 2^32 is 2^9, therefore
	 * i * (1000000000 / 2^9) / (2^32 / 2^9) = i * 1953125 / 8388608,
	 * which is equal to i * 5^9 / 2^23.
	 * This can be done using sequential division and multiplication,
	 * which in turn can be done using shifts and additions.
	 */
	nsec = (nsec >> 1) + (nsec >> 3); // nsec = nsec/2 + nsec/8 = (5*nsec) / 8
	nsec = (nsec >> 1) + (nsec >> 3); // nsec = (5*nsec) / 8 = (25*i) / 64
	nsec = (nsec >> 1) + (nsec >> 3); // (125*i) / 512 = (5^3*i) / 2^9
	
	/* Now we can multiply by 5^2 because then the total
	 * multiplication coefficient for the original number i
	 * will be: i * (1/(2^3)^4)*5^5 = i * 0.762939453,
	 * which is less then 1, so it can not overflow.
	 */
	nsec = (nsec << 1) + nsec + (nsec >> 3); // nsec*3 + nsec/8 = (25*nsec) / 8

	nsec = (nsec >> 1) + (nsec >> 3);
	nsec = (nsec >> 1) + (nsec >> 3);
	
	/* Again we can multiply by 5^2.
	 * Total coefficient will be i * (1/(2^3)^7)*5^9 = i * 0.931322575
	 */
	nsec = (nsec << 1) + nsec + (nsec >> 3); // nsec*3 + nsec/8 = (25*nsec) / 8

	/* Last shift to agree with division by 2^23 can not be
	 * done earlier since coefficient would always be greater than 1.
	 */
	nsec = nsec >> 2;
#elif 0
	nsec = ((double)nsec * 1000000000) / 0xFFFFFFFF;
#else
	nsec = ((uint64_t)nsec * 1000000000) >> 32;
#endif
	return nsec;
}
