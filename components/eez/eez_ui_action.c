#include "eez_ui.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <string.h>
#include "eez_ui_action.h"
#include "vars.h"

static const char *TAG = "EEZ_UI_ACTION";   

//void action_change_to_setting_page(lv_event_t * e);
//void action_settings_mm_switch_pressed(lv_event_t * e);


void action_change_to_setting_page(lv_event_t * e)
{
    ESP_LOGI(TAG, "action_change_to_setting_page");
}

void action_settings_mm_switch_pressed(lv_event_t * e)
{
    ESP_LOGI(TAG, "action_settings_mm_switch_pressed");
}