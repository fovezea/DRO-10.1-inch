idf_component_register(
    SRCS 
        "eez_ui_action.c"
        "eez_ui.c"
        "eez_ui_events.c"
        "generated/ui.c"
        "generated/screens.c"
        "eez_ui_vars.c"
        "generated/styles.c"
        "generated/images.c"
        "e_screw_screen.c"
        "dro_axis_helper.c"
        "fonts/ui_font_jet_brains_mono_bold_48.c"
        "fonts/ui_font_jet_brains_mono_bold_64.c"
        "calculator.c"

    INCLUDE_DIRS 
        "."
        "generated"
        "../../common"
    REQUIRES 
        lvgl
        esp_lvgl_port
        esp_wifi_remote
        esp_wifi
        frontend_usb
        machine_params
        screw_calc
        cone_calc
        axis_mapping
        dro_system
)

# Add EEZ Studio generated files
# When you export from EEZ Studio, replace the example files with your actual generated files:
# SRCS "eez_ui_action.c" "generated/your_ui.c" "generated/your_assets.c"
# INCLUDE_DIRS "generated" 