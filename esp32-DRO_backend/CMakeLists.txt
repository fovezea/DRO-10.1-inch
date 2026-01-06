cmake_minimum_required(VERSION 3.16)

# Add the common directory to the component path so we can include protocol_defs.h
set(EXTRA_COMPONENT_DIRS ../common)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-dro-backend)
