idf_component_register(SRCS "src/fpga_comms.c" "src/fpga_protocol.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver freertos esp_common dro_pins)
