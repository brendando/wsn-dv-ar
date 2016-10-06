/*
 * Author: Brendan Do
 * Original source: udp-server.c
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <string.h>

//debugging includes
#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define UIP_IP_BUF ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define MAX_PAYLOAD_LEN 120

static struct uip_udp_conn *server_conn;

PROCESS(listen_and_forward, "Listen and forward");
AUTOSTART_PROCESSES(&resolv_process, &listen_and_forward);

static void recv_handler(void)
{
  if(uip_newdata()) {
    ((char *)uip_appdata)[uip_datalen()] = 0;
    PRINTF("Received: '%s' (RSSI: %d) from ", (char *)uip_appdata, (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI));
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF("\n");

    memset(&server_conn->ripaddr, 0, sizeof(server_conn->ripaddr));
  }
}

static void print_local_addresses(void)
{
  int i;
  uint8_t state;
  
  PRINTF("Server IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n\r");
    }
  }
}

PROCESS_THREAD(listen_and_forward, ev, data)
{
#if UIP_CONF_ROUTER
  uip_ipaddr_t ipaddr;
#endif

  PROCESS_BEGIN();
  PRINTF("Process started\n\r");

#if RESOLV_CONF_SUPPORTS_MDNS
  resolv_set_hostname("home-base");
#endif

#if UIP_CONF_ROUTER
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
#endif
  
  print_local_addresses();

  server_conn = udp_new(NULL, UIP_HTONS(3001), NULL);
  udp_bind(server_conn, UIP_HTONS(3000));

  while(1) {
    PROCESS_YIELD();

    if(ev == tcpip_event) {
      recv_handler();
    }
  }

  PROCESS_END();
}
