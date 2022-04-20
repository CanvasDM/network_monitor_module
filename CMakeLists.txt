#
# Copyright (c) 2022 Laird Connectivity LLC
#
# SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
#
if(CONFIG_LCZ_NETWORK_MONITOR)

zephyr_include_directories(include)

zephyr_sources(src/lcz_network_monitor.c)

endif()
