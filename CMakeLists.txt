# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

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
