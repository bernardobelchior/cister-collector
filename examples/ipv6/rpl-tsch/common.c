#include "common.h"
#include "net/ip/uip-debug.h"

void print_network_status(void)
{
    int i;
    uint8_t state;
    uip_ds6_defrt_t *default_route;
    uip_ds6_route_t *route;

    PRINTA("--- Network status ---\n");

    /* Our IPv6 addresses */
    PRINTA("- Server IPv6 addresses:\n");
    for (i = 0; i < UIP_DS6_ADDR_NB; i++)
    {
        state = uip_ds6_if.addr_list[i].state;
        if (uip_ds6_if.addr_list[i].isused &&
            (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
        {
            PRINTA("-- ");
            uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
            PRINTA("\n");
        }
    }

    /* Our default route */
    PRINTA("- Default route:\n");
    default_route = uip_ds6_defrt_lookup(uip_ds6_defrt_choose());
    if (default_route != NULL)
    {
        PRINTA("-- ");
        uip_debug_ipaddr_print(&default_route->ipaddr);
        ;
        PRINTA(" (lifetime: %lu seconds)\n", (unsigned long)default_route->lifetime.interval);
    }
    else
    {
        PRINTA("-- None\n");
    }

    /* Our routing entries */
    PRINTA("- Routing entries (%u in total):\n", uip_ds6_route_num_routes());
    route = uip_ds6_route_head();
    while (route != NULL)
    {
        PRINTA("-- ");
        uip_debug_ipaddr_print(&route->ipaddr);
        PRINTA(" via ");
        uip_debug_ipaddr_print(uip_ds6_route_nexthop(route));
        PRINTA(" (lifetime: %lu seconds)\n", (unsigned long)route->state.lifetime);
        route = uip_ds6_route_next(route);
    }

    PRINTA("----------------------\n");
}