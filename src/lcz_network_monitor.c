/**
 * @file lcz_network_monitor.c
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lcz_network_monitor, CONFIG_LCZ_NETWORK_MONITOR_LOG_LEVEL);

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dns_resolve.h>

#if defined(CONFIG_ATTR)
#include <attr.h>
#endif

#include "lcz_network_monitor.h"

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/

struct mgmt_events {
	uint32_t event;
	net_mgmt_event_handler_t handler;
	struct net_mgmt_event_callback cb;
};

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
static int lcz_network_monitor_init(void);
static void event_handler(enum lcz_nm_event event);

static void iface_dns_added_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					struct net_if *iface);
static void iface_up_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				 struct net_if *iface);
static void iface_down_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				   struct net_if *iface);
#if defined(CONFIG_NET_DHCPV4)
static void iface_dhcp_bound_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					 struct net_if *iface);
#endif
static void setup_iface_events(void);

#if defined(CONFIG_ATTR)
static void update_ip_address_strings(void);
static void clear_ip_address_strings(void);
#endif

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
static struct net_if *iface;
static struct net_if_config *cfg;
#if defined(CONFIG_DNS_RESOLVER)
static struct dns_resolve_context *dns;
#endif
static sys_slist_t event_callback_list;

static struct mgmt_events iface_events[] = {
	{ .event = NET_EVENT_DNS_SERVER_ADD, .handler = iface_dns_added_evt_handler },
	{ .event = NET_EVENT_IF_UP, .handler = iface_up_evt_handler },
	{ .event = NET_EVENT_IF_DOWN, .handler = iface_down_evt_handler },
#if defined(CONFIG_NET_DHCPV4)
	{ .event = NET_EVENT_IPV4_DHCP_BOUND, .handler = iface_dhcp_bound_evt_handler },
#endif
	{ 0 } /* setup_iface_events requires this extra location. */
};

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/

static void event_handler(enum lcz_nm_event event)
{
	sys_snode_t *node;
	struct lcz_nm_event_agent *agent;

	SYS_SLIST_FOR_EACH_NODE (&event_callback_list, node) {
		agent = CONTAINER_OF(node, struct lcz_nm_event_agent, node);
		if (agent->callback != NULL) {
			agent->callback(event);
		}
	}
}

static void iface_dns_added_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_DNS_SERVER_ADD) {
		return;
	}

	event_handler(LCZ_NM_EVENT_IFACE_DNS_ADDED);

#if defined(CONFIG_ATTR)
	update_ip_address_strings();
#endif
}

static void iface_up_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				 struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IF_UP) {
		return;
	}

	event_handler(LCZ_NM_EVENT_IFACE_UP);

#if defined(CONFIG_ATTR)
	update_ip_address_strings();
#endif
}

static void iface_down_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				   struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IF_DOWN) {
		return;
	}

	event_handler(LCZ_NM_EVENT_IFACE_DOWN);

#if defined(CONFIG_ATTR)
	clear_ip_address_strings();
#endif
}

#if defined(CONFIG_NET_DHCPV4)
static void iface_dhcp_bound_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					 struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_DHCP_BOUND) {
		return;
	}

	event_handler(LCZ_NM_EVENT_IFACE_DHCP_DONE);

#if defined(CONFIG_ATTR)
	update_ip_address_strings();
#endif
}
#endif

static void setup_iface_events(void)
{
	int i;

	for (i = 0; iface_events[i].event; i++) {
		net_mgmt_init_event_callback(&iface_events[i].cb, iface_events[i].handler,
					     iface_events[i].event);

		net_mgmt_add_event_callback(&iface_events[i].cb);
	}
}

