/*
 * Copyright (c) 2015, SICS Swedish ICT.
 * All rights reserved.
 *
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
 */
/**
 * \file
 *         A RPL+TSCH node able to act as either a simple node (6ln),
 *         DAG Root (6dr) or DAG Root with security (6dr-sec)
 *         Press use button at startup to configure.
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "common.h"
#include "contiki.h"
#include "node-id.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"
#include "net/ip/uip-debug.h"
#include "net/mac/tsch/tsch.h"
#include "simple-udp.h"

/* Sensor information */
#include "dev/sht11/sht11-sensor.h"

#if WITH_ORCHESTRA
#include "orchestra.h"
#endif /* WITH_ORCHESTRA */

#define DEBUG DEBUG_NONE

#include <stdio.h>
#include <string.h>

#define UDP_PORT 1234

static struct simple_udp_connection unicast_connection;
/*---------------------------------------------------------------------------*/
PROCESS(node_process, "CISTERMON Router");
AUTOSTART_PROCESSES(&node_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
}
/*---------------------------------------------------------------------------*/
static void get_sensor_information(struct sensor_info *info)
{
  int analogTemp = sht11_sensor.value(SHT11_SENSOR_TEMP);
  int analogHum = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);

  info->temp = -39.6 + 0.01 * analogTemp;
  info->hum = -4 + 0.0405 * analogHum + (-2.8 * 0.000001) * analogHum * analogHum;

  //info->hum = (info->temp - 25) * (0.01 + 0.00008 * analogHum) + humidity;
}
/*---------------------------------------------------------------------------*/
static void
net_init(void)
{
  // uip_ipaddr_t br_prefix;
  // uip_ipaddr_t global_ipaddr;

  // uip_ip6addr(&br_prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  //rpl_init();
  /* We are RPL root. Will be set automatically
                     as TSCH pan coordinator via the tsch-rpl module */
  //  memcpy(&global_ipaddr, &br_prefix, 16);
  //  uip_ip6addr(&global_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  //  uip_ds6_set_addr_iid(&global_ipaddr, &uip_lladdr);
  //  uip_ds6_addr_add(&global_ipaddr, 0, ADDR_AUTOCONF);
  /*rpl_set_root(RPL_DEFAULT_INSTANCE, &global_ipaddr);
  rpl_set_prefix(rpl_get_any_dag(), &br_prefix, 64);
  rpl_repair_root(RPL_DEFAULT_INSTANCE);*/

  //tsch_set_coordinator(1);

  NETSTACK_MAC.on();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();

#if CISTERMON_DEBUG
  printf("Init: node starting with role 6dr\n");
#endif /* CISTERMON_DEBUG */

  net_init();

#if WITH_ORCHESTRA
  orchestra_init();
#endif /* WITH_ORCHESTRA */

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

#if CISTERMON_DEBUG
  //print_network_status();
#endif /* CISTERMON_DEBUG */

  int msg_id = 0;
  etimer_set(&et, CLOCK_SECOND * MEASUREMENT_INTERVAL);
  while (1)
  {
    struct sensor_info info;
    info.msg_id = msg_id++;
    info.node_id = node_id;

    SENSORS_ACTIVATE(sht11_sensor);
    get_sensor_information(&info);
    SENSORS_DEACTIVATE(sht11_sensor);

    uip_ipaddr_t addr;
    //uip_ip6addr(&addr, 0xaaaa, 0, 0, 0, 0x200, 0, 0x13b7, 0x6cc7);
    uip_ip6addr(&addr, 0xaaaa, 0, 0, 0, 0xc30c, 0, 0, 0x0001);

    simple_udp_sendto(&unicast_connection, &info, sizeof(info), &addr);

#if CISTERMON_DEBUG
    printf("Sent message with ID %d to: ", info.msg_id);
    uip_debug_ipaddr_print(&addr);
    printf("\n");
#endif /* CISTERMON_DEBUG */

    PROCESS_YIELD_UNTIL(etimer_expired(&et));
    etimer_reset(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
