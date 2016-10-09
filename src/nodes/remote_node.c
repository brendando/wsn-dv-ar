/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/resolv.h"
#include "sys/etimer.h"
#include "board-peripherals.h"
#include "ti-lib.h"
#include "ieee-addr.h"
#include "dev/serial-line.h"
#include "dev/cc26xx-uart.h"


#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define SEND_INTERVAL		5 * CLOCK_SECOND
#define MAX_PAYLOAD_LEN		120

static struct uip_udp_conn *client_conn;
/*---------------------------------------------------------------------------*/
static int hdc1000_humid;
//static int hdc1000_temp;
static int bmp280_pressure;
//static int bmp280_temp;
static int tmp007_temp_obj;
static int tmp007_temp_amb;
static int odt3001_light;
static char addr[17];
static char gps_string[128] = "null";

PROCESS(networking_process, "Networking process");
PROCESS(sensor_process, "Sensor process");
PROCESS(serial_process, "Serial process");
AUTOSTART_PROCESSES(&resolv_process,&networking_process,&sensor_process,&serial_process);
/*---------------------------------------------------------------------------*/
static char buf[MAX_PAYLOAD_LEN];
static void
timeout_handler(void)
{
  int uptime = (int) clock_seconds();
  //remove the checksum to save bytes on tx
  int cap_gps = strlen(gps_string) - 5;
  snprintf(buf, MAX_PAYLOAD_LEN, "{\"seq\":0,\"id\":\"%s\",\"cpu_t\":%d,\"t_obj\":%d,\"t_amb\":%d,\"humid\":%d,\"press\":%d,\"light\":%d}", 
    addr,uptime, tmp007_temp_obj, tmp007_temp_amb, hdc1000_humid, bmp280_pressure, odt3001_light);
  PRINTF("%s\n\r", buf);
  uip_udp_packet_send(client_conn, buf, strlen(buf));
  snprintf(buf, MAX_PAYLOAD_LEN, "{\"seq\":1,\"id\":\"%s\",\"cpu_t\":%d,\"gps\":\"%.*s\"}", 
    addr, uptime, cap_gps, gps_string);
  PRINTF("%s\n\r", buf);
  uip_udp_packet_send(client_conn, buf, strlen(buf));
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Client IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n\r");
    }
  }
}
/*---------------------------------------------------------------------------*/
#if UIP_CONF_ROUTER
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
}
#endif /* UIP_CONF_ROUTER */
/*---------------------------------------------------------------------------*/
static resolv_status_t
set_connection_address(uip_ipaddr_t *ipaddr)
{
#ifndef UDP_CONNECTION_ADDR
#if RESOLV_CONF_SUPPORTS_MDNS
#define UDP_CONNECTION_ADDR       home-base.local
#elif UIP_CONF_ROUTER
#define UDP_CONNECTION_ADDR       aaaa:0:0:0:0212:7404:0004:0404
#else
#define UDP_CONNECTION_ADDR       fe80:0:0:0:6466:6666:6666:6666
#endif
#endif /* !UDP_CONNECTION_ADDR */

#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)

  resolv_status_t status = RESOLV_STATUS_ERROR;

  if(uiplib_ipaddrconv(QUOTEME(UDP_CONNECTION_ADDR), ipaddr) == 0) {
    uip_ipaddr_t *resolved_addr = NULL;
    status = resolv_lookup(QUOTEME(UDP_CONNECTION_ADDR),&resolved_addr);
    if(status == RESOLV_STATUS_UNCACHED || status == RESOLV_STATUS_EXPIRED) {
      PRINTF("Attempting to look up %s\n\r",QUOTEME(UDP_CONNECTION_ADDR));
      resolv_query(QUOTEME(UDP_CONNECTION_ADDR));
      status = RESOLV_STATUS_RESOLVING;
    } else if(status == RESOLV_STATUS_CACHED && resolved_addr != NULL) {
      PRINTF("Lookup of \"%s\" succeded!\n\r",QUOTEME(UDP_CONNECTION_ADDR));
    } else if(status == RESOLV_STATUS_RESOLVING) {
      PRINTF("Still looking up \"%s\"...\n\r",QUOTEME(UDP_CONNECTION_ADDR));
    } else {
      PRINTF("Lookup of \"%s\" failed. status = %d\n\r",QUOTEME(UDP_CONNECTION_ADDR),status);
    }
    if(resolved_addr)
      uip_ipaddr_copy(ipaddr, resolved_addr);
  } else {
    status = RESOLV_STATUS_CACHED;
  }

  return status;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(networking_process, ev, data)
{
  static struct etimer et;
  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();
  PRINTF("Networking process started\n\r");
  
  uint8_t ieeeaddr[8];
  ieee_addr_cpy_to(ieeeaddr, 8);
  //shortened ID
  sprintf(addr, "%02x%02x%02x%02x",
    ieeeaddr[4], ieeeaddr[5], ieeeaddr[6], ieeeaddr[7]);
  //sprintf(addr, "%02x%02x%02x%02x%02x%02x%02x%02x", 
  //  ieeeaddr[0], ieeeaddr[1], ieeeaddr[2], ieeeaddr[3],
  //  ieeeaddr[4], ieeeaddr[5], ieeeaddr[6], ieeeaddr[7]);

#if UIP_CONF_ROUTER
  set_global_address();
#endif

  print_local_addresses();		//Show current address

  static resolv_status_t status = RESOLV_STATUS_UNCACHED;
  while(status != RESOLV_STATUS_CACHED) {
    status = set_connection_address(&ipaddr);

    if(status == RESOLV_STATUS_RESOLVING) {
      PROCESS_WAIT_EVENT_UNTIL(ev == resolv_event_found);
    } else if(status != RESOLV_STATUS_CACHED) {
      PRINTF("Can't get connection address.\n\r");
      PROCESS_YIELD();
    }
  }

  /* new connection with remote host */
  client_conn = udp_new(&ipaddr, UIP_HTONS(3000), NULL);
  udp_bind(client_conn, UIP_HTONS(3001));

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n\r",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  etimer_set(&et, SEND_INTERVAL);
  
  while(1) {
    PROCESS_YIELD();
  
	  //Wait for timer to expire
	  if(etimer_expired(&et)) {
      timeout_handler();
      etimer_restart(&et);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_process, ev, data)
{
  static struct etimer hdc1000_t;
  static struct etimer bmp280_t;
  static struct etimer tmp007_t;
  static struct etimer opt3001_t;

  PROCESS_BEGIN();

  SENSORS_ACTIVATE(hdc_1000_sensor);
  SENSORS_ACTIVATE(bmp_280_sensor);
  SENSORS_ACTIVATE(tmp_007_sensor);
  SENSORS_ACTIVATE(opt_3001_sensor);
  

  while(1) {
    PROCESS_YIELD();
    if(ev == sensors_event) {
      if(data == &hdc_1000_sensor) {
        hdc1000_humid = hdc_1000_sensor.value(HDC_1000_SENSOR_TYPE_HUMIDITY);
        //hdc1000_temp = hdc_1000_sensor.value(HDC_1000_SENSOR_TYPE_TEMP);
        etimer_set(&hdc1000_t, 3 * CLOCK_SECOND);
      } else if(data == &bmp_280_sensor) {
        bmp280_pressure = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_PRESS);
        //bmp280_temp = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_TEMP);
        etimer_set(&bmp280_t, 3 * CLOCK_SECOND);
      } else if(data == &tmp_007_sensor) {
        tmp_007_sensor.value(TMP_007_SENSOR_TYPE_ALL);
        tmp007_temp_obj = tmp_007_sensor.value(TMP_007_SENSOR_TYPE_OBJECT);
        tmp007_temp_amb = tmp_007_sensor.value(TMP_007_SENSOR_TYPE_AMBIENT);
        etimer_set(&tmp007_t, 3 * CLOCK_SECOND);
      } else if(data == &opt_3001_sensor) {
        odt3001_light = opt_3001_sensor.value(0);
        etimer_set(&opt3001_t, 3 * CLOCK_SECOND);
      }
    } else if(ev == PROCESS_EVENT_TIMER) {
      if(etimer_expired(&hdc1000_t)) {
        SENSORS_ACTIVATE(hdc_1000_sensor);
      }
      if(etimer_expired(&bmp280_t)) {
        SENSORS_ACTIVATE(bmp_280_sensor);
      }
      if(etimer_expired(&tmp007_t)) {
        SENSORS_ACTIVATE(tmp_007_sensor);
      }
      if(etimer_expired(&opt3001_t)) {
        SENSORS_ACTIVATE(opt_3001_sensor);
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(serial_process, ev, data)
{
  static char gps_title[] = "$GPGGA";
  PROCESS_BEGIN();

  PRINTF("Serial process started...\n\r");
  cc26xx_uart_set_input(serial_line_input_byte);


  while(1) {
    PROCESS_YIELD();
    if(ev == serial_line_event_message) {
      if(!memcmp(gps_title, data, 6)) {
        PRINTF("%s\n\r", (char *) data);
        strncpy(gps_string, (char *)data, 127);
      }
    }
  }
  PROCESS_END();
}
