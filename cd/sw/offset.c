/* This computes the local clock offset */
#include <stdio.h>
#include <inttypes.h>
#include <math.h>

struct time_spec
{
	int32_t sec;
	int32_t nsec;
};

long double
compute(long double dts, long double drec, long double dxmt, long double ddst)
{
	struct time_spec adjts;
	long double dadj;

	struct time_spec ts, rects, xmtts, dstts;
	ts.sec = trunc(dts);
	ts.nsec = 1000000000 * (dts - trunc(dts));
	//ts.nsec = 1000000000 * (dts - trunc(dts)) - 1000000000;
	rects.sec = trunc(drec);
	rects.nsec = 1000000000 * (drec - trunc(drec));
	xmtts.sec = trunc(dxmt);
	xmtts.nsec = 1000000000 * (dxmt - trunc(dxmt));
	dstts.sec = trunc(ddst);
	dstts.nsec = 1000000000 * (ddst - trunc(ddst));

	adjts.sec = ((rects.sec - ts.sec) + (xmtts.sec - dstts.sec)) / 2;
	adjts.nsec = ((rects.nsec - ts.nsec) + (xmtts.nsec - dstts.nsec)) / 2;
	dadj = ((drec - dts) + (dxmt - ddst)) / 2.0;

//	printf("ADJTS.SEC = %d ADJTS.NSEC = %09d\n", adjts.sec, adjts.nsec);
	if (adjts.sec == 0)
	{
		if (adjts.nsec != 0)
		{
/*			printf("ts = %d %09d\n", ts.sec, ts.nsec);
			printf("rects = %d %09d\n", rects.sec, rects.nsec);
			printf("xmtts = %d %09d\n", xmtts.sec, xmtts.nsec);
			printf("dstts = %d %09d\n", dstts.sec, dstts.nsec);*/
		}

		if (ts.sec < rects.sec)
		{
			ts.nsec -= 1000000000;
		}

		if (xmtts.sec < dstts.sec)
		{
			dstts.nsec += 1000000000;
		}

		adjts.nsec = ((rects.nsec - ts.nsec) + (xmtts.nsec - dstts.nsec)) / 2;
//		printf("corrected = %d %+010d\n", adjts.sec, adjts.nsec);
	}
//	printf("DOUBLE ADJ = %.9Lf\n", dadj);
	return dadj;
}

int
main(int argc, char *argv[])
{
//	compute(3.992187500, 4.003053842, 9.001294824, 9.014404280);
/*
	printf("OFFSET = 0s\n");
	printf("---------------------\n");
	compute(33.9,34.0,34.1,34.2); // client sends in different second
	printf("---------------------\n");
	compute(33.8,33.9,34.1,34.2); // client sends and server receives in different second
	printf("---------------------\n");
	compute(33.7,33.9,33.9,34.1); // client sends, server receives and sends in different second
	printf("---------------------\n");

	printf("OFFSET = 0.1s\n");
	printf("---------------------\n");
	compute(33.8,34.0,34.1,34.1); // client sends in different second
	printf("---------------------\n");
	compute(33.7,33.9,34.1,34.1); // client sends and server receives in different second
	printf("---------------------\n");
	compute(33.8,33.8,33.9,34.1); // client sends, server receives and sends in different second
	printf("---------------------\n");
*/

	FILE *f = fopen(argv[1], "r");
	if (f == NULL)
	{
		perror(argv[1]);
		return 1;
	}

	char *buf = NULL;
	size_t n;
	int line = 0;
	long double ts, rec, xmt, dst, myres;
	while (!feof(f))
	{
		getline(&buf, &n, f);
		line++;
		if (buf[67] == '-')
		{
			buf[67] = '0';
			buf[64] = '-';
		}
		sscanf(buf, "%Lf %Lf %Lf %Lf %Lf\n", &rec, &ts, &xmt, &dst, &myres);
//		printf("%.9Lf %.9Lf %.9Lf %.9Lf %.9Lf\n", rec, ts, xmt, dst, myres);
//		sscanf(buf, "%Lf", &myres);
		long double res = compute(ts, rec, xmt, dst);
		if (fabsl(res - myres) > (long double)0.0000000001)
		{
			printf("line %d: res - myres = %.9Lf - %.9Lf = %.9Lf\n", line, res, myres, res - myres);
		}
	}

	return 0;
}
