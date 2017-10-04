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
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "net/mac/tsch/tsch.h"
#include "dev/sht11/sht11-sensor.h"

const linkaddr_t coordinator_addr = {{1, 0}};
const linkaddr_t destination_addr = {{1, 0}};

struct sensor_info
{
  int temp;
  int hum;
};

/*---------------------------------------------------------------------------*/
PROCESS(unicast_test_process, "Rime Node");
AUTOSTART_PROCESSES(&unicast_test_process);

/*---------------------------------------------------------------------------*/
static void get_humidity(struct sensor_info *info)
{
  info->hum = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);
  /*
      s = (((0.0405 * val) - 4) + ((-2.8 * 0.000001) * (pow(val, 2))));
      dec = s;
      frac = s - dec;
      printf("Humidity=%d.%02u %% (%d)\n", dec, (unsigned int)(frac * 100), val);
*/
}

/*---------------------------------------------------------------------------*/
static void get_temperature(struct sensor_info *info)
{
  info->temp = sht11_sensor.value(SHT11_SENSOR_TEMP);
  /*s = ((0.01 * val) - 39.60);
      dec = s;
      frac = s - dec;
      printf("\nTemperature=%d.%02u C (%d)\n", dec, (unsigned int)(frac * 100), val);*/
}

/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  struct sensor_info *info = (struct sensor_info *)packetbuf_dataptr();
  printf("Received from %u.%u: Temp: %d Hum: %d\n",
         from->u8[0], from->u8[1], info->temp, info->hum);
}
/*---------------------------------------------------------------------------*/
static void
sent_uc(struct unicast_conn *ptr, int status, int num_tx)
{
  printf("Message sent, status %u, num_tx %u\n",
         status, num_tx);
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static struct unicast_conn uc;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_test_process, ev, data)
{
  PROCESS_BEGIN();

  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
  NETSTACK_MAC.on();

  SENSORS_ACTIVATE(sht11_sensor);
  unicast_open(&uc, 146, &unicast_callbacks);

  while (1)
  {
    static struct etimer et;

    etimer_set(&et, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    struct sensor_info info;
    get_temperature(&info);
    get_humidity(&info);

    packetbuf_copyfrom(&info, sizeof(info));

    if (!linkaddr_cmp(&destination_addr, &linkaddr_node_addr))
    {
      unicast_send(&uc, &destination_addr);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
