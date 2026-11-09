# ESP-IDF project for the ESP32-WROOM-32E 5-axis DRO backend board.
#
# Board design files: C:/SRC/esp32-DRO-backend (KiCad, Rev C)
# This firmware: ESP-NOW + UART scale-collection node.

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32_wroom_dro_backend)
