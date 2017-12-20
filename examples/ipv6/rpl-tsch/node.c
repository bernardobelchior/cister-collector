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
#include "common.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 1234

static struct simple_udp_connection unicast_connection;
/*---------------------------------------------------------------------------*/
PROCESS(node_process, "RPL Node");
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

#ifdef CISTERMON_DEBUG
  printf("Data received on port %d from port %d with length %d. Sender: ",
         receiver_port, sender_port, datalen);
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
#endif /* CISTERMON_DEBUG */

  struct sensor_info *info = (struct sensor_info *)data;
  printf("Sensor: %d %d.%d %d.%d\n", info->id, (int)info->temp, (int)((info->temp - (int)info->temp) * 100), (int)info->hum, (int)((info->hum - (int)info->hum) * 100));
}
/*---------------------------------------------------------------------------*/
static void get_sensor_information(struct sensor_info *info)
{
  int analogTemp = sht11_sensor.value(SHT11_SENSOR_TEMP);
  int analogHum = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);

  info->temp = -39.6 + 0.01 * analogTemp;

  float humidity = -4 + 0.0405 * analogHum + (-2.8 * 0.000001) * analogHum * analogHum;
  info->hum = humidity;
  //info->hum = (info->temp - 25) * (0.01 + 0.00008 * analogHum) + humidity;
}
/*---------------------------------------------------------------------------*/
static void
net_init(uip_ipaddr_t *br_prefix)
{
  uip_ipaddr_t global_ipaddr;
  rpl_init();

  if (br_prefix)
  { /* We are RPL root. Will be set automatically
                     as TSCH pan coordinator via the tsch-rpl module */
    memcpy(&global_ipaddr, br_prefix, 16);
    uip_ip6addr(&global_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    uip_ds6_set_addr_iid(&global_ipaddr, &uip_lladdr);
    uip_ds6_addr_add(&global_ipaddr, 0, ADDR_AUTOCONF);
    rpl_set_root(RPL_DEFAULT_INSTANCE, &global_ipaddr);
    rpl_set_prefix(rpl_get_any_dag(), br_prefix, 64);
    rpl_repair_root(RPL_DEFAULT_INSTANCE);
  }

  NETSTACK_MAC.on();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();

  /* 3 possible roles:
     * - role_6ln: simple node, will join any network, secured or not
     * - role_6dr: DAG root, will advertise (unsecured) beacons
     * - role_6dr_sec: DAG root, will advertise secured beacons
     * */
  static int is_coordinator = 0;
  static enum { role_6ln,
                role_6dr,
                role_6dr_sec } node_role;
  node_role = role_6ln;

  /* Set node with ID == 1 as coordinator, convenient in Cooja. */
  if (node_id == 1)
  {
    if (LLSEC802154_CONF_SECURITY_LEVEL)
    {
      node_role = role_6dr_sec;
    }
    else
    {
      node_role = role_6dr;
    }
  }
  else
  {
    node_role = role_6ln;
  }

#ifdef CISTERMON_DEBUG
  printf("Init: node starting with role %s\n",
         node_role == role_6ln ? "6ln" : (node_role == role_6dr) ? "6dr" : "6dr-sec");
#endif /* CISTERMON_DEBUG */

  tsch_set_pan_secured(LLSEC802154_CONF_SECURITY_LEVEL && (node_role == role_6dr_sec));
  is_coordinator = node_role > role_6ln;

  if (is_coordinator)
  {
    uip_ipaddr_t prefix;
    uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    net_init(&prefix);
  }
  else
  {
    net_init(NULL);
  }

#if WITH_ORCHESTRA
  orchestra_init();
#endif /* WITH_ORCHESTRA */

  uip_ipaddr_t addr;

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  uip_ds6_addr_t *ll = uip_ds6_get_link_local(-1);

#ifdef CISTERMON_DEBUG
//print_network_status();
#endif /* CISTERMON_DEBUG */

  /* Print out routing tables every minute */
  etimer_set(&et, CLOCK_SECOND * 1);
  while (1)
  {
    struct sensor_info info;
    info.id = node_id;

    SENSORS_ACTIVATE(sht11_sensor);
    get_sensor_information(&info);
    SENSORS_DEACTIVATE(sht11_sensor);

    uip_ip6addr(&addr, 0xaaaa, 0, 0, 0, 0x200, 0, 0x13b7, 0x7422);

    if (!is_coordinator)
    {
#ifdef CISTERMON_DEBUG
      printf("Sending message to: ");
      uip_debug_ipaddr_print(&addr);
      printf("\n");
#endif /* CISTERMON_DEBUG */
      simple_udp_sendto(&unicast_connection, &info, sizeof(info), &addr);
    }
    PROCESS_YIELD_UNTIL(etimer_expired(&et));
    etimer_reset(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
