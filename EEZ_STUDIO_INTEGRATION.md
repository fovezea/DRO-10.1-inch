# EEZ Studio Integration Guide

## 🎯 **Overview**

This guide explains how to integrate EEZ Studio generated LVGL UI files with your ESP32-P4 WiFi Scanner project. The project now includes a dedicated EEZ component that provides a clean interface between EEZ Studio generated code and the application.

## 📁 **Project Structure**

```
ESP4-JC8012P4A1C_I_W_Y-LVGL9/
├── components/
│   └── eez/                    # EEZ Studio integration component
│       ├── CMakeLists.txt      # Build configuration
│       ├── eez_ui.h           # Main API header
│       ├── eez_ui.c           # Main implementation
│       ├── eez_ui_events.c    # Event handlers
│       ├── README.md          # Component documentation
│       └── generated/         # EEZ Studio exported files
│           ├── .gitkeep       # Placeholder
│           ├── example_ui.h   # Example structure
│           └── example_ui.c   # Example implementation
├── main/
│   └── main.c                 # Updated to use EEZ component
└── README.md
```

## 🚀 **Quick Start**

### 1. **Current Status**
✅ **Working**: The project now uses the EEZ component for UI management
✅ **WiFi**: ESP-Hosted over SDIO working perfectly
✅ **Display**: LVGL memory issues resolved
✅ **UI**: Basic WiFi scanner interface functional

### 2. **EEZ Studio Integration Ready**
The project is now ready for EEZ Studio integration. You can:

1. **Design UI in EEZ Studio**
2. **Export as C code**
3. **Drop files in `components/eez/generated/`**
4. **Update CMakeLists.txt**
5. **Build and test**

## 🔧 **EEZ Studio Workflow**

### Step 1: Design in EEZ Studio

1. **Open EEZ Studio**
2. **Create new LVGL project**
3. **Design your WiFi scanner interface:**
   - Title label
   - Scan button
   - Status label
   - WiFi network list
   - Network selection buttons
4. **Set up event handlers** for buttons
5. **Configure for ESP32-P4 display** (1280x800)

### Step 2: Export from EEZ Studio

1. **File → Export → C Code**
2. **Select LVGL target**
3. **Choose export options:**
   - Include assets (images, fonts)
   - Generate event handlers
   - Optimize for embedded
4. **Export to temporary folder**

### Step 3: Integrate with Project

1. **Copy generated files:**
   ```bash
   cp /path/to/eez/export/* components/eez/generated/
   ```

2. **Update `components/eez/CMakeLists.txt`:**
   ```cmake
   idf_component_register(
       SRCS 
           "eez_ui.c"
           "eez_ui_events.c"
           "generated/your_ui.c"        # Your generated files
           "generated/your_assets.c"
       INCLUDE_DIRS 
           "."
           "generated"
       REQUIRES 
           lvgl
           esp-bsp
   )
   ```

3. **Update `components/eez/eez_ui.c`** to use your generated UI:
   ```c
   // Replace the current UI creation with your generated code
   esp_err_t eez_ui_create_screen(void)
   {
       // Call your EEZ Studio generated initialization
       your_generated_ui_init();
       
       // Create your generated screen
       your_generated_ui_create_main_screen();
       
       return ESP_OK;
   }
   ```

### Step 4: Build and Test

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## 📋 **API Reference**

### EEZ Component Functions

```c
// Initialize EEZ UI system
esp_err_t eez_ui_init(const eez_ui_config_t *config);

// Create the main UI screen
esp_err_t eez_ui_create_screen(void);

// Update WiFi network list
esp_err_t eez_ui_update_wifi_list(const wifi_ap_record_t *networks, uint16_t count);

// Set status message
esp_err_t eez_ui_set_status(const char *message);

// Register callbacks
esp_err_t eez_ui_register_scan_callback(void (*callback)(void));
esp_err_t eez_ui_register_network_callback(void (*callback)(const char *ssid));
```

### Configuration Structure

```c
typedef struct {
    lv_obj_t *screen;           // Main screen object
    lv_obj_t *container;        // Main container
    bool auto_scan;             // Enable automatic WiFi scanning
    uint32_t scan_interval_ms;  // Scan interval in milliseconds
} eez_ui_config_t;
```

## 🎨 **UI Design Guidelines**

### Recommended EEZ Studio Setup

