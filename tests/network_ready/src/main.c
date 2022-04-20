/*
 * Copyright (c) 2022 Laird Connectivity LLC
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr.h>

#include "lcz_network_monitor.h"

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/

static struct lcz_nm_event_agent event_agent;

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/

static void nm_event_callback(enum lcz_nm_event event)
{
	LOG_INF("Event %d", event);
}

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/

void main(void)
{
	LOG_INF("Network Monitor Test");

	event_agent.callback = nm_event_callback;
	lcz_nm_register_event_callback(&event_agent);

	while (!lcz_nm_network_ready()) {
		k_sleep(K_MSEC(250));
	}

	LOG_INF("Network interface is ready!");
}
