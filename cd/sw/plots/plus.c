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
	char *b = malloc(1024);
	while (!feof(f))
	{
		long long i;
		fscanf(f, "%lld %s\n", &i, b);
		printf("%lld %s\n", i + 1554516735000, b);
		
		
		
		/*
		m++;
		if (m == atoi(argv[1]))
		{
			printf("%lld\n", i);
			m = 0;
		}
		*/
	}
	return 0;
}
