/*
 * This program read file of the following format:
 * time bus\n
 * time bus\n
 *
 * example:
 * 292940 00
 * 292950 10
 * 292970 11
 * 293080 10
 *
 * where bus is the state of logic analyser bus (binary form)
 * and time is the bus capture time,
 *
 * and prints the time differences between
 * the rising edge of both signals.
 *
 * The input format can be obtained as a csv file from
 * the TechTools logic analyser.
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * Bus state - bits.
 * GPS clock connected to bit 0,
 * AVR Raven connected to bit 1.
 */
#define NONE 00
#define GPS 01
#define RAVEN 10
#define BOTH 11

/*
 * Positive offset = clock is behind the current time
 * PLOT WITH 1s INTERVAL BETWEEN MEASUREMENTS!
 */

int
main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "File name required\n");
		return 1;
	}
	FILE *f = fopen(argv[1], "r");
	if (f == NULL)
	{
		perror(argv[1]);
		return 1;
	}
	
	long long i, gps, raven;
	int bus = 0, prev = 0;
	while (!feof(f))
	{
		fscanf(f, "%lld %d\n", &i, &bus);
		
		if (bus == RAVEN)
		{
			raven = i;
		}
		else if (bus == GPS)
		{
			gps = i;
		}
		else if (bus == BOTH)
		{
			if (prev == GPS)
			{
				printf("%lld\n", i - gps);
			}
			else if (prev == RAVEN)
			{
				printf("%lld\n", raven - i);
			}
			else
			{
				printf("0");
			}
		}
		
		
		prev = bus;
	}
	
	return 0;
}
