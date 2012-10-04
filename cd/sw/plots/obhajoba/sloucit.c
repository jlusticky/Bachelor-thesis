#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	FILE *nontp = fopen("no-ntp", "r");
	if (nontp == NULL)
	{
		perror("no-ntp");
		return 1;
	}
	FILE *ntp = fopen("ntp", "r");
	if (ntp == NULL)
	{
		perror("ntp");
		return 1;
	}
	//char line[200];
	//while (getline(&line, NULL, nontp) != -1)
	long long int i,j;
	while (!feof(nontp))
	{
		fscanf(nontp, "%lld\n", &i);
		fscanf(ntp, "%lld\n", &j);
		printf("%lld  %lld\n", i, j);
	}
	return 0;
}
