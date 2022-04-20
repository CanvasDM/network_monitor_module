/**
 * @file lcz_network_monitor.h
 * @brief
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */
#ifndef __LCZ_NETWORK_MONITOR_H__
#define __LCZ_NETWORK_MONITOR_H__

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/* Global Constants, Macros and Type Definitions                                                  */
/**************************************************************************************************/

enum lcz_nm_event {
	LCZ_NM_EVENT_IFACE_UP = 0,
	LCZ_NM_EVENT_IFACE_DOWN,
	LCZ_NM_EVENT_IFACE_DNS_ADDED,
	LCZ_NM_EVENT_IFACE_DHCP_DONE,
};

typedef void (*lcz_nm_event_callback_t)(enum lcz_nm_event event);

struct lcz_nm_event_agent {
	sys_snode_t node;
	lcz_nm_event_callback_t callback;
};

/**************************************************************************************************/
/* Global Function Prototypes                                                                     */
/**************************************************************************************************/

/**
 * @brief Used to determine if the network interface is ready for IP based communication
 *
 * @return true network interface is ready
 * @return false network interface is not ready
 */
bool lcz_nm_network_ready(void);

/**
 * @brief Register a callback to receive network monitor events
 *
 * @param agent Callback agent to register
 */
void lcz_nm_register_event_callback(struct lcz_nm_event_agent *agent);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_NETWORK_MONITOR_H__ */
