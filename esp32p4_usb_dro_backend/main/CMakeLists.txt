idf_component_register(
    SRCS "main.c" "backend_device.c"
    INCLUDE_DIRS "." "../modules/BSP"
    REQUIRES "esp_tinyusb" "usb" "esp_driver_pcnt"
)
