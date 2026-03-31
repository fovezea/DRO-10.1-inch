idf_component_register(
    SRCS "main.c" "backend_device.c"
    INCLUDE_DIRS "."
    REQUIRES "esp_tinyusb" "usb"
)
