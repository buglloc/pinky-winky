# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pinky-winky)

target_sources(app
PRIVATE
    src/main.c
    src/led.c
    src/batt.c
    src/btn.c
    src/ble.c
)
