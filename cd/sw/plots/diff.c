/*
 * This program read file of the following format:
 * nsec\n
 * nsec\n
 *
 * example:
 * 292940
 * -393949230
 *
 * and prints the maximal positive value,
 * minimal negative value.
 *
 * The input format can be obtained using plus.c program.
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
	long long maxdelta = 0LL;
	long long maxpos = 0LL;
	long long minneg = 0LL;
	long long prev = 0LL;
	while (!feof(f))
	{
		long long n;
		fscanf(f, "%lld\n", &n);

		if (n > maxpos)
		{
			maxpos = n;
		}
		else if (n < minneg)
		{
			minneg = n;
		}
		if (llabs(n - prev) > maxdelta)
		{
			maxdelta = llabs(n - prev);
		}
		prev = n;
	}
	printf("Maximal positive value %lld\n", maxpos);
	printf("Minimal negative value %lld\n", minneg);
	printf("Maximal delta = %lld\n", maxdelta);
	return 0;
}
