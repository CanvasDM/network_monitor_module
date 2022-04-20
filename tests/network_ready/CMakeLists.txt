#
# Copyright (c) 2022 Laird Connectivity
#
# SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(lcz_network_monitor)

target_sources(app PRIVATE src/main.c)
