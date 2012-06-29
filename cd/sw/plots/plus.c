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
