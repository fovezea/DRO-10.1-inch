idf_component_register(SRCS "esp_lcd_jd9366.c" "esp_lcd_touch_jd9366.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_lcd espressif__esp_lcd_touch esp_driver_gpio)
