CONTIKI_PROJECT=remote_node
all: $(CONTIKI_PROJECT)
WORKING_DIR=$(CURDIR)

CONTIKI = $(WORKING_DIR)/../../contiki
DEFINES=CC26XX_UART_CONF_ENABLE
ifeq ($(CONTIKI_PROJECT),remote_node)
	DEFINES+= CC26XX_UART_CONF_BAUD_RATE=9600
  $(info $$DEFINES is [${DEFINES}])
endif
CONTIKI_WITH_IPV6 = 1
#UIP_CONF_ROUTER = 0
CFLAGS += -DUIP_CONF_ND6_SEND_NA=1
include $(CONTIKI)/Makefile.include

prog: 
	$(HOME)/Downloads/DSLite/DebugServer/bin/DSLite load -c ~/Downloads/DSLite/CC2650F128_TIXDS110_Connection.ccxml -f $(CONTIKI_PROJECT).elf
