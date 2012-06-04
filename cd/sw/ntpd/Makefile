all: ntpd

UIP_CONF_IPV6=1

CFLAGS+=-DMAC_ADDRESS="{0x02, 0x11, 0x22, 0xff, 0xfe, 0x33, 0x44, 0x55}"
CFLAGS+=-D'REMOTE_HOST="aaaa::1"'
#CFLAGS+=-D'REMOTE_HOST="bbbb::260:6eff:fe7a:d4b8"'
#CFLAGS+=-D'HARD_CODED_ADDRESS="aaaa::11:22ff:fe33:4455"' # try it

CONTIKI=${HOME}/contiki
include $(CONTIKI)/Makefile.include

AVRDUDE_PROGRAMMER := -c dragon_jtag
