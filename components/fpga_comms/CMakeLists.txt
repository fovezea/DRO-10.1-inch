idf_component_register(SRCS "src/fpga_comms.c"
                       INCLUDE_DIRS "include" "../../common"
                       REQUIRES driver freertos esp_common dro_pins esp_driver_uart esp_driver_gpio)
