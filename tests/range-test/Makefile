CONTIKI_PROJECT=range-test-server
all: $(CONTIKI_PROJECT)
WORKING_DIR=$(CURDIR)

CONTIKI = $(WORKING_DIR)/../../contiki
CONTIKI_WITH_IPV6 = 1
#UIP_CONF_ROUTER = 0
CFLAGS += -DUIP_CONF_ND6_SEND_NA=1
include $(CONTIKI)/Makefile.include


prog: 
	$(HOME)/Downloads/DSLite/DebugServer/bin/DSLite load -c ~/Downloads/DSLite/CC2650F128_TIXDS110_Connection.ccxml -f $(CONTIKI_PROJECT).elf

progu: 
	$(HOME)/uniflash_3.4/uniflash.sh -ccxml ~/Downloads/sensortag.ccxml -targetOp reset -operation Erase
	$(HOME)/uniflash_3.4/uniflash.sh -ccxml ~/Downloads/sensortag.ccxml -program $(CONTIKI_PROJECT).hex