1. **Display Configuration:**
   - Resolution: 1280x800
   - Color depth: 16-bit (RGB565)
   - Orientation: Landscape

2. **UI Elements:**
   - **Title**: "WiFi Scanner - ESP32-P4"
   - **Scan Button**: Top-right corner
   - **Status Label**: Top-left corner
   - **Network List**: Scrollable area, bottom section
   - **Network Items**: Buttons with SSID, channel, RSSI, security

3. **Styling:**
   - Background: Dark theme (#000000)
   - Text: White (#FFFFFF)
   - Buttons: Rounded corners, hover effects
   - List: Dark gray background (#1a1a1a)

4. **Event Handlers:**
   - Scan button: `LV_EVENT_CLICKED`
   - Network buttons: `LV_EVENT_CLICKED`

## 🔄 **Integration Examples**

### Basic Integration

```c
#include "eez_ui.h"

void app_main(void)
{
    // Initialize EEZ UI
    eez_ui_config_t config = EEZ_UI_CONFIG_DEFAULT();
    eez_ui_init(&config);
    
    // Create UI screen
    eez_ui_create_screen();
    
    // Register callbacks
    eez_ui_register_scan_callback(wifi_scan_callback);
    eez_ui_register_network_callback(network_selected_callback);
}

void wifi_scan_callback(void)
{
    esp_wifi_scan_start(NULL, false);
}

void network_selected_callback(const char *ssid)
{
    // Handle network selection
    ESP_LOGI(TAG, "Selected: %s", ssid);
}
```

### Advanced Integration with Generated UI

```c
#include "eez_ui.h"
#include "generated/your_ui.h"  // Your EEZ Studio generated header

void app_main(void)
{
    // Initialize your generated UI
    your_generated_ui_init();
    
    // Create your generated screen
    your_generated_ui_create_main_screen();
    
    // Register event handlers
    your_generated_ui_register_event_handler(SCREEN_MAIN, OBJ_SCAN_BUTTON, 
                                           LV_EVENT_CLICKED, scan_button_handler);
}

void scan_button_handler(lv_event_t *e)
{
    esp_wifi_scan_start(NULL, false);
}
```

## 🛠️ **Troubleshooting**

### Common Issues

1. **Build Errors:**
   - Check that all generated files are in `CMakeLists.txt`
   - Verify include paths are correct
   - Ensure LVGL dependencies are satisfied

2. **UI Not Showing:**
   - Verify `eez_ui_create_screen()` is called after display init
   - Check that generated UI initialization is called
   - Monitor console for error messages

3. **Events Not Working:**
   - Ensure event handlers are properly registered
   - Check that object IDs match between code and EEZ Studio
   - Verify callback functions are defined

4. **Memory Issues:**
   - Monitor heap usage during UI creation
   - Check LVGL memory configuration
   - Consider reducing UI complexity if needed

### Debug Tips

```c
// Enable debug logging
esp_log_level_set("EEZ_UI", ESP_LOG_DEBUG);
esp_log_level_set("EEZ_EVENTS", ESP_LOG_DEBUG);

// Check memory usage
ESP_LOGI(TAG, "Free heap: %lu", esp_get_free_heap_size());

// Verify object creation
if (your_object) {
    ESP_LOGI(TAG, "Object created successfully");
} else {
    ESP_LOGE(TAG, "Failed to create object");
}
```

## 📚 **Resources**

- **EEZ Studio Documentation**: [https://eezstudio.org/](https://eezstudio.org/)
- **LVGL Documentation**: [https://docs.lvgl.io/](https://docs.lvgl.io/)
- **ESP-IDF LVGL Port**: [https://github.com/espressif/esp-bsp](https://github.com/espressif/esp-bsp)

## 🎉 **Success Criteria**

Your EEZ Studio integration is successful when:

✅ **UI displays correctly** on ESP32-P4 screen
✅ **Scan button works** and triggers WiFi scanning
✅ **Network list updates** with scan results
✅ **Network selection** triggers appropriate callbacks
✅ **No memory crashes** or LVGL errors
✅ **Smooth performance** with responsive UI

## 🚀 **Next Steps**

1. **Design your UI** in EEZ Studio
2. **Export and integrate** the generated files
3. **Test the integration** with the ESP32-P4
4. **Customize and enhance** the UI as needed
5. **Add more features** like network connection, settings, etc.

The foundation is now ready for your EEZ Studio designs! 