#if defined(CONFIG_ATTR)
static void update_string(sa_family_t family, void *net_addr, uint16_t id, const char *msg)
{
	char *addr_status;
	char addr[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")];
	int r = 0;

	memset(addr, 0, sizeof(addr));
	addr_status = net_addr_ntop(family, net_addr, addr, sizeof(addr));
	if (addr_status == NULL) {
		LOG_ERR("Error converting %s address string", msg);
	} else {
		r = attr_set_string(id, addr, strlen(addr));
		if (r < 0) {
			LOG_ERR("Update address string error: %d", r);
		}
	}
}

static void update_ip_address_strings(void)
{
	void *net_addr;

	if (iface == NULL || cfg == NULL || !net_if_is_up(iface)) {
		return;
	}

#if defined(CONFIG_NET_IPV6)
	net_addr = &cfg->ip.ipv6->unicast->address.in6_addr;
	if (!net_ipv6_is_addr_unspecified(net_addr)) {
		update_string(AF_INET6, net_addr, ATTR_ID_ipv6_addr, "IPV6");
	}
#endif

#if defined(CONFIG_NET_IPV4)
	net_addr = &cfg->ip.ipv4->unicast->address.in_addr;
	if (!net_ipv4_is_addr_unspecified(net_addr)) {
		update_string(AF_INET, net_addr, ATTR_ID_ipv4_addr, "IPV4");
	}
#endif

#if defined(ATTR_ID_gw_ipv4_addr)
	net_addr = &cfg->ip.ipv4->gw;
	if (!net_ipv4_is_addr_unspecified(net_addr)) {
		update_string(AF_INET, net_addr, ATTR_ID_gw_ipv4_addr, "Gateway");
	}
#endif
}

static void clear_ip_address_strings(void)
{
	char addr[] = "";

#if defined(CONFIG_NET_IPV6)
	attr_set_string(ATTR_ID_ipv6_addr, addr, strlen(addr));
#endif

#if defined(CONFIG_NET_IPV4)
	attr_set_string(ATTR_ID_ipv4_addr, addr, strlen(addr));
#endif

#if defined(ATTR_ID_gw_ipv4_addr)
	attr_set_string(ATTR_ID_gw_ipv4_addr, addr, strlen(addr));
#endif
}
#endif /* CONFIG_ATTR */

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
SYS_INIT(lcz_network_monitor_init, APPLICATION, CONFIG_LCZ_NETWORK_MONITOR_INIT_PRIORITY);

bool lcz_nm_network_ready(void)
{
	bool ready = false;
#if defined(CONFIG_DNS_RESOLVER)
#if defined(CONFIG_NET_IPV4)
	struct sockaddr_in *dnsAddr;
#endif
#if defined(CONFIG_NET_IPV6)
	struct sockaddr_in6 *dnsAddr6;
#endif
#endif

	if (iface == NULL || cfg == NULL) {
		goto exit;
	}

#if defined(CONFIG_DNS_RESOLVER)
#if defined(CONFIG_NET_IPV6)
	dnsAddr6 = net_sin6(&dns->servers[0].dns_server);
	ready = net_if_is_up(iface) && cfg->ip.ipv6 &&
		!net_ipv6_is_addr_unspecified(&cfg->ip.ipv6->unicast->address.in6_addr) &&
		!net_ipv6_is_addr_unspecified(&dnsAddr6->sin6_addr);
#endif
#if defined(CONFIG_NET_IPV4)
	dnsAddr = net_sin(&dns->servers[0].dns_server);
	ready |= net_if_is_up(iface) && cfg->ip.ipv4 &&
		 !net_ipv4_is_addr_unspecified(&cfg->ip.ipv4->unicast->address.in_addr) &&
		 !net_ipv4_is_addr_unspecified(&dnsAddr->sin_addr);
#endif
#else /* !DNS_RESOLVER */
#if defined(CONFIG_NET_IPV6)
	ready = net_if_is_up(iface) && cfg->ip.ipv6 &&
		!net_ipv6_is_addr_unspecified(&cfg->ip.ipv6->unicast->address.in6_addr);
#endif
#if defined(CONFIG_NET_IPV4)
	ready |= net_if_is_up(iface) && cfg->ip.ipv4 &&
		 !net_ipv4_is_addr_unspecified(&cfg->ip.ipv4->unicast->address.in_addr);
#endif
#endif /* DNS_RESOLVER */

exit:
	return ready;
}

void lcz_nm_register_event_callback(struct lcz_nm_event_agent *agent)
{
	if (agent->callback != NULL) {
		sys_slist_append(&event_callback_list, &agent->node);
	}
}

/**************************************************************************************************/
/* SYS INIT                                                                                       */
/**************************************************************************************************/
static int lcz_network_monitor_init(void)
{
	int ret;

	ret = 0;
	setup_iface_events();

	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("Could not get default iface");
		ret = -EIO;
		goto exit;
	}

	cfg = net_if_get_config(iface);
	if (!cfg) {
		LOG_ERR("Could not get iface config");
		ret = -EIO;
		goto exit;
	}

#if defined(CONFIG_DNS_RESOLVER)
	dns = dns_resolve_get_default();
	if (!dns) {
		LOG_ERR("Could not get DNS context");
		ret = -EIO;
		goto exit;
	}
#endif

#if defined(CONFIG_NET_DHCPV4)
	net_dhcpv4_start(iface);
#endif

	LOG_DBG("network monitor initialized");
exit:
	return ret;
}
