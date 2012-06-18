CONTIKI_PROJECT = ntpd
all: $(CONTIKI_PROJECT)

UIP_CONF_IPV6=1

CFLAGS+=-DMAC_ADDRESS="{0x02, 0x11, 0x22, 0xff, 0xfe, 0x33, 0x44, 0x55}"
CFLAGS+=-D'REMOTE_HOST="aaaa::1"'
#CFLAGS+=-D'REMOTE_HOST="bbbb::260:6eff:fe7a:d4b8"'

PROJECT_SOURCEFILES = convert.c

CONTIKI=${HOME}/contiki
include $(CONTIKI)/Makefile.include

AVRDUDE_PROGRAMMER := -c dragon_jtag
