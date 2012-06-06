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
#if 1 /* highest precision but slowest */
	nsec = nsec >> 4;
	nsec = nsec * 10;
	nsec = nsec >> 4;
	nsec = nsec * 10;
	nsec = nsec >> 4;
	nsec = nsec * 10;
	nsec = nsec >> 4;
	nsec = nsec * 10;
	nsec = nsec >> 4;
	nsec = nsec * 100; // now we can multiply by 100 without overflow
	nsec = nsec >> 4;
	nsec = nsec * 10;
	nsec = nsec >> 4;
	nsec = nsec * 10;
	nsec = nsec >> 4;
	nsec = nsec * 10;
#elif 0
	nsec = nsec >> 8;
	nsec = nsec * 100;
	nsec = nsec >> 8;
	nsec = nsec * 100;
	nsec = nsec >> 8;
	nsec = nsec * 100;
	nsec = nsec >> 8;
	nsec = nsec * 1000;
#else /* lowest precision but fastest */
	nsec = nsec >> 16;
	nsec = nsec * 10000;
	nsec = nsec >> 16;
	nsec = nsec * 100000;
#endif
	return nsec;
}
