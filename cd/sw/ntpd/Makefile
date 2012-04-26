all: ntpd

UIP_CONF_IPV6=1
#CFLAGS += -DNTP_SERVER_SUPPORT
#CFLAGS+=-DRAVEN_LCD_INTERFACE=1
#CFLAGS+=-DMAC_ADDRESS="{0x02, 0x11, 0x22, 0xff, 0xfe, 0x33, 0x44, 0x66}"

CONTIKI=${HOME}/contiki
#CONTIKI=${HOME}/contiki-git
include $(CONTIKI)/Makefile.include

AVRDUDE_PROGRAMMER := -c dragon_jtag
