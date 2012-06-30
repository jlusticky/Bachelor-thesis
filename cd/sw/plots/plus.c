/*
 * This program loads the given file of the following format:
 * sec nsec\n
 * sec nsec\n
 *
 * exmaple:
 * 1 82923993
 * -1 -593929029
 *
 * and prints total count of nanoseconds in the following format:
 * 1082923993
 * -1593929029
 *
 * Contiki NTP Client can print in the input format using serial communication.
 */

#include <stdio.h>
#include <stdlib.h>

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

	long long b;
	while (!feof(f))
	{
		long long i;
		fscanf(f, "%lld %lld\n", &i, &b);
		printf("%lld\n", i*1000000000 + b);
	}
	return 0;
}
