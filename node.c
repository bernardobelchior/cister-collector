/*
 * Copyright (c) 2016, Inria.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         An example of Rime/TSCH
 * \author
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 *
 */

#include <stdio.h>
#include "contiki-conf.h"
#include "sys/clock.h"
#include "sys/node-id.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "net/mac/tsch/tsch.h"
#include "dev/sht11/sht11-sensor.h"
#include <math.h>

const linkaddr_t coordinator_addr = {{1, 0}};
const linkaddr_t destination_addr = {{1, 0}};

struct sensor_info
{
  int id;
  unsigned long timestamp;
  int temp;
  int hum;
};

/*---------------------------------------------------------------------------*/
PROCESS(unicast_test_process, "CISTER Collector");
AUTOSTART_PROCESSES(&unicast_test_process);

/*---------------------------------------------------------------------------*/
static void get_sensor_information(struct sensor_info *info)
{
  int analogTemp = sht11_sensor.value(SHT11_SENSOR_TEMP);
  info->temp = -39.600 + 0.01 * analogTemp;

  int analogHum = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);
  int humidity = -4 + 0.0405 * analogHum + (-2.8 * pow(10, -6)) * (pow(analogHum,2));
  info->hum = (info->temp - 25) * (0.01 + 0.00008 * analogHum) + humidity;

  info->timestamp = clock_seconds();
}

/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  struct sensor_info *info = (struct sensor_info *)packetbuf_dataptr();

#if DEBUG
  printf("Received from %u.%u: Temp: %d Hum: %d\n",
         from->u8[0], from->u8[1], info->temp, info->hum);
#else  /* DEBUG */
  printf("Sensor: %d %lu %d %d", info->id, info->timestamp, info->temp, info->hum);
#endif /* DEBUG */
}
/*---------------------------------------------------------------------------*/
static void
sent_uc(struct unicast_conn *ptr, int status, int num_tx)
{
#if DEBUG
  printf("Message sent, status %u, num_tx %u\n",
         status, num_tx);
#endif /* DEBUG */
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static struct unicast_conn uc;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_test_process, ev, data)
{
  PROCESS_BEGIN();

  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
  NETSTACK_MAC.on();

  clock_init();
  SENSORS_ACTIVATE(sht11_sensor);
  unicast_open(&uc, 146, &unicast_callbacks);

  struct sensor_info info;
  info.id = node_id;
  while (1)
  {
    static struct etimer et;

    etimer_set(&et, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    get_sensor_information(&info);

    packetbuf_copyfrom(&info, sizeof(info));

    if (!linkaddr_cmp(&destination_addr, &linkaddr_node_addr))
    {
      unicast_send(&uc, &destination_addr);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
