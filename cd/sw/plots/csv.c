#include <stdio.h>
#include <stdlib.h>

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
