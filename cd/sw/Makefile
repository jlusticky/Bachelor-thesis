.PHONY: clean all

all: conversion sntp

conversion: conversion.c
	gcc -std=gnu99 -Wextra -Wall conversion.c -g -o conversion

sntp: sntp.c
	gcc -lrt -std=gnu99 -Wextra -Wall sntp.c -g -o sntp

clean:
	rm -f sntp conversion
