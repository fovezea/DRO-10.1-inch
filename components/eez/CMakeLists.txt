idf_component_register(
    SRCS 
        "eez_ui.c"
        "eez_ui_events.c"
        "generated/ui.c"
        "generated/screens.c"
        "generated/styles.c"
        "generated/images.c"
    INCLUDE_DIRS 
        "."
        "generated"
    REQUIRES 
        lvgl
        esp_lvgl_port
        esp_wifi_remote
        esp_wifi
)

# Add EEZ Studio generated files
# When you export from EEZ Studio, replace the example files with your actual generated files:
# SRCS "generated/your_ui.c" "generated/your_assets.c"
# INCLUDE_DIRS "generated" 