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
	int m = 0;
	long long b;
	long long prev = 0LL;
	while (!feof(f))
	{
		long long i;
		fscanf(f, "%lld %lld\n", &i, &b);
//		printf("%lld\n", i*1000000000 + b);
		if (llabs(i*1000000000+b - prev) > 40000000LL)
		{
			printf("%lld\n", prev);
			printf("%lld %lld\n", i, b);
		}
		prev = i*1000000000+b;
	}
	return 0;
}
