# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# ---------------------------------------------------------------------------
# Board profile
#
#   guition -- the Guition board this project was written on: ESP32-P4 rev < 3.0
#              + ESP32-C6, JD9365 panel. Config lives in the tracked `sdkconfig`.
#   osptek  -- the osptek ESP32-P4C5 board: ESP32-P4 rev >= 3.0 + ESP32-C5,
#              JD9366 panel with in-cell touch. Config lives in
#              `sdkconfig.osptek`, a copy of the Guition config with only the
#              revision family, the flash clock and the co-processor target
#              changed.
#
# Two profiles rather than one, because IDF treats P4 rev < 3.0 and rev >= 3.0
# as mutually exclusive silicon: an image built for one family refuses to boot
# on the other. See docs/board-port.md.
#
# Select with:  idf.py -DDRO_BOARD=osptek ...    or   $env:DRO_BOARD = 'osptek'
# Build each profile in its own directory (-B build_osptek), and note that
# project.cmake reads SDKCONFIG while being *included*, so this block has to
# stay above the include($ENV{IDF_PATH}/tools/cmake/project.cmake) line.
# ---------------------------------------------------------------------------
if(NOT DEFINED DRO_BOARD)
    if(DEFINED ENV{DRO_BOARD} AND NOT "$ENV{DRO_BOARD}" STREQUAL "")
        set(DRO_BOARD "$ENV{DRO_BOARD}")
    else()
        set(DRO_BOARD "guition")
    endif()
endif()
set(DRO_BOARD "${DRO_BOARD}" CACHE STRING "Board profile: guition or osptek" FORCE)

if(DRO_BOARD STREQUAL "osptek")
    set(_dro_sdkconfig "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.osptek")
elseif(DRO_BOARD STREQUAL "guition")
    set(_dro_sdkconfig "${CMAKE_CURRENT_LIST_DIR}/sdkconfig")
else()
    message(FATAL_ERROR "Unknown DRO_BOARD='${DRO_BOARD}'. Expected 'guition' or 'osptek'.")
endif()
set(SDKCONFIG "${_dro_sdkconfig}" CACHE STRING "Path to sdkconfig" FORCE)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_compile_options(-Wno-format)
add_compile_options(-Wno-ignored-qualifiers)
add_compile_options(-Wno-attributes)
add_compile_options(-Wno-unused-function)

# Compatibility shim for the IDF 6.1 migration.
# LVGL 9.2 has no LV_COLOR_FORMAT_RGB565_SWAPPED (it arrived in 9.3), but
# esp_lvgl_port 2.9.0 references it in its color-format validation.  There is
# no configuration on LVGL 9.2 that can request it, so aliasing it to plain
# RGB565 keeps the semantics identical.  Remove this when LVGL is bumped to 9.3.
add_compile_definitions(LV_COLOR_FORMAT_RGB565_SWAPPED=LV_COLOR_FORMAT_RGB565)

project(lvgl_demo_v9)

# Available to the BSP and app: which board this build targets.
if(DRO_BOARD STREQUAL "osptek")
    add_compile_definitions(DRO_BOARD_OSPTEK=1)
else()
    add_compile_definitions(DRO_BOARD_GUITION=1)
endif()
message(STATUS "DRO board profile: ${DRO_BOARD} (sdkconfig: ${_dro_sdkconfig})")
