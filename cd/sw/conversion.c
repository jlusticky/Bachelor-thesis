/*
 * NTP Timestamp Conversion
 *
 * This short program converts all posssible values in fraction part of 64-bit
 * NTP timestamp (found in Ref, Org, Rec and Xmt fields in NTP packet)
 * and prints them to screen together with their representation in nanoseconds.
 *
 * First conversion is done using casting to double data type and division.
 * This gives correct result, but requires floating point support and 64 bit
 * dat type (double).
 *
 * Second conversion uses 32 bit, no floating point numbers, logical shift
 * (which effectively divides number) and multiplication.
 * Such conversion gives biggest error of 27 nanoseconds in it's most precise form,
 * which is totaly adequate for most platforms without floating point unit or
 * for platforms where usage of 64bit is expansive (embedded systems).
 *
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

#include <stdio.h>
#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/*
 * By default this program outputs only if bigger error is found.
 * Use define VERBOSE to get output for all ntp timestamps.
 */

// #define VERBOSE

#ifdef VERBOSE
	#define verbose(...) \
		do { \
			printf(__VA_ARGS__); \
		} while (0)
#else
	#define verbose(...) {}
#endif /* VERBOSE */


int main(void)
{
	int biggestdelta = 0;

	printf("NTP\t\tFLOAT\t\tOUR\t\tDELTA\n");

	uint64_t i = 0; // current ntp fraction part (32 bit in packet)
	for (i = 0; i <= 0xFFFFFFFF; i += 1) // try all possible ntp fraction values
	{
		verbose("%" PRIu64, i); // NTP
		uint32_t correct, xmf;
		/*
		 * Compute the correct result using FPU
		 */
		correct = (double)i * 1000000000 / 0xFFFFFFFF; // >> 32
		verbose("\t\t%" PRIu32, correct); // FLOAT - correct result using FPU

		/*
		 * We need to compute i * 1000000000 / 2^32.
		 * Greatest common divisor of 1000000000 and 2^32 is 2^9
		 * i * (1000000000 / 2^9) / (2^32 / 2^9) = i * 1953125 / 8388608
		 * which is equal to i * 5^9 / 2^23.
		 * This can be done using sequential shifts and multiplication.
		 */
#if 0 /* highest precision but slowest */
		xmf = i >> 3;
		xmf = xmf * 5;
		xmf = xmf >> 3;
		xmf = xmf * 5;
		xmf = xmf >> 3;
		xmf = xmf * 5;
		xmf = xmf >> 3;
		/* Now we can multiply by 5^2 because then the total factor
		 * will be (1/(2^3)^4)*5^5 = 0.762939453
		 * which is less then 1, so it can not overflow.
		 */
		xmf = xmf * 25;
		xmf = xmf >> 3;
		xmf = xmf * 5;
		xmf = xmf >> 3;
		xmf = xmf * 5;
		xmf = xmf >> 3;
		/* Again we can multiply by 5^2
		 * factor will be (1/(2^3)^7)*5^9 = 0.931322575
		 */
		xmf = xmf * 25;
		/* Last shift to agree with division by 2^23 can not be
		 * done earlier since factor would always be greater than 1.
		 */
		xmf = xmf >> 2;
#elif 0
		xmf = i >> 5;
		xmf = xmf * 25;
		xmf = xmf >> 6;
		xmf = xmf * 25;
		xmf = xmf >> 6;
		xmf = xmf * 125; // (1/((2^(6*4)))*(25^5) = 0.582076609
		xmf = xmf >> 6;
		xmf = xmf * 25;
/**
		xmf = i >> 8;
		xmf = xmf * 100;
		xmf = xmf >> 8;
		xmf = xmf * 100;
		xmf = xmf >> 8;
		xmf = xmf * 100;
		xmf = xmf >> 8;
		xmf = xmf * 1000;*/
#else /* lowest precision but fastest */
		xmf = i >> 10;
		xmf = xmf * 625;
		xmf = xmf >> 13;
		xmf = xmf * 3125;
#endif
	
		verbose("\t\t%" PRIu32, xmf); // output from our conversion
		
		int delta = correct - xmf;
		verbose("\t\t%d\n", delta);  // difference between FPU and our conversion
		
		if (abs(delta) > biggestdelta) // always output if difference is the biggest
		{
			biggestdelta = delta;
			printf("%" PRIu64, i);
			printf("\t\t%" PRIu32, correct);
			printf("\t\t%" PRIu32, xmf);
			printf("\t\t%d\n", biggestdelta);
		}
	}
	return 0;
}